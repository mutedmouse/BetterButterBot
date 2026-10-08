#include "EightBallRoutine.h"
#include "States/BBStateMachine.h"
#include <Statics/ApplicationStatics.h>
#include <Util/stdafx.h>
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

DEFINE_LOG(EightBallRoutine)

EightBallRoutine::EightBallRoutine(BBStateMachine* sm) : Routine(sm){
	AudioFrontend* af = ApplicationStatics::getApplication()->getService<AudioFrontend>();
	af->loadModel();
}

EightBallRoutine::~EightBallRoutine(){
	AudioFrontend* af = ApplicationStatics::getApplication()->getService<AudioFrontend>();
	if(af == nullptr){
		return;
	}

	sm->unbindRoutine(af->onVAD);
	af->setMode(AudioFrontend::Mode::Off);
	af->unloadModel();
}

void EightBallRoutine::handleVAD(bool active){
	CMF_LOG(EightBallRoutine, LogLevel::Info, "VAD: %s", active ? "SPEECH" : "SILENCE");
	if(active && !speechDetected){
		speechDetected = true;
	}else if(!active && speechDetected){
		speechEnded = true;
	}
}

Routine::TickingState EightBallRoutine::tick(float deltaTime){
	const Application* app = ApplicationStatics::getApplication();
	Audio* audio = app->getService<Audio>();
	Com* com = app->getService<Com>();
	AudioFrontend* af = app->getService<AudioFrontend>();

	if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !af){
		CMF_LOG(EightBallRoutine, LogLevel::Error, "Missing required service(s)");
		return TickingState::Done;
	}

	auto startPhrase = [&](Phrase phrase){
		const int16_t id = Phrases::get(phrase);
		if(id < 0){
			CMF_LOG(EightBallRoutine, LogLevel::Warning, "No phrase output for phrase id %d", (int)phrase);
			return;
		}
		const std::string text = Phrases::map(phrase, id);
		CMF_LOG(EightBallRoutine, LogLevel::Info, "Playing: '%s'", text.c_str());
		auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, text);
		audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
		audio->waitEnd(portMAX_DELAY);
	};

	switch(phase){
		case Phase::Init: {
			{
				EightBallData data{};
				data.phase = EightBallData::Phase::Listening;
				com->sendData(BB::State::Scenario, BB::Action::Scenario::EightBall, data);
			}

			speechDetected = false;
			speechEnded = false;
			startPhrase(Phrase::EightBallListening);
			delayMillis(800);
			sm->bindRoutine(af->onVAD, this, &EightBallRoutine::handleVAD);
			af->setVADParams(VAD_MODE_3, 128, 1000, 1024); // TODO might improve speech detection??
			af->setMode(AudioFrontend::Mode::VAD);
			listenStartMs = millis();
			phase = Phase::Listening;
			break;
		}

		case Phase::Listening: {
			if(speechDetected){
				CMF_LOG(EightBallRoutine, LogLevel::Info, "Speech detected, waiting for end...");
				speechStartMs = millis();
				phase = Phase::WaitingForSpeechEnd;
			}else if(millis() - listenStartMs >= 4000){
				CMF_LOG(EightBallRoutine, LogLevel::Info, "Timeout — no speech detected");
				af->setMode(AudioFrontend::Mode::Off);
				sm->unbindRoutine(af->onVAD);
				startPhrase(Phrase::EightBallNoQuestion);
				return TickingState::Done;
			}
			break;
		}

		case Phase::WaitingForSpeechEnd: {
			if(speechEnded || millis() - speechStartMs >= MaxSpeechDurationMs){
				af->setMode(AudioFrontend::Mode::Off);
				sm->unbindRoutine(af->onVAD);

				const int16_t id = Phrases::get(Phrase::EightBall);
				const std::string responseText = Phrases::map(Phrase::EightBall, id);
				CMF_LOG(EightBallRoutine, LogLevel::Info, "Response: '%s'", responseText.c_str());

				{
					EightBallData data{};
					data.phase = EightBallData::Phase::Response;
					data.responseId = static_cast<uint8_t>(id);
					com->sendData(BB::State::Scenario, BB::Action::Scenario::EightBall, data);
				}

				auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, responseText);
				audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
				audio->waitEnd(portMAX_DELAY);
				return TickingState::Done;
			}
			break;
		}

		default:
			break;
	}

	return TickingState::Continue;
}
