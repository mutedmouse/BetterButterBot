#ifndef BUTTERBOT_FIRMWARE_FACTROUTINE_H
#define BUTTERBOT_FIRMWARE_FACTROUTINE_H

#include "Routine.h"

class FactRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_FACTROUTINE_H
