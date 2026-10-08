#ifndef CMF_OUTPUTGPIO_H
#define CMF_OUTPUTGPIO_H

#include "Object/Class.h"
#include "Drivers/Interface/OutputDriver.h"
#include "Periphery/GPIOPeriph.h"

class OutputGPIO : public OutputDriver {
	GENERATED_BODY(OutputGPIO, OutputDriver, CONSTRUCTOR_PACK(const std::vector<OutputPinDef>&, StrongObjectPtr<GPIOPeriph>));
public:
	OutputGPIO() noexcept = default;
	OutputGPIO(const std::vector<OutputPinDef>& outputs, StrongObjectPtr<GPIOPeriph> gpio) noexcept;

private:
	void performRegister(const OutputPinDef& output) noexcept override;

	void performWrite(int port, float value) noexcept override;

	StrongObjectPtr<GPIOPeriph> gpio;
};

#endif //CMF_OUTPUTGPIO_H
