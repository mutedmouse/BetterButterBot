#ifndef BUTTERBOT_FIRMWARE_BBSTATEMACHINE_H
#define BUTTERBOT_FIRMWARE_BBSTATEMACHINE_H

#include <cstddef>
#include <functional>
#include <new>
#include <utility>
#include <vector>
#include "Entity/AsyncEntity.h"
#include "Object/Object.h"
#include "Memory/SmartPtr/StrongObjectPtr.h"
#include "Memory/ObjectMemory.h"
#include "Event/EventBroadcaster.h"
#include "State.h"
#include "StateRoutineFactory.h"

/**
 * @brief Trivial Object used purely as an event-binding owner. EventHandles are keyed on an Object
 * (broadcast safety + unbind), so non-Object states/routines bind through one of these anchors,
 * which are children of the BBStateMachine and therefore scanned by its thread.
 */
class EventAnchor : public Object {
	GENERATED_BODY(EventAnchor, Object, void)
};

/**
 * @brief Firmware-local state machine. It is the single point of contact with the CMF Object
 * system for states and routines: it is an AsyncEntity (owns the ticking thread and scans events),
 * owns two event anchors (one for the active state, one for the active routine), and allocates the
 * active state and active routine via placement-new into reused buffers (no per-instance heap).
 *
 * Separate from CMF's StateMachine - states/routines here are plain classes, not Objects.
 */
class BBStateMachine : public AsyncEntity {
	GENERATED_BODY(BBStateMachine, AsyncEntity, CONSTRUCTOR_PACK(TickType_t, size_t, uint8_t, int8_t))

public:
	DECLARE_EVENT(OnTransitionEvent, BBStateMachine);
	OnTransitionEvent OnTransition{ this };

public:
	explicit BBStateMachine(TickType_t interval = CONFIG_CMF_STATEMACHINE_TICK_INTERVAL / portTICK_PERIOD_MS,
		size_t stackSize = CONFIG_CMF_STATEMACHINE_STACK_SIZE, uint8_t threadPriority = CONFIG_CMF_STATEMACHINE_THREAD_PRIORITY,
		int8_t cpuCore = CONFIG_CMF_STATEMACHINE_CPU_CORE, bool internalStack = true) noexcept;

	~BBStateMachine() noexcept override;

	void postInitProperties() noexcept override;

	/**
	 * @brief Queue a transition to state T. The state is constructed (placement-new) on the next tick,
	 * after the current state has been torn down. Wakes the ticking thread so it happens promptly.
	 */
	template<class T>
	void transitionTo(){
		next = &makeState<T>;
		OnTransition.broadcast();
	}

	bool isTransitionPending() const noexcept { return next != nullptr; }
	State* getActiveState() const noexcept { return currentState; }
	Routine* getActiveRoutine() const noexcept { return currentRoutine; }

	/**
	 * @brief Routine lifecycle. The routine lives in the reused routine buffer; only one at a time.
	 */
	Routine* startRoutine(RoutineFactory factory);
	Routine* startEventRoutine(EventRoutineFactory factory, EventBag::EventData data);
	void endRoutine();

	/**
	 * @brief Start a routine constructed in-place from explicit arguments, for routines whose data
	 * comes straight from the triggering event (Motion, Poke) instead of the EventBag queue - so
	 * they run the instant the event arrives, or not at all if a routine is already active.
	 */
	template<class R, typename... Args>
	R* startRoutine(Args&&... args){
		static_assert(sizeof(R) <= RoutineBufSize, "Routine exceeds RoutineBufSize - bump RoutineBufSize");

		if(currentRoutine != nullptr){
			endRoutine();
		}

		R* routine = new(routineBuf) R(this, std::forward<Args>(args)...);
		currentRoutine = routine;
		return routine;
	}

	/**
	 * @brief Tear down whatever state/routine is currently running. Used on shutdown to interrupt.
	 */
	void interrupt();

	/**
	 * @brief Bind a state/routine member function to a service event broadcaster through the matching
	 * anchor. The handle is owned by the anchor, so unbindStateEvents()/unbindRoutineEvents() (called
	 * on teardown) reclaim it. Lambdas capture only two pointers, fitting std::function SBO (no heap).
	 */
	template<typename E, typename Obj, typename... Args>
	void bindState(E& broadcaster, Obj* obj, void (Obj::*method)(Args...)){
		broadcaster.bind(stateAnchor.get(), std::function<void(Args...)>([obj, method](Args... args){ (obj->*method)(args...); }));
		stateUnbinds.emplace_back([&broadcaster, this]{ broadcaster.unbind(stateAnchor.get()); });
	}

	template<typename E, typename Obj, typename... Args>
	void bindRoutine(E& broadcaster, Obj* obj, void (Obj::*method)(Args...)){
		broadcaster.bind(routineAnchor.get(), std::function<void(Args...)>([obj, method](Args... args){ (obj->*method)(args...); }));
		routineUnbinds.emplace_back([&broadcaster, this]{ broadcaster.unbind(routineAnchor.get()); });
	}

	template<typename E>
	void unbindRoutine(E& broadcaster){ broadcaster.unbind(routineAnchor.get()); }

	template<typename E>
	void unbindState(E& broadcaster){ broadcaster.unbind(stateAnchor.get()); }

	void unbindStateEvents();
	void unbindRoutineEvents();

	TickType_t getEventScanningTime() const noexcept override;

protected:
	void tick(float deltaTime) noexcept override;

private:
	void destroyState();

	StrongObjectPtr<EventAnchor> stateAnchor;
	StrongObjectPtr<EventAnchor> routineAnchor;

	alignas(std::max_align_t) std::byte stateBuf[StateBufSize];
	alignas(std::max_align_t) std::byte routineBuf[RoutineBufSize];
	State* currentState = nullptr;
	Routine* currentRoutine = nullptr;

	StateFactory next = nullptr;

	std::vector<std::function<void()>> stateUnbinds;
	std::vector<std::function<void()>> routineUnbinds;
};

#endif //BUTTERBOT_FIRMWARE_BBSTATEMACHINE_H
