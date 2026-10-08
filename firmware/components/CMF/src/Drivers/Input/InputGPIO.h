#ifndef CMF_INPUTGPIO_H
#define CMF_INPUTGPIO_H

#include "Object/Class.h"
#include "Drivers/Interface/InputDriver.h"
#include "Periphery/GPIOPeriph.h"

enum class PullMode : uint8_t {
	None, Up, Down
};

struct GPIOPinDef : InputPinDef {
	PullMode pullMode;
};

class InputGPIO : public InputDriver {
	GENERATED_BODY(InputGPIO, InputDriver, CONSTRUCTOR_PACK(const std::vector<GPIOPinDef>&, StrongObjectPtr<GPIOPeriph>));

public:
	InputGPIO() noexcept = default;
	InputGPIO(const std::vector<GPIOPinDef>& inputs, StrongObjectPtr<GPIOPeriph> gpio) noexcept;

	void registerInput(const GPIOPinDef& pinDef);

private:
	void scan() noexcept override;

	void performRegister(const InputPinDef& input) noexcept override;

	std::map<int, PullMode> pullModes;

	StrongObjectPtr<GPIOPeriph> gpio;
};

#endif //CMF_INPUTGPIO_H
