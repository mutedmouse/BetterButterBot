#include "InputGPIO.h"
#include "Log/Log.h"

DEFINE_LOG(InputGPIO)

InputGPIO::InputGPIO(const std::vector<GPIOPinDef>& inputs, StrongObjectPtr<GPIOPeriph> gpio) noexcept:
	Super(toInputPinDef(inputs)), gpio(std::move(gpio)){

	for(const auto& input : inputs){
		pullModes[input.port] = input.pullMode;
	}
}

void InputGPIO::scan() noexcept{
	Super::scan();

	forEachInput([this](const InputPinDef& input){
		const auto pin = (gpio_num_t) input.port;
		getStates()[input.port] = gpio->read(pin);
	});
}

void InputGPIO::registerInput(const GPIOPinDef& pinDef){
	pullModes[pinDef.port] = pinDef.pullMode;
	InputDriver::registerInput(pinDef);
}

void InputGPIO::performRegister(const InputPinDef& input) noexcept{
	gpio->setMode((gpio_num_t) input.port, GPIOMode::Input);

	if(!pullModes.contains(input.port)){
		CMF_LOG(InputGPIO, LogLevel::Error, "No pull mode defined for pin %d", input.port);
	}

	const auto pin = (gpio_num_t) input.port;
	switch(pullModes[input.port]){
		case PullMode::Up:
			gpio->setPullup(pin, true);
			break;
		case PullMode::Down:
			gpio->setPulldown(pin, true);
			break;
		case PullMode::None:
			gpio->setPullup(pin, false);
			gpio->setPulldown(pin, false);
			break;
	}
}
