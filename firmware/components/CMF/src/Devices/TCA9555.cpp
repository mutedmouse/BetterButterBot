#include "TCA9555.h"
#include "Core/Application.h"
#include "Log/Log.h"
#include "Periphery/I2CMaster.h"
#include "Periphery/I2CDevice.h"
#include <array>

static inline uint8_t TCA_IT(uint8_t pin){ return (pin <= 7) ? 0 : 1; }

static inline uint8_t TCA_REG(uint8_t reg, uint8_t pin){ return reg + TCA_IT(pin); }

static inline uint8_t TCA_BIT(uint8_t pin){ return (pin <= 7) ? pin : (pin - 8); }

static inline uint8_t TCA_MASK(uint8_t pin){ return 1 << TCA_BIT(pin); }

DEFINE_LOG(TCA9555)


TCA9555::TCA9555(I2CMaster* i2c, uint8_t addr){
	if(!i2c){
		CMF_LOG(TCA9555, LogLevel::Error, "I2C master is null");
		abort();
	}

	dev = i2c->addDevice(addr);
	if(!dev){
		CMF_LOG(TCA9555, LogLevel::Error, "I2C device is null");
		abort();
	}

	reset();
}

void TCA9555::reset(){
	std::lock_guard lock(mutex);

	constexpr std::array<uint8_t, 3> dirReset{ REG_DIR, 0xff, 0xff };
	constexpr std::array<uint8_t, 3> polarityReset{ REG_POLARITY, 0x00, 0x00 };
	dev->write(dirReset); // All pins to input
	dev->write(polarityReset); // Turn off polarity inversion for all pins
	regs = Regs();
}

void TCA9555::pinMode(uint8_t pin, TCA9555::PinMode mode){
	if(pin >= 16) return;

	std::lock_guard lock(mutex);

	const uint8_t it = TCA_IT(pin);
	const uint8_t mask = TCA_MASK(pin);
	const uint8_t regDir = TCA_REG(REG_DIR, pin);
	uint8_t& intRegDir = regs.dir[it];

	if(mode == PinMode::OUT){
		intRegDir = intRegDir & ~mask;
	}else{
		intRegDir = intRegDir | mask;
	}

	ESP_ERROR_CHECK(dev->writeRegister(regDir, intRegDir));
}

bool TCA9555::read(uint8_t pin){
	if(pin >= 16) return false;

	std::lock_guard lock(mutex);

	const uint8_t reg = TCA_REG(REG_INPUT, pin);

	uint8_t regVal;
	ESP_ERROR_CHECK(dev->readRegister(reg, regVal));

	return regVal & TCA_MASK(pin);
}

uint16_t TCA9555::readAll(){
	std::lock_guard lock(mutex);

	std::array<uint8_t, 2> val{};
	ESP_ERROR_CHECK(dev->write_read(REG_INPUT, val));
	return (val[1] << 8) | val[0];
}

void TCA9555::write(uint8_t pin, bool state){
	if(pin >= 16) return;

	std::lock_guard lock(mutex);

	const uint8_t reg = TCA_REG(REG_OUTPUT, pin);
	const uint8_t mask = TCA_MASK(pin);
	uint8_t& intReg = regs.output[TCA_IT(pin)];

	if(state){
		intReg = intReg | mask;
	}else{
		intReg = intReg & ~mask;
	}

	ESP_ERROR_CHECK(dev->writeRegister(reg, intReg));

}

uint16_t TCA9555::readRegister16(uint8_t reg){
	std::lock_guard lock(mutex);

	std::array<uint8_t, 2> val{};
	if(dev->write_read(reg, val) != ESP_OK){
		return 0xdead;
	}
	return (val[1] << 8) | val[0];
}
