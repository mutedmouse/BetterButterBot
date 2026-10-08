#ifndef BUTTERBOT_FIRMWARE_LEDROUTINE_H
#define BUTTERBOT_FIRMWARE_LEDROUTINE_H

#include "Routine.h"
#include "Phrases.h"
#include "Services/LEDModuleControl.h"
#include "Services/Modules/ModuleService.h"
#include "Services/Com.h"

class LEDRoutine : public Routine {
	public:
	using Routine::Routine;

public:
	TickingState tick(float deltaTime) override;

protected:
	virtual Phrase run(LEDModuleControl* leds, ModuleService* modules);
	virtual void sendResult(Com* com, int16_t phraseId, bool missing);
};

#endif //BUTTERBOT_FIRMWARE_LEDROUTINE_H
