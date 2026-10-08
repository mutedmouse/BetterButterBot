#ifndef BUTTERBOT_FIRMWARE_CAMERASTREAM_H
#define BUTTERBOT_FIRMWARE_CAMERASTREAM_H

#include <atomic>
#include <memory>
#include <esp_camera.h>
#include <freertos/FreeRTOS.h>
#include "Thread/Threaded.h"

class Camera;
class VoiceLink;

/**
 * @brief Live camera view on the home network, relayed by the voice gateway.
 *
 * Off by default. The gateway turns it on ({"type":"camera","on":true}) when asked by voice, and it
 * stays wanted until asked to stop ({"type":"camera","on":false}). While wanted it pauses (and then
 * resumes by itself) when something else needs the camera or the gateway link is down.
 *
 * Resolution adapts: it starts at the best mode in Modes[] whose camera buffers fit in memory, then
 * steps down while frames are slow to send over Wi-Fi and back up once the link has been fast for a
 * while (never above the best mode that fitted).
 *
 * The camera has one owner at a time. Everything else that uses it (face detection, object
 * recognition, the RC feed) claims it first: a claim pauses the stream and waits until the camera is
 * back in its normal 128x128 RGB565 mode.
 */
class CameraStream {
public:
	CameraStream(Camera* camera, VoiceLink* link);
	~CameraStream();

	/** Want the stream on until stop(). Non-blocking; the stream task does the work. */
	void start();
	/** Stop wanting the stream. Non-blocking. */
	void stop();

	enum Claim : uint8_t { FaceDetection = 1, ObjectDetection = 2, RCFeed = 4 };
	/** Take the camera for `who`: pauses a running stream and waits until the camera is restored. */
	static void claim(Claim who);
	static void release(Claim who);
	/** True while the stream is wanted (camera-using idle routines stay off meanwhile). */
	static bool streaming();

private:
	struct Mode {
		pixformat_t format;   // JPEG: the sensor encodes; RGB565: software JPEG (frame2jpg)
		framesize_t size;
		int quality;          // JPEG: sensor quality 0-63, lower is better; RGB565: frame2jpg 1-100
		const char* name;
	};
	// The camera's own JPEG output doesn't work on this robot: at 640x480 frames arrived truncated, and
	// at 320x240 the driver dropped nearly every frame ("NO-EOI"), even with a 10 MHz clock. Raw RGB565
	// frames compressed on the robot (frame2jpg, like the RC feed) stream reliably at ~2-3 frames/s.
	static constexpr Mode Modes[] = {
		{ PIXFORMAT_RGB565, FRAMESIZE_QVGA,    50, "320x240 (software JPEG)" },
		{ PIXFORMAT_RGB565, FRAMESIZE_128X128, 60, "128x128 (software JPEG)" },
	};
	static constexpr int ModeCount = sizeof(Modes) / sizeof(Modes[0]);

	static constexpr uint32_t MinInternalFree = 18 * 1024;  // Wi-Fi and the voice link need this much
	static constexpr uint32_t MinFrameIntervalMs = 150;     // at most ~6-7 frames per second
	static constexpr uint32_t SlowSendMs = 250;             // average send time that steps the mode down
	static constexpr uint32_t FastSendMs = 70;              // ... and that, sustained, steps it back up
	static constexpr uint32_t StepUpAfterFrames = 150;      // ~30 s of fast sends before trying better
	// Camera clock in JPEG stream mode. At the normal 27 MHz the JPEG data outran the DMA into PSRAM
	// (shared with Wi-Fi), so nearly every frame lost its end marker ("NO-EOI") and was dropped.
	static constexpr int StreamXclkMhz = 10;
	static constexpr uint32_t MaxMissedFrames = 3;          // frames in a row the driver couldn't deliver
	static constexpr uint32_t SettleAfterInitMs = 700;      // frames discarded after leaving stream mode

	static CameraStream* instance;

	Camera* camera;
	VoiceLink* link;
	std::unique_ptr<Threaded> task;

	std::atomic<bool> wantOn{ false };
	std::atomic<bool> on{ false };          // camera is in stream mode (switching in or out counts)
	std::atomic<uint8_t> claims{ 0 };

	// Stream-task only.
	int mode = 0;               // current index into Modes
	int ceiling = 0;            // best mode that fitted in memory this session
	uint32_t lastFrameMs = 0;
	uint32_t avgSendMs = 0;     // moving average of frame send time
	uint32_t slowFrames = 0;
	uint32_t fastFrames = 0;
	uint32_t frames = 0;
	uint32_t badFrames = 0;     // truncated JPEGs (no end marker) in the current window
	uint32_t missedFrames = 0;  // getFrame() failures in a row (each one is a ~4 s driver timeout)
	uint32_t checkedFrames = 0;

	void loop();
	bool enterMode(int index);   // switch the camera to Modes[index]; false if it didn't fit
	bool enterFrom(int start);   // first mode from `start` downwards that fits
	bool enterBestMode();        // enterFrom(ceiling)
	void restoreNormal();
	void leave(const char* reason);
	void adapt(uint32_t sendMs);
	static bool completeJpeg(const uint8_t* data, size_t len);
	void report(bool isOn, const char* reason);
};

#endif //BUTTERBOT_FIRMWARE_CAMERASTREAM_H
