#include "OutputPWM.h"
#include "Log/Log.h"

DEFINE_LOG(PWM)

OutputPWM::OutputPWM(const std::vector<OutputPWMPinDef>& outputs) : Super(toOutputPinDef(outputs)){
	for(const auto& output : outputs){
		gpios[output.port] = output.pin;
	}
}

void OutputPWM::setFreq(int port, uint16_t freq){
	if(!checkFrequency(freq)){
		CMF_LOG(PWM, Warning, "couldnt write frequency %d because of clock divisor limitations\n", freq);
		return;
	}

	const auto group = getSpeedMode(port);
	const auto timer = getTimer(port);
	const auto chan = getChannel(port);

	ledc_set_freq(group, timer, freq);
	ledc_update_duty(group, chan);
}

void OutputPWM::stop(int port){
	ledc_stop(getSpeedMode(port), getChannel(port), getInversions()[port]);
}

void OutputPWM::attach(int port){
	if(!gpios.count(port)) return;

	const ledc_channel_config_t ledc_channel = {
			.gpio_num       = gpios[port],
			.speed_mode     = getSpeedMode(port),
			.channel        = getChannel(port),
			.intr_type      = LEDC_INTR_DISABLE,
			.timer_sel      = getTimer(port),
			.duty           = 0,
			.hpoint         = 0,
			.flags = { .output_invert = getInversions()[port] }
	};
	ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void OutputPWM::detach(int port){
	if(!gpios.count(port)) return;

	const auto pin = gpios[port];

	const gpio_config_t cfg = {
			.pin_bit_mask = 1ULL << pin,
			.mode = GPIO_MODE_OUTPUT
	};

	gpio_config(&cfg);
	gpio_set_level((gpio_num_t) pin, getInversions()[port]);
}

void OutputPWM::performWrite(int port, float value) noexcept{
	/**
	 * LEDC's (hardware-based) inversion needs to be used, otherwise pin will experience an initial glitch.
	 * Since parent interface OutputDriver already handles inversion, we need to reverse it here.
	 */
	if(getInversions()[port]){
		value = 1.0f - value;
	}

	const uint32_t duty = (uint32_t) (FullDuty * value);
	const auto group = getSpeedMode(port);
	const auto chan = getChannel(port);

	ledc_set_duty(group, chan, duty);
	ledc_update_duty(group, chan);
}

void OutputPWM::performRegister(const OutputPinDef& output) noexcept{
	if(!gpios.contains(output.port)){
		CMF_LOG(PWM, Error, "Output port %d not found in pin map", output.port);
		return;
	}

	const auto& pin = gpios[output.port];

	const ledc_timer_config_t ledc_timer = {
			.speed_mode       = getSpeedMode(output.port),
			.duty_resolution  = DutyResDefault,
			.timer_num        = getTimer(output.port),
			.freq_hz          = DefaultFreq,
			.clk_cfg          = LEDC_AUTO_CLK
	};

	if(ledc_timer_config(&ledc_timer) != ESP_OK){
		CMF_LOG(PWM, Error, "timer config failed!");
		return;
	}

	attach(output.port);
}

void OutputPWM::performDeregister(const OutputPinDef& output) noexcept{
	stop(output.port);
	detach(output.port);
}
