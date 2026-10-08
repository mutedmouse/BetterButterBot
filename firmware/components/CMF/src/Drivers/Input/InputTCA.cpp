#include "InputTCA.h"

InputTCA::InputTCA(const std::vector<InputPinDef>& inputs, TCA9555* tca9555) : Super(inputs), tca9555(tca9555){

}

void InputTCA::scan() noexcept{
	const auto inputReg = tca9555->readAll();

	forEachInput([this, &inputReg](const InputPinDef& input){
		getStates()[input.port] = (inputReg & (1 << input.port));
	});
}

void InputTCA::performRegister(const InputPinDef& input) noexcept{
	tca9555->pinMode(input.port, TCA9555::PinMode::IN);
}
