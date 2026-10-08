#ifndef BUTTERBOT_FIRMWARE_MODULEEVENTROUTINE_H
#define BUTTERBOT_FIRMWARE_MODULEEVENTROUTINE_H

#include "EventBag.h"
#include "PhraseEventRoutine.h"

class ModuleEventRoutine : public PhraseEventRoutine {

public:
    ModuleEventRoutine(BBStateMachine* sm, EventBag::EventData data);

private:
    static const std::map<Modules::Type, Phrase> ModuleInsertMapping;
    static const std::map<Modules::Type, Phrase> ModuleRemoveMapping;

    int16_t phraseID = -1;

private:
    virtual TickingState tick(float deltaTime) override;
    virtual int16_t getPhraseID() override;
    virtual Phrase getPhraseCategory() override;
    virtual SpeechGen::InputType getPhraseType() override;
};

#endif //BUTTERBOT_FIRMWARE_MODULEEVENTROUTINE_H