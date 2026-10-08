#include "ButtonInput.h"
#include "Log/Log.h"

ButtonInput::ButtonInput(const std::vector<std::pair<Enum<int>, InputPin>>& registrations, bool internalStack) :
		Super(CONFIG_CMF_BUTTONINPUT_TICK_INTERVAL / portTICK_PERIOD_MS, CONFIG_CMF_BUTTONINPUT_STACK_SIZE, CONFIG_CMF_BUTTONINPUT_THREAD_PRIORITY, CONFIG_CMF_BUTTONINPUT_CPU_CORE, internalStack){
	reg(registrations);
}

void ButtonInput::reg(const std::vector<std::pair<Enum<int>, InputPin>>& registrations) noexcept{
	for(auto& r: registrations){
		reg(r.first, r.second);
	}
}

void ButtonInput::reg(Enum<int> button, InputPin pin) noexcept{
	std::lock_guard guard(accessMutex);

	inputSources.insert(StrongObjectPtr<InputDriver>(pin.driver));
	buttons[button] = pin;
	dbTime[button] = 0;
	btnState[button] = false;
}

bool ButtonInput::getState(Enum<int> button) noexcept{
	std::lock_guard guard(accessMutex);

	if(!btnState.contains(button)){
		CMF_LOG(CMF, Error, "Button %d not registered", (int) button);
		return false;
	}

	return btnState[button];
}

void ButtonInput::tick(float deltaTime) noexcept{
	Super::tick(deltaTime);

	std::lock_guard guard(accessMutex);

	for(auto& source: inputSources){
		source->scan();
	}

	for(const auto& pair: buttons){
		const auto btn = pair.first;
		const auto pin = pair.second;

		const bool state = static_cast<bool>(pin.driver->read(pin.port));
		if(state){
			pressed(btn);
		}else{
			released(btn);
		}
	}
}

void ButtonInput::pressed(int btn){
	if(btnState[btn]){
		dbTime[btn] = 0;
		return;
	}

	auto t = millis();

	if(dbTime[btn] == 0){
		dbTime[btn] = t;
		return;
	}

	if(t - dbTime[btn] < DebounceTime){
		return;
	}

	btnState[btn] = true;
	dbTime[btn] = 0;

	OnButtonEvent.broadcast(btn, Action::Press);
}

void ButtonInput::released(int btn){
	if(!btnState[btn]){
		dbTime[btn] = 0;
		return;
	}

	auto t = millis();

	if(dbTime[btn] == 0){
		dbTime[btn] = t;
		return;
	}else if(t - dbTime[btn] < DebounceTime){
		return;
	}

	btnState[btn] = false;
	dbTime[btn] = 0;

	OnButtonEvent.broadcast(btn, Action::Release);
}
