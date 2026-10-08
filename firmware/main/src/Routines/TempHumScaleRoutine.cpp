#include "TempHumScaleRoutine.h"
#include <Statics/ApplicationStatics.h>
#include "TempHumModuleRoutine.h"
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/Settings.h"

static constexpr const char* TAG = "TempHumScaleRoutine";

static bool playScalePhrase(Phrase phrase, BB::Action::Scenario action, SettingsStruct::Scale scale){
    const Application* app = ApplicationStatics::getApplication();
    if(app == nullptr){
        ESP_LOGE(TAG, "Application instance is nullptr");
        abort();
    }

    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();

    if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || com == nullptr){
        ESP_LOGE(TAG, "Missing required service(s)");
        abort();
    }

    if(ServiceLocator::SettingsInstance){
        SettingsStruct s = ServiceLocator::SettingsInstance->get();
        s.tempHumScale = scale;
        ServiceLocator::SettingsInstance->set(s);
        ServiceLocator::SettingsInstance->store();
    }

    const int16_t id = Phrases::get(phrase);
    if(id < 0) return true;

    TempHumScaleData data{};
    data.id = static_cast<uint8_t>(id);
    data.scale = scale;
    com->sendData(BB::State::Scenario, action, data);

    auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(phrase, id));
    audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
    audio->waitEnd(portMAX_DELAY);

    return true;
}

Routine::TickingState TempHumCelsiusRoutine::tick(float deltaTime){
	TempHumModuleRoutine::currentScale = SettingsStruct::Scale::Celsius;
	playScalePhrase(Phrase::TempHumScaleCelsius, BB::Action::Scenario::TempHumScaleCelsius, SettingsStruct::Scale::Celsius);
	return TickingState::Done;
}

Routine::TickingState TempHumFahrenheitRoutine::tick(float deltaTime){
	TempHumModuleRoutine::currentScale = SettingsStruct::Scale::Fahrenheit;
	playScalePhrase(Phrase::TempHumScaleFahrenheit, BB::Action::Scenario::TempHumScaleFahrenheit, SettingsStruct::Scale::Fahrenheit);
	return TickingState::Done;
}

Routine::TickingState TempHumKelvinRoutine::tick(float deltaTime){
	TempHumModuleRoutine::currentScale = SettingsStruct::Scale::Kelvin;
	playScalePhrase(Phrase::TempHumScaleKelvin, BB::Action::Scenario::TempHumScaleKelvin, SettingsStruct::Scale::Kelvin);
	return TickingState::Done;
}
