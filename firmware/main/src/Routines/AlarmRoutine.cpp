#include "AlarmRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Util/ServiceLocator.h>
#include <driver/gpio.h>
#include <sys/stat.h>
#include <Services/Audio/AACAudioGenerator.h>
#include <Services/Audio/FileAudioSource.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Util/TextSanitize.h"
#include "Pins.hpp"

DEFINE_LOG(AlarmRoutine)

std::string AlarmRoutine::pending;
bool AlarmRoutine::pendingTimer = false;

void AlarmRoutine::setText(const std::string& text, const std::string& song){
	sanitizeToAscii(text, pending);
	pendingTimer = song == "timer";
}

bool AlarmRoutine::buttonDown(){
	return gpio_get_level(static_cast<gpio_num_t>(PIN_BTN)) != 0;
}

bool AlarmRoutine::waitOrDismiss(Audio* audio){
	// Polled rather than event-driven: this routine blocks the state machine like SpeakReplyRoutine,
	// so button events would only be delivered after the alarm had finished.
	vTaskDelay(pdMS_TO_TICKS(50)); // let playback start
	while(audio->isPlaying()){
		if(buttonDown()){
			audio->stop();
			return true;
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	}
	return buttonDown();
}

Routine::TickingState AlarmRoutine::tick(float deltaTime){
	const Application* app = ApplicationStatics::getApplication();
	Audio* audio = app->getService<Audio>();
	AACAudioGenerator* aacGen = app->getService<AACAudioGenerator>();
	if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance){
		return TickingState::Done;
	}

	std::string text;
	text.swap(pending);
	const bool timer = pendingTimer;
	pendingTimer = false;
	if(text.empty()){
		text = timer ? "Your timer is done." : "Your alarm is going off.";
	}

	const char* songPath = timer ? TimerSongPath : AlarmSongPath;
	struct stat st{};
	const bool haveSong = aacGen != nullptr && stat(songPath, &st) == 0;
	CMF_LOG(AlarmRoutine, LogLevel::Info, "%s: '%s'%s", timer ? "Timer" : "Alarm", text.c_str(), haveSong ? "" : " (song file missing)");

	bool dismissed = false;
	for(int round = 0; round < Rounds && !dismissed; ++round){
		if(haveSong){
			audio->play(aacGen, std::make_unique<FileAudioSource>(songPath));
			dismissed = waitOrDismiss(audio);
		}
		if(!dismissed){
			audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, text));
			dismissed = waitOrDismiss(audio);
		}
	}

	if(dismissed){
		while(buttonDown()){ // a held press is the long-press power-off; don't speak over it
			vTaskDelay(pdMS_TO_TICKS(50));
		}
		audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text,
																						   timer ? "Timer off. Toast on." : "Alarm off. Butter on."));
		audio->waitEnd(portMAX_DELAY);
	}
	CMF_LOG(AlarmRoutine, LogLevel::Info, "%s %s", timer ? "Timer" : "Alarm", dismissed ? "dismissed" : "finished");
	return TickingState::Done;
}
