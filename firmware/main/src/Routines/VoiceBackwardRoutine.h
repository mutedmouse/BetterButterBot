#ifndef BUTTERBOT_FIRMWARE_VOICEBACKWARDROUTINE_H
#define BUTTERBOT_FIRMWARE_VOICEBACKWARDROUTINE_H

#include "MovementCommandRoutine.h"

class VoiceBackwardRoutine : public MovementCommandRoutine {
    public:
	using MovementCommandRoutine::MovementCommandRoutine;
protected:
    MovementCommand getMovementCommand() const override;
};

#endif //BUTTERBOT_FIRMWARE_VOICEBACKWARDROUTINE_H
