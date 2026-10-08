#ifndef BUTTERBOT_FIRMWARE_BATTERYEVENTROUTINE_H
#define BUTTERBOT_FIRMWARE_BATTERYEVENTROUTINE_H

#include "EventBag.h"
#include "PhraseEventRoutine.h"

class BatteryEventRoutine : public PhraseEventRoutine {

public:
    BatteryEventRoutine(BBStateMachine* sm, EventBag::EventData data);

private:
    int16_t phraseID = -1;

private:
    virtual TickingState tick(float deltaTime) override;
    virtual int16_t getPhraseID() override;
    virtual Phrase getPhraseCategory() override;
    virtual SpeechGen::InputType getPhraseType() override;
};

#endif //BUTTERBOT_FIRMWARE_BATTERYEVENTROUTINE_H