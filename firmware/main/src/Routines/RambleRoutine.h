#ifndef BUTTERBOT_FIRMWARE_RAMBLEROUTINE_H
#define BUTTERBOT_FIRMWARE_RAMBLEROUTINE_H

#include "Routine.h"

class RambleRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_RAMBLEROUTINE_H
