#include "OutputCurrAW.h"

OutputCurrAW::OutputCurrAW(const std::vector<OutputPinDef>& outputs, AW9523* aw9523) : OutputDriver(outputs), aw9523(aw9523){

}

void OutputCurrAW::performWrite(int port, float value) noexcept{
	if(!getInversions().contains(port)){
		CMF_LOG(CMF, Warning, "Output port %d inversion missing", port);
		return;
	}
	uint8_t val = (uint8_t) (255.f * value);
	aw9523->dim(port, val);
}

void OutputCurrAW::performRegister(const OutputPinDef& output) noexcept{
	aw9523->pinMode(output.port, AW9523::LED);
}
