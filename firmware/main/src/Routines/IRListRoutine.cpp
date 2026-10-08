#include "IRListRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <cstdio>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/IRStorage.h"

DEFINE_LOG(IRListRoutine)

Routine::TickingState IRListRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();

    if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !ServiceLocator::IRStorageInstance){
        CMF_LOG(IRListRoutine, LogLevel::Error, "Missing required service(s)");
        return TickingState::Done;
    }

    auto playText = [&](const std::string& text){
        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, text);
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
    };

    auto playPhrase = [&](Phrase phrase){
        const int16_t id = Phrases::get(phrase);
        if(id < 0) return;
        playText(Phrases::map(phrase, id));
    };

    if(ServiceLocator::IRStorageInstance->getCount() == 0){
        IR_listData data{};
        data.state = IR_listData::State::NoActions;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_list, data);
        playPhrase(Phrase::IRListEmpty);
        return TickingState::Done;
    }

    {
        IR_listData data{};
        data.state = IR_listData::State::ListingActions;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_list, data);
    }
    playPhrase(Phrase::IRListPrefix);

    for(uint8_t i = 0; i < ServiceLocator::IRStorageInstance->getCount(); ++i){
        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Phonemes, ServiceLocator::IRStorageInstance->getCommand(i).phonemes);
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
        delayMillis(200);
    }

    return TickingState::Done;
}
