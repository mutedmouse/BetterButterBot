#ifndef BUTTERBOT_FIRMWARE_GASCONFIGUREDEVENTROUTINE_H
#define BUTTERBOT_FIRMWARE_GASCONFIGUREDEVENTROUTINE_H

#include <Phrases.h>
#include <Audio/SpeechGen.h>
#include "PhraseEventRoutine.h"

class GasConfiguredEventRoutine : public PhraseEventRoutine {

public:
	GasConfiguredEventRoutine(BBStateMachine* sm, EventBag::EventData data);

private:
	int16_t phraseID = -1;

private:
	virtual Phrase getPhraseCategory() override;
	virtual int16_t getPhraseID() override;
	virtual SpeechGen::InputType getPhraseType() override;
};

#endif //BUTTERBOT_FIRMWARE_GASCONFIGUREDEVENTROUTINE_H