#include "OutputTCA.h"

OutputTCA::OutputTCA(const std::vector<OutputPinDef>& outputs, TCA9555* tca9555) : Super(outputs), tca9555(tca9555){

}

void OutputTCA::performWrite(int port, float value) noexcept{
	if(!getInversions().contains(port)){
		CMF_LOG(CMF, Warning, "Output port %d inversion missing", port);
		return;
	}

	tca9555->write(port, static_cast<bool>(value));
}

void OutputTCA::performRegister(const OutputPinDef& output) noexcept{
	tca9555->pinMode(output.port, TCA9555::PinMode::OUT);
}
