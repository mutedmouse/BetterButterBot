#include "TempHumModuleRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Services/Modules/ModuleService.h>
#include <Services/Modules/ModuleDevices/RM_TempHumModule.h>
#include <BBData.h>
#include <Phrases.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/Settings.h"
#include <cstdio>
#include <Util/ServiceLocator.h>

static constexpr const char* TAG = "TempHumModuleRoutine";

SettingsStruct::Scale TempHumModuleRoutine::currentScale = SettingsStruct::Scale::Celsius;

Routine::TickingState TempHumModuleRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    if(app == nullptr){
        ESP_LOGE(TAG, "Application instance is nullptr");
        abort();
    }

    if(!scaleLoaded){
        if(ServiceLocator::SettingsInstance){
            currentScale = ServiceLocator::SettingsInstance->get().tempHumScale;
            scaleLoaded = true;
        }
    }

    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();
    ModuleService* modules = app->getService<ModuleService>();

    if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || com == nullptr || modules == nullptr){
        ESP_LOGE(TAG, "Missing required service(s)");
        abort();
    }

    const bool moduleInserted = modules->getInserted() == Modules::Type::RM_TempHum;

    if(!moduleInserted){
        const int16_t id = Phrases::get(Phrase::TempHumModuleMissing);
        if(id < 0){
	        return TickingState::Done;
        }

        TempHumModuleData data{};
        data.id = static_cast<uint8_t>(id);
        data.missing = true;
        data.temperature = 0;
        data.humidity = 0;
        data.scale = currentScale;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::TempHumModule, data);

        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::TempHumModuleMissing, id));
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
        return TickingState::Done;
    }

    RM_TempHumModule* sensor = cast<RM_TempHumModule>(modules->getDevice().get());
    if(sensor == nullptr){
        ESP_LOGE(TAG, "Failed to cast module device to RM_TempHumModule");
        return TickingState::Done;
    }

    sensor->sample();
    const int16_t celsius = sensor->getTemperature();
    const uint8_t humidity = sensor->getHumidity();
    ESP_LOGI(TAG, "TempHum reading: %d°C, %d%%", celsius, humidity);

    const int16_t id = Phrases::get(Phrase::TempHumReading);
    if(id < 0){
	    return TickingState::Done;
    }

    const std::string templ = Phrases::map(Phrase::TempHumReading, id);
    const int16_t converted = convertTemp(celsius);
    const std::string tempStr = formatTemp(converted);

    char buf[256];
    snprintf(buf, sizeof(buf), templ.c_str(), tempStr.c_str(), static_cast<int>(humidity));

    TempHumModuleData data{};
    data.id = static_cast<uint8_t>(id);
    data.missing = false;
    data.temperature = converted;
    data.humidity = humidity;
    data.scale = currentScale;
    com->sendData(BB::State::Scenario, BB::Action::Scenario::TempHumModule, data);

    auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, std::string(buf));
    audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
    audio->waitEnd(portMAX_DELAY);

    return TickingState::Done;
}

int16_t TempHumModuleRoutine::convertTemp(int16_t celsius){
    switch(currentScale){
        case SettingsStruct::Scale::Celsius:
            return celsius;
        case SettingsStruct::Scale::Fahrenheit:
            return static_cast<int16_t>((static_cast<int32_t>(celsius) * 9) / 5 + 32);
        case SettingsStruct::Scale::Kelvin:
            return static_cast<int16_t>(static_cast<int32_t>(celsius) + 273);
    }
    return celsius;
}

std::string TempHumModuleRoutine::formatTemp(int16_t converted){
    char buf[64];
    switch(currentScale){
        case SettingsStruct::Scale::Celsius:
            snprintf(buf, sizeof(buf), "%d degrees Celsius", converted);
            break;
        case SettingsStruct::Scale::Fahrenheit:
            snprintf(buf, sizeof(buf), "%d degrees Fahrenheit", converted);
            break;
        case SettingsStruct::Scale::Kelvin:
            snprintf(buf, sizeof(buf), "%d Kelvin", converted);
            break;
    }
    return buf;
}
