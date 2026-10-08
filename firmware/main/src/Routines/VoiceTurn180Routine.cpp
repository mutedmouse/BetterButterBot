#include "VoiceTurn180Routine.h"

static constexpr uint32_t TurnDuration = 4000;

MovementCommandRoutine::MovementCommand VoiceTurn180Routine::getMovementCommand() const {
    return { Phrase::VoiceTurn180, VoiceControlData::Direction::Rotate, -100, 100, TurnDuration };
}
