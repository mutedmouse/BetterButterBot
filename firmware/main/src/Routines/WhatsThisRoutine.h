#ifndef BUTTERBOT_FIRMWARE_WHATSTHISROUTINE_H
#define BUTTERBOT_FIRMWARE_WHATSTHISROUTINE_H

#include "Routine.h"

class WhatsThisRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    virtual TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_WHATSTHISROUTINE_H
