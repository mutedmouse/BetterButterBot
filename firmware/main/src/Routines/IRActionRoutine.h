#ifndef BUTTERBOT_FIRMWARE_IRACTIONROUTINE_H
#define BUTTERBOT_FIRMWARE_IRACTIONROUTINE_H

#include "Routine.h"

class IRActionRoutine : public Routine {
    public:
	using Routine::Routine;
public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_IRACTIONROUTINE_H
