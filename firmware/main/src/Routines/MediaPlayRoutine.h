#ifndef BUTTERBOT_FIRMWARE_MEDIAPLAYROUTINE_H
#define BUTTERBOT_FIRMWARE_MEDIAPLAYROUTINE_H

#include "Routine.h"

class MediaPlayRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_MEDIAPLAYROUTINE_H
