#include "LEDSlowerRoutine.h"
#include <BBData.h>

Phrase LEDSlowerRoutine::run(LEDModuleControl* leds, ModuleService*){
	if(leds->isAtMaxSpeed()) return Phrase::LEDAlreadySlowerPhrases;
	leds->turnLedSlower();
	return Phrase::LEDSlowerPhrases;
}

void LEDSlowerRoutine::sendResult(Com* com, int16_t phraseId, bool missing){
	LEDSlowerData data{};
	data.id = phraseId;
	data.missing = missing;
	com->sendData(BB::State::Scenario, BB::Action::Scenario::LEDSlower, data);
}
