#ifndef BUTTERBOT_FIRMWARE_GASMODULEROUTINE_H
#define BUTTERBOT_FIRMWARE_GASMODULEROUTINE_H

#include "Routine.h"

class GasModuleRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_GASMODULEROUTINE_H
