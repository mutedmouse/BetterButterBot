#include "ISRButtonInput.h"
#include <Drivers/Input/InputGPIO.h>

ISRButtonInput::ISRButtonInput(int buttonId, InputPin pin, bool internalStack) :
	Super(CONFIG_CMF_BUTTONINPUT_TICK_INTERVAL, CONFIG_CMF_BUTTONINPUT_STACK_SIZE, CONFIG_CMF_BUTTONINPUT_THREAD_PRIORITY, CONFIG_CMF_BUTTONINPUT_CPU_CORE, internalStack),
	btnId(buttonId), btnPin(pin){
	sem = xSemaphoreCreateBinary();
	inputSource = StrongObjectPtr<InputDriver>(pin.driver);

	auto gpioPin = (gpio_num_t)pin.port;
	gpio_isr_handler_add(gpioPin, isr, &sem);
	gpio_set_intr_type(gpioPin, GPIO_INTR_ANYEDGE);
}

ISRButtonInput::~ISRButtonInput() noexcept{
	gpio_isr_handler_remove((gpio_num_t)btnPin.port);
	xSemaphoreGive(sem);
}

bool ISRButtonInput::getState() noexcept{
	std::lock_guard guard(accessMutex);
	return btnState;
}

void ISRButtonInput::tick(float deltaTime) noexcept{
	const TickType_t timeout = pendingActive ? pdMS_TO_TICKS(DebounceTime) : portMAX_DELAY;

	xSemaphoreTake(sem, timeout);

	std::lock_guard guard(accessMutex);

	inputSource->scan();

	const bool rawState = static_cast<bool>(inputSource->read(btnPin.port));
	const int64_t now = esp_timer_get_time();

	if(!pendingActive){
		if(rawState != btnState){
			pendingActive = true;
			pendingState = rawState;
			pendingTimestamp = now;
		}
	} else{
		if(rawState == pendingState){
			if((now - pendingTimestamp) >= (int64_t)DebounceTime * 1000LL){
				if(rawState){
					pressed();
				} else{
					released();
				}
				pendingActive = false;
			}
		} else{
			pendingState = rawState;
			pendingTimestamp = now;
		}
	}
}

void ISRButtonInput::pressed(){
	if(btnState){
		return;
	}

	btnState = true;

	OnButtonEvent.broadcast(btnId, Action::Press);
}

void ISRButtonInput::released(){
	if(!btnState){
		return;
	}

	btnState = false;

	OnButtonEvent.broadcast(btnId, Action::Release);
}


void IRAM_ATTR ISRButtonInput::isr(void* arg){
	BaseType_t higherPrioTask = pdFALSE;
	xSemaphoreGiveFromISR(*static_cast<SemaphoreHandle_t*>(arg), &higherPrioTask);
	portYIELD_FROM_ISR(higherPrioTask);
}
