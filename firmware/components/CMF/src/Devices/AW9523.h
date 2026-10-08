#ifndef CMF_AW9523_H
#define CMF_AW9523_H

#include "Object/Class.h"
#include "Object/Object.h"
#include "Periphery/I2CMaster.h"
#include "Periphery/I2CDevice.h"

/**
 * GPIO expander and LED driver, 16-pin.
 * https://cdn-shop.adafruit.com/product-files/4886/AW9523+English+Datasheet.pdf
 */


class AW9523 : public Object {
	GENERATED_BODY(AW9523, Object, CONSTRUCTOR_PACK(I2CMaster*))
public:
	explicit AW9523(I2CMaster* i2c = nullptr, uint8_t addr = 0);

	void resetDimOutputs();

	enum PinMode : uint8_t {
		IN, OUT, LED
	};

	/**
	 * Set pin mode:
	 *   IN - GPIO input
	 *   OUT - GPIO output
	 *   LED - GPIO output with current control. Use dim(pin, factor) function to set dimming factor.
	 * @param pin Pin index
	 * @param mode Pin mode
	 */
	void pinMode(uint8_t pin, PinMode mode);

	/**
	 * Read pin input state.
	 * @param pin Pin index
	 * @return State - true for high, false for low
	 */
	bool read(uint8_t pin);

	/**
	 * Get read register for all pins.
	 * Does not check if all pins are Input-defined, so user is responsible for correct masking!
	 * @return bit-vector of pin states
	 */
	uint16_t readAll();

	/**
	 * Set pin output state.
	 * @param pin Pin index
	 * @param state True for high, false for low
	 */
	void write(uint8_t pin, bool state);

	/**
	 * Set LED dimming factor for pin.
	 * @param pin Pin index
	 * @param factor Dimming factor. Range 0-255
	 */
	void dim(uint8_t pin, uint8_t factor);

	/**
	 * Enable or disable interrupt triggering for pin.
	 * @param pin Pin index
	 * @param enabled
	 */
	void setInterrupt(uint8_t pin, bool enabled);

	enum CurrentLimit : uint8_t {
		IMAX, // I_max
		IMAX_3Q, // I_max * 3/4
		IMAX_2Q, // I_max * 2/4
		IMAX_1Q // I_max * 1/4
	};

	/**
	 * Set LED drive current limit. Only applies to pins configured as LED output. I_max is 37mA.
	 *   IMAX - full I_max value (37mA)
	 *   IMAX_3Q - 3/4 * I_max
	 *   IMAX_2Q - 2/4 * I_max
	 *   IMAX_1Q - 1/4 * I_max
	 */
	void setCurrentLimit(CurrentLimit limit);

private:
	std::unique_ptr<I2CDevice> dev;

	static const uint8_t dimmap[16];

	struct Regs {
		uint8_t conf = 0b00000000;
		uint8_t dir[2] = { 0, 0 };
		uint8_t output[2] = { 0, 0 };
		uint8_t intr[2] = { 0, 0 };
		uint8_t mode[2] = { 0xff, 0xff };
		uint8_t dim[16] = { 0 };
	} regs;

	uint8_t readReg(uint8_t reg) const;
	void writeReg(uint8_t reg, const uint8_t* data, size_t size) const;
	void writeReg(uint8_t reg, uint8_t data) const;

};

#endif //CMF_AW9523_H
