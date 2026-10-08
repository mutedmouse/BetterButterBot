#ifndef BUTTERBOT_FIRMWARE_VOICETURN180ROUTINE_H
#define BUTTERBOT_FIRMWARE_VOICETURN180ROUTINE_H

#include "MovementCommandRoutine.h"

class VoiceTurn180Routine : public MovementCommandRoutine {
    public:
	using MovementCommandRoutine::MovementCommandRoutine;
protected:
    MovementCommand getMovementCommand() const override;
};

#endif //BUTTERBOT_FIRMWARE_VOICETURN180ROUTINE_H
