#include "IRForgetAllRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <BBData.h>
#include <Phrases.h>
#include <Util/ServiceLocator.h>
#include "Audio/AudioFrontend.h"
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Audio/Audio.h"
#include "Services/Com.h"
#include "Services/IRStorage.h"

DEFINE_LOG(IRForgetAllRoutine)

Routine::TickingState IRForgetAllRoutine::tick(float deltaTime){
    const Application* application = ApplicationStatics::getApplication();
    Audio* audio = application->getService<Audio>();
    Com* com = application->getService<Com>();

    if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !ServiceLocator::IRStorageInstance){
        CMF_LOG(IRForgetAllRoutine, LogLevel::Error, "Missing required service(s)");
        return TickingState::Done;
    }

    auto playPhrase = [&](Phrase phrase){
        const int16_t id = Phrases::get(phrase);
        if(id < 0) return;
        const std::string text = Phrases::map(phrase, id);
        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, text);
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
    };

    if(ServiceLocator::IRStorageInstance->getCount() == 0){
        IR_forgetAllData data{};
        data.state = IR_forgetAllData::State::Empty;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_forgetAll, data);
        playPhrase(Phrase::IRForgetAllEmpty);
        return TickingState::Done;
    }

    ServiceLocator::IRStorageInstance->clear();
    ServiceLocator::IRStorageInstance->store();

    IR_forgetAllData data{};
    data.state = IR_forgetAllData::State::Done;
    com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_forgetAll, data);

    playPhrase(Phrase::IRForgetAllSuccess);

    return TickingState::Done;
}
