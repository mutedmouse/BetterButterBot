#include "CameraStream.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <esp_heap_caps.h>
#include <driver/ledc.h>
#include <img_converters.h>
#include <Devices/Camera.h>
#include <Util/stdafx.h>
#include "VoiceLink.h"

DEFINE_LOG(CameraStream)

CameraStream* CameraStream::instance = nullptr;

CameraStream::CameraStream(Camera* camera, VoiceLink* link) : camera(camera), link(link){
	instance = this;
	// Stack in internal RAM (last argument true): with it in PSRAM, the stream task, Wi-Fi and the
	// camera's DMA all competed for PSRAM and the camera lost data mid-frame.
	task = std::make_unique<Threaded>([this](){ loop(); }, "CameraStream", 20 / portTICK_PERIOD_MS, 4 * 1024, 4, 0, true);
	task->start();
}

CameraStream::~CameraStream(){
	instance = nullptr;
	wantOn = false;
	task->stop(portMAX_DELAY);
}

void CameraStream::start(){
	wantOn = true;
}

void CameraStream::stop(){
	wantOn = false;
}

void CameraStream::claim(Claim who){
	CameraStream* self = instance;
	if(self == nullptr) return;
	self->claims |= who;
	// The stream task sees the claim before its next frame and restores the camera.
	const uint32_t until = millis() + 3000;
	while(self->on && millis() < until){
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	if(self->on){
		CMF_LOG(CameraStream, LogLevel::Warning, "Camera claim (%u) timed out waiting for the stream to pause", (unsigned) who);
	}
}

void CameraStream::release(Claim who){
	if(CameraStream* self = instance){
		self->claims &= ~who;
	}
}

bool CameraStream::streaming(){
	return instance != nullptr && (instance->on || instance->wantOn);
}

void CameraStream::loop(){
	if(!wantOn){
		if(on) leave("stopped");
		return;
	}

	// Wanted, but the camera is needed elsewhere or the gateway can't take frames: pause, keep wanting.
	if(claims != 0 || !link->ready()){
		if(on) leave(claims != 0 ? "paused" : "offline");
		return;
	}

	if(!on){
		on = true;   // before switching, so a claim made meanwhile waits for us
		if(!enterBestMode()){
			restoreNormal();
			on = false;
			wantOn = false;
			report(false, "camera error");
			return;
		}
		frames = 0;
		report(true, "");
		return;
	}

	const uint32_t interval = avgSendMs * 13 / 10 > MinFrameIntervalMs ? avgSendMs * 13 / 10 : MinFrameIntervalMs;
	if(millis() - lastFrameMs < interval) return;
	lastFrameMs = millis();

	camera_fb_t* fb = camera->getFrame();
	if(fb == nullptr || fb->buf == nullptr || fb->len == 0){
		camera->releaseFrame();
		// The driver itself drops frames it can't complete; if that keeps happening, try a lighter mode.
		if(++missedFrames >= MaxMissedFrames && mode + 1 < ModeCount){
			CMF_LOG(CameraStream, LogLevel::Warning, "Camera dropping frames at %s, stepping down", Modes[mode].name);
			missedFrames = 0;
			if(!enterFrom(mode + 1)){
				leave("camera error");
				return;
			}
			ceiling = mode;
			report(true, "camera dropping frames");
		}
		return;
	}
	missedFrames = 0;

	const uint8_t* data = fb->buf;
	size_t len = fb->len;
	uint8_t* encoded = nullptr;
	if(Modes[mode].format == PIXFORMAT_JPEG){
		// A frame cut short by the camera shows as garbage below the cut: drop it, and step down if
		// it keeps happening at this mode.
		checkedFrames++;
		if(!completeJpeg(data, len)){
			camera->releaseFrame();
			if(++badFrames * 5 > checkedFrames && checkedFrames >= 10 && mode + 1 < ModeCount){
				CMF_LOG(CameraStream, LogLevel::Warning, "%lu of %lu frames truncated at %s, stepping down",
						(unsigned long) badFrames, (unsigned long) checkedFrames, Modes[mode].name);
				if(!enterFrom(mode + 1)){
					leave("camera error");
					return;
				}
				ceiling = mode;   // that mode doesn't work on this robot
				report(true, "truncated frames");
			}
			return;
		}
		if(checkedFrames >= 100){
			checkedFrames = badFrames = 0;
		}
	}else{
		// Software JPEG; frame2jpg allocates the output (PSRAM when internal RAM is short).
		if(!frame2jpg(fb, Modes[mode].quality, &encoded, &len) || encoded == nullptr){
			camera->releaseFrame();
			return;
		}
		data = encoded;
	}

	const uint32_t t0 = millis();
	const VoiceLink::FrameResult result = link->sendFrame(data, len);
	const uint32_t sendMs = millis() - t0;
	free(encoded);
	camera->releaseFrame();

	if(result == VoiceLink::FrameResult::Sent){
		frames++;
		adapt(sendMs);
	}else if(result == VoiceLink::FrameResult::Failed){
		adapt(SlowSendMs * 2);   // treat a failed send as a very slow one
	}
	// Busy (the robot is listening): skipped, doesn't count either way.
}

void CameraStream::adapt(uint32_t sendMs){
	avgSendMs = avgSendMs == 0 ? sendMs : (avgSendMs * 7 + sendMs) / 8;

	if(avgSendMs > SlowSendMs){
		fastFrames = 0;
		if(++slowFrames >= 8 && mode + 1 < ModeCount){
			CMF_LOG(CameraStream, LogLevel::Info, "Frames slow (avg %lu ms), stepping down from %s", (unsigned long) avgSendMs, Modes[mode].name);
			if(!enterFrom(mode + 1)){
				leave("camera error");
				return;
			}
			report(true, "slower link");
			slowFrames = 0;
			avgSendMs = 0;
		}
		return;
	}
	slowFrames = 0;

	if(avgSendMs < FastSendMs && mode > ceiling){
		if(++fastFrames >= StepUpAfterFrames){
			CMF_LOG(CameraStream, LogLevel::Info, "Frames fast (avg %lu ms), stepping up from %s", (unsigned long) avgSendMs, Modes[mode].name);
			const int previous = mode;
			if(enterMode(mode - 1)){
				report(true, "faster link");
			}else if(!enterFrom(previous)){
				leave("camera error");
				return;
			}
			fastFrames = 0;
			avgSendMs = 0;
		}
	}else{
		fastFrames = 0;
	}
}

bool CameraStream::completeJpeg(const uint8_t* data, size_t len){
	if(len < 4 || data[0] != 0xFF || data[1] != 0xD8) return false;
	// The end-of-image marker can be followed by a little padding.
	const size_t from = len > 64 ? len - 64 : 2;
	for(size_t i = len - 2; i >= from; i--){
		if(data[i] == 0xFF && data[i + 1] == 0xD9) return true;
		if(i == 0) break;
	}
	return false;
}

bool CameraStream::enterMode(int index){
	const Mode& m = Modes[index];
	camera->setFormat(m.format);
	camera->setRes(m.size);
	if(camera->init() != ESP_OK){
		CMF_LOG(CameraStream, LogLevel::Warning, "Camera won't start at %s", m.name);
		return false;
	}
	// The camera's DMA buffers live in internal RAM; keep enough of it for Wi-Fi and the voice link.
	const uint32_t freeInternal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
	if(freeInternal < MinInternalFree){
		CMF_LOG(CameraStream, LogLevel::Warning, "%s leaves only %lu bytes of internal RAM", m.name, (unsigned long) freeInternal);
		return false;
	}
	if(sensor_t* sensor = esp_camera_sensor_get()){
		if(m.format == PIXFORMAT_JPEG){
			sensor->set_quality(sensor, m.quality);
		}
		// Slower pixel clock while streaming (restoreNormal() re-inits at the configured 27 MHz).
		if(sensor->set_xclk){
			sensor->set_xclk(sensor, LEDC_TIMER_0, StreamXclkMhz);
		}
	}
	mode = index;
	checkedFrames = badFrames = 0;
	missedFrames = 0;
	CMF_LOG(CameraStream, LogLevel::Info, "Streaming at %s (internal RAM free %lu)", m.name, (unsigned long) freeInternal);
	return true;
}

bool CameraStream::enterFrom(int start){
	for(int i = start; i < ModeCount; i++){
		if(enterMode(i)){
			if(i > ceiling && start == ceiling){
				ceiling = i;   // better modes didn't fit: don't try them again this session
			}
			avgSendMs = 0;
			slowFrames = fastFrames = 0;
			return true;
		}
	}
	return false;
}

bool CameraStream::enterBestMode(){
	return enterFrom(ceiling);
}

void CameraStream::restoreNormal(){
	// Back to the format face and object detection expect (set up in main.cpp).
	camera->setFormat(PIXFORMAT_RGB565);
	camera->setRes(FRAMESIZE_128X128);
	if(camera->init() != ESP_OK){
		CMF_LOG(CameraStream, LogLevel::Error, "Camera re-init in normal mode failed");
		return;
	}
	// A re-initialised camera starts with untuned exposure and white balance: discard frames for a moment
	// so whoever claimed the camera (find me, face recognition) gets usable images from the start.
	const uint32_t until = millis() + SettleAfterInitMs;
	while(millis() < until){
		camera->getFrame();
		camera->releaseFrame();
	}
}

void CameraStream::leave(const char* reason){
	restoreNormal();
	CMF_LOG(CameraStream, LogLevel::Info, "Camera stream %s after %lu frames", reason, (unsigned long) frames);
	on = false;
	if(strcmp(reason, "stopped") == 0 || strcmp(reason, "camera error") == 0){
		wantOn = false;
	}
	report(false, reason);
}

void CameraStream::report(bool isOn, const char* reason){
	char msg[128];
	snprintf(msg, sizeof(msg), "{\"type\":\"camera\",\"on\":%s,\"reason\":\"%s\",\"mode\":\"%s\"}",
			 isOn ? "true" : "false", reason, isOn ? Modes[mode].name : "");
	link->sendText(msg);
}
