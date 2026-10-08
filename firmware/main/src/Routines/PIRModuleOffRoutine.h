#ifndef BUTTERBOT_FIRMWARE_PIRMODULEOFFROUTINE_H
#define BUTTERBOT_FIRMWARE_PIRMODULEOFFROUTINE_H

#include "Routine.h"

class PIRModuleOffRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_PIRMODULEOFFROUTINE_H
