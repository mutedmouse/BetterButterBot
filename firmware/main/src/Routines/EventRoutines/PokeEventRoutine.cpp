#include "PokeEventRoutine.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include <BBData.h>
#include "Services/Com.h"
#include <Util/stdafx.h>

DEFINE_LOG(PokeEventRoutine)


PokeEventRoutine::PokeEventRoutine(BBStateMachine* sm) : PhraseEventRoutine(sm, {}){
    Application* app = ApplicationStatics::getApplication();

    if(millis() - LastPokeTime >= ResetTime){
        Repetitions = 1;
    }else{
        ++Repetitions;
    }

    LastPokeTime = millis();

    Phrase phrase = PokeEventRoutine::getPhraseCategory();
    if(phrase == Phrase::COUNT || phrase == Phrase::None){
        CMF_LOG(PokeEventRoutine, LogLevel::Warning, "Skipping poke routine, no valid phrase category");
        return;
    }

    phraseID = Phrases::get(phrase);

    Com* com = app->getService<Com>();
    if(com == nullptr){
        CMF_LOG(PokeEventRoutine, LogLevel::Error, "Com service missing!");
        return;
    }

    com->sendData(BB::State::Idle, BB::Action::Idle::Poke, PokeData{
        .level = static_cast<PokeData::Level>(std::min(Repetitions / LevelIncreaseRepetitions, static_cast<int>(PokeData::Level::Three))),
        .id = static_cast<uint8_t>(phraseID)
    });
}

int16_t PokeEventRoutine::getPhraseID(){
    return phraseID;
}

Phrase PokeEventRoutine::getPhraseCategory(){
    switch(std::min(Repetitions / LevelIncreaseRepetitions, static_cast<int>(PokeData::Level::Three))){
        case 0: {
            return Phrase::PokeLvl1;
        }
        case 1: {
            return Phrase::PokeLvl2;
        }
        case 2: {
            return Phrase::PokeLvl3;
        }
        default: {
            CMF_LOG(PokeEventRoutine, LogLevel::Error, "PokeEventRoutine level None!");
            return Phrase::None;
        }
    }
}
