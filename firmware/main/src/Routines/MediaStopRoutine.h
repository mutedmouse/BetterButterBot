#ifndef BUTTERBOT_FIRMWARE_MEDIASTOPROUTINE_H
#define BUTTERBOT_FIRMWARE_MEDIASTOPROUTINE_H

#include "Routine.h"

class MediaStopRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_MEDIASTOPROUTINE_H
