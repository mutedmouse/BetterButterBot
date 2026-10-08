#include "InputAW.h"

InputAW::InputAW(const std::vector<InputPinDef>& inputs, AW9523* aw9523) : Super(inputs), aw9523(aw9523){

}

void InputAW::scan() noexcept{
	const auto inputReg = aw9523->readAll();

	forEachInput([this, &inputReg](const InputPinDef& input){
		getStates()[input.port] = (inputReg & (1 << input.port));
	});
}

void InputAW::performRegister(const InputPinDef& input) noexcept{
	aw9523->pinMode(input.port, AW9523::PinMode::IN);
}
