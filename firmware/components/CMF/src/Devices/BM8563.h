#ifndef BUTTERBOT_FIRMWARE_BM8563_H
#define BUTTERBOT_FIRMWARE_BM8563_H

#include "Object/Object.h"
#include "Periphery/I2CDevice.h"
#include "Object/Class.h"

/**
 * BM8563 - RTC clock/calendar)
 * (https://www.makerguides.com/wp-content/uploads/2025/01/BM8563-datasheet.pdf)
 */

class BM8563 : public Object {
	GENERATED_BODY(BM8563, Object, CONSTRUCTOR_PACK(I2CMaster*, uint8_t))
public:
	explicit BM8563(I2CMaster* i2c, uint8_t Addr = 0x51);

	tm getTime();
	void setTime(const tm& time);

private:
	std::unique_ptr<I2CDevice> dev;
	uint8_t Addr = 0x51;

	uint8_t bcd2dec(uint8_t bcd);
	uint8_t dec2bcd(uint8_t dec);
};


#endif //BUTTERBOT_FIRMWARE_BM8563_H
