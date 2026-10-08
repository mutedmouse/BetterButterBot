#include "FaceDet.h"
#include <cstring>
#include <esp_heap_caps.h>
#include "Services/CameraStream.h"

DEFINE_LOG(FaceDet)

FaceDet::FaceDet(Camera* camera) : camera(camera){
	threadGate = xEventGroupCreate();
	// RunBit starts cleared so the thread blocks at its gate as soon as it begins.

	thread = std::make_unique<Threaded>([this]() {
		threadFunc();
	}, "FaceDet", 0, 3 * 1024, 5, 0, true);

	thread->start();

	xEventGroupWaitBits(threadGate, AtGateBit, pdFALSE, pdTRUE, portMAX_DELAY);
}

FaceDet::~FaceDet(){
	// Set RunBit so any thread parked at the gate unblocks; otherwise the Threaded destructor's
	// stop() would wait forever for a thread that is blocked on our event group.
	xEventGroupSetBits(threadGate, RunBit);
	thread->stop();
	// Clear RunBit so isStarted() returns false and unloadModel()'s guard passes.
	xEventGroupClearBits(threadGate, RunBit);
	unloadModel();
	vEventGroupDelete(threadGate);
}

void FaceDet::start(bool stopAfterDetection, bool enrollFace){
	if(!human_face_detect){
		CMF_LOG(FaceDet, LogLevel::Error, "Model not loaded!");
		return;
	}

	this->stopAfterDetection = stopAfterDetection;
	enrollNextFace = enrollFace;
	xEventGroupSetBits(threadGate, RunBit);
}

void FaceDet::stop(){
	enrollNextFace = false;
	xEventGroupClearBits(threadGate, RunBit);
	xEventGroupWaitBits(threadGate, AtGateBit, pdFALSE, pdTRUE, portMAX_DELAY);
}

bool FaceDet::isStarted() const{
	return (xEventGroupGetBits(threadGate) & RunBit) != 0;
}

void FaceDet::loadModel(bool longRange){
	// Hold the camera for the whole routine (load ... unload), not per start/stop: routines like find me
	// start and stop detection on every turn, and the stream re-taking the camera in between cost each
	// look-around window a camera re-init. A running stream pauses here and resumes after unloadModel().
	CameraStream::claim(CameraStream::FaceDetection);
	this->longRange = longRange;
	setCameraMode(longRange ? FRAMESIZE_QVGA : FRAMESIZE_128X128);
	if(longRange && cropBuf == nullptr){
		cropBuf = static_cast<uint8_t*>(heap_caps_malloc(CropW * CropH * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
	}

	if(human_face_detect){
		CMF_LOG(FaceDet, LogLevel::Error, "Model already loaded!");
		return;
	}

	if(isStarted()){
		CMF_LOG(FaceDet, LogLevel::Error, "Cannot load model while running!");
		return;
	}

	uint32_t startMillis = millis();

	human_face_detect = std::make_unique<HumanFaceDetect>();
	human_face_recognizer = std::make_unique<HumanFaceRecognizer>(db_path);

	CMF_LOG(FaceDet, Debug, "load time: %d", (int)millis() - (int)startMillis);
}

void FaceDet::unloadModel(){
	if(!human_face_detect){
		CameraStream::release(CameraStream::FaceDetection);
		return;
	}

	if(isStarted()){
		CMF_LOG(FaceDet, LogLevel::Error, "Cannot unload model while running!");
		return;
	}
	uint32_t startMillis = millis();

	human_face_detect.reset();
	human_face_recognizer.reset();
	setCameraMode(FRAMESIZE_128X128);   // the normal mode everything else expects
	longRange = false;
	if(cropBuf != nullptr){
		heap_caps_free(cropBuf);
		cropBuf = nullptr;
	}
	CameraStream::release(CameraStream::FaceDetection);   // the camera stream may resume
	CMF_LOG(FaceDet, Debug, "unload time: %d", (int)millis() - (int)startMillis);
}

bool FaceDet::knowsFace(){
	if(!human_face_recognizer) return false;
	return human_face_recognizer->get_num_feats() > 0;
}

bool FaceDet::forgetFace(){
	if(isStarted()){
		CMF_LOG(FaceDet, LogLevel::Error, "Cannot forget face while FaceDet is running");
		return false;
	}
	if(!knowsFace()) return false;

	human_face_recognizer->clear_all_feats();
	return true;
}

void FaceDet::learnFace(){
	if(isStarted()){
		CMF_LOG(FaceDet, LogLevel::Error, "Cannot learn face while FaceDet is running");
		return;
	}

	start(true, true);
}

void FaceDet::threadFunc(){
	if((xEventGroupGetBits(threadGate) & RunBit) == 0){
		xEventGroupSetBits(threadGate, AtGateBit);
		xEventGroupWaitBits(threadGate, RunBit, pdFALSE, pdTRUE, portMAX_DELAY);
		xEventGroupClearBits(threadGate, AtGateBit);
	}

	camera_fb_t* frameData = camera->getFrame();
	if(frameData == nullptr || frameData->buf == nullptr || frameData->len == 0){
		CMF_LOG(FaceDet, LogLevel::Error, "Couldnt get frame");
		camera->releaseFrame();
		return;
	}

	dl::image::img_t img{ frameData->buf, (uint16_t)frameData->width, (uint16_t)frameData->height, dl::image::DL_IMAGE_PIX_TYPE_RGB565 };

	detectionProcess(img);

	camera->releaseFrame();
}

void FaceDet::detectionProcess(const dl::image::img_t& img){
	if(!human_face_detect){
		CMF_LOG(FaceDet, LogLevel::Error, "Model not loaded in detection task");
		return;
	}

	if(longRange && cropBuf != nullptr && img.width >= CropX[2] + CropW && img.height >= CropY + CropH){
		detectLongRange(img);
		return;
	}

	framesAnalysed++;
	auto& detect_results = human_face_detect->run(img);

	if(detect_results.empty()) return;

	const auto largestBox = std::ranges::max_element(detect_results,
	                                                 [](const dl::detect::result_t& a, const dl::detect::result_t& b) -> bool {
		                                                 return a.box_area() > b.box_area();
	                                                 });

	const float w = img.width, h = img.height;   // positions as fractions of the frame, whatever its size
	glm::vec2 pos = { (float)largestBox->box[0] / w, (float)largestBox->box[1] / h };
	glm::vec2 size = { (float)(largestBox->box[2] - largestBox->box[0]) / w, (float)(largestBox->box[3] - largestBox->box[1]) / h };
	CMF_LOG(FaceDet, Info, "detected face");

	if(enrollNextFace){
		CMF_LOG(FaceDet, Info, "stored face");
		ESP_ERROR_CHECK_WITHOUT_ABORT(human_face_recognizer->enroll(img, detect_results));
		enrollNextFace = false;
		OnDetect.broadcast(detect_results.size(), true, pos, size);
	} else{
		bool recognized = false;

		if(!knowsFace()){
			recognized = false;
		} else{
			const auto recognize_results = human_face_recognizer->recognize(img, detect_results);
			recognized = !recognize_results.empty();
		}

		OnDetect.broadcast(detect_results.size(), recognized, pos, size);
	}

	if(stopAfterDetection){
		// Self-pause: clear RunBit. The thread parks itself at the gate on the next iteration.
		xEventGroupClearBits(threadGate, RunBit);
	}
}

void FaceDet::setCameraMode(framesize_t res){
	if(camera == nullptr || camera->getRes() == res) return;
	camera->setFormat(PIXFORMAT_RGB565);
	camera->setRes(res);
	if(camera->init() != ESP_OK){
		CMF_LOG(FaceDet, LogLevel::Error, "Camera re-init failed");
		return;
	}
	// A re-initialised camera starts with untuned exposure: discard frames until it adjusts.
	const uint32_t until = millis() + SettleAfterInitMs;
	while(millis() < until){
		camera->getFrame();
		camera->releaseFrame();
	}
}

void FaceDet::detectLongRange(const dl::image::img_t& img){
	framesAnalysed++;
	const float W = img.width, H = img.height;
	int faces = 0;
	int bestArea = 0;
	glm::vec2 bestPos{}, bestSize{};

	// Each run() returns the detector's own result list, so it's read before the next run.
	auto consider = [&](std::list<dl::detect::result_t>& results, int ox, int oy){
		for(const auto& r : results){
			faces++;
			const int area = (r.box[2] - r.box[0]) * (r.box[3] - r.box[1]);
			if(area > bestArea){
				bestArea = area;
				bestPos = { (r.box[0] + ox) / W, (r.box[1] + oy) / H };
				bestSize = { (r.box[2] - r.box[0]) / W, (r.box[3] - r.box[1]) / H };
			}
		}
	};

	// Whole frame (shrunk by the detector): faces close to the robot.
	consider(human_face_detect->run(img), 0, 0);

	// Crops at full resolution: faces farther away.
	const auto* src = static_cast<const uint8_t*>(img.data);
	for(const int ox : CropX){
		for(int row = 0; row < CropH; row++){
			memcpy(cropBuf + row * CropW * 2, src + ((CropY + row) * img.width + ox) * 2, CropW * 2);
		}
		dl::image::img_t crop{ cropBuf, (uint16_t) CropW, (uint16_t) CropH, dl::image::DL_IMAGE_PIX_TYPE_RGB565 };
		consider(human_face_detect->run(crop), ox, CropY);
	}

	if(faces == 0) return;
	OnDetect.broadcast(faces, false, bestPos, bestSize);   // long range: detection only, no recognition
	if(stopAfterDetection){
		xEventGroupClearBits(threadGate, RunBit);
	}
}
