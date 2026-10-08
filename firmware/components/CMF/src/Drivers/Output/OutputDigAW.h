#ifndef CMF_OUTPUTDIGAW_H
#define CMF_OUTPUTDIGAW_H

#include "Object/Class.h"
#include "Drivers/Interface/OutputDriver.h"
#include "Devices/AW9523.h"

class OutputDigAW : public OutputDriver {
	GENERATED_BODY(OutputDigAW, OutputDriver, CONSTRUCTOR_PACK(const std::vector<OutputPinDef>&, AW9523*));

public:
	OutputDigAW() = default;
	OutputDigAW(const std::vector<OutputPinDef>& outputs, AW9523* aw9523);

private:
	StrongObjectPtr<AW9523> aw9523;

	void performWrite(int port, float value) noexcept override;
	void performRegister(const OutputPinDef& output) noexcept override;
};


#endif //CMF_OUTPUTDIGAW_H
