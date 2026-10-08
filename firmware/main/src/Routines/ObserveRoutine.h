#ifndef BUTTERBOT_FIRMWARE_OBSERVEROUTINE_H
#define BUTTERBOT_FIRMWARE_OBSERVEROUTINE_H

#include "Routine.h"

class ObserveRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_OBSERVEROUTINE_H
