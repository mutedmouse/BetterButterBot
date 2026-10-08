#include "StateMachine.h"
#include "Memory/ObjectMemory.h"

StateMachine::StateMachine(const SubclassOf<State>& startingState /*= nullptr*/, TickType_t interval /*= CONFIG_CMF_STATEMACHINE_TICK_INTERVAL / portTICK_PERIOD_MS*/, size_t stackSize /*= CONFIG_CMF_STATEMACHINE_STACK_SIZE*/,
	uint8_t threadPriority /*= CONFIG_CMF_STATEMACHINE_THREAD_PRIORITY*/, int8_t cpuCore /*= CONFIG_CMF_STATEMACHINE_CPU_CORE*/, bool internalStack /*= false*/) noexcept :
		Super(interval, stackSize, threadPriority, cpuCore, internalStack), next(startingState){
	OnNextStateSet.bind(this, [](){}); // This is just to unblock the ticking thread

	OnNextStateSet.broadcast();
}

State* StateMachine::getActiveState() const noexcept{
	return current.get();
}

const Class* StateMachine::getNextState() const noexcept{
	return *next;
}

TickType_t StateMachine::getEventScanningTime() const noexcept{
	if(next != nullptr){
		return 0;
	}

	if(!current.isValid()){
		return Super::getEventScanningTime();
	}

	const int64_t dynamicTickInterval = current->getDynamicTickInterval();

	if(dynamicTickInterval < 0){
		return Super::getEventScanningTime();
	}

	if(dynamicTickInterval >= portMAX_DELAY){
		return portMAX_DELAY;
	}

	return dynamicTickInterval;
}

void StateMachine::transitionTo(const SubclassOf<State>& state){
	if(state == nullptr){
		return;
	}

	next = state;

	OnNextStateSet.broadcast();
}

void StateMachine::tick(float deltaTime) noexcept{
	if(next != nullptr) {
		const Class* previousType = nullptr;
		if(current.isValid()){
			previousType = current->getStaticClass();
			current->onTransitionTo(*next);
			delete *current;
		}

		current = newObject<State>(*next, this);
		current->onTransitionFrom(previousType);
		next = nullptr;
	}
}
