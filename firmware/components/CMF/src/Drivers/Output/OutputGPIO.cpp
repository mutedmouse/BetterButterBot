#include "OutputGPIO.h"
#include "Log/Log.h"

OutputGPIO::OutputGPIO(const std::vector<OutputPinDef>& outputs, StrongObjectPtr<GPIOPeriph> gpio) noexcept : OutputDriver(outputs), gpio(std::move(gpio)){

}

void OutputGPIO::performWrite(int port, float value) noexcept{
	const auto pin = (gpio_num_t) port;
	if(!getInversions().contains(port)){
		CMF_LOG(CMF, Warning, "Output port %d inversion missing", port);
		return;
	}
	gpio->write(pin, (bool) value);
}

void OutputGPIO::performRegister(const OutputPinDef& output) noexcept {
	const auto pin = (gpio_num_t) output.port;
	gpio->setMode(pin, GPIOMode::Output);
	//setting initial state
	write((gpio_num_t) output.port, false);
}
