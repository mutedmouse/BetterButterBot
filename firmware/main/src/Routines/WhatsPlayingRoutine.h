#ifndef BUTTERBOT_FIRMWARE_WHATSPLAYINGROUTINE_H
#define BUTTERBOT_FIRMWARE_WHATSPLAYINGROUTINE_H

#include "Routine.h"

class WhatsPlayingRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_WHATSPLAYINGROUTINE_H
