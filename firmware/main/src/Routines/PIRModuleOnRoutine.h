#ifndef BUTTERBOT_FIRMWARE_PIRMODULEONROUTINE_H
#define BUTTERBOT_FIRMWARE_PIRMODULEONROUTINE_H

#include "Routine.h"

class PIRModuleOnRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_PIRMODULEONROUTINE_H
