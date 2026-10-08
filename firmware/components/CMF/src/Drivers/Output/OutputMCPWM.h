#ifndef CMF_OUTPUTMCPWM_H
#define CMF_OUTPUTMCPWM_H

#include "Object/Class.h"
#include "Drivers/Interface/OutputDriver.h"
#include "driver/mcpwm_prelude.h"
#include <hal/gpio_types.h>

/**
 * OutputPinDef.port represents an independently controlled timer within an MCPWM group,
 * while OutputPWMPinDef.pin is its attached GPIO
 */
struct MCPWM_config {
	gpio_num_t pin;

	uint32_t TimerResolution;  // Number of timer ticks per second
	uint32_t PeriodLength;    // Duration of a period in ticks
	uint32_t MinPulseWidth;  // Minimum pulse width in TimerResolution ticks, [0 - PeriodLength]
	uint32_t MaxPulseWidth;  // Maximum pulse width in TimerResolution ticks, [0 - PeriodLength]
};

struct OutputMCPWMPinDef : OutputPinDef {
	MCPWM_config config;
};


class OutputMCPWM : public OutputDriver {
	GENERATED_BODY(OutputMCPWM, OutputDriver, CONSTRUCTOR_PACK(const std::vector<OutputMCPWMPinDef>&))

public:
	OutputMCPWM() = default;

	explicit OutputMCPWM(const std::vector<OutputMCPWMPinDef>& outputs);

	void enable(int port);

	void disable(int port);

private:
	void performWrite(int port, float value) noexcept override;

	void performRegister(const OutputPinDef& output) noexcept override;

	void performDeregister(const OutputPinDef& output) noexcept override;

	static constexpr uint8_t PortsNum = SOC_MCPWM_GROUPS * SOC_MCPWM_TIMERS_PER_GROUP;

	struct PortState {
		uint32_t MinPulseWidth;  // Minimum pulse width in TimerResolution ticks, [0 - PeriodLength]
		uint32_t MaxPulseWidth;  // Maximum pulse width in TimerResolution ticks, [0 - PeriodLength]

		mcpwm_timer_handle_t timerHandle;
		mcpwm_oper_handle_t operatorHandle;
		mcpwm_cmpr_handle_t compHandle;
		mcpwm_gen_handle_t genHandle;

		std::atomic_bool enabled = false;

		MCPWM_config config; //set on construction
	} states[PortsNum];

};

#endif //CMF_OUTPUTMCPWM_H
