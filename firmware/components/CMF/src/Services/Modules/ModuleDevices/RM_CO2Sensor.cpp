#include "RM_CO2Sensor.h"
#include "Memory/ObjectMemory.h"

RM_CO2Sensor::RM_CO2Sensor(const Modules::BusPins& pins) : Super(Modules::Type::RM_CO2, pins){
	static constexpr adc_oneshot_chan_cfg_t cfg = {
			.atten = ADC_ATTEN_DB_12,
			.bitwidth = ADC_BITWIDTH_12
	};

	adc_pin = newObject<ADCReader>(this, static_cast<gpio_num_t>(pins.subAddressPins[5].inputPort), cfg, true);
}

RM_CO2Sensor::~RM_CO2Sensor() noexcept{
	if(adc_pin.isValid()){
		delete *adc_pin;
	}
}

uint32_t RM_CO2Sensor::getReading(){
	return static_cast<uint32_t>(adc_pin->sample());
}

bool RM_CO2Sensor::isOK(){
	return getReading() > OKThreshold;
}