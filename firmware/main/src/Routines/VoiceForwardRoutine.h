#ifndef BUTTERBOT_FIRMWARE_VOICEFORWARDROUTINE_H
#define BUTTERBOT_FIRMWARE_VOICEFORWARDROUTINE_H

#include "MovementCommandRoutine.h"

class VoiceForwardRoutine : public MovementCommandRoutine {
    public:
	using MovementCommandRoutine::MovementCommandRoutine;
protected:
    MovementCommand getMovementCommand() const override;
};

#endif //BUTTERBOT_FIRMWARE_VOICEFORWARDROUTINE_H
