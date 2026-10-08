#ifndef BUTTERBOT_FIRMWARE_MEDIAPREVROUTINE_H
#define BUTTERBOT_FIRMWARE_MEDIAPREVROUTINE_H

#include "Routine.h"

class MediaPrevRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_MEDIAPREVROUTINE_H
