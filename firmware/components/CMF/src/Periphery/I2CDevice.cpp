#include "I2CDevice.h"
#include "I2CMaster.h"
#include <driver/i2c_master.h>

I2CDevice::I2CDevice(I2CMaster* master, i2c_master_dev_handle_t hndl) : master(master), hndl(hndl){

}

I2CDevice::~I2CDevice(){
	i2c_master_bus_rm_device(hndl);
}

esp_err_t I2CDevice::write(std::span<const uint8_t> data, TickType_t wait){
	return write(data.data(), data.size(), wait);
}

esp_err_t I2CDevice::write(uint8_t data, TickType_t wait){
	return write(&data, 1, wait);
}

esp_err_t I2CDevice::write(const uint8_t* data, size_t size, TickType_t wait){
	return master->write(*this, data, size, wait);
}

esp_err_t I2CDevice::read(std::span<uint8_t> data, TickType_t wait){
	return read(data.data(), data.size(), wait);
}

esp_err_t I2CDevice::read(uint8_t& data, TickType_t wait){
	return read(&data, 1, wait);
}

esp_err_t I2CDevice::read(uint8_t* data, size_t size, TickType_t wait){
	return master->read(*this, data, size, wait);
}

esp_err_t I2CDevice::write_read(std::span<const uint8_t> writeData, std::span<uint8_t> readData, TickType_t wait){
	return write_read(writeData.data(), writeData.size(), readData.data(), readData.size(), wait);
}

esp_err_t I2CDevice::write_read(uint8_t writeData, std::span<uint8_t> readData, TickType_t wait){
	return write_read(&writeData, 1, readData.data(), readData.size(), wait);
}

esp_err_t I2CDevice::write_read(std::span<const uint8_t> writeData, uint8_t& readData, TickType_t wait){
	return write_read(writeData.data(), writeData.size(), &readData, 1, wait);
}

esp_err_t I2CDevice::write_read(uint8_t writeData, uint8_t& readData, TickType_t wait){
	return write_read(&writeData, 1, &readData, 1, wait);
}

esp_err_t I2CDevice::write_read(const uint8_t* writeData, size_t writeSize, uint8_t* readData, size_t readSize, TickType_t wait){
	return master->write_read(*this, writeData, writeSize, readData, readSize, wait);
}

esp_err_t I2CDevice::writeRegister(uint8_t reg, std::span<const uint8_t> data, TickType_t wait){
	return writeRegister(reg, data.data(), data.size(), wait);
}

esp_err_t I2CDevice::writeRegister(uint8_t reg, uint8_t data, TickType_t wait){
	return writeRegister(reg, &data, 1, wait);
}

esp_err_t I2CDevice::writeRegister(uint8_t reg, const uint8_t* data, size_t size, TickType_t wait){
	return master->writeRegister(*this, reg, data, size, wait);
}

esp_err_t I2CDevice::readRegister(uint8_t reg, std::span<uint8_t> data, TickType_t wait){
	return readRegister(reg, data.data(), data.size(), wait);
}

esp_err_t I2CDevice::readRegister(uint8_t reg, uint8_t& data, TickType_t wait){
	return readRegister(reg, &data, 1, wait);
}

esp_err_t I2CDevice::readRegister(uint8_t reg, uint8_t* data, size_t size, TickType_t wait){
	return write_read(&reg, 1, data, size, wait);
}
