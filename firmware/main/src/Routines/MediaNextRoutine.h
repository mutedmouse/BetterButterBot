#ifndef BUTTERBOT_FIRMWARE_MEDIANEXTROUTINE_H
#define BUTTERBOT_FIRMWARE_MEDIANEXTROUTINE_H

#include "Routine.h"

class MediaNextRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_MEDIANEXTROUTINE_H
