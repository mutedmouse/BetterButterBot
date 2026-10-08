#include "InputTouchGPIO.h"
#include "Log/Log.h"
#include <driver/touch_pad.h>

DEFINE_LOG(InputTouchGPIO)

InputTouchGPIO::InputTouchGPIO(const std::vector<TouchPinDef>& inputs) noexcept: Super(toInputPinDef(inputs)){
	for(const auto& input : inputs){
		thresholds[input.port] = input.threshold;
	}

	touch_pad_init();

	touch_pad_denoise_t denoise = {
			.grade = TOUCH_PAD_DENOISE_BIT4,
			.cap_level = TOUCH_PAD_DENOISE_CAP_L4,
	};
	touch_pad_denoise_set_config(&denoise);
	touch_pad_denoise_enable();

	touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
	touch_pad_fsm_start();
}

void InputTouchGPIO::registerInput(const TouchPinDef& pinDef){
	InputDriver::registerInput(pinDef);
	thresholds[pinDef.port] = pinDef.threshold;
}

void InputTouchGPIO::scan() noexcept{
	const auto touchStatus = touch_pad_get_status();

	forEachInput([this, &touchStatus](const InputPinDef& input){
		getStates()[input.port] = (touchStatus & (1UL << input.port));
	});
}

void InputTouchGPIO::performRegister(const InputPinDef& input) noexcept{
	touch_pad_fsm_stop();

	const auto touchPin = (touch_pad_t) input.port;
	touch_pad_config(touchPin);

	if(!thresholds.contains(input.port)){
		CMF_LOG(InputTouchGPIO, LogLevel::Error, "Threshold for pin %d not defined", input.port);
		return;
	}

	touch_pad_set_thresh(touchPin, thresholds[input.port]);

	touch_pad_fsm_start();

	/*vTaskDelay(100 / portTICK_PERIOD_MS);

	uint32_t benchmark;
	ESP_ERROR_CHECK(touch_pad_read_benchmark(touchPin, &benchmark));
	printf("%d benchmark: %lu\n", input.port, benchmark);
	benchmarks[input.port] = benchmark;*/
}
