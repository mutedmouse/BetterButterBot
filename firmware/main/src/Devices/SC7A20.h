#ifndef BUTTERBOT_FIRMWARE_SC7A20_H
#define BUTTERBOT_FIRMWARE_SC7A20_H

#include <Periphery/I2CMaster.h>
#include <Drivers/lis3dh-pid/lis3dh_reg.h>

/**
 * SC7A20 3-axis accelerometer (https://www.lcsc.com/product-detail/C5126709.html).
 * Functional replacement for LIS3DH (https://www.st.com/en/mems-and-sensors/lis3dh.html)
 *
 * Note - SC7A20 is functionally different from SC7A20H, despite a similar naming scheme.
 *
 * Axis orientation:
 * X - upwards direction, from base to top
 * Y - from BB's body to his right-hand side
 * Z - from BB's body to his front (direction of the camera).
 */
class SC7A20 {
public:
	struct Sample {
		float accelX;
		float accelY;
		float accelZ;
	};

	explicit SC7A20(I2CMaster* i2c, uint8_t addr = 0x18);

	static constexpr uint8_t WhoAmIReg = LIS3DH_WHO_AM_I;
	static constexpr uint8_t SC7A20_ID = 0x11;

	void startFIFO();

	void stopFIFO();

	Sample getSample();

	uint8_t readFIFO(std::vector<Sample>& output);

private:
	std::unique_ptr<I2CDevice> dev;
	const uint8_t Addr;

	void init();

	static int32_t platform_write(void* hndl, uint8_t reg, const uint8_t* data, uint16_t len);
	static int32_t platform_read(void* hndl, uint8_t reg, uint8_t* data, uint16_t len);

	stmdev_ctx_t ctx = {
		.write_reg = platform_write,
		.read_reg = platform_read,
		.mdelay = vTaskDelay,
		.handle = this
	};
};


#endif //BUTTERBOT_FIRMWARE_SC7A20_H
