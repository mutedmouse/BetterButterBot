#ifndef BUTTERBOT_FIRMWARE_VOICETURNLEFTROUTINE_H
#define BUTTERBOT_FIRMWARE_VOICETURNLEFTROUTINE_H

#include "MovementCommandRoutine.h"

class VoiceTurnLeftRoutine : public MovementCommandRoutine {
    public:
	using MovementCommandRoutine::MovementCommandRoutine;
protected:
    MovementCommand getMovementCommand() const override;
};

#endif //BUTTERBOT_FIRMWARE_VOICETURNLEFTROUTINE_H
