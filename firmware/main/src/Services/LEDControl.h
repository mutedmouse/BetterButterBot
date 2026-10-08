#ifndef BUTTERBOT_FIRMWARE_LEDCONTROL_H
#define BUTTERBOT_FIRMWARE_LEDCONTROL_H

#include "Services/LED/LED.h"
#include <mutex>
#include "Enums.h"
#include "Services/Audio/Audio.h"

class LEDControl{
public:
	LEDControl(LED<MonoLED, RGBLED>* ledService);
	~LEDControl();

	void registerToAudio(Audio* audio);

	void startIdle();
	void startInit();
	void startBatteryCritical(bool continuous = false);
	bool waitForBatteryCritical(TickType_t wait = portMAX_DELAY) const;
	void startPlay();
	void stopPlay();
	void startListen();
	void stopListen();

private:
	enum class Pattern : uint8_t {
		Off,
		On,
		Blink,
		BattCritical
	};

	LED<MonoLED, RGBLED>* ledService;
	Audio* audioService = nullptr;
	Pattern pattern = Pattern::Off;
	float blinkOnTime = 0.0f;
	float blinkOffTime = 0.0f;
	bool playing = false;
	bool listening = false;
	std::mutex stateMutex;

	static constexpr uint32_t TickIntervalMs = 10;
	static constexpr float FastBlinkOnTime = 0.05f;
	static constexpr float FastBlinkOffTime = 0.30f;
	static constexpr float SlowBlinkOnTime = 0.05f;
	static constexpr float SlowBlinkOffTime = 1.0f;
	static constexpr uint32_t BattCriticalBlinkOnTime = 200; // [ms]
	static constexpr uint32_t BattCriticalBlinkOffTime = 300; // [ms]
	static constexpr float LedBrightness = 1.0f;

	void setSolidOn();
	void setBlink(float onTime, float offTime);
	void setLedState(bool on) const;
	void applyState();

	void onAudioPlayStatusChanged(bool value);
};

#endif //BUTTERBOT_FIRMWARE_LEDCONTROL_H