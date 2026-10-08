#include "PhraseEventRoutine.h"
#include <utility>
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include <Services/Com.h>
#include <Services/Audio/Audio.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include <Util/ServiceLocator.h>

DEFINE_LOG(PhraseEventRoutine)

PhraseEventRoutine::PhraseEventRoutine(BBStateMachine* sm, EventBag::EventData data) : EventRoutine(sm, std::move(data)) {}

Routine::TickingState PhraseEventRoutine::tick(float deltaTime){
    Phrase category = getPhraseCategory();
    if(category == Phrase::None || category == Phrase::COUNT){
        CMF_LOG(PhraseEventRoutine, LogLevel::Warning, "No phrase category, skipping");
        return TickingState::Done;
    }

    int16_t id = getPhraseID();
    if(id < 0){
        CMF_LOG(PhraseEventRoutine, LogLevel::Warning, "No phrase id, skipping");
        return TickingState::Done;
    }

    const Application* app = ApplicationStatics::getApplication();

    if(!ServiceLocator::SpeechGenInstance){
        CMF_LOG(PhraseEventRoutine, LogLevel::Error, "SpeechGen service missing!");
        return TickingState::Done;
    }

    Audio* audio = app->getService<Audio>();
    if(audio == nullptr){
        CMF_LOG(PhraseEventRoutine, LogLevel::Error, "Audio service missing!");
        return TickingState::Done;
    }

    if(!ServiceLocator::SpeechAudioGenInstance){
        CMF_LOG(PhraseEventRoutine, LogLevel::Error, "SpeechAudioGen service missing!");
        return TickingState::Done;
    }

    audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(getPhraseType(), Phrases::map(category, id)));
    audio->waitEnd(portMAX_DELAY);

    return TickingState::Done;
}

int16_t PhraseEventRoutine::getPhraseID(){
    return -1;
}

Phrase PhraseEventRoutine::getPhraseCategory(){
    return Phrase::None;
}

SpeechGen::InputType PhraseEventRoutine::getPhraseType(){
    return SpeechGen::InputType::Text;
}
