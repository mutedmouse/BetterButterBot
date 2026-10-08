#include "LEDBreatheRoutine.h"
#include <BBData.h>

Phrase LEDBreatheRoutine::run(LEDModuleControl* leds, ModuleService*){
	leds->turnLedBreathe();
	return Phrase::LEDBreathePhrases;
}

void LEDBreatheRoutine::sendResult(Com* com, int16_t phraseId, bool missing){
	LEDBreatheData data{};
	data.id = phraseId;
	data.missing = missing;
	com->sendData(BB::State::Scenario, BB::Action::Scenario::LEDBreathe, data);
}
