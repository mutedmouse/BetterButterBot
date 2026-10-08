#include "ShutdownService.h"
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Phrases.h>
#include <Core/Application.h>
#include <driver/gpio.h>
#include <Services/LED/LED.h>
#include "States/BBStateMachine.h"
#include <Util/ServiceLocator.h>
#include "BaseBoard.h"
#include "Com.h"
#include "Enums.h"
#include "Pins.hpp"
#include "LEDControl.h"
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"

DEFINE_LOG(ShutdownService)

void ShutdownService::Shutdown(ShutdownReason reason) {
	const Application* app = ApplicationStatics::getApplication();

	const Phrase phrase = (reason == ShutdownReason::Battery) ? Phrase::BatteryCritical : Phrase::TurningOff;
	const int16_t id = Phrases::get(phrase);

	if(Com* com = app->getService<Com>()){
		com->sendData(BB::State::Scenario, BB::Action::Scenario::Shutdown,
		              ShutdownData{.id = static_cast<uint8_t>(id), .reason = reason});
	}

	// Needed to interrupt whatever is going on in either of the states
	if(BBStateMachine* stateMachine = app->getService<BBStateMachine>()){
		stateMachine->interrupt();
	}

	if(BaseBoard* baseBoard = app->getService<BaseBoard>()){
		baseBoard->setMotors(0, 0);
	}

	Audio* audio = app->getService<Audio>();
	if(audio != nullptr) {
		audio->stop();

		if(ServiceLocator::SpeechGenInstance && ServiceLocator::SpeechAudioGenInstance) {
			audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(phrase, id)));
		}
	}

	LED<MonoLED, RGBLED>* leds = app->getService<LED<MonoLED, RGBLED>>();
	if(leds != nullptr){
		leds->off(MonoLED::Module);
	}

	if(reason == ShutdownReason::Battery){
		if(ServiceLocator::LEDControlInstance){
			ServiceLocator::LEDControlInstance->startBatteryCritical(true);
		}
	}else if(leds != nullptr){
		// The Audio->LEDControl binding doesn't dispatch during Shutdown, so the
		// talking indicator has to be driven manually while the phrase plays.
		leds->on(MonoLED::Status, 1.0f);
	}

	if(audio != nullptr){
		audio->waitEnd(portMAX_DELAY);
		audio->stop();
	}

	if(leds != nullptr){
		leds->off(MonoLED::Status);
	}

	PowerOff();
}

void ShutdownService::PowerOff(){
	while(gpio_get_level(static_cast<gpio_num_t>(PIN_BTN)) == 1){
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	gpio_set_level(static_cast<gpio_num_t>(PIN_PWDN), 0);
	static constexpr gpio_config_t pwdnConfig = {
		.pin_bit_mask = 1ULL << PIN_PWDN,
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&pwdnConfig);

	while(true){
		vTaskDelay(portMAX_DELAY);
	}
}
