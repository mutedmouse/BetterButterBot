#include "I2CMaster.h"
#include "I2CDevice.h"
#include <Log/Log.h>
#include <driver/i2c_master.h>

DEFINE_LOG(I2CMaster)

I2CMaster::I2CMaster(I2CPort port, gpio_num_t sda, gpio_num_t scl) noexcept : Super(), port(i2c_port_t(port)){
	if(port == I2CPort::None){
		CMF_LOG(I2CMaster, LogLevel::Error, "I2CMaster constructed with port=None, instance is unusable");
		return;
	}

	if(sda == GPIO_NUM_NC || scl == GPIO_NUM_NC){
		CMF_LOG(I2CMaster, LogLevel::Error, "I2CMaster constructed without SDA/SCL pins (sda=%d, scl=%d), instance is unusable", (int) sda, (int) scl);
		return;
	}

	const i2c_master_bus_config_t i2c_mst_config = {
			.i2c_port = (int) port,
			.sda_io_num = sda,
			.scl_io_num = scl,
			.clk_source = I2C_CLK_SRC_DEFAULT,
			.glitch_ignore_cnt = 7
	};
	ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &hndl));
}

I2CMaster::~I2CMaster() noexcept{
	ESP_ERROR_CHECK(i2c_del_master_bus(hndl));
}

I2CPort I2CMaster::getPort() const noexcept{
	return (I2CPort) port;
}

std::lock_guard<std::mutex> I2CMaster::lockBus() noexcept{
	return std::lock_guard<std::mutex>(mutex);
}

std::unique_ptr<I2CDevice> I2CMaster::addDevice(uint16_t addr, i2c_addr_bit_len_t addrLen, uint32_t speedHz){
	if(const auto err = probe(addr); err != ESP_OK){
		CMF_LOG(I2CMaster, LogLevel::Warning, "addDevice: probe failed for address 0x%x: %s", addr, esp_err_to_name(err));
		return {};
	}

	const i2c_device_config_t cfg = {
			.dev_addr_length = addrLen,
			.device_address = addr,
			.scl_speed_hz = speedHz
	};

	i2c_master_dev_handle_t devHndl;
	if(const auto err = i2c_master_bus_add_device(hndl, &cfg, &devHndl); err != ESP_OK){
		CMF_LOG(I2CMaster, LogLevel::Error, "addDevice: i2c_master_bus_add_device failed for address 0x%x: %s", addr, esp_err_to_name(err));
		return {};
	}

	return std::make_unique<I2CDevice>(this, devHndl);
}

void I2CMaster::scan(TickType_t timeout) noexcept{
	CMF_LOG(I2CMaster, LogLevel::Info, "I2C scan:");

	for(size_t i = 0; i < 127; ++i){
		if(probe(i, timeout) == ESP_OK){
			CMF_LOG(I2CMaster, LogLevel::Info, "Found device on address 0x%x", (unsigned) i);
		}
	}

	CMF_LOG(I2CMaster, LogLevel::Info, "I2C scan done.");
}

esp_err_t I2CMaster::probe(uint8_t address, TickType_t timeout) noexcept{
	std::lock_guard lock(mutex);
	return i2c_master_probe(hndl, address, timeout);
}

esp_err_t I2CMaster::write(I2CDevice& dev, const uint8_t* data, size_t size, TickType_t wait){
	std::lock_guard lock(mutex);
	return i2c_master_transmit(dev.hndl, data, size, wait);
}

esp_err_t I2CMaster::read(I2CDevice& dev, uint8_t* data, size_t size, TickType_t wait){
	std::lock_guard lock(mutex);
	return i2c_master_receive(dev.hndl, data, size, wait);
}

esp_err_t I2CMaster::write_read(I2CDevice& dev, const uint8_t* writeData, size_t writeSize, uint8_t* readData, size_t readSize, TickType_t wait){
	std::lock_guard lock(mutex);
	return i2c_master_transmit_receive(dev.hndl, writeData, writeSize, readData, readSize, wait);
}

esp_err_t I2CMaster::writeRegister(I2CDevice& dev, uint8_t reg, const uint8_t* data, size_t size, TickType_t wait){
	i2c_master_transmit_multi_buffer_info_t multi[] = {
			{ .write_buffer = &reg, .buffer_size = 1 },
			{ .write_buffer = (uint8_t*) data, .buffer_size = size }
	};
	std::lock_guard lock(mutex);
	return i2c_master_multi_buffer_transmit(dev.hndl, multi, sizeof(multi) / sizeof(multi[0]), wait);
}
