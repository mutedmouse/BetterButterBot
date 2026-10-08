#ifndef BUTTERBOT_FIRMWARE_FEED_H
#define BUTTERBOT_FIRMWARE_FEED_H

#include <esp_camera.h>
#include <Entity/SyncEntity.h>

class Feed : public SyncEntity{
	GENERATED_BODY(Feed, SyncEntity, void)

public:
	Feed();
	virtual ~Feed() override;

	virtual void tick(float deltaTime) noexcept override;

	bool isActive() const noexcept;

private:
	static constexpr size_t MaxJPEGBufSize = 10 * 1024;

	uint8_t* buffer = nullptr;
	bool active = true;
	StrongObjectPtr<class UDPEmitter> udp;

private:
	void sendFrame(camera_fb_t* frameData) const;
};

#endif //BUTTERBOT_FIRMWARE_FEED_H