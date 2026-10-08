#ifndef BUTTERBOT_FIRMWARE_STATE_H
#define BUTTERBOT_FIRMWARE_STATE_H

#include <cstdint>

class BBStateMachine;

/**
 * @brief Base class for all firmware states. Deliberately NOT part of the CMF Object system.
 * A state is created via placement-new into a buffer owned by BBStateMachine; its constructor is
 * the "enter" hook and its destructor is the "exit" hook. It binds to service events through the
 * owning BBStateMachine (sm->bindState(...)).
 */
class State {
public:
	explicit State(BBStateMachine* sm) noexcept : sm(sm) {}
	virtual ~State() = default;

	State(const State&) = delete;
	State& operator=(const State&) = delete;

	/**
	 * @brief Called every state-machine tick while this state is active.
	 */
	virtual void tick(float deltaTime) {}

	/**
	 * @return If less than 0, the state machine ticks at its default interval, otherwise it uses
	 * this value (capped at portMAX_DELAY). Lets event-driven states sleep until something happens.
	 */
	virtual int64_t getDynamicTickInterval() const { return -1; }

protected:
	BBStateMachine* sm = nullptr;
};

#endif //BUTTERBOT_FIRMWARE_STATE_H
