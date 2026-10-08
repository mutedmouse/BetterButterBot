#include "GasEventRoutine.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include "Phrases.h"
#include "Services/Com.h"

DEFINE_LOG(GasEventRoutine);


GasEventRoutine::GasEventRoutine(BBStateMachine* sm, EventBag::EventData data) : PhraseEventRoutine(sm, data){
    Application* app = ApplicationStatics::getApplication();

    const EventBag::GasData* gasData = std::get_if<EventBag::GasData>(&data.payload);
    if(gasData == nullptr){
        CMF_LOG(GasEventRoutine, LogLevel::Warning, "Gas routine triggered with gas data nullptr.");
        return;
    }

    Phrase phrase = getPhraseCategory();
    if(phrase == Phrase::COUNT || phrase == Phrase::None){
        return;
    }

    phraseID = Phrases::get(phrase);

    Com* com = app->getService<Com>();
    if(com == nullptr){
        return;
    }

    com->sendData(BB::State::Idle, BB::Action::Idle::Gas, GasData{.id = static_cast<uint8_t>(phraseID), .ok = gasData->ok});
}

Routine::TickingState GasEventRoutine::tick(float deltaTime){
    if(data.type != EventBag::EventType::Gas){
        return TickingState::Done;
    }

    return PhraseEventRoutine::tick(deltaTime);
}

int16_t GasEventRoutine::getPhraseID(){
    return phraseID;
}

Phrase GasEventRoutine::getPhraseCategory(){
    const EventBag::GasData* gasData = std::get_if<EventBag::GasData>(&data.payload);
    if(gasData == nullptr){
        CMF_LOG(GasEventRoutine, LogLevel::Warning, "Gas routine triggered with gas data nullptr.");
        return Phrase::None;
    }

    if(gasData->ok){
        return Phrase::GasOver;
    }

    return Phrase::GasUnder;
}

SpeechGen::InputType GasEventRoutine::getPhraseType(){
    return SpeechGen::InputType::Text;
}
