#include "LEDTurnOffRoutine.h"
#include <BBData.h>

Phrase LEDTurnOffRoutine::run(LEDModuleControl* leds, ModuleService*){
	if(!leds->isLedOn()) return Phrase::LEDAlreadyTurnOFFPhrases;
	leds->turnLedOff();
	return Phrase::LEDTurnOFFPhrases;
}

void LEDTurnOffRoutine::sendResult(Com* com, int16_t phraseId, bool missing){
	LEDTurnOffData data{};
	data.id = phraseId;
	data.missing = missing;
	com->sendData(BB::State::Scenario, BB::Action::Scenario::LEDTurnOff, data);
}
