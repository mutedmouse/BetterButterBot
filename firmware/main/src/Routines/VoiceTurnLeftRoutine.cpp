#include "VoiceTurnLeftRoutine.h"

static constexpr uint32_t TurnDuration = 1700;

MovementCommandRoutine::MovementCommand VoiceTurnLeftRoutine::getMovementCommand() const {
    return { Phrase::VoiceTurnLeft, VoiceControlData::Direction::Left, -100, 100, TurnDuration };
}
