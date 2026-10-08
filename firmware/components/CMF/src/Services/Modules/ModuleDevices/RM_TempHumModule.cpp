#include "RM_TempHumModule.h"
#include <Periphery/I2CDevice.h>
#include <Util/stdafx.h>

#include "Util/stdafx.h"

RM_TempHumModule::RM_TempHumModule(const Modules::BusPins& busPins) : ModuleDevice(Modules::Type::RM_TempHum, busPins){
	i2c = busPins.i2c->addDevice(Addr);
	assert(i2c != nullptr);
	ESP_ERROR_CHECK(i2c->write(0x00));
}

void RM_TempHumModule::sample(){
	const auto data = readData();
	temp = static_cast<int16_t>(getTemp(data));
	hum = static_cast<uint16_t>(getHumidity(data));
}

int16_t RM_TempHumModule::getTemperature() const{
	return temp;
}

uint8_t RM_TempHumModule::getHumidity() const{
	return hum;
}

//TODO - separate this out to its own Device class for AHT20
std::array<uint8_t, 6> RM_TempHumModule::readData(){
	std::array<uint8_t, 6> data{};
	constexpr std::array<uint8_t, 3> trigger{ 0xAC, 0x33, 0x00 };
	i2c->write(trigger);
	delayMillis(80);
	i2c->read(data);

	return data;
}

float RM_TempHumModule::getHumidity(const std::array<uint8_t, 6>& data){
	const int h = data[1] << 12 | data[2] << 4 | data[3] >> 4;
	const float hum = (h * 100.0f) / 0x100000;

	return hum;
}

float RM_TempHumModule::getTemp(const std::array<uint8_t, 6>& data){
	const int t = (data[3] & 0x0F) << 16 | data[4] << 8 | data[5];
	const float temp = (t * 200.0f / 0x100000) - 50;

	return temp;
}