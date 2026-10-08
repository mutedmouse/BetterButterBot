#include "BatteryEventRoutine.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>

#include "EventBag.h"
#include "Phrases.h"
#include "Services/Com.h"

DEFINE_LOG(BatteryEventRoutine);


BatteryEventRoutine::BatteryEventRoutine(BBStateMachine* sm, EventBag::EventData data) : PhraseEventRoutine(sm, data){
	Application* app = ApplicationStatics::getApplication();

	Phrase phrase = getPhraseCategory();
	if(phrase == Phrase::COUNT || phrase == Phrase::None){
		return;
	}

	phraseID = Phrases::get(phrase);

	Com* com = app->getService<Com>();
	if(com == nullptr){
		return;
	}

	const EventBag::BatteryData* batteryData = std::get_if<EventBag::BatteryData>(&data.payload);
	if(batteryData == nullptr){
		CMF_LOG(BatteryEventRoutine, LogLevel::Warning, "Battery routine triggered with battery data nullptr.");
		return;
	}

	BB::Action::Idle action = BB::Action::Idle::None;
	BatteryData::State state = BatteryData::State::Low;

	if(batteryData->level == Battery::Level::COUNT){
		if(batteryData->chargingState == ChargingState::Unplugged){
			return;
		}

		// Not triggered by level change but by charging state
		if(batteryData->chargingState == ChargingState::Full){
			action = BB::Action::Idle::ChargingFull;
			state = BatteryData::State::ChargingFull;
		}else{
			action = BB::Action::Idle::Charging;
			state = BatteryData::State::Charging;
		}
	}else if(batteryData->level == Battery::Level::VeryLow){
		action = BB::Action::Idle::BatteryLow;
	}else{
		return;
	}

	com->sendData(BB::State::Idle, action, BatteryData{.id = static_cast<uint8_t>(phraseID), .state = state});
}

Routine::TickingState BatteryEventRoutine::tick(float deltaTime){
	if(data.type != EventBag::EventType::Battery){
		return TickingState::Done;
	}

	const EventBag::BatteryData* batteryData = std::get_if<EventBag::BatteryData>(&data.payload);
	if(batteryData == nullptr){
		CMF_LOG(BatteryEventRoutine, LogLevel::Warning, "Battery routine triggered with battery data nullptr.");
		return TickingState::Done;
	}

	if(batteryData->level != Battery::Level::VeryLow && batteryData->chargingState == ChargingState::Unplugged){
		return TickingState::Done;
	}

	return PhraseEventRoutine::tick(deltaTime);
}

int16_t BatteryEventRoutine::getPhraseID(){
	return phraseID;
}

Phrase BatteryEventRoutine::getPhraseCategory(){
	const EventBag::BatteryData* batteryData = std::get_if<EventBag::BatteryData>(&data.payload);
	if(batteryData == nullptr){
		CMF_LOG(BatteryEventRoutine, LogLevel::Warning, "Battery routine triggered with battery data nullptr.");
		return Phrase::None;
	}

	if(batteryData->level == Battery::Level::COUNT){
		// Not triggered by level change but by charging state
		if(batteryData->chargingState == ChargingState::Full){
			return Phrase::BatteryChargingFull;
		}

		return Phrase::BatteryCharging;
	}

	if(batteryData->level == Battery::Level::VeryLow){
		return Phrase::BatteryLow;
	}

	return Phrase::None;
}

SpeechGen::InputType BatteryEventRoutine::getPhraseType(){
	return SpeechGen::InputType::Text;
}
