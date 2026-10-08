#ifndef BUTTERBOT_FIRMWARE_POKEEVENTROUTINE_H
#define BUTTERBOT_FIRMWARE_POKEEVENTROUTINE_H

#include "PhraseEventRoutine.h"

class PokeEventRoutine : public PhraseEventRoutine {

public:
    explicit PokeEventRoutine(BBStateMachine* sm);

private:

    virtual int16_t getPhraseID() override;

    virtual Phrase getPhraseCategory() override;

private:
    int16_t phraseID = -1;

    inline static constexpr uint64_t ResetTime = 60000; // [ms]
    inline static constexpr uint8_t LevelIncreaseRepetitions = 5;

    inline static uint64_t LastPokeTime = 0;
    inline static uint8_t Repetitions = 0;
};

#endif //BUTTERBOT_FIRMWARE_POKEEVENTROUTINE_H