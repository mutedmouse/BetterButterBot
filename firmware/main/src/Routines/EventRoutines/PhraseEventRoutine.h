#ifndef BUTTERBOT_FIRMWARE_PHRASEEVENTROUTINE_H
#define BUTTERBOT_FIRMWARE_PHRASEEVENTROUTINE_H

#include <Phrases.h>
#include "Audio/SpeechGen.h"
#include "Routines/EventRoutine.h"

class PhraseEventRoutine : public EventRoutine {
public:
    PhraseEventRoutine(BBStateMachine* sm, EventBag::EventData data);

    virtual TickingState tick(float deltaTime) override;

    virtual int16_t getPhraseID();

    virtual Phrase getPhraseCategory();

    virtual SpeechGen::InputType getPhraseType();
};

#endif //BUTTERBOT_FIRMWARE_PHRASEEVENTROUTINE_H
