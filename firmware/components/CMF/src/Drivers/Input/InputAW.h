#ifndef CMF_INPUTAW_H
#define CMF_INPUTAW_H

#include "Object/Class.h"
#include "Drivers/Interface/InputDriver.h"
#include "Devices/AW9523.h"

class InputAW : public InputDriver {
	GENERATED_BODY(InputAW, InputDriver, CONSTRUCTOR_PACK(const std::vector<InputPinDef>&, AW9523*))
public:
	InputAW() = default;
	InputAW(const std::vector<InputPinDef>& inputs, AW9523* aw9523);

private:
	void scan() noexcept override;

	void performRegister(const InputPinDef& input) noexcept override;

	StrongObjectPtr<AW9523> aw9523;
};


#endif //CMF_INPUTAW_H
