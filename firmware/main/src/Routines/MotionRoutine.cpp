#include "MotionRoutine.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include <Phrases.h>
#include <Audio/SpeechAudioGen.h>
#include <Audio/SpeechAudioSource.h>
#include <Audio/SpeechGen.h>
#include <Services/Com.h>
#include <Services/Audio/Audio.h>
#include <Util/ServiceLocator.h>

DEFINE_LOG(MotionRoutine)

MotionRoutine::MotionRoutine(BBStateMachine* sm, MotionType type) : Routine(sm), motionType(type){}

Routine::TickingState MotionRoutine::tick(float deltaTime){
	BB::Action::Idle action;
	Phrase phrase;
	switch(motionType){
		case MotionType::Fall:
			phrase = Phrase::Fall;
			action = BB::Action::Idle::Fall;
			break;
		case MotionType::UpsideDown:
			phrase = Phrase::UpsideDown;
			action = BB::Action::Idle::UpsideDown;
			break;
		case MotionType::Pickup:
			phrase = Phrase::PickedUp;
			action = BB::Action::Idle::PickUp;
			break;
		case MotionType::Shake:
			phrase = Phrase::Shake;
			action = BB::Action::Idle::Shake;
			break;
		default:
			CMF_LOG(MotionRoutine, LogLevel::Warning, "Unhandled motion type: %d", static_cast<int>(motionType));
			return TickingState::Done;
	}

	const Application* app = ApplicationStatics::getApplication();
	Audio* audio = app->getService<Audio>();
	Com* com = app->getService<Com>();

	if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || com == nullptr){
		CMF_LOG(MotionRoutine, LogLevel::Error, "Missing required service(s)");
		return TickingState::Done;
	}

	com->sendData(BB::State::Idle, action, BBData{});

	const int16_t id = Phrases::get(phrase);
	if(id < 0){
		CMF_LOG(MotionRoutine, LogLevel::Warning, "Phrases::get returned no phrase id for phrase %d", static_cast<int>(phrase));
		return TickingState::Done;
	}

	auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(phrase, id));
	audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
	audio->waitEnd(portMAX_DELAY);

	return TickingState::Done;
}
