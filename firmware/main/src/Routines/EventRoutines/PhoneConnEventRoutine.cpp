#include "PhoneConnEventRoutine.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include "Phrases.h"
#include "Services/Com.h"

DEFINE_LOG(PhoneConnEventRoutine)


PhoneConnEventRoutine::PhoneConnEventRoutine(BBStateMachine* sm, EventBag::EventData data) : PhraseEventRoutine(sm, data){
    Application* app = ApplicationStatics::getApplication();

    EventBag::PhoneData* phoneData = std::get_if<EventBag::PhoneData>(&data.payload);
    if(phoneData == nullptr){
        CMF_LOG(PhoneConnEventRoutine, LogLevel::Warning, "Phone connection routine without event data");
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

    com->sendData(BB::State::Idle, BB::Action::Idle::PhoneConnect, PhoneConnData{.id = static_cast<uint8_t>(phraseID), .connected = phoneData->connected});
}

Routine::TickingState PhoneConnEventRoutine::tick(float deltaTime){
    if(data.type != EventBag::EventType::Phone){
        return TickingState::Done;
    }

    return PhraseEventRoutine::tick(deltaTime);
}

int16_t PhoneConnEventRoutine::getPhraseID() {
    return phraseID;
}

Phrase PhoneConnEventRoutine::getPhraseCategory() {
    EventBag::PhoneData* phoneData = std::get_if<EventBag::PhoneData>(&data.payload);
    if(phoneData == nullptr){
        CMF_LOG(PhoneConnEventRoutine, LogLevel::Warning, "Phone connection routine without event data");
        return Phrase::None;
    }

    if(phoneData->connected){
        return Phrase::PhoneConnected;
    }

    return Phrase::PhoneDisconnected;
}

SpeechGen::InputType PhoneConnEventRoutine::getPhraseType(){
    return SpeechGen::InputType::Text;
}
