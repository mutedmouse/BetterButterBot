#include "LEDModuleRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"

static constexpr const char* TAG = "LEDModuleRoutine";

Routine::TickingState LEDModuleRoutine::tick(float deltaTime){
	const Application* app = ApplicationStatics::getApplication();
	if(app == nullptr){
		ESP_LOGE(TAG, "Application instance is nullptr");
		abort();
	}

	Audio* audio = app->getService<Audio>();
	Com* com = app->getService<Com>();

	if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com){
		ESP_LOGE(TAG, "Missing required service(s)");
		abort();
	}

	const int16_t id = Phrases::get(Phrase::LEDModuleMissingPhrases);
	if(id < 0){
		return TickingState::Done;
	}

	LEDModuleData ledData{};
	ledData.id = id;
	ledData.missing = false;
	com->sendData(BB::State::Scenario, BB::Action::Scenario::LEDModule, ledData);

	auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::LEDModuleMissingPhrases, id));
	audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
	audio->waitEnd(portMAX_DELAY);

	return TickingState::Done;
}
