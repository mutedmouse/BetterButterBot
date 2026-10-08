#ifndef BUTTERBOT_FIRMWARE_LEDSERVICE_H
#define BUTTERBOT_FIRMWARE_LEDSERVICE_H

#include "Entity/AsyncEntity.h"
#include "Services/LED/LED.h"
#include "Services/Settings.h"
#include "Enums.h"
#include <mutex>

class LEDModuleControl : public Object {
	GENERATED_BODY(LEDModuleControl, Object, CONSTRUCTOR_PACK(StrongObjectPtr<LED<MonoLED, RGBLED>>))
public:
	LEDModuleControl(StrongObjectPtr<LED<MonoLED, RGBLED>> ledService);

	void initSettings();

	// Module LED
	void turnLedOn();
	void turnLedOff();
	void turnLedStrobe();
	void turnLedBreathe();
	void turnLedFaster();
	void turnLedSlower();
	bool isLedOn() const;
	bool isAtMinSpeed() const;
	bool isAtMaxSpeed() const;

private:
	enum class ModulePattern : uint8_t { Off, On, Strobe, Breathe };

	StrongObjectPtr<LED<MonoLED, RGBLED>> ledService;

	// Module LED state
	ModulePattern modulePattern = ModulePattern::Off;
	float strobePeriod = StrobeDefaultPeriod;
	float breathePeriod = BreatheDefaultPeriod;
	std::mutex moduleMutex;

	// Module LED constants
	static constexpr float ModuleLedBrightness = 0.1f;
	static constexpr float StrobeDefaultPeriod = 0.4f;
	static constexpr float StrobeOnTime = 0.05f;
	static constexpr float StrobeMinPeriod = 0.1f;
	static constexpr float StrobeMaxPeriod = 2.0f;
	static constexpr float BreatheDefaultPeriod = 2.0f;
	static constexpr float BreatheMinPeriod = 0.5f;
	static constexpr float BreatheMaxPeriod = 8.0f;
	static constexpr float SpeedChangeFactor = 1.5f;

	void setSolidOn();
	void setBlink(float onTime, float offTime);
	void setStatusLedState(bool on);
	void applyStatusState();
	void applyStrobe();
	void applyBreathe();
};

#endif //BUTTERBOT_FIRMWARE_LEDSERVICE_H
