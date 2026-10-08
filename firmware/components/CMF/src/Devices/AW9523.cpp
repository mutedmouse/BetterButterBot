#include "AW9523.h"
#include "Util/stdafx.h"
#include "Log/Log.h"
#include <Core/Application.h>
#define REG_RESET 0x7F
#define REG_ID 0x10
#define REG_CONF 0x11
#define REG_INPUT 0x00
#define REG_OUTPUT 0x02
#define REG_DIR 0x04
#define REG_INTR 0x06
#define REG_MODE 0x12
#define REG_DIM 0x20

#define VAL_RESET 0x00
#define VAL_ID 0x23

#define CFG_MASK (0b00010011)

static uint8_t AW_IT(uint8_t pin){ return (pin <= 7) ? 0 : 1; }

static uint8_t AW_REG(uint8_t reg, uint8_t pin){ return reg + AW_IT(pin); }

static uint8_t AW_BIT(uint8_t pin){ return (pin <= 7) ? pin : (pin - 8); }

static uint8_t AW_MASK(uint8_t pin){ return 1 << AW_BIT(pin); }

DEFINE_LOG(AW9523)

const uint8_t AW9523::dimmap[16] = { 4, 5, 6, 7, 8, 9, 10, 11, 0, 1, 2, 3, 12, 13, 14, 15 };

AW9523::AW9523(I2CMaster* i2c, uint8_t addr){
	if(i2c == nullptr){
		CMF_LOG(AW9523, LogLevel::Error, "I2C master is null, aborting initialization");
		return;
	}

	const Application* app = getApp();
	if(app == nullptr){
		CMF_LOG(AW9523, LogLevel::Error, "Application instance unavailable, aborting initialization");
		return;
	}

	dev = std::move(i2c->addDevice(addr));

	if(!dev){
		CMF_LOG(AW9523, LogLevel::Error, "I2C device is null");
		abort();
	}

	uint8_t id = 0;
	if(dev->readRegister(REG_ID, id) != ESP_OK){
		CMF_LOG(AW9523, LogLevel::Error, "Transmission error reading ID register");
		abort();
	}

	if(id != VAL_ID){
		CMF_LOG(AW9523, LogLevel::Error, "ID missmatch: expected %d, got %d", VAL_ID, id);
		return;
	}

	// Reset
	writeReg(REG_RESET, VAL_RESET);
	regs = Regs();

	delayMicros(50);

	// All input
	regs.dir[0] = regs.dir[1] = 0xff;
	writeReg(REG_DIR, regs.dir, 2);

	// Push-pull mode for port 0
	regs.conf |= 0b00010000;
	writeReg(REG_CONF, regs.conf);

	// Disable interrupts
	regs.intr[0] = regs.intr[1] = 0xff;
	writeReg(REG_INTR, regs.intr, 2);
}

void AW9523::resetDimOutputs(){
	for(int i = 0; i < 16; ++i){
		dim(i, 0);
	}
}

void AW9523::pinMode(uint8_t pin, AW9523::PinMode mode){
	if(pin >= 16) return;

	const uint8_t it = AW_IT(pin);
	const uint8_t mask = AW_MASK(pin);
	const uint8_t regDir = AW_REG(REG_DIR, pin);
	const uint8_t regMode = AW_REG(REG_MODE, pin);
	uint8_t& intRegDir = regs.dir[it];
	uint8_t& intRegMode = regs.mode[it];

	if(mode == LED){
		intRegMode = intRegMode & ~mask;
		writeReg(regMode, intRegMode);
	}else{
		intRegMode = intRegMode | mask;
		writeReg(regMode, intRegMode);

		if(mode == OUT){
			intRegDir = intRegDir & ~mask;
		}else{
			intRegDir = intRegDir | mask;
		}

		writeReg(regDir, intRegDir);
	}
}

bool AW9523::read(uint8_t pin){
	if(pin >= 16) return false;

	const uint8_t reg = AW_REG(REG_INPUT, pin);
	return readReg(reg) & AW_MASK(pin);
}


uint16_t AW9523::readAll(){
	const auto regL = AW_REG(REG_INPUT, 0);
	const auto regH = AW_REG(REG_INPUT, 15);

	return (readReg(regH) << 8) | readReg(regL);
}

void AW9523::write(uint8_t pin, bool state){
	if(pin >= 16) return;

	const uint8_t reg = AW_REG(REG_OUTPUT, pin);
	const uint8_t mask = AW_MASK(pin);
	uint8_t& intReg = regs.output[AW_IT(pin)];

	if(state){
		intReg |= mask;
	}else{
		intReg &= ~mask;
	}

	writeReg(reg, intReg);
}

void AW9523::dim(uint8_t pin, uint8_t factor){
	if(pin >= 16) return;

	pin = dimmap[pin];
	regs.dim[pin] = factor;
	writeReg(REG_DIM + pin, factor);
}

void AW9523::setInterrupt(uint8_t pin, bool enabled){
	if(pin >= 16) return;

	const uint8_t reg = AW_REG(REG_INTR, pin);
	const uint8_t mask = AW_MASK(pin);
	uint8_t& intReg = regs.intr[AW_IT(pin)];

	if(enabled){
		intReg |= mask;
	}else{
		intReg &= ~mask;
	}

	writeReg(reg, intReg);
}

void AW9523::setCurrentLimit(AW9523::CurrentLimit limit){
	const uint8_t mask = 0b00000011;
	uint8_t& conf = regs.conf;
	conf = (conf & ~mask) | (limit & mask);
	writeReg(REG_CONF, conf & CFG_MASK);
}

uint8_t AW9523::readReg(uint8_t reg) const{
	uint8_t data;
	ESP_ERROR_CHECK(dev->readRegister(reg, data));
	return data;
}

void AW9523::writeReg(uint8_t reg, const uint8_t* data, size_t size) const{
	ESP_ERROR_CHECK(dev->writeRegister(reg, data, size));
}

void AW9523::writeReg(uint8_t reg, uint8_t data) const{
	ESP_ERROR_CHECK(dev->writeRegister(reg, data));
}
