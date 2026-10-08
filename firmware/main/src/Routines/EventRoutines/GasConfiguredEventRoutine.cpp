#include "GasConfiguredEventRoutine.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include <Services/Com.h>

GasConfiguredEventRoutine::GasConfiguredEventRoutine(BBStateMachine* sm, EventBag::EventData data) : PhraseEventRoutine(sm, std::move(data)){
	phraseID = Phrases::get(Phrase::GasCalibrationFinished);

	if(phraseID < 0){
		return;
	}

	const Application* app = ApplicationStatics::getApplication();
	if(Com* com = app->getService<Com>()){
		com->sendData(BB::State::Idle, BB::Action::Idle::GasConfigureEnd, GasConfigureEndData{ .id = static_cast<uint8_t>(phraseID) });
	}
}

Phrase GasConfiguredEventRoutine::getPhraseCategory(){
	return Phrase::GasCalibrationFinished;
}

int16_t GasConfiguredEventRoutine::getPhraseID(){
	return phraseID;
}

SpeechGen::InputType GasConfiguredEventRoutine::getPhraseType(){
	return SpeechGen::InputType::Text;
}
