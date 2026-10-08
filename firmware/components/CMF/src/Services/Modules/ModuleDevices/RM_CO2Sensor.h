#ifndef CMF_RM_CO2SENSOR_H
#define CMF_RM_CO2SENSOR_H

#include "Object/Class.h"
#include "Services/Modules/ModuleDevice.h"
#include "Services/ADCReader.h"

class RM_CO2Sensor : public ModuleDevice{
	GENERATED_BODY(RM_CO2Sensor, ModuleDevice, CONSTRUCTOR_PACK(const Modules::BusPins&))

public:
	RM_CO2Sensor(const Modules::BusPins& pins = {});
	~RM_CO2Sensor() noexcept override;

	uint32_t getReading();

	bool isOK();
private:
	static constexpr uint32_t OKThreshold = 1100;
	StrongObjectPtr<ADCReader> adc_pin;
};


#endif //CMF_RM_CO2SENSOR_H
