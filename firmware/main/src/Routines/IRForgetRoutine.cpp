#include "IRForgetRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <Util/ServiceLocator.h>
#include "Audio/AudioFrontend.h"
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/IRStorage.h"

DEFINE_LOG(IRForgetRoutine)


Routine::TickingState IRForgetRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();
    AudioFrontend* af = app->getService<AudioFrontend>();

    if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !af || !ServiceLocator::IRStorageInstance){
        CMF_LOG(IRForgetRoutine, LogLevel::Error, "Missing required service(s)");
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
        IR_forgetData data{};
        data.state = IR_forgetData::State::Empty;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_forget, data);
        playPhrase(Phrase::IRListEmpty);
        return TickingState::Done;
    }

    {
        IR_forgetData data{};
        data.state = IR_forgetData::State::Listening;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_forget, data);
    }
    playPhrase(Phrase::IRForgetAskCommand);

			std::vector<AudioFrontend::Phrase> phrases;
			for(uint8_t i = 0; i < ServiceLocator::IRStorageInstance->getCount(); ++i){
				const IRCommand& cmd = ServiceLocator::IRStorageInstance->getCommand(i);
				phrases.push_back({ cmd.phonemes, cmd.phonemes, "", 0.35f });
			}

    af->setSpeechPhrases(phrases);
    const AudioFrontend::PhraseResult result = af->waitForPhrase();

    if(result.index == -1 && result.recognized){
        return TickingState::Done;
    }

    if(!result.recognized){
        if(result.transcript.empty()){
            playPhrase(Phrase::IRForgetTimeout);
        } else {
            IR_forgetData data{};
            data.state = IR_forgetData::State::NotRecognised;
            com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_forget, data);
            playPhrase(Phrase::IRForgetUnknown);
        }
        return TickingState::Done;
    }

    if(result.index < 0 || result.index >= static_cast<int>(ServiceLocator::IRStorageInstance->getCount())){
        IR_forgetData data{};
        data.state = IR_forgetData::State::NotRecognised;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_forget, data);
        playPhrase(Phrase::IRForgetUnknown);
        return TickingState::Done;
    }

    ServiceLocator::IRStorageInstance->remove(static_cast<uint8_t>(result.index));
    ServiceLocator::IRStorageInstance->store();

    {
        IR_forgetData data{};
        data.state = IR_forgetData::State::Done;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_forget, data);
    }
    playPhrase(Phrase::IRForgetSuccess);

    return TickingState::Done;
}
