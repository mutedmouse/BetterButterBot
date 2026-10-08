#ifndef CMF_I2C_MASTER_H
#define CMF_I2C_MASTER_H

#include "GPIOPeriph.h"
#include "Object/Class.h"
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <driver/i2c_types.h>
#include "I2CDevice.h"

enum class I2CPort : int8_t {
	None = -1,
	Zero = 0,
#if SOC_I2C_NUM >= 2
	One = 1,
#endif
#if SOC_LP_I2C_NUM >= 1
	LpZero,
#endif
	COUNT
};

class I2CMaster : public Object {
	GENERATED_BODY(I2CMaster, Object, CONSTRUCTOR_PACK(I2CPort, gpio_num_t, gpio_num_t))

public:
	explicit I2CMaster(I2CPort port = I2CPort::None, gpio_num_t sda = GPIO_NUM_NC, gpio_num_t scl = GPIO_NUM_NC) noexcept;
	virtual ~I2CMaster() noexcept override;

	I2CPort getPort() const noexcept;

	std::lock_guard<std::mutex> lockBus() noexcept;

	std::unique_ptr<I2CDevice> addDevice(uint16_t addr, i2c_addr_bit_len_t addrLen = I2C_ADDR_BIT_LEN_7, uint32_t speedHz = 400000);

	void scan(TickType_t timeout = CONFIG_CMF_I2C_default_timeout) noexcept;
	esp_err_t probe(uint8_t address, TickType_t timeout = CONFIG_CMF_I2C_default_timeout) noexcept;

	esp_err_t write(I2CDevice& dev, const uint8_t* data, size_t size, TickType_t wait = portMAX_DELAY);
	esp_err_t read(I2CDevice& dev, uint8_t* data, size_t size, TickType_t wait = portMAX_DELAY);
	esp_err_t write_read(I2CDevice& dev, const uint8_t* writeData, size_t writeSize, uint8_t* readData, size_t readSize, TickType_t wait = portMAX_DELAY);
	esp_err_t writeRegister(I2CDevice& dev, uint8_t reg, const uint8_t* data, size_t size, TickType_t wait = portMAX_DELAY);


private:
	const i2c_port_t port;
	i2c_master_bus_handle_t hndl;
	std::mutex mutex;
};

#endif //CMF_I2C_MASTER_H