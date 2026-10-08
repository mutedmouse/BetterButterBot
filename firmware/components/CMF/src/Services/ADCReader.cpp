#include "ADCReader.h"
#include <algorithm>
#include <utility>
#include "Log/Log.h"

DEFINE_LOG(ADCReader)

ADCReader::ADCReader(gpio_num_t gpio, adc_oneshot_chan_cfg_t config, bool calibration, StrongObjectPtr<ADCFilter> filter) : filter(std::move(filter)){
	adc_unit_t unit;
	adc_channel_t chan;
	auto err = adc_oneshot_io_to_channel(gpio, &unit, &chan);
	if(err == ESP_ERR_NOT_FOUND){
		CMF_LOG(ADCReader, Error, "GPIO %d is not a valid ADC pin", gpio);
	}else if(err != ESP_OK){
		CMF_LOG(ADCReader, Error, "ADCReader error");
	}

	this->chan = chan;
	adc = ADCUnit::getADCUnit(unit);
	adc->config(chan, config);

	if(calibration){
		esp_err_t ret = ESP_FAIL;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
		CMF_LOG(ADCReader, Info, "calibration scheme version is %s", "Curve Fitting");
		adc_cali_curve_fitting_config_t cali_config = {
				.unit_id = unit,
				.chan = chan,
				.atten = config.atten,
				.bitwidth = config.bitwidth,
		};
		ret = adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
		CMF_LOG(ADCReader, Info, "calibration scheme version is %s", "Line Fitting");
		adc_cali_line_fitting_config_t cali_config = {
			.unit_id = unit,
			.atten = config.atten,
			.bitwidth = config.bitwidth,
		};
		ret = adc_cali_create_scheme_line_fitting(&cali_config, &cali_handle);
#endif
		if(ret != ESP_OK){
			CMF_LOG(ADCReader, Warning, "Calibration failed");
		}
	}
}

ADCReader::~ADCReader(){
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
	adc_cali_delete_scheme_curve_fitting(cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
	adc_cali_delete_scheme_line_fitting(cali_handle);
#endif
}

float ADCReader::sample(){
	int raw = 0;
	if(adc->read(chan, raw, cali_handle) != ESP_OK){
		return getValue();
	}

	value = raw;

	return getValue();
}

float ADCReader::getValue() const{
	if(filter.isValid()){
		return filter->apply(value);
	}

	return value;
}
