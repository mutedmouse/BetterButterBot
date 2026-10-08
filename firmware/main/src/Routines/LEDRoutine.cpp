#include "LEDRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <esp_log.h>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"

static constexpr const char* TAG = "LEDRoutine";

Routine::TickingState LEDRoutine::tick(float deltaTime){
	const Application* app = ApplicationStatics::getApplication();
	if(!app){
		ESP_LOGE(TAG, "Application instance is nullptr");
		abort();
	}

	Audio* audio = app->getService<Audio>();
	Com* com = app->getService<Com>();
	LEDModuleControl* leds = app->getService<LEDModuleControl>();
	ModuleService* moduleService = app->getService<ModuleService>();

	if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !leds){
		ESP_LOGE(TAG, "Missing required service(s)");
		abort();
	}

	Phrase phrase;
	bool missing = false;
	if(!moduleService || moduleService->getInserted(0) != Modules::Type::RM_LED){
		phrase = Phrase::LEDModuleMissingPhrases;
		missing = true;
	}else{
		phrase = run(leds, moduleService);
	}

	const int16_t id = Phrases::get(phrase);
	if(id < 0){
		return TickingState::Done;
	}

	sendResult(com, id, missing);

	auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(phrase, id));
	audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
	audio->waitEnd(portMAX_DELAY);

	return TickingState::Done;
}

Phrase LEDRoutine::run(LEDModuleControl*, ModuleService*){
	abort();
}

void LEDRoutine::sendResult(Com*, int16_t, bool){
	abort();
}
