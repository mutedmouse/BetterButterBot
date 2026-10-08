#ifndef BUTTERBOT_FIRMWARE_PROFANITYROUTINE_H
#define BUTTERBOT_FIRMWARE_PROFANITYROUTINE_H

#include "Routine.h"

class ProfanityRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_PROFANITYROUTINE_H
