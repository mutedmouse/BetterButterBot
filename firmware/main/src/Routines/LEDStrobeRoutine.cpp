#include "LEDStrobeRoutine.h"
#include <BBData.h>

Phrase LEDStrobeRoutine::run(LEDModuleControl* leds, ModuleService*){
	leds->turnLedStrobe();
	return Phrase::LEDStrobePhrases;
}

void LEDStrobeRoutine::sendResult(Com* com, int16_t phraseId, bool missing){
	LEDStrobeData data{};
	data.id = phraseId;
	data.missing = missing;
	com->sendData(BB::State::Scenario, BB::Action::Scenario::LEDStrobe, data);
}
