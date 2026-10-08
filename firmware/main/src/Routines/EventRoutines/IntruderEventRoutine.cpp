#include "IntruderEventRoutine.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include "Phrases.h"
#include "Services/Com.h"
#include "Services/LED/LEDBlinkFunction.h"
#include "Enums.h"

DEFINE_LOG(IntruderEventRoutine)


IntruderEventRoutine::IntruderEventRoutine(BBStateMachine* sm, EventBag::EventData data) : PhraseEventRoutine(sm, data){
	Application* app = ApplicationStatics::getApplication();

	EventBag::IntruderData* intruderData = std::get_if<EventBag::IntruderData>(&data.payload);
	if(intruderData == nullptr){
		CMF_LOG(IntruderEventRoutine, LogLevel::Warning, "Intruder routine without event data");
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

	com->sendData(BB::State::Idle, BB::Action::Idle::Intruder, IntruderData{ .id = static_cast<uint8_t>(phraseID), .detected = intruderData->detected });

	LED<MonoLED, RGBLED>* ledService = app->getService<LED<MonoLED, RGBLED>>();
	if(ledService){
		if(intruderData->detected){
			constexpr float period = PIRBlinkPeriodMs * 2 / 1000.0f;
			constexpr float onTime = PIRBlinkPeriodMs / 1000.0f;
			auto func = std::make_unique<LEDBlinkFunction<MonoLED, float>>(1.0f, period, onTime, 0);
			ledService->set(MonoLED::PIRIndicator, std::move(func));
		} else{
			ledService->on(MonoLED::PIRIndicator, 1.0f);
		}
	}
}

Routine::TickingState IntruderEventRoutine::tick(float deltaTime){
	if(data.type != EventBag::EventType::Intruder){
		return TickingState::Done;
	}

	return PhraseEventRoutine::tick(deltaTime);
}

int16_t IntruderEventRoutine::getPhraseID(){
	return phraseID;
}

Phrase IntruderEventRoutine::getPhraseCategory(){
	EventBag::IntruderData* intruderData = std::get_if<EventBag::IntruderData>(&data.payload);
	if(intruderData == nullptr){
		CMF_LOG(IntruderEventRoutine, LogLevel::Warning, "Intruder routine without event data");
		return Phrase::None;
	}

	if(intruderData->detected){
		return Phrase::IntruderYes;
	}

	return Phrase::IntruderNo;
}

SpeechGen::InputType IntruderEventRoutine::getPhraseType(){
	return SpeechGen::InputType::Text;
}
