#ifndef BUTTERBOT_FIRMWARE_PASSTHEBUTTERROUTINE_H
#define BUTTERBOT_FIRMWARE_PASSTHEBUTTERROUTINE_H

#include "Routine.h"

class PassTheButterRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_PASSTHEBUTTERROUTINE_H
