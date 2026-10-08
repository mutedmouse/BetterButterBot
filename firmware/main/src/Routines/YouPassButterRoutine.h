#ifndef BUTTERBOT_FIRMWARE_YOUPASSBUTTERROUTINE_H
#define BUTTERBOT_FIRMWARE_YOUPASSBUTTERROUTINE_H

#include "Routine.h"

class YouPassButterRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_YOUPASSBUTTERROUTINE_H
