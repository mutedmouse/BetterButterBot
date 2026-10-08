#include "PIRModuleOffRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Services/Modules/ModuleService.h>
#include <BBData.h>
#include <Phrases.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/LED/LED.h"
#include "Enums.h"
#include "EventBag.h"
#include <Util/ServiceLocator.h>

static constexpr const char* TAG = "PIRModuleOffRoutine";

Routine::TickingState PIRModuleOffRoutine::tick(float deltaTime){
	const Application* app = ApplicationStatics::getApplication();
	if(app == nullptr){
		ESP_LOGE(TAG, "Application instance is nullptr");
		abort();
	}

	Audio* audio = app->getService<Audio>();
	Com* com = app->getService<Com>();
	ModuleService* modules = app->getService<ModuleService>();
	EventBag* eventBag = app->getService<EventBag>();

	if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || com == nullptr || modules == nullptr){
		ESP_LOGE(TAG, "Missing required service(s)");
		abort();
	}

	const bool moduleInserted = modules->getInserted() == Modules::Type::RM_Motion;
	ESP_LOGI(TAG, "PIR module inserted: %s", moduleInserted ? "yes" : "no");

	if(!moduleInserted){
		if(eventBag != nullptr){
			eventBag->stopIntruderMonitoring();
		}

		LED<MonoLED, RGBLED>* ledService = app->getService<LED<MonoLED, RGBLED>>();
		if(ledService) ledService->off(MonoLED::PIRIndicator);

		const int16_t id = Phrases::get(Phrase::PIRModuleMissing);
		if(id < 0){
			return TickingState::Done;
		}

		PIRModuleData data{};
		data.id = static_cast<uint8_t>(id);
		data.missing = true;
		com->sendData(BB::State::Scenario, BB::Action::Scenario::IntruderDetectionOff, data);

		auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::PIRModuleMissing, id));
		audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
		audio->waitEnd(portMAX_DELAY);
		return TickingState::Done;
	}

	LED<MonoLED, RGBLED>* ledService = app->getService<LED<MonoLED, RGBLED>>();
	if(ledService) ledService->off(MonoLED::PIRIndicator);

	if(eventBag != nullptr){
		eventBag->stopIntruderMonitoring();
	}

	const int16_t id = Phrases::get(Phrase::IntruderDetectionOff);
	if(id < 0) return TickingState::Done;

	PIRModuleData data{};
	data.id = static_cast<uint8_t>(id);
	data.missing = false;
	com->sendData(BB::State::Scenario, BB::Action::Scenario::IntruderDetectionOff, data);

	auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::IntruderDetectionOff, id));
	audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
	audio->waitEnd(portMAX_DELAY);

	return TickingState::Done;
}
