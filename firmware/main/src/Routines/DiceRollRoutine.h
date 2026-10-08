#ifndef BUTTERBOT_FIRMWARE_DICEROLLROUTINE_H
#define BUTTERBOT_FIRMWARE_DICEROLLROUTINE_H

#include <cstdint>
#include <BBData.h>
#include "PhraseListeningRoutine.h"

class DiceRollRoutine : public PhraseListeningRoutine {
public:
	explicit DiceRollRoutine(BBStateMachine* sm) : PhraseListeningRoutine(sm) {}

	TickingState tick(float deltaTime) override;

protected:
    DiceRollData::DiceType knownDiceType = DiceRollData::DiceType::None;

private:
    enum class Phase : uint8_t { Init, WaitCount, WaitType };
    Phase phase = Phase::Init;
    uint8_t count = 0;
};

class DiceRollD4Routine : public DiceRollRoutine {
public:
    explicit DiceRollD4Routine(BBStateMachine* sm);
};

class DiceRollD6Routine : public DiceRollRoutine {
public:
    explicit DiceRollD6Routine(BBStateMachine* sm);
};

class DiceRollD8Routine : public DiceRollRoutine {
public:
    explicit DiceRollD8Routine(BBStateMachine* sm);
};

class DiceRollD10Routine : public DiceRollRoutine {
public:
    explicit DiceRollD10Routine(BBStateMachine* sm);
};

class DiceRollD12Routine : public DiceRollRoutine {
public:
    explicit DiceRollD12Routine(BBStateMachine* sm);
};

class DiceRollD20Routine : public DiceRollRoutine {
public:
    explicit DiceRollD20Routine(BBStateMachine* sm);
};

class DiceRollD100Routine : public DiceRollRoutine {
public:
    explicit DiceRollD100Routine(BBStateMachine* sm);
};

#endif //BUTTERBOT_FIRMWARE_DICEROLLROUTINE_H
