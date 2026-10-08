#ifndef BUTTERBOT_FIRMWARE_GASEVENTROUTINE_H
#define BUTTERBOT_FIRMWARE_GASEVENTROUTINE_H

#include "EventBag.h"
#include "PhraseEventRoutine.h"

class GasEventRoutine : public PhraseEventRoutine {

public:
    GasEventRoutine(BBStateMachine* sm, EventBag::EventData data);

private:
    int16_t phraseID = -1;

private:
    virtual TickingState tick(float deltaTime) override;
    virtual int16_t getPhraseID() override;
    virtual Phrase getPhraseCategory() override;
    virtual SpeechGen::InputType getPhraseType() override;
};

#endif //BUTTERBOT_FIRMWARE_GASEVENTROUTINE_H