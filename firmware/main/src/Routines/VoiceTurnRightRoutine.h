#ifndef BUTTERBOT_FIRMWARE_VOICETURNRIGHTROUTINE_H
#define BUTTERBOT_FIRMWARE_VOICETURNRIGHTROUTINE_H

#include "MovementCommandRoutine.h"

class VoiceTurnRightRoutine : public MovementCommandRoutine {
    public:
	using MovementCommandRoutine::MovementCommandRoutine;
protected:
    MovementCommand getMovementCommand() const override;
};

#endif //BUTTERBOT_FIRMWARE_VOICETURNRIGHTROUTINE_H
