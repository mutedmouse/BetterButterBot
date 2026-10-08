#ifndef CMF_LEDFADEFUNCTION_H
#define CMF_LEDFADEFUNCTION_H

#include "LED.h"

template<typename LED, typename DataT>
class LEDFadeFunction : public LEDFunction<LED, DataT> {
	using Super = LEDFunction<LED, DataT>;
public:
	LEDFadeFunction(DataT start, DataT end, float period, uint32_t loopsCount, float startDelay = 0.0f) :
			start(start), end(end), period(period), count(loopsCount), startDelay(startDelay){}

	DataT getValue() const noexcept override{
		if(!delayCleared && startDelay > 0.0f) return DataT{};

		const float t = -0.5 * cos(2 * M_PI * timeCounter / period) + 0.5;

		return start * (1 - t) + end * t;
	}

	bool isDone() const noexcept override{
		if(count != 0 && elapsedCount >= count){
			return true;
		}

		return false;
	}

	virtual void tick(float deltaTime) noexcept override{
		Super::tick(deltaTime);

		timeCounter += deltaTime;

		if(!delayCleared && startDelay > 0.0f){
			if(timeCounter < startDelay){
				return;
			}
			delayCleared = true;
			timeCounter = 0;
		}

		if(timeCounter > period){
			elapsedCount++;
			timeCounter = fmod(timeCounter, period);
		}
	}

	virtual TickType_t getInterval() const noexcept override{
		if(!delayCleared && startDelay > 0.0f){
			return static_cast<TickType_t>(startDelay * 1000 / portTICK_PERIOD_MS);
		}

		return 0;
	}

private:
	DataT start;
	DataT end;
	float period;
	uint32_t count;
	uint32_t elapsedCount = 0;
	float startDelay = 0;
	bool delayCleared = false;
	float timeCounter;
};


#endif //CMF_LEDFADEFUNCTION_H
