#ifndef BUTTERBOT_FIRMWARE_PERSONEVENTROUTINE_H
#define BUTTERBOT_FIRMWARE_PERSONEVENTROUTINE_H

#include "EventBag.h"
#include "PhraseEventRoutine.h"

class IntruderEventRoutine : public PhraseEventRoutine {

public:
	IntruderEventRoutine(BBStateMachine* sm, EventBag::EventData data);

private:
	int16_t phraseID = -1;

	virtual TickingState tick(float deltaTime) override;
	virtual int16_t getPhraseID() override;
	virtual Phrase getPhraseCategory() override;
	virtual SpeechGen::InputType getPhraseType() override;

	static constexpr uint32_t PIRBlinkPeriodMs = 500;
};

#endif //BUTTERBOT_FIRMWARE_PERSONEVENTROUTINE_H
