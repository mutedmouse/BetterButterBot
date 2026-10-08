#ifndef CMF_OUTPUTPWM_H
#define CMF_OUTPUTPWM_H

#include "Object/Class.h"
#include "Drivers/Interface/OutputDriver.h"
#include <driver/ledc.h>

/**
 * OutputPinDef.port represents an independently controlled LEDC channel,
 * while OutputPWMPinDef.pin is its attached GPIO
 */
struct OutputPWMPinDef : OutputPinDef {
	gpio_num_t pin;
};

class OutputPWM : public OutputDriver {
	GENERATED_BODY(OutputPWM, OutputDriver, CONSTRUCTOR_PACK(const std::vector<OutputPWMPinDef>&))

public:
	OutputPWM() = default;
	OutputPWM(const std::vector<OutputPWMPinDef>& outputs);

	/**
	 * Sets PWM frequency, default is 5000Hz
	 * @param freq [Hz]
	 */
	void setFreq(int port, uint16_t freq);

	/**
	 * Stops the PWM signal output
	 */
	void stop(int port);

	/**
	 * Attaches the PWM signal generator to GPIO
	 */
	void attach(int port);

	/**
	 * Detaches the PWM signal generator from GPIO.
	 * Signal is still initialized and "running".
	 */
	void detach(int port);

	//Taken from esp_driver_ledc/ledc.c
	static constexpr bool IRAM_ATTR checkFrequency(uint16_t freq){
		uint64_t divParam = 0;
		uint32_t precision = FullDuty;

		divParam = (((uint64_t) src_clk_freq << 8) + ((freq * precision) / 2)) / (freq * precision);

		if(!(divParam > 256 && divParam < (0x3FFFF))){
			return false;
		}
		return true;
	}

private:
	void performWrite(int port, float value) noexcept override;

	void performRegister(const OutputPinDef& output) noexcept override;

	void performDeregister(const OutputPinDef& output) noexcept override;

	std::map<int, gpio_num_t> gpios;

	static constexpr uint32_t DefaultFreq = 5000;    //placeholder, usually changed before attaching to a channel or pin

	static constexpr ledc_timer_bit_t DRAM_ATTR DutyResDefault = LEDC_TIMER_10_BIT;
	static constexpr uint32_t DRAM_ATTR FullDuty = 1 << DutyResDefault;

	static constexpr ledc_mode_t getSpeedMode(int port){
		return (ledc_mode_t) (port / (int) LEDC_CHANNEL_MAX);
	}

	static constexpr ledc_timer_t getTimer(int port){
		return (ledc_timer_t) (port % LEDC_TIMER_MAX);
	}

	static constexpr ledc_channel_t getChannel(int port){
		return (ledc_channel_t) (port % LEDC_CHANNEL_MAX);
	}

	static constexpr uint32_t DRAM_ATTR src_clk_freq = 80000000; //80 MHz

};

#endif //CMF_OUTPUTPWM_H