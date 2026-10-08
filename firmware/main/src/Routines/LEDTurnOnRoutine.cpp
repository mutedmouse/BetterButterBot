#include "LEDTurnOnRoutine.h"
#include <BBData.h>

Phrase LEDTurnOnRoutine::run(LEDModuleControl* leds, ModuleService*){
	if(leds->isLedOn()) return Phrase::LEDAlreadyTurnONPhrases;
	leds->turnLedOn();
	return Phrase::LEDTurnONPhrases;
}

void LEDTurnOnRoutine::sendResult(Com* com, int16_t phraseId, bool missing){
	LEDTurnOnData data{};
	data.id = phraseId;
	data.missing = missing;
	com->sendData(BB::State::Scenario, BB::Action::Scenario::LEDTurnOn, data);
}
