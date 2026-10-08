#ifndef CMF_RM_TEMPHUMMODULE_H
#define CMF_RM_TEMPHUMMODULE_H

#include "Object/Class.h"
#include "Services/Modules/ModuleDevice.h"

class RM_TempHumModule : public ModuleDevice {
	GENERATED_BODY(RM_TempHumModule, ModuleDevice, CONSTRUCTOR_PACK(const Modules::BusPins&))

public:
	RM_TempHumModule(const Modules::BusPins& busPins = {});

	/**
	 * Fetches new data from sensor
	 */
	void sample();

	/**
	 * Returns last read temperature reading
	 * @return Temperature in degrees celsius [°C]
	 */
	int16_t getTemperature() const;

	/**
	 * Returns last read humidity reading
	 * @return Humidity in percent [%]
	 */
	uint8_t getHumidity() const;


private:
	static constexpr uint8_t Addr = 0x38;
	std::unique_ptr<I2CDevice> i2c;

	int16_t temp = 0;
	uint8_t hum = 0;

	std::array<uint8_t, 6> readData();

	static float getHumidity(const std::array<uint8_t, 6>& data);

	static float getTemp(const std::array<uint8_t, 6>& data);
};


#endif //CMF_RM_TEMPHUMMODULE_H
