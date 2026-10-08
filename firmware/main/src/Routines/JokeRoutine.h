#ifndef BUTTERBOT_FIRMWARE_JOKEROUTINE_H
#define BUTTERBOT_FIRMWARE_JOKEROUTINE_H

#include "Routine.h"

class JokeRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_JOKEROUTINE_H
