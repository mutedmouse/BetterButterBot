#include "LEDControl.h"
#include <Util/stdafx.h>
#include <Log/Log.h>
#include <Services/Audio/Audio.h>
#include <Services/LED/LEDBlinkFunction.h>

DEFINE_LOG(LEDControl);

LEDControl::LEDControl(LED<MonoLED, RGBLED>* ledService) : ledService(ledService){

}

LEDControl::~LEDControl(){
	if(audioService == nullptr){
		return;
	}

	Application* app = Application::getApp();
	audioService->OnAudioStatusChanged.unbind(app);

}

void LEDControl::registerToAudio(Audio* audio){
	if(audio == nullptr){
		CMF_LOG(LEDControl, LogLevel::Warning, "registerToAudio: audio is null!");
		return;
	}

	audioService = audio;
	audio->OnAudioStatusChanged.bind(Application::getApp(), [this](bool value) { onAudioPlayStatusChanged(value); });
}

void LEDControl::startIdle(){
	CMF_LOG(LEDControl, Debug, "startIdle");
	std::lock_guard lock(stateMutex);
	playing = false;
	listening = false;
	applyState();
}

void LEDControl::startInit(){
	CMF_LOG(LEDControl, Debug, "startInit");
	setSolidOn();
}

void LEDControl::startBatteryCritical(bool continuous){
	CMF_LOG(LEDControl, Debug, "startBatteryCritical");
	std::lock_guard lock(stateMutex);
	pattern = Pattern::BattCritical;
	blinkOnTime = BattCriticalBlinkOnTime / 1000.0f;
	blinkOffTime = BattCriticalBlinkOffTime / 1000.0f;

	if(ledService == nullptr){
		CMF_LOG(LEDControl, LogLevel::Warning, "startBatteryCritical: ledService is null!");
		return;
	}

	ledService->set(MonoLED::Status, std::make_unique<LEDBlinkFunction<MonoLED, float>>(LedBrightness, blinkOnTime + blinkOffTime, blinkOnTime, continuous ? 0 : 3));
}

bool LEDControl::waitForBatteryCritical(TickType_t wait) const{
	if(pattern != Pattern::BattCritical){
		CMF_LOG(LEDControl, LogLevel::Warning, "Waiting for battery critical, but pattern is not battery critical. Early returning...");
		return true;
	}

	if(ledService == nullptr){
		CMF_LOG(LEDControl, LogLevel::Warning, "waitForBatteryCritical: ledService is null!");
		return true;
	}

	return ledService->waitFor(MonoLED::Status, wait);
}

void LEDControl::startPlay(){
	CMF_LOG(LEDControl, Debug, "startPlay");
	std::lock_guard lock(stateMutex);
	playing = true;
	applyState();
}

void LEDControl::stopPlay(){
	CMF_LOG(LEDControl, Debug, "stopPlay");
	std::lock_guard lock(stateMutex);
	playing = false;
	applyState();
}

void LEDControl::startListen(){
	CMF_LOG(LEDControl, Debug, "startListen");
	std::lock_guard lock(stateMutex);
	listening = true;
	applyState();
}

void LEDControl::stopListen(){
	CMF_LOG(LEDControl, Debug, "stopListen");
	std::lock_guard lock(stateMutex);
	listening = false;
	applyState();
}

void LEDControl::setSolidOn(){
	pattern = Pattern::On;
	setLedState(true);
}

void LEDControl::setBlink(float onTime, float offTime){
	pattern = Pattern::Blink;
	blinkOnTime = onTime;
	blinkOffTime = offTime;

	if(ledService == nullptr){
		CMF_LOG(LEDControl, LogLevel::Warning, "ledBlink: ledService is null!");
		return;
	}

	ledService->set(MonoLED::Status, std::make_unique<LEDBlinkFunction<MonoLED, float>>(LedBrightness, blinkOnTime + blinkOffTime, blinkOnTime));
}

void LEDControl::setLedState(bool on) const{
	if(ledService == nullptr){
		CMF_LOG(LEDControl, LogLevel::Warning, "setLedState: ledService is null!");
		return;
	}

	if(on){
		ledService->on(MonoLED::Status, LedBrightness);
	}else{
		ledService->off(MonoLED::Status);
	}
}

void LEDControl::applyState(){
	if(listening){
		setBlink(FastBlinkOnTime, FastBlinkOffTime);
		return;
	}

	if(playing){
		setSolidOn();
		return;
	}

	setBlink(SlowBlinkOnTime, SlowBlinkOffTime);
}

void LEDControl::onAudioPlayStatusChanged(bool value){
	CMF_LOG(LEDControl, Debug, "onAudioPlayStatusChanged");
	if(value){
		startPlay();
	}else{
		stopPlay();
	}
}
