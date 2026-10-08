#include "VoiceTurnRightRoutine.h"

static constexpr uint32_t TurnDuration = 1700;

MovementCommandRoutine::MovementCommand VoiceTurnRightRoutine::getMovementCommand() const {
    return { Phrase::VoiceTurnRight, VoiceControlData::Direction::Right, 100, -100, TurnDuration };
}
