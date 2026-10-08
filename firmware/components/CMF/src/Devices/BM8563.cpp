#include "BM8563.h"
#include "Log/Log.h"
#include "Periphery/I2CMaster.h"
#include <array>

DEFINE_LOG(BM8563)

BM8563::BM8563(I2CMaster* i2c, uint8_t Addr) : Addr(Addr){
	if(!i2c){
		CMF_LOG(BM8563, LogLevel::Error, "No I2C peripheral provided");
		abort();
	}

	dev = i2c->addDevice(Addr);
	if(!dev){
		CMF_LOG(BM8563, LogLevel::Error, "I2C device is null");
		abort();
	}

	// Clear status registers (0x0 and 0x1)
	constexpr std::array<uint8_t, 3> clear{ 0, 0, 0 };
	if(const auto ret = dev->write(clear, 10); ret != ESP_OK){
		CMF_LOG(BM8563, LogLevel::Error, "Error clearing status registers: %d", ret);
		abort();
	}
}

tm BM8563::getTime(){
	uint8_t data[7];
	constexpr uint8_t writeData = 0x02;
	if(dev->write_read(&writeData, 1, data, 7) != ESP_OK){
		CMF_LOG(BM8563, LogLevel::Warning, "Failed reading time registers, returning zero time");
		return {};
	}

	tm time = {};

	/* 0..59 */
	time.tm_sec = bcd2dec(data[0] & 0b01111111);

	/* 0..59 */
	time.tm_min = bcd2dec(data[1] & 0b01111111);

	/* 0..23 */
	time.tm_hour = bcd2dec(data[2] & 0b00111111);

	/* 1..31 */
	time.tm_mday = bcd2dec(data[3] & 0b00111111);

	/* 0..6 */
	time.tm_wday = bcd2dec(data[4] & 0b00000111);

	/* 0..11 */
	time.tm_mon = bcd2dec(data[5] & 0b00011111) - 1;

	/* If century bit set assume it is 2000. Note that it seems */
	/* that unlike PCF8563, the BM8563 does NOT automatically */
	/* toggle the century bit when year overflows from 99 to 00. */
	const uint8_t century = (data[5] & 0b10000000) ? 100 : 0;

	/* Number of years since 1900 */
	time.tm_year = bcd2dec(data[6] & 0b11111111) + century;

	/* Calculate tm_yday. */
	mktime(&time);

	return time;
}

void BM8563::setTime(const tm& time){
	uint8_t data[7] = { 0 };

	/* 0..59 */
	data[0] = dec2bcd(time.tm_sec) & 0b01111111;

	/* 0..59 */
	data[1] = dec2bcd(time.tm_min) & 0b01111111;

	/* 0..23 */
	data[2] = dec2bcd(time.tm_hour) & 0b00111111;

	/* 1..31 */
	data[3] = dec2bcd(time.tm_mday) & 0b00111111;

	/* 0..6 */
	data[4] = dec2bcd(time.tm_wday) & 0b00000111;

	/* 1..12 */
	data[5] = dec2bcd(time.tm_mon + 1) & 0b00011111;

	/* If 2000 set the century bit. */
	if(time.tm_year >= 100){
		data[5] |= 0b10000000;
	}

	/* 0..99 */
	data[6] = dec2bcd(time.tm_year % 100) & 0b11111111;

	uint8_t wdata[8] = { 0x02 };
	memcpy(wdata + 1, data, 7);
	dev->write(wdata, 8);
}

uint8_t BM8563::bcd2dec(uint8_t bcd){
	return (((bcd >> 4) * 10) + (bcd & 0x0f));
}

uint8_t BM8563::dec2bcd(uint8_t dec){
	return (((dec / 10) << 4) | (dec % 10));
}
