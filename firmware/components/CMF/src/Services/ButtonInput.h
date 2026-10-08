#ifndef CMF_BUTTONINPUT_H
#define CMF_BUTTONINPUT_H

#include "Object/Class.h"
#include "Drivers/Interface/InputDriver.h"
#include "Entity/AsyncEntity.h"
#include "Event/EventBroadcaster.h"

class ButtonInput : public AsyncEntity {
	GENERATED_BODY(ButtonInput, AsyncEntity, CONSTRUCTOR_PACK(const std::vector<std::pair<Enum<int>, InputPin>>&))

public:
	ButtonInput(const std::vector<std::pair<Enum<int>, InputPin>>& registrations = {}, bool internalStack = true);

	enum class Action : uint8_t {
		Release, Press
	};

	DECLARE_EVENT(ButtonInputEvent, ButtonInput, Enum<int>, Action)
	ButtonInputEvent OnButtonEvent = ButtonInputEvent(this);

	void reg(const std::vector<std::pair<Enum<int>, InputPin>>& registrations) noexcept;

	void reg(Enum<int> button, InputPin pin) noexcept;

	bool getState(Enum<int> button) noexcept;

protected:
	void tick(float deltaTime) noexcept override;

private:
	void pressed(int btn);
	void released(int btn);

	std::set<StrongObjectPtr<InputDriver>> inputSources;

	std::map<int, InputPin> buttons;

	std::unordered_map<int, bool> btnState;
	std::unordered_map<int, uint64_t> dbTime;

	std::mutex accessMutex;

	static constexpr uint64_t DebounceTime = 5; // [ms]
};

#endif //CMF_BUTTONINPUT_H
