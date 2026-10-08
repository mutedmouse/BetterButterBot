#ifndef BUTTERBOT_FIRMWARE_IRLISTROUTINE_H
#define BUTTERBOT_FIRMWARE_IRLISTROUTINE_H

#include "Routine.h"

class IRListRoutine : public Routine {
    public:
	using Routine::Routine;
public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_IRLISTROUTINE_H
