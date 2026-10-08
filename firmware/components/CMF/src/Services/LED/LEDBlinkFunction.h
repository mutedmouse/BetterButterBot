#ifndef CMF_LEDBLINKFUNCTION_H
#define CMF_LEDBLINKFUNCTION_H

#include "LED.h"
#include <Drivers/Interface/InputDriver.h>

template<typename LED, typename DataT>
class LEDBlinkFunction : public LEDFunction<LED, DataT> {
	using Super = LEDFunction<LED, DataT>;

public:
	/**
	 * @param value - ON value for blinking
	 * @param period - blink period in seconds
	 * @param onTime - blink duration in seconds, will be repeated every period, must be less than period!
	 * @param count - number of blink cycles to be repeated, or 0 for infinite blinks
	 */
	LEDBlinkFunction(DataT value, float period, float onTime, uint32_t count = 0) : value(value), count(count), period(period), onTime(onTime){
		if(onTime >= period){
			CMF_LOG(CMF, LogLevel::Error, "Blink on-time must be less than period!");
			invalidConfig = true;
		}
	}

	DataT getValue() const noexcept override{
		if(state) return value;

		return DataT{};
	}

	bool isDone() const noexcept override{
		if(count != 0 && elapsedCount >= count){
			return true;
		}

		return false;
	}

	virtual TickType_t getInterval() const noexcept override{
		if(!Super::hasBegun() || !started){
			return 0;
		}

		const float phase = fmodf(timer, period);
		const float untilNext = (phase <= onTime) ? (onTime - phase) : (period - phase);

		return std::max<TickType_t>(1, static_cast<TickType_t>(untilNext * 1000.0f)) / portTICK_PERIOD_MS;
	}

private:
	virtual void tick(float deltaTime) noexcept override{
		if(invalidConfig) return;

		// Track elapsed time via an absolute timestamp, instead of accumulated deltaTime
		// Reason: the service sleeps while idle, so the first deltaTime after waking would jump past intended count
		if(!started){
			started = true;
			startTime = millis();
		}

		timer = (millis() - startTime) / 1000.0f;

		state = false;
		if(fmodf(timer, period) <= onTime){
			state = true;
		}

		elapsedCount = timer / period;
	}

	DataT value;
	uint32_t count;
	float period;
	float onTime;

	uint64_t startTime = 0;
	bool started = false;

	float timer = 0;
	uint32_t elapsedCount = 0;
	bool state = false;

	bool invalidConfig = false;
};

#endif //CMF_LEDBLINKFUNCTION_H
