#include "OutputMCPWM.h"
#include "Log/Log.h"
#include <driver/gpio.h>

DEFINE_LOG(OutputMCPWM)

OutputMCPWM::OutputMCPWM(const std::vector<OutputMCPWMPinDef>& outputs) : Super(toOutputPinDef(outputs)){
	for(const auto& output : outputs){
		states[output.port].config = output.config;
	}
}

void OutputMCPWM::performRegister(const OutputPinDef& output) noexcept{
	if(output.port >= PortsNum){
		CMF_LOG(OutputMCPWM, LogLevel::Error, "MCPWM ports must be in range 0 - %d (got %d)", PortsNum, output.port);
		return;
	}

	const auto& config = states[output.port].config;

	if(config.MinPulseWidth > config.MaxPulseWidth){
		CMF_LOG(OutputMCPWM, LogLevel::Error, "MCPWM minimum pulse width bigger than maximum (%lu > %lu)", config.MinPulseWidth, config.MaxPulseWidth);
		return;
	}

	gpio_config_t gpioConfig{
		1ULL << config.pin, GPIO_MODE_OUTPUT, GPIO_PULLUP_DISABLE, GPIO_PULLDOWN_DISABLE, GPIO_INTR_DISABLE
	};
	gpio_config(&gpioConfig);

	auto& state = states[output.port];

	state.MinPulseWidth = config.MinPulseWidth;
	state.MaxPulseWidth = config.MaxPulseWidth;

	mcpwm_timer_config_t timer_config = {
			.group_id = output.port / SOC_MCPWM_TIMERS_PER_GROUP,
			.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
			.resolution_hz = config.TimerResolution,
			.count_mode = MCPWM_TIMER_COUNT_MODE_UP,
			.period_ticks = config.PeriodLength,
	};
	ESP_ERROR_CHECK(mcpwm_new_timer(&timer_config, &state.timerHandle));

	mcpwm_operator_config_t operator_config = {
			.group_id = 0, // operator must be in the same group to the timer
	};
	ESP_ERROR_CHECK(mcpwm_new_operator(&operator_config, &state.operatorHandle));

	ESP_ERROR_CHECK(mcpwm_operator_connect_timer(state.operatorHandle, state.timerHandle));

	mcpwm_comparator_config_t comparator_config = {
			.flags = { .update_cmp_on_tez = true },
	};
	ESP_ERROR_CHECK(mcpwm_new_comparator(state.operatorHandle, &comparator_config, &state.compHandle));

	mcpwm_generator_config_t generator_config = {
			.gen_gpio_num = config.pin,
	};
	ESP_ERROR_CHECK(mcpwm_new_generator(state.operatorHandle, &generator_config, &state.genHandle));

	// set the initial compare value, so that the servo will spin to the center position
	ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(state.compHandle, (config.MaxPulseWidth - config.MinPulseWidth) / 2));

	// go high on counter empty
	ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
			state.genHandle, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH)));
	// go low on compare threshold
	ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
			state.genHandle, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, state.compHandle, MCPWM_GEN_ACTION_LOW)));

}

void OutputMCPWM::performDeregister(const OutputPinDef& output) noexcept{

	disable(output.port);

	auto& state = states[output.port];
	mcpwm_del_generator(state.genHandle);
	mcpwm_del_comparator(state.compHandle);
	mcpwm_del_operator(state.operatorHandle);
	mcpwm_del_timer(state.timerHandle);
}

void OutputMCPWM::performWrite(int port, float value) noexcept{
	auto& state = states[port];

	if(!state.enabled){
		enable(port);
	}

	value = std::clamp(value, 0.f, 1.f);
	const auto ticks = (uint32_t) ((float) state.MinPulseWidth + value * (float) (state.MaxPulseWidth - state.MinPulseWidth));
	mcpwm_comparator_set_compare_value(state.compHandle, ticks);
}

void OutputMCPWM::enable(int port){
	auto& state = states[port];

	if(state.enabled) return;

	ESP_ERROR_CHECK(mcpwm_timer_enable(state.timerHandle));
	ESP_ERROR_CHECK(mcpwm_timer_start_stop(state.timerHandle, MCPWM_TIMER_START_NO_STOP));
	state.enabled = true;
}

void OutputMCPWM::disable(int port){
	auto& state = states[port];
	if(!state.enabled) return;

	mcpwm_timer_start_stop(state.timerHandle, MCPWM_TIMER_STOP_EMPTY);
	mcpwm_timer_disable(state.timerHandle);
	state.enabled = false;
}
