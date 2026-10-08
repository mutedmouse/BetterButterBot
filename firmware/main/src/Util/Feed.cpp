#include "Feed.h"
#include <Devices/Camera.h>
#include <Services/Com.h>
#include <FeedFrame.h>
#include "UDPEmitter.h"
#include "Services/CameraStream.h"

DEFINE_LOG(Feed);

Feed::Feed(){
	CameraStream::claim(CameraStream::RCFeed); // normally already off: RC mode pauses the voice link
	buffer = static_cast<uint8_t*>(heap_caps_malloc(MaxJPEGBufSize, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM));

	udp = newObject<UDPEmitter>(this);

	const Application* app = getApp();
	Camera* camera = app->getDevice<Camera>();
	if(camera == nullptr){
		return;
	}

	Com* com = app->getService<Com>();
	if(com == nullptr){
		return;
	}

	if(!camera->isInited()){
		const esp_err_t err = camera->init();
		if(err != ESP_OK){
			active = false;
			FeedStatusData statusData{.status = active};
			com->sendData(BB::State::RC, BB::Action::RC::FeedStatus, statusData);
			return;
		}

		//init finally ok!
		if(camera->isInited()){
			active = true;
			FeedStatusData statusData{.status = active};
			com->sendData(BB::State::RC, BB::Action::RC::FeedStatus, statusData);
		}
	}else{
		active = true;
		FeedStatusData statusData{.status = active};
		com->sendData(BB::State::RC, BB::Action::RC::FeedStatus, statusData);
	}
}

Feed::~Feed(){
	CameraStream::release(CameraStream::RCFeed);
	if(buffer != nullptr){
		free(buffer);
		buffer = nullptr;
	}

	delete *udp;
}

void Feed::tick(float deltaTime) noexcept{
	const Application* app = getApp();

	Camera* camera = app->getDevice<Camera>();
	if(camera == nullptr){
		CMF_LOG(Feed, LogLevel::Error, "Camera is nullptr.");
		return;
	}

	Com* com = app->getService<Com>();
	if(com == nullptr){
		CMF_LOG(Feed, LogLevel::Error, "Com is nullptr.");
		return;
	}

	if(!camera->isInited()){
		const esp_err_t err = camera->init();
		if(err != ESP_OK){
			active = false;
			FeedStatusData statusData{.status = active};
			com->sendData(BB::State::RC, BB::Action::RC::FeedStatus, statusData);
			return;
		}

		//init finally ok!
		if(camera->isInited()){
			active = true;
			FeedStatusData statusData{.status = active};
			com->sendData(BB::State::RC, BB::Action::RC::FeedStatus, statusData);
		}
	}

	camera_fb_t* frameData = camera->getFrame();
	if(frameData == nullptr || frameData->buf == nullptr || frameData->len == 0){
		CMF_LOG(Feed, LogLevel::Error, "Couldnt get frame");
		camera->releaseFrame();
		return;
	}

	sendFrame(frameData);
}

bool Feed::isActive() const noexcept{
	return active;
}

void Feed::sendFrame(camera_fb_t* frameData) const{
	if(frameData == nullptr){
		return;
	}

	const Application* app = getApp();

	Camera* camera = app->getDevice<Camera>();
	if(camera == nullptr) {
		return;
	}

	if(udp == nullptr) {
		CMF_LOG(Feed, LogLevel::Warning, "UDPEmitter is invalid.");
		camera->releaseFrame();
		return;
	}

	size_t size = 0;
	uint8_t* out = nullptr;

	if(!frame2jpg(frameData, 15, &out, &size) || out == nullptr){
		CMF_LOG(Feed, LogLevel::Warning, "frame2jpg failed to encode frame.");
		camera->releaseFrame();
		return;
	}

	const size_t frameSize = size;
	const size_t sendSize = frameSize + sizeof(FeedFrame::Header) + sizeof(FeedFrame::Trailer) + sizeof(size_t) * 2;

	if(sendSize > MaxJPEGBufSize){
		CMF_LOG(Feed, LogLevel::Warning, "Data frame buffer larger than send buffer. %zu > %zu\n", sendSize, MaxJPEGBufSize);
		camera->releaseFrame();
		return;
	}

	size_t cursor = 0;
	auto addData = [&cursor, this](const void* data, size_t size){
		memcpy(buffer + cursor, data, size);
		cursor += size;
	};

	uint8_t shiftedFrame[4];
	for(uint8_t i = 0; i < 4; i++){
		shiftedFrame[FeedFrame::SizeShift[i]] = ((uint8_t*) &frameSize)[i];
	}

	addData(FeedFrame::Header, sizeof(FeedFrame::Header));
	addData(&frameSize, sizeof(size_t));
	addData(shiftedFrame, sizeof(size_t));
	addData(out, size);
	addData(FeedFrame::Trailer, sizeof(FeedFrame::Trailer));

	udp->write(buffer, sendSize);

	free(out);
	camera->releaseFrame();
}
