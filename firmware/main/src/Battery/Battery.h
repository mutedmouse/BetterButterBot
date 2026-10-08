#ifndef BUTTERBOT_FIRMWARE_BATTERY_H
#define BUTTERBOT_FIRMWARE_BATTERY_H

#include <atomic>
#include <Entity/AsyncEntity.h>
#include <Services/ADCReader.h>
#include <Drivers/Interface/OutputDriver.h>
#include <mutex>
#include <memory>
#include <Event/EventBroadcaster.h>
#include "Util/Hysteresis.h"
#include "Devices/Timer.h"
#include "Services/BaseBoard.h"

class Battery : public Object {
	GENERATED_BODY(Battery, Object, CONSTRUCTOR_PACK(OutputPin))
public:
	Battery(OutputPin refSwitch);
	~Battery() override;

	// For translation between levels and battery percentage, check the HysteresisLimits array.
	enum class Level : uint8_t{
		Critical = 0, VeryLow, Low, Mid, High, VeryHigh, Full, COUNT
	};

	DECLARE_EVENT(LevelEvent, Battery, Level);

	LevelEvent OnLevelChanged{ this };

	DECLARE_EVENT(ChargeEvent, Battery, ChargingState)

	ChargeEvent OnChargeStatus{ this };

	bool isShutdown() const;

	float getPerc() const;
	Level getLevel() const;

	ChargingState getChargingState() const;

	void setSleep(bool sleep);

private:
	//Constants
	// VolFull & VoltEmpty values are not real mV values because of reading discrepancies to the actual power source. This is the easiest way to deal with it.
	static constexpr float VoltEmpty = 3550; // 3.7V
	static constexpr float VoltFull = 4100; // 4.2V
	static constexpr float Factor = 4.0f;
	static constexpr float Offset = 50;
	static constexpr float EmaA = 0.2f;
	static constexpr float EmaA_sleep = 0.5f;
	static constexpr int CalReads = 10;

	//Calibration
	static constexpr float CalExpected = 2500;

	/**
	 * Main battery reading thread. Will be periodically unblocked via a HW timer (Timer class).
	 */
	TaskHandle_t batThread;
	OutputPin refSwitch;
	void calibrate();

	//Reading
	static constexpr std::array HysteresisLimits = { 0, 4, 15, 30, 50, 70, 90, 100 };
	Hysteresis<std::size(HysteresisLimits)> hysteresis{ HysteresisLimits, 3 };

	static constexpr adc_oneshot_chan_cfg_t cfg = {
		.atten = ADC_ATTEN_DB_2_5,
		.bitwidth = ADC_BITWIDTH_12
	};
	StrongObjectPtr<ADCReader> readerBatt;

	// Battery percentage is computed inline (EMA smoothing -> voltage-divider conversion -> remap to %)
	// rather than through a chain of ADCFilter objects. Each ADCFilter was a full CMF Object, and the
	// per-Object overhead (event queue + semaphore + 2 mutexes + reference tracking) dwarfed the few
	// bytes of state these filters actually hold.
	float emaValue = -1.0f;     // EMA accumulator; < 0 means "seed from next sample" (fresh)
	float emaFactor = EmaA;     // smoothing factor, lowered to EmaA_sleep while sleeping
	float calibOffset = Offset; // mV offset applied after the divider factor, tuned by calibrate()
	float percent = 0.0f;       // last computed battery percentage [0-100]

	void sample(bool fresh = false);

	/** Applies EMA smoothing to a raw ADC reading [mV] and converts it to a battery percentage [0-100]. */
	float computePercent(float rawMillivolts, bool fresh);
	void tick();
	bool sleep = false;

	//Timer/sleep related
	static constexpr uint32_t ShortMeasureIntverval = 100;
	static constexpr uint32_t LongMeasureIntverval = 6000;

	std::mutex mut;

	std::atomic_bool abortFlag = false;

	static SemaphoreHandle_t sem;
	StrongObjectPtr<Timer> timer;
	void startTimer();


	//Charge detection
	ChargingState lastCharging = ChargingState::Unplugged;

	void chargeChanged(ChargingState newState);

	static void timerFunc(void*);

	/**
	 * Sometimes ADC will start having an offset during sleep and after wakeup.
	 * This method will be called every time the Battery class wakes up during sleep,
	 * and also after the final wakeup of the rest of the system.
	 *
	 * Necessary on certain HW revisions because of this ADC reading glitch after/during light sleep
	 * (https://github.com/espressif/esp-idf/issues/12612)
	 */
	void inSleepReconfigure();

	bool shutdown = false;
};

#endif //BUTTERBOT_FIRMWARE_BATTERY_H
