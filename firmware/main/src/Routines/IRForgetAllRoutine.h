#ifndef BUTTERBOT_FIRMWARE_IRFORGETALLROUTINE_H
#define BUTTERBOT_FIRMWARE_IRFORGETALLROUTINE_H

#include "Routine.h"

class IRForgetAllRoutine : public Routine {
    public:
	using Routine::Routine;
public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_IRFORGETALLROUTINE_H
