#ifndef BUTTERBOT_FIRMWARE_IRFORGETROUTINE_H
#define BUTTERBOT_FIRMWARE_IRFORGETROUTINE_H

#include <cstdint>
#include "PhraseListeningRoutine.h"

class IRForgetRoutine : public PhraseListeningRoutine {
public:
	explicit IRForgetRoutine(BBStateMachine* sm) : PhraseListeningRoutine(sm) {}
	TickingState tick(float deltaTime) override;

private:
	enum class Phase : uint8_t { Init, Listening };
	Phase phase = Phase::Init;
};

#endif //BUTTERBOT_FIRMWARE_IRFORGETROUTINE_H
