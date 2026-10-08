#ifndef BUTTERBOT_FIRMWARE_LEDMODULEROUTINE_H
#define BUTTERBOT_FIRMWARE_LEDMODULEROUTINE_H

#include "Routine.h"

class LEDModuleRoutine : public Routine {
	public:
	using Routine::Routine;
public:
	virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_LEDMODULEROUTINE_H
