#include "SpeakReplyRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Util/TextSanitize.h"
#include "States/ListenState.h"

DEFINE_LOG(SpeakReplyRoutine)

std::string SpeakReplyRoutine::pending;
bool SpeakReplyRoutine::followUpAfter = false;

void SpeakReplyRoutine::setText(const std::string& text, bool followUp){
	// flite only handles plain ASCII; the gateway already strips markdown and caps the length.
	sanitizeToAscii(text, pending);
	followUpAfter = followUp;
}

Routine::TickingState SpeakReplyRoutine::tick(float deltaTime){
	Audio* audio = ApplicationStatics::getApplication()->getService<Audio>();
	if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || pending.empty()){
		return TickingState::Done;
	}

	std::string text;
	text.swap(pending);
	CMF_LOG(SpeakReplyRoutine, LogLevel::Info, "Speaking reply (%u chars)", (unsigned) text.size());

	audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, text));
	audio->waitEnd(portMAX_DELAY);

	// Conversation mode: after an answer, listen again briefly for a follow-up (ScenarioState
	// routes Done -> ListenState instead of IdleState).
	if(followUpAfter){
		ListenState::requestFollowUp();
	}

	return TickingState::Done;
}
