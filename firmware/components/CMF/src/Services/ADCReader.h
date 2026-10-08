#ifndef CMF_ADCREADER_H
#define CMF_ADCREADER_H

#include "Object/Class.h"
#include "Periphery/ADCUnit.h"
#include "driver/gpio.h"
#include "Object/Interface.h"
#include "Log/Log.h"

class ADCFilter : public Object {
	GENERATED_BODY(ADCFilter, Object, void)
public:
	virtual float apply(float sample){
		return sample;
	};
};

class EMA_ADCFilter : public ADCFilter {
	GENERATED_BODY(EMA_ADCFilter, ADCFilter, CONSTRUCTOR_PACK(float))
public:
	EMA_ADCFilter(float factor = 1.0) : factor(factor){};

	float apply(float sample) override{
		if(last == -1){
			last = sample;
			return last;
		}

		const auto val = factor * sample + (1.0f - factor) * last;
		last = val;
		return val;
	}

	/**
	 * Resets EMA filter and takes a fresh sample on the next call.
	 */
	void reset(){
		last = -1;
	}

	void setFactor(float factor){
		this->factor = factor;
	}

private:
	float factor;
	float last = -1;
};

class ADCReader : public Object {
	GENERATED_BODY(ADCReader, Object, CONSTRUCTOR_PACK(gpio_num_t, adc_oneshot_chan_cfg_t, bool, StrongObjectPtr<ADCFilter>))

public:
	/**
	 * Calibration is first applied (if present), then ADCFilter.
	 * @param gpio GPIO pin
	 * @param cali Turn on/off internal ESP calibration
	 * @param filter optional filter applied after reading, must be an ADCFilter implementation
	 */
	ADCReader(gpio_num_t gpio = GPIO_NUM_NC, adc_oneshot_chan_cfg_t config = {}, bool calibration = false, StrongObjectPtr<ADCFilter> filter = nullptr);
	~ADCReader() override;

	/** Sample and return new value. */
	float sample();

	/** Return current value without sampling. */
	float getValue() const;

	void reinit();

private:
	StrongObjectPtr<ADCUnit> adc;
	adc_channel_t chan;
	StrongObjectPtr<ADCFilter> filter;
	adc_cali_handle_t cali_handle = nullptr;

	float value = -1.0f;

};


#endif //CMF_ADCREADER_H
