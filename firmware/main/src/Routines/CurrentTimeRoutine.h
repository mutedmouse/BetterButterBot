#ifndef BUTTERBOT_FIRMWARE_CURRENTTIMEROUTINE_H
#define BUTTERBOT_FIRMWARE_CURRENTTIMEROUTINE_H

#include "Routine.h"

class CurrentTimeRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_CURRENTTIMEROUTINE_H
