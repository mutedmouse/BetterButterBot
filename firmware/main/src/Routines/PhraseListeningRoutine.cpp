#include "PhraseListeningRoutine.h"
#include <vector>
#include <utility>
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include "States/BBStateMachine.h"

PhraseListeningRoutine::PhraseListeningRoutine(BBStateMachine* sm) : Routine(sm){
	af = ApplicationStatics::getApplication()->getService<AudioFrontend>();
	if(af != nullptr){
		af->loadModel();
	}
}

PhraseListeningRoutine::~PhraseListeningRoutine(){
	if(af == nullptr){
		return;
	}

	af->setMode(AudioFrontend::Mode::Off);
	if(bound){
		sm->unbindRoutine(af->onPhrase);
	}
	af->unloadModel();
}

void PhraseListeningRoutine::listen(std::span<const AudioFrontend::Phrase> phrases){
	if(af == nullptr){
		return;
	}

	if(!bound){
		sm->bindRoutine(af->onPhrase, this, &PhraseListeningRoutine::onPhrase);
		bound = true;
	}

	// Phrases can only be set while recognition is stopped.
	af->setMode(AudioFrontend::Mode::Off);

	std::vector<AudioFrontend::Phrase> mutablePhrases(phrases.begin(), phrases.end());
	af->setSpeechPhrases(mutablePhrases);

	result = {};
	resultReady = false;

	af->setMode(AudioFrontend::Mode::Speech);
}

void PhraseListeningRoutine::onPhrase(bool recognized, int index, std::string transcript, float confidence){
	if(resultReady){
		return;
	}

	result = { recognized, index, std::move(transcript), confidence };
	resultReady = true;

	// Stop recognition the moment a result lands, mirroring waitForPhrase, so the AFE doesn't keep
	// listening (and re-firing) while the routine speaks its response.
	af->setMode(AudioFrontend::Mode::Off);
}
