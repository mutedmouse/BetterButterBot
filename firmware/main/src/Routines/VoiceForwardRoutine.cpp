#include "VoiceForwardRoutine.h"

static constexpr uint32_t MoveDuration = 2700; // cca. 10 cm

MovementCommandRoutine::MovementCommand VoiceForwardRoutine::getMovementCommand() const {
    return { Phrase::VoiceForward, VoiceControlData::Direction::Forward, 100, 100, MoveDuration };
}
