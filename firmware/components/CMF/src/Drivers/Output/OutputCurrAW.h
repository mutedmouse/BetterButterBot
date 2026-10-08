#ifndef CMF_OUTPUTCURRAW_H
#define CMF_OUTPUTCURRAW_H

#include "Object/Class.h"
#include "Drivers/Interface/OutputDriver.h"
#include "Devices/AW9523.h"

class OutputCurrAW : public OutputDriver {
	GENERATED_BODY(OutputCurrAW, OutputDriver, CONSTRUCTOR_PACK(const std::vector<OutputPinDef>&, AW9523*));

public:
	OutputCurrAW() = default;
	OutputCurrAW(const std::vector<OutputPinDef>& outputs, AW9523* aw9523);

private:
	StrongObjectPtr<AW9523> aw9523;

	void performWrite(int port, float value) noexcept override;
	void performRegister(const OutputPinDef& output) noexcept override;
};


#endif //CMF_OUTPUTCURRAW_H
