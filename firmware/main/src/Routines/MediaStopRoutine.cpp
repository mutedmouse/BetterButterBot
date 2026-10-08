#include "MediaStopRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Phone/Phone.h"

DEFINE_LOG(MediaStopRoutine)

Routine::TickingState MediaStopRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();
    Phone* phone = app->getService<Phone>();

    if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !phone){
        CMF_LOG(MediaStopRoutine, LogLevel::Error, "Missing required service(s)");
        return TickingState::Done;
    }

    auto playPhrase = [&](Phrase phrase) {
        const int16_t id = Phrases::get(phrase);
        if(id < 0){
            CMF_LOG(MediaStopRoutine, LogLevel::Warning, "No phrase output for phrase %d", (int)phrase);
            return;
        }
        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(phrase, id));
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
    };

    if(!phone->isConnected()){
        const int16_t id = Phrases::get(Phrase::PhoneNotConnected);
        if(id >= 0){
            PhoneNotConnectedData bbData{};
            bbData.id = static_cast<uint8_t>(id);
            com->sendData(BB::State::Scenario, BB::Action::Scenario::PhoneNotConnected, bbData);
            playPhrase(Phrase::PhoneNotConnected);
        }
        return TickingState::Done;
    }

    phone->doMediaPause();

    const int16_t id = Phrases::get(Phrase::PhoneStopMusic);
    if(id < 0) return TickingState::Done;

    MediaControlData bbData{};
    bbData.id = static_cast<uint8_t>(id);
    com->sendData(BB::State::Scenario, BB::Action::Scenario::PhoneStopMusic, bbData);

    playPhrase(Phrase::PhoneStopMusic);
    return TickingState::Done;
}
