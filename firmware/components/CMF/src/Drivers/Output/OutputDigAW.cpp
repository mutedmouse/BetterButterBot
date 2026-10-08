#include "OutputDigAW.h"

OutputDigAW::OutputDigAW(const std::vector<OutputPinDef>& outputs, AW9523* aw9523) : Super(outputs), aw9523(aw9523){

}

void OutputDigAW::performWrite(int port, float value) noexcept{
	if(!getInversions().contains(port)){
		CMF_LOG(CMF, Warning, "Output port %d inversion missing", port);
		return;
	}

	aw9523->write(port, static_cast<bool>(value));
}

void OutputDigAW::performRegister(const OutputPinDef& output) noexcept{
	aw9523->pinMode(output.port, AW9523::OUT);
}
