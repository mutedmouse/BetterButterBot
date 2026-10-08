#ifndef BUTTERBOT_FIRMWARE_STATEROUTINEFACTORY_H
#define BUTTERBOT_FIRMWARE_STATEROUTINEFACTORY_H

#include <cstddef>
#include <new>
#include <utility>
#include "Routines/Routine.h"
#include "EventBag.h"

class BBStateMachine;
class State;

/**
 * @brief Fixed sizes of the reused placement-new buffers held by BBStateMachine.
 * Exactly one state and one routine are alive at a time, so a single buffer each is enough and
 * state/routine churn allocates zero heap. The static_asserts below fail at compile time if a
 * class outgrows its buffer - bump the constant if that happens.
 */
inline constexpr size_t StateBufSize   = 512;
inline constexpr size_t RoutineBufSize = 512;

/**
 * @brief Factory function pointers used in place of CMF SubclassOf<>/staticClass(). They
 * placement-new the concrete type into the caller-provided buffer and return the base pointer.
 */
using RoutineFactory      = Routine*(*)(void* buf, BBStateMachine* sm);
using EventRoutineFactory = Routine*(*)(void* buf, BBStateMachine* sm, EventBag::EventData data);
using StateFactory        = State*(*)(void* buf, BBStateMachine* sm);

template<class R>
Routine* makeRoutine(void* buf, BBStateMachine* sm){
	static_assert(sizeof(R) <= RoutineBufSize, "Routine exceeds RoutineBufSize - bump RoutineBufSize");
	return new(buf) R(sm);
}

template<class R>
Routine* makeEventRoutine(void* buf, BBStateMachine* sm, EventBag::EventData data){
	static_assert(sizeof(R) <= RoutineBufSize, "EventRoutine exceeds RoutineBufSize - bump RoutineBufSize");
	return new(buf) R(sm, std::move(data));
}

template<class S>
State* makeState(void* buf, BBStateMachine* sm){
	static_assert(sizeof(S) <= StateBufSize, "State exceeds StateBufSize - bump StateBufSize");
	return new(buf) S(sm);
}

#endif //BUTTERBOT_FIRMWARE_STATEROUTINEFACTORY_H
