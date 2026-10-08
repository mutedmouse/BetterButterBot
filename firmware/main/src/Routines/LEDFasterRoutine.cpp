#include "LEDFasterRoutine.h"
#include <BBData.h>

Phrase LEDFasterRoutine::run(LEDModuleControl* leds, ModuleService*){
	if(leds->isAtMinSpeed()) return Phrase::LEDAlreadyFasterPhrases;
	leds->turnLedFaster();
	return Phrase::LEDFasterPhrases;
}

void LEDFasterRoutine::sendResult(Com* com, int16_t phraseId, bool missing){
	LEDFasterData data{};
	data.id = phraseId;
	data.missing = missing;
	com->sendData(BB::State::Scenario, BB::Action::Scenario::LEDFaster, data);
}
