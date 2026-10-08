#include "BBStateMachine.h"

BBStateMachine::BBStateMachine(TickType_t interval /*= CONFIG_CMF_STATEMACHINE_TICK_INTERVAL / portTICK_PERIOD_MS*/,
	size_t stackSize /*= CONFIG_CMF_STATEMACHINE_STACK_SIZE*/, uint8_t threadPriority /*= CONFIG_CMF_STATEMACHINE_THREAD_PRIORITY*/,
	int8_t cpuCore /*= CONFIG_CMF_STATEMACHINE_CPU_CORE*/, bool internalStack /*= true*/) noexcept :
		Super(interval, stackSize, threadPriority, cpuCore, internalStack){
	stateUnbinds.reserve(8);
	routineUnbinds.reserve(8);
}

BBStateMachine::~BBStateMachine() noexcept{
	// Run dtors of the placement-new'd state/routine and drop their event bindings.
	destroyState();
}

void BBStateMachine::postInitProperties() noexcept{
	Super::postInitProperties();

	// Anchors are owned by this machine, so their event handles register on and are scanned by this thread.
	stateAnchor = newObject<EventAnchor>(this);
	routineAnchor = newObject<EventAnchor>(this);

	// Bound to a no-op purely so broadcasting OnTransition wakes the ticking thread for prompt transitions.
	OnTransition.bind(this, [](){});
}

Routine* BBStateMachine::startRoutine(RoutineFactory factory){
	if(currentRoutine != nullptr){
		endRoutine();
	}

	if(factory == nullptr){
		return nullptr;
	}

	currentRoutine = factory(routineBuf, this);
	return currentRoutine;
}

Routine* BBStateMachine::startEventRoutine(EventRoutineFactory factory, EventBag::EventData data){
	if(currentRoutine != nullptr){
		endRoutine();
	}

	if(factory == nullptr){
		return nullptr;
	}

	currentRoutine = factory(routineBuf, this, std::move(data));
	return currentRoutine;
}

void BBStateMachine::endRoutine(){
	if(currentRoutine == nullptr){
		return;
	}

	unbindRoutineEvents();

	currentRoutine->~Routine();
	currentRoutine = nullptr;
}

void BBStateMachine::destroyState(){
	if(currentState == nullptr){
		return;
	}

	// Routine lives within the state, tear it down first.
	endRoutine();

	unbindStateEvents();

	currentState->~State();
	currentState = nullptr;
}

void BBStateMachine::interrupt(){
	destroyState();
}

void BBStateMachine::unbindStateEvents(){
	for(const std::function<void()>& unbind : stateUnbinds){
		unbind();
	}
	stateUnbinds.clear();
}

void BBStateMachine::unbindRoutineEvents(){
	for(const std::function<void()>& unbind : routineUnbinds){
		unbind();
	}
	routineUnbinds.clear();
}

TickType_t BBStateMachine::getEventScanningTime() const noexcept{
	if(next != nullptr){
		return 0;
	}

	if(currentState == nullptr){
		return Super::getEventScanningTime();
	}

	const int64_t dynamicTickInterval = currentState->getDynamicTickInterval();

	if(dynamicTickInterval < 0){
		return Super::getEventScanningTime();
	}

	if(dynamicTickInterval >= portMAX_DELAY){
		return portMAX_DELAY;
	}

	return dynamicTickInterval;
}

void BBStateMachine::tick(float deltaTime) noexcept{
	if(next != nullptr){
		destroyState();
		currentState = next(stateBuf, this);
		next = nullptr;
	}

	if(currentState != nullptr){
		currentState->tick(deltaTime);
	}
}
