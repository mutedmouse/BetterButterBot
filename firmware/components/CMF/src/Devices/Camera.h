#ifndef CMF_CAMERA_H
#define CMF_CAMERA_H

#include <esp_camera.h>
#include "Periphery/GPIOPeriph.h"

class Camera : public Object {
	GENERATED_BODY(Camera, Object, CONSTRUCTOR_PACK(camera_config_t, class  I2CMaster*, std::function<void(sensor_t*)>))

public:
	Camera(camera_config_t config = {}, I2CMaster* i2c = nullptr, std::function<void(sensor_t*)> sensorConfig = {});

	~Camera() override;

	camera_fb_t* getFrame();
	void releaseFrame();

	void setRes(framesize_t res);
	framesize_t getRes() const;

	pixformat_t getFormat() const;
	void setFormat(pixformat_t format);

	esp_err_t init();
	void deinit();
	bool isInited();

private:
	bool inited = false;
	framesize_t resWait;
	pixformat_t formatWait;

	camera_fb_t* frame = nullptr;

	framesize_t res = FRAMESIZE_INVALID;
	pixformat_t format = PIXFORMAT_RGB444;

	static constexpr int MaxFailedFrames = 100;
	int failedFrames = 0;

	camera_config_t config;
	StrongObjectPtr<I2CMaster> i2c;
	StrongObjectPtr<GPIOPeriph> gpio;
	std::function<void(sensor_t*)> sensorConfig;
};


#endif //CMF_CAMERA_H
