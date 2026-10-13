#ifndef BUTTERBOT_FIRMWARE_IDLESTATE_H
#define BUTTERBOT_FIRMWARE_IDLESTATE_H

#include <array>
#include <vector>
#include <CtrlData.h>
#include "State.h"
#include "StateRoutineFactory.h"
#include "EventBag.h"
#include "Services/Com.h"
#include "Services/ISRButtonInput.h"
#include "Services/MotionService.h"

class IdleState : public State {
public:
	explicit IdleState(BBStateMachine* sm);
	~IdleState() override;

	virtual void tick(float deltaTime) override;
	virtual int64_t getDynamicTickInterval() const override;

private:
	struct RandomRoutineDef {
		RoutineFactory routine;
		uint8_t weight; // The higher the weight, the more likely the routine will be chosen randomly. If 0, routine will never be chosen.
	};

	static const std::vector<RandomRoutineDef> RandomRoutines;
	static const std::array<EventRoutineFactory, static_cast<uint8_t>(EventBag::EventType::COUNT)> EventRoutines;

	// Random delay works in a [MinRandomDelay, MaxRandomDelay> range
	static constexpr TickType_t MinRandomDelay = 5 * 60000;
	static constexpr TickType_t MaxRandomDelay = 20 * 60000;

private:
	EventBag* eventBag = nullptr;
	uint64_t lastBtnPressTime;
	uint64_t nextRandomRoutineTime;
	Com* com = nullptr;
	Routine::TickingState lastTickingState = Routine::TickingState::None;
	bool buttonHeld = false;
	// Poke/Motion events handled before this time are stale (queued while a routine ran) and are dropped.
	uint64_t instantEventCutoff = 0;

	static constexpr uint64_t MaximumBtnHoldForTransition = 500;
	static constexpr uint64_t StaleEventWindow = 100; // ms

private:
	void onCommand(Ctrl::Command cmd);
	void onRCData(const RCData& data);
	void onButtonEvent(int button, ISRButtonInput::Action action);
	void onMotion(MotionType type);
	void onPush();
	void onWake();
	void onMode(bool rc);
	bool startPush();

	// tick() helpers. All run on the StateMachine thread, serialized with onButtonEvent.
	bool shouldYieldToEvent(bool eventPending) const;
	// True when a Motion/Poke routine may start now: nothing is running and no transition is queued.
	bool canStartInstantRoutine() const;
	void endActiveRoutine();
	void startEventRoutine(const EventBag::EventData& data);
	void startScenario(const EventBag::EventData& data);
	void maybeStartRandomRoutine();
	RoutineFactory pickRandomRoutine() const;
	void rescheduleRandomRoutine();
};

#endif //BUTTERBOT_FIRMWARE_IDLESTATE_H
