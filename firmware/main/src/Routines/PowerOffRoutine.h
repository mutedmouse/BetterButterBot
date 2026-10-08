#ifndef BUTTERBOT_FIRMWARE_POWEROFFROUTINE_H
#define BUTTERBOT_FIRMWARE_POWEROFFROUTINE_H

#include "Routine.h"

class PowerOffRoutine : public Routine {
	public:
	using Routine::Routine;

public:
	virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_POWEROFFROUTINE_H
