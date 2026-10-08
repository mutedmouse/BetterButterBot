#include "ADCUnit.h"
#include "Log/Log.h"
#include "Memory/ObjectMemory.h"

DEFINE_LOG(ADCUnit)

StrongObjectPtr<ADCUnit> ADCUnit::units[SOC_ADC_PERIPH_NUM] = { nullptr };

ADCUnit* ADCUnit::getADCUnit(adc_unit_t unit){
	if(!units[unit].isValid()){
		units[unit] = newObject<ADCUnit>(nullptr, unit);
	}

	return units[unit].get();
}

ADCUnit* ADCUnit::getADCUnit(gpio_num_t gpio){
	adc_channel_t chan;
	adc_unit_t unit;
	auto err = adc_oneshot_io_to_channel(gpio, &unit, &chan);
	if(err == ESP_ERR_NOT_FOUND){
		CMF_LOG(ADCUnit, Error, "GPIO %d is not a valid ADC pin", gpio);
	} else if(err != ESP_OK){
		CMF_LOG(ADCUnit, Error, "Invalid getADCUnit argument");
	}
	return getADCUnit(unit);
}

ADCUnit::ADCUnit(adc_unit_t unit) : Super(), unit(unit){
	if(units[unit].isValid()){
		CMF_LOG(CMF, LogLevel::Error, "Attempted to create an ADC unit %d that has already been initialized in the past. "
		        "'getADCUnit' function should be used instead of creating ones own ADCUnit.", (int)unit);
		return;
	}

	const adc_oneshot_unit_init_cfg_t config = {
		.unit_id = unit,
		.clk_src = ADC_RTC_CLK_SRC_DEFAULT,
		.ulp_mode = ADC_ULP_MODE_DISABLE
	};

	ESP_ERROR_CHECK(adc_oneshot_new_unit(&config, &hndl));
}

ADCUnit::~ADCUnit(){
	ESP_ERROR_CHECK(adc_oneshot_del_unit(hndl));
}

void ADCUnit::config(adc_channel_t chan, const adc_oneshot_chan_cfg_t& cfg){
	std::lock_guard lock(mutex);
	ESP_ERROR_CHECK(adc_oneshot_config_channel(hndl, chan, &cfg));
}

adc_unit_t ADCUnit::getUnitID() const{
	return unit;
}

esp_err_t ADCUnit::read(adc_channel_t chan, int& valueOut, const adc_cali_handle_t cali) const{
	std::lock_guard lock(mutex);
	if(cali != nullptr){
		return adc_oneshot_get_calibrated_result(hndl, cali, chan, &valueOut);
	} else{
		return adc_oneshot_read(hndl, chan, &valueOut);
	}
}

void ADCUnit::reinit(){
	std::lock_guard lock(mutex);
	ESP_ERROR_CHECK(adc_oneshot_del_unit(hndl));
	const adc_oneshot_unit_init_cfg_t config = {
		.unit_id = unit,
		.clk_src = ADC_RTC_CLK_SRC_DEFAULT,
		.ulp_mode = ADC_ULP_MODE_DISABLE
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&config, &hndl));
}
