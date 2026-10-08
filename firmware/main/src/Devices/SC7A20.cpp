#include "SC7A20.h"
#include <Util/stdafx.h>

DEFINE_LOG(SC7A20)

SC7A20::SC7A20(I2CMaster* i2c, uint8_t addr) : Addr(addr){
	if(!i2c){
		CMF_LOG(SC7A20, LogLevel::Error, "No I2C peripheral provided");
		abort();
	}

	dev = i2c->addDevice(Addr);
	if(!dev){
		CMF_LOG(SC7A20, LogLevel::Error, "I2C device is null");
		abort();
	}

	init();
}

void SC7A20::startFIFO(){
	lis3dh_fifo_mode_set(&ctx, LIS3DH_STREAM_TO_FIFO_MODE);
	lis3dh_fifo_set(&ctx, 1);
}

void SC7A20::stopFIFO(){
	lis3dh_fifo_mode_set(&ctx, LIS3DH_BYPASS_MODE);
	lis3dh_fifo_set(&ctx, 0);
}

SC7A20::Sample SC7A20::getSample(){
	int16_t raw[3];
	auto ret = lis3dh_acceleration_raw_get(&ctx, raw);
	if(ret != 0){
		CMF_LOG(SC7A20, LogLevel::Error, "GetSample read error %d", ret);
		return {};
	}

	Sample sample{
		lis3dh_from_fs2_nm_to_mg(raw[0]) / 1000.0f,
		lis3dh_from_fs2_nm_to_mg(raw[1]) / 1000.0f,
		lis3dh_from_fs2_nm_to_mg(raw[2]) / 1000.0f
	};
	return sample;
}

uint8_t SC7A20::readFIFO(std::vector<Sample>& output){

	//Setting mode to FIFO stops streaming and updating the data mid-reading.
	lis3dh_fifo_mode_set(&ctx, LIS3DH_FIFO_MODE);
	uint8_t fifoNum;
	lis3dh_fifo_data_level_get(&ctx, &fifoNum);

	for(uint8_t i = 0; i < fifoNum; i++){
		output.emplace_back(getSample());
	}

	/**
	  * Note - necessary to restart fifo, from LIS3DH datasheet:
	  *
	  * After the last read it is necessary to exit Bypass mode in order to reset the FIFO content.
	  * After this reset command, it is possible to restart FIFO mode just by selecting the FIFO mode configuration
	  */
	lis3dh_fifo_mode_set(&ctx, LIS3DH_BYPASS_MODE);
	lis3dh_fifo_mode_set(&ctx, LIS3DH_STREAM_TO_FIFO_MODE);

	return fifoNum;
}

void SC7A20::init(){
	uint8_t id;
	ESP_ERROR_CHECK(lis3dh_device_id_get(&ctx, &id));

	if(id != SC7A20_ID){
		CMF_LOG(SC7A20, LogLevel::Error, "Init error, got ID 0x%x, expected 0x%x", id, SC7A20_ID);
		return;
	}

	ESP_ERROR_CHECK(lis3dh_block_data_update_set(&ctx, 1)); //for reading 2 byte long values

	lis3dh_int1_src_t src;
	ESP_ERROR_CHECK(lis3dh_int1_gen_source_get(&ctx, &src)); //Reading interrupt sources cleares interrupts


	ESP_ERROR_CHECK(lis3dh_operating_mode_set(&ctx, LIS3DH_NM_10bit));

	ESP_ERROR_CHECK(lis3dh_full_scale_set(&ctx, LIS3DH_2g));

	ESP_ERROR_CHECK(lis3dh_data_rate_set(&ctx, LIS3DH_ODR_100Hz));

	ESP_ERROR_CHECK(lis3dh_fifo_mode_set(&ctx, LIS3DH_BYPASS_MODE));

	delayMillis(5);
}

int32_t SC7A20::platform_write(void* hndl, uint8_t reg, const uint8_t* data, uint16_t len){
	auto imu = (SC7A20*)hndl;
	return imu->dev->writeRegister(reg, data, len);
}

int32_t SC7A20::platform_read(void* hndl, uint8_t reg, uint8_t* data, uint16_t len){
	auto imu = (SC7A20*)hndl;

	/**
	 * Excerpt from LIS3DH/SC7A20 datasheet:
	 *
	 * In order to read multiple bytes, it is necessary to assert the most significant bit of the subaddress field.
	 * In other words, SUB(7) must be equal to 1 while SUB(6-0) represents the address of first register to be read.
	 */
	if(len > 1){
		reg = reg | 0x80;
	}
	return imu->dev->readRegister(reg, data, len);
}
