#ifndef BUTTERBOT_FIRMWARE_LEDSTROBEROUTINE_H
#define BUTTERBOT_FIRMWARE_LEDSTROBEROUTINE_H

#include "LEDRoutine.h"

class LEDStrobeRoutine : public LEDRoutine {
	public:
	using LEDRoutine::LEDRoutine;

protected:
	Phrase run(LEDModuleControl* leds, ModuleService* modules) override;
	void sendResult(Com* com, int16_t phraseId, bool missing) override;
};

#endif //BUTTERBOT_FIRMWARE_LEDSTROBEROUTINE_H
