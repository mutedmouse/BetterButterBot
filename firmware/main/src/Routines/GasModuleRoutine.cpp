#include "GasModuleRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Services/Modules/ModuleService.h>
#include <Services/Modules/ModuleDevices/RM_CO2Sensor.h>
#include <BBData.h>
#include <Phrases.h>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"

static constexpr const char* TAG = "GasModuleRoutine";

Routine::TickingState GasModuleRoutine::tick(float deltaTime) {
    const Application* app = ApplicationStatics::getApplication();
    if(app == nullptr){
        ESP_LOGE(TAG, "Application instance is nullptr");
        abort();
    }

    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();
    ModuleService* modules = app->getService<ModuleService>();

    if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || com == nullptr || modules == nullptr){
        ESP_LOGE(TAG, "Missing required service(s)");
        abort();
    }

    const bool moduleInserted = modules->getInserted() == Modules::Type::RM_CO2;

    ESP_LOGI(TAG, "Gas module inserted: %s", moduleInserted ? "yes" : "no");

    if(!moduleInserted){
        const int16_t id = Phrases::get(Phrase::GasModuleMissing);
        if(id < 0){
	        return TickingState::Done;
        }

        GasData data{};
        data.id = static_cast<uint8_t>(id);
        data.missing = true;
        data.ok = false;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::GasModule, data);

        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::GasModuleMissing, id));
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
    	return TickingState::Done;
    }

    RM_CO2Sensor* sensor = cast<RM_CO2Sensor>(modules->getDevice().get());
    if(sensor == nullptr){
        ESP_LOGE(TAG, "Failed to cast module device to RM_CO2Sensor");
    	return TickingState::Done;
    }

    const uint32_t reading = sensor->getReading();
    const bool ok = sensor->isOK();
    ESP_LOGI(TAG, "CO2 sensor reading: %d (status: %s)", reading, ok ? "OK" : "BAD");
    const Phrase phrase = ok ? Phrase::AirQualityOK : Phrase::AirQualityBad;

    const int16_t id = Phrases::get(phrase);
    if(id < 0){
	    return TickingState::Done;
    }

    GasData data{};
    data.id = static_cast<uint8_t>(id);
    data.missing = false;
    data.ok = ok;
    com->sendData(BB::State::Scenario, BB::Action::Scenario::GasModule, data);

    auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(phrase, id));
    audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
    audio->waitEnd(portMAX_DELAY);

	return TickingState::Done;
}
