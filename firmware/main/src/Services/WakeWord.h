#ifndef BUTTERBOT_FIRMWARE_WAKEWORD_H
#define BUTTERBOT_FIRMWARE_WAKEWORD_H

#include <array>
#include <atomic>
#include <cstdint>
#include <esp_partition.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include "frontend.h"
#include "Object/Object.h"
#include "Event/EventBroadcaster.h"

/**
 * @brief Always-on "butterbot" wake word (a microWakeWord streaming model).
 *
 * Reads the mic in 10 ms chunks, turns them into 40-channel log-mel features with the TFLM
 * microfrontend, runs the streaming model every `stride` features and fires OnWake when the
 * average probability over the last few runs crosses the cutoff.
 *
 * The model is memory-mapped from the "wakeword" flash partition (flash it separately; without it
 * the service stays disabled). Arena, feature state and task stack all live in PSRAM. The mic is
 * shared, so ListenState pauses detection while it records, and nothing is detected while the
 * robot itself is playing audio.
 */
class WakeWord : public Object {
	GENERATED_BODY(WakeWord, Object, void);
public:
	WakeWord();
	~WakeWord() override;

	bool available() const { return interpreter != nullptr; }

	/**
	 * Stop reading the mic (blocks until the task has let go of it). Nestable with resume().
	 * Starts paused: IdleState resumes it on entry and pauses it on exit, so every other state
	 * (listening, mic-using routines, RC) has the mic to itself.
	 */
	void pause();
	void resume();

	/**
	 * Detection on someone else's mic stream while this service is paused (e.g. ListenState's
	 * follow-up window, which owns the mic). beginExternal() resets state; feedExternal() takes
	 * 16 kHz mono int16 and may fire OnWake. Ignored unless the internal task is parked.
	 */
	void beginExternal();
	void feedExternal(const int16_t* mono, size_t count);

	DECLARE_EVENT(WakeEvent, WakeWord);
	WakeEvent OnWake{ this };

private:
	static constexpr size_t SamplesPer10ms = 160;
	static constexpr int Features = 40;
	static constexpr size_t MaxWindow = 16;
	static constexpr uint32_t LockoutMs = 1500;          // after a detection
	static constexpr uint32_t SelfAudioLockoutMs = 1000; // after our own audio / returning to idle
	                                                     // (speaker + mic buffer tails are ~0.3 s)

	bool loadModel();
	static void taskEntry(void* arg);
	void loop();
	void process(const int16_t* mono, size_t count);
	void resetDetector();

	const esp_partition_t* partition = nullptr;
	esp_partition_mmap_handle_t mapHandle = 0;
	const uint8_t* modelData = nullptr;

	tflite::MicroMutableOpResolver<20> resolver;
	tflite::MicroInterpreter* interpreter = nullptr; // placement-new into PSRAM
	uint8_t* interpreterMem = nullptr;
	uint8_t* arena = nullptr;
	uint8_t* varArena = nullptr;
	size_t arenaSize = 0;

	FrontendState* frontend = nullptr;
	int16_t* micBuf = nullptr;

	int stride = 3;
	int strideStep = 0;
	std::array<uint8_t, MaxWindow> probs{};
	size_t probIdx = 0;
	size_t window = 5;
	uint8_t cutoff = 247;
	uint32_t lockoutUntil = 0;

	TaskHandle_t task = nullptr;
	std::atomic<int> pauseDepth{ 1 }; // starts paused; IdleState resumes it
	std::atomic<bool> parked{ false };
	std::atomic<bool> stopping{ false };

};

#endif //BUTTERBOT_FIRMWARE_WAKEWORD_H
