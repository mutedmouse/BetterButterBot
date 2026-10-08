#ifndef BUTTERBOT_FIRMWARE_ISRBUTTONINPUT_H
#define BUTTERBOT_FIRMWARE_ISRBUTTONINPUT_H

#include "Object/Class.h"
#include "Drivers/Interface/InputDriver.h"
#include "Entity/AsyncEntity.h"
#include "Event/EventBroadcaster.h"
#include "esp_timer.h"

/**
 * Quick and dirty ISR-driven GPIO input class for a single button.
 * Only works with InputPins using InputGPIO.
 */
class ISRButtonInput : public AsyncEntity {
	GENERATED_BODY(ISRButtonInput, AsyncEntity, CONSTRUCTOR_PACK(int, InputPin))

public:
	ISRButtonInput(int buttonId, InputPin pin, bool internalStack = true);
	~ISRButtonInput() noexcept override;

	enum class Action : uint8_t {
		Release, Press
	};

	DECLARE_EVENT(ISRButtonInputEvent, ISRButtonInput, int, Action)
	ISRButtonInputEvent OnButtonEvent = ISRButtonInputEvent(this);

	bool getState() noexcept;

protected:
	void tick(float deltaTime) noexcept override;

private:
	void pressed();
	void released();

	int btnId;
	InputPin btnPin;
	StrongObjectPtr<InputDriver> inputSource;

	bool btnState = false;
	bool pendingActive = false;
	bool pendingState = false;
	int64_t pendingTimestamp = 0;

	std::mutex accessMutex;

	//Note: Using a quite large debounce value, since hand-soldered units are mechanically gunked-up and bounce more often.
	static constexpr uint64_t DebounceTime = 35; // [ms]

	SemaphoreHandle_t sem;

	static void IRAM_ATTR isr(void* arg);
};

#endif //BUTTERBOT_FIRMWARE_ISRBUTTONINPUT_H
