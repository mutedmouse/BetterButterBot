#ifndef CMF_STATEMACHINE_H
#define CMF_STATEMACHINE_H

#include "Object/Class.h"
#include "State.h"
#include "Entity/AsyncEntity.h"
#include "Object/SubclassOf.h"
#include "Event/EventBroadcaster.h"

/**
 * @brief State machine abstraction AsyncEntity that ticks on its own and transitions between the states depending on the given state type.
 */
class StateMachine : public AsyncEntity {
	GENERATED_BODY(StateMachine, AsyncEntity, CONSTRUCTOR_PACK(const SubclassOf<State>&, TickType_t, size_t, uint8_t, int8_t))

public:
	DECLARE_EVENT(OnNextStateSetEvent, StateMachine);
	OnNextStateSetEvent OnNextStateSet {this};

public:
	/**
	 * @brief Constructs the state machine which starts ticking immediately and waits for the starting state type to be set.
	 * @param startingState The class of the starting state.
	 * @param interval The ticking interval of the state machine.
	 */
	explicit StateMachine(const SubclassOf<State>& startingState = nullptr, TickType_t interval = CONFIG_CMF_STATEMACHINE_TICK_INTERVAL / portTICK_PERIOD_MS, size_t stackSize = CONFIG_CMF_STATEMACHINE_STACK_SIZE, uint8_t threadPriority = CONFIG_CMF_STATEMACHINE_THREAD_PRIORITY, int8_t cpuCore = CONFIG_CMF_STATEMACHINE_CPU_CORE, bool internalStack = true) noexcept;

	/**
	 *
	 * @return The current state.
	 */
	State* getActiveState() const noexcept;

	/**
	 *
	 * @return The next state waiting to transition into.
	 */
	const Class* getNextState() const noexcept;

	/**
	 * @brief This is used to avoid large pauses between states when transitioning
	 * @return 0 if there is next state to go to, otherwise default of AsyncEntity
	 */
	virtual TickType_t getEventScanningTime() const noexcept override;

	/**
	 *
	 * @param state The state to transition to.
	 */
	void transitionTo(const SubclassOf<State>& state);

protected:
	/**
	 * @brief Checks if the current state is ready to transition to another one.
	 * If that is the case, the state transition is triggered.
	 * @param deltaTime Time passed between the last tick and this tick.
	 */
	virtual void tick(float deltaTime) noexcept override;

private:
	SubclassOf<State> next;
	StrongObjectPtr<State> current;
};

#endif //CMF_STATEMACHINE_H