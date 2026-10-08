#include "FaceRoutine.h"
#include "States/BBStateMachine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Util/ServiceLocator.h>
#include <Util/stdafx.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "FaceDet.h"

DEFINE_LOG(FaceRoutine)

FaceRoutine::~FaceRoutine() noexcept{
	if(FaceDet* faceDet = ApplicationStatics::getApplication()->getService<FaceDet>()){
		if(faceDetActive){
			faceDet->stop();
		}
		sm->unbindRoutine(faceDet->OnDetect);
		if(faceDetActive){
			faceDet->unloadModel();
		}
	}
}

Routine::TickingState FaceRoutine::tick(float deltaTime){
	const Application* app = ApplicationStatics::getApplication();

	FaceDet* faceDet = app->getService<FaceDet>();
	if(faceDet == nullptr){
		CMF_LOG(FaceRoutine, LogLevel::Error, "Missing FaceDet or AudioFrontend service");
		return TickingState::Done;
	}

	switch(phase){
		case Phase::Init: {
			// The AudioFrontend and FaceDet models share PSRAM and cannot be loaded at the same time.
			faceDet->loadModel();
			faceDetActive = true;

			if(!prepare(faceDet)){
				faceDet->unloadModel();
				faceDetActive = false;
				return TickingState::Done;
			}

			onScanStart();

			detected = false;
			detectedKnown = false;
			sm->bindRoutine(faceDet->OnDetect, this, &FaceRoutine::onDetect);

			startScan(faceDet);
			scanStartMs = millis();
			phase = Phase::Scanning;
			return TickingState::Continue;
		}

		case Phase::Scanning: {
			const bool timedOut = !detected && (millis() - scanStartMs >= DetectionTimeoutMs);
			if(!detected && !timedOut){
				return TickingState::Continue;
			}

			faceDet->stop();
			sm->unbindRoutine(faceDet->OnDetect);

			if(detected){
				onFaceDetected(faceDet, detectedKnown);
			} else{
				onScanTimeout();
			}

			faceDet->unloadModel();
			faceDetActive = false;
			return TickingState::Done;
		}
	}

	return TickingState::Done;
}

void FaceRoutine::startScan(FaceDet* faceDet){
	faceDet->start(true);
}

void FaceRoutine::speak(Phrase phrase, int16_t id) const{
	if(id < 0){
		return;
	}

	const Application* app = ApplicationStatics::getApplication();
	Audio* audio = app->getService<Audio>();

	if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance){
		CMF_LOG(FaceRoutine, LogLevel::Error, "Missing audio service(s)");
		return;
	}

	auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(phrase, id));
	audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
	audio->waitEnd(portMAX_DELAY);
}

void FaceRoutine::onDetect(int faceCount, bool known, glm::vec2 pos, glm::vec2 size){
	CMF_LOG(FaceRoutine, Info, "faceCount %d, known %d, pos (%.2f, %.2f), size (%.2f, %.2f)", faceCount, known, pos.x, pos.y, size.x, size.y);

	if(faceCount <= 0){
		return;
	}

	// Called on the StateMachine task between event-scan and the next routine tick;
	// same task as the reader, so a plain bool is sufficient.
	detectedKnown = known;
	detected = true;
}
