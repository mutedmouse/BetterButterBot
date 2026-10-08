#ifndef BUTTERBOT_FIRMWARE_ROUTINE_H
#define BUTTERBOT_FIRMWARE_ROUTINE_H

#include <cstdint>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class BBStateMachine;

/**
 * @brief Base class for all routines. Deliberately NOT part of the CMF Object system.
 * A routine is created via placement-new into a buffer owned by BBStateMachine and ticked by the
 * active state. It binds to service events through the owning BBStateMachine (sm->bindRoutine(...)).
 */
class Routine {
public:
	enum class TickingState : uint8_t {
		None, // WARNING: None is not intended to be used as a Routine::tick return value, but just as a fallback for IdleState
		Block,
		Continue,
		Done
	};

	explicit Routine(BBStateMachine* sm) noexcept : sm(sm) {}
	virtual ~Routine() = default;

	Routine(const Routine&) = delete;
	Routine& operator=(const Routine&) = delete;

	virtual TickingState tick(float deltaTime){
		return TickingState::Done;
	}

	/**
	 * @return True for routines that carry an event payload (EventRoutine and derivatives). Replaces
	 * the old cast<EventRoutine>() check; an active event routine runs to completion in IdleState.
	 */
	virtual bool isEventRoutine() const { return false; }

protected:
	BBStateMachine* sm = nullptr;
};

#endif //BUTTERBOT_FIRMWARE_ROUTINE_H
