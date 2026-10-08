#include "ForgetFaceRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include "Util/ServiceLocator.h"
#include "Audio/AudioFrontend.h"
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "FaceDet.h"
#include "Services/Com.h"

DEFINE_LOG(ForgetFaceRoutine)

Routine::TickingState ForgetFaceRoutine::tick(float deltaTime){
	const Application* app = ApplicationStatics::getApplication();
	Audio* audio = app->getService<Audio>();
	Com* com = app->getService<Com>();
	FaceDet* faceDet = app->getService<FaceDet>();

	if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || faceDet == nullptr){
		CMF_LOG(ForgetFaceRoutine, LogLevel::Error, "Missing required service(s)");
		return TickingState::Done;
	}

	// The recognizer's stored features live with the FaceDet model, so we have to swap models
	// even though no camera scan happens.
	faceDet->loadModel();

	const bool hadFace = faceDet->forgetFace();

	faceDet->unloadModel();

	const Phrase phrase = hadFace ? Phrase::FaceForgotten : Phrase::FaceNoOwner;
	const int16_t id = Phrases::get(phrase);
	if(id < 0) return TickingState::Done;

	if(com != nullptr){
		FaceForgetData data{
			.state = hadFace ? FaceForgetData::State::Done : FaceForgetData::State::NoDataExist
		};
		com->sendData(BB::State::Scenario, BB::Action::Scenario::FaceDetectForget, data);
	}

	auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(phrase, id));
	audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
	audio->waitEnd(portMAX_DELAY);

	return TickingState::Done;
}
