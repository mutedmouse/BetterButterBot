#include "VoiceBackwardRoutine.h"

static constexpr uint32_t MoveDuration = 2700; // cca. 10 cm

MovementCommandRoutine::MovementCommand VoiceBackwardRoutine::getMovementCommand() const {
    return { Phrase::VoiceBackward, VoiceControlData::Direction::Backward, -100, -100, MoveDuration };
}
