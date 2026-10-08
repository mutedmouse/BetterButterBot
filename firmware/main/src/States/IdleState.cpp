#include "IdleState.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include <Routines/MotionRoutine.h>
#include <Services/MotionService.h>
#include <Util/ServiceLocator.h>
#include "BBStateMachine.h"
#include "Enums.h"
#include "ListenState.h"
#include "ScenarioState.h"
#include "ScenarioRoutineMappings.h"
#include "Services/Com.h"
#include "RCState.h"
#include "Routines/RambleRoutine.h"
#include "Routines/ObserveRoutine.h"
#include "Routines/PersonRoutine.h"
#include "Routines/EventRoutines/BatteryEventRoutine.h"
#include "Routines/EventRoutines/GasEventRoutine.h"
#include "Routines/EventRoutines/IntruderEventRoutine.h"
#include "Routines/EventRoutines/ModuleEventRoutine.h"
#include "Routines/EventRoutines/PhoneConnEventRoutine.h"
#include "Routines/EventRoutines/PokeEventRoutine.h"
#include "Routines/EventRoutines/GasConfiguredEventRoutine.h"
#include "Routines/EventRoutines/SummonEventRoutine.h"
#include "Routines/WanderRoutine.h"
#include "Routines/SpeakReplyRoutine.h"
#include "Routines/AlarmRoutine.h"
#include "Services/VoiceLink.h"
#include "Services/WakeWord.h"
#include "Services/CameraStream.h"
#include <Scenarios.h>

DEFINE_LOG(IdleState)

const std::vector<IdleState::RandomRoutineDef> IdleState::RandomRoutines = {
	{ &makeRoutine<RambleRoutine>, 3 },
	{ &makeRoutine<ObserveRoutine>, 1 },
	{ &makeRoutine<WanderRoutine>, 2 },
	{ &makeRoutine<PersonRoutine>, 1 },
};

const std::array<EventRoutineFactory, static_cast<uint8_t>(EventBag::EventType::COUNT)> IdleState::EventRoutines = {
	nullptr, // None
	nullptr, // IncomingCall
	&makeEventRoutine<BatteryEventRoutine>,
	nullptr, // Notification
	&makeEventRoutine<GasEventRoutine>,
	&makeEventRoutine<IntruderEventRoutine>,
	&makeEventRoutine<ModuleEventRoutine>,
	&makeEventRoutine<ModuleEventRoutine>,
	&makeEventRoutine<GasConfiguredEventRoutine>,
	&makeEventRoutine<PhoneConnEventRoutine>,
	&makeEventRoutine<SummonEventRoutine>,
	nullptr, // Scenario
};

IdleState::IdleState(BBStateMachine* sm) : State(sm){
	nextRandomRoutineTime = millis() + MinRandomDelay;

	const Application* app = ApplicationStatics::getApplication();
	eventBag = app->getService<EventBag>();
	if(eventBag == nullptr){
		CMF_LOG(IdleState, LogLevel::Error, "EventBag service missing in constructor");
		return;
	}

	ISRButtonInput* buttonInput = app->getService<ISRButtonInput>();
	if(buttonInput == nullptr){
		CMF_LOG(IdleState, LogLevel::Error, "ButtonInput service missing in constructor");
		return;
	}

	sm->bindState(buttonInput->OnButtonEvent, this, &IdleState::onButtonEvent);

	com = app->getService<Com>();
	if(com == nullptr){
		CMF_LOG(IdleState, LogLevel::Error, "Com service missing in constructor");
		return;
	}

	sm->bindState(com->OnCommand, this, &IdleState::onCommand);
	sm->bindState(com->OnRCData, this, &IdleState::onRCData);

	auto motionService = app->getService<MotionService>();
	if(motionService == nullptr){
		CMF_LOG(IdleState, LogLevel::Error, "MotionService missing in constructor");
		return;
	}

	motionService->enable();

	// Motion and Poke are not queued in the EventBag; they are handled the instant they arrive,
	// dropped while a routine is running, and dropped for StaleEventWindow after a routine ends
	// (events broadcast during a blocking routine are drained right after it retires - see
	// endActiveRoutine). Both callbacks run on the StateMachine thread.
	sm->bindState(motionService->onMotion, this, &IdleState::onMotion);

	// Background-agent pushes (reminders, timers, ...) from the voice gateway wait for idle.
	if(VoiceLink* voiceLink = app->getService<VoiceLink>()){
		sm->bindState(voiceLink->OnPush, this, &IdleState::onPush);
	}
	ListenState::cancelFollowUp();

	// "butterbot" wake word: only listened for while idle (the mic is shared).
	if(WakeWord* wakeWord = app->getService<WakeWord>()){
		sm->bindState(wakeWord->OnWake, this, &IdleState::onWake);
		wakeWord->resume();
	}

	if(!eventBag->probe()){
		com->sendData(BB::State::Idle, BB::Action::Idle::None);
	}
}

IdleState::~IdleState(){
	// The active routine and event bindings are torn down by BBStateMachine::destroyState().
	const Application* app = ApplicationStatics::getApplication();

	if(MotionService* motionService = app->getService<MotionService>()){
		motionService->disable();
	}

	if(WakeWord* wakeWord = app->getService<WakeWord>()){
		wakeWord->pause(); // hand the mic to whatever state comes next
	}
}

void IdleState::tick(float deltaTime){
	if(sm->isTransitionPending()){
		return;
	}

	if(sm->getActiveRoutine() == nullptr && startPush()){
		return;
	}

	if(eventBag == nullptr){
		CMF_LOG(IdleState, LogLevel::Error, "Event Bag is nullptr.");
		return;
	}

	const bool eventPending = eventBag->probe();

	// A running random routine yields to a pending event so the event is handled now.
	if(sm->getActiveRoutine() != nullptr && shouldYieldToEvent(eventPending)){
		endActiveRoutine();
	}

	// Start a new routine if nothing is active. Pending events take priority over random routines.
	if(sm->getActiveRoutine() == nullptr){
		if(eventPending){
			startEventRoutine(eventBag->pop());
		} else{
			maybeStartRandomRoutine();
		}
	}

	// Tick whatever is active now - including a routine created during this same tick -
	// and retire it the moment it reports completion.
	if(Routine* routine = sm->getActiveRoutine()){
		lastTickingState = routine->tick(deltaTime);

		if(lastTickingState == Routine::TickingState::Done){
			endActiveRoutine();
		}
	}
}

bool IdleState::shouldYieldToEvent(bool eventPending) const{
	// Only random (non-event) routines yield; an active EventRoutine runs to completion.
	const Routine* routine = sm->getActiveRoutine();
	return routine != nullptr && !routine->isEventRoutine() && eventPending;
}

void IdleState::endActiveRoutine(){
	sm->endRoutine();

	// Poke/Motion events broadcast while the routine blocked this thread sit in their CMF callQueues
	// and get drained on the very next scanEvents pass, where the canStartInstantRoutine() guard would
	// let the first one through. They arrive within ~1ms of this point; anything handled inside the
	// window is stale and dropped.
	instantEventCutoff = millis() + StaleEventWindow;

	lastTickingState = Routine::TickingState::None;

	com->sendData(BB::State::Idle, BB::Action::Idle::None);
}

void IdleState::startEventRoutine(const EventBag::EventData& data){
	if(data.type == EventBag::EventType::Scenario){
		startScenario(data);
		return;
	}

	// Some event types (e.g. IncomingCall, Notification) have no routine mapped. Drop them instead of constructing a null routine.
	const EventRoutineFactory factory = EventRoutines[static_cast<uint8_t>(data.type)];
	if(factory == nullptr){
		CMF_LOG(IdleState, LogLevel::Warning, "No EventRoutine mapped for event type %u", static_cast<uint8_t>(data.type));
		com->sendData(BB::State::Idle, BB::Action::Idle::None);
		rescheduleRandomRoutine();
		return;
	}

	// Each routine is implemented to take only one parameter of type EventBag::EventData, that it then interprets in its own way.
	sm->startEventRoutine(factory, data);

	rescheduleRandomRoutine();
}

void IdleState::startScenario(const EventBag::EventData& data){
	const EventBag::ScenarioEventData* scenarioEvent = std::get_if<EventBag::ScenarioEventData>(&data.payload);
	if(scenarioEvent == nullptr || !ServiceLocator::ScenarioRoutineServiceInstance){
		CMF_LOG(IdleState, LogLevel::Error, "Scenario event missing payload or ScenarioRoutineService");
		com->sendData(BB::State::Idle, BB::Action::Idle::None);
		rescheduleRandomRoutine();
		return;
	}

	const RoutineFactory factory = routineForScenario({ scenarioEvent->scenario, scenarioEvent->data });
	if(factory == nullptr){
		CMF_LOG(IdleState, LogLevel::Error, "No routine mapped for scenario %d", static_cast<int>(scenarioEvent->scenario));
		com->sendData(BB::State::Idle, BB::Action::Idle::None);
		rescheduleRandomRoutine();
		return;
	}

	ServiceLocator::ScenarioRoutineServiceInstance->setRoutineFactory(factory);
	sm->transitionTo<ScenarioState>();
}

void IdleState::maybeStartRandomRoutine(){
	if(RandomRoutines.empty()){
		rescheduleRandomRoutine();
		return;
	}

	// While the button is held, suppress new random routines so none start before the transition to ListenState commits on release.
	if(buttonHeld){
		return;
	}

	if(millis() < nextRandomRoutineTime){
		return;
	}

	RoutineFactory nextRandomRoutine = pickRandomRoutine();
	if(nextRandomRoutine != nullptr){
		sm->startRoutine(nextRandomRoutine);
	}

	rescheduleRandomRoutine();
}

RoutineFactory IdleState::pickRandomRoutine() const{
	// While the camera is streaming, skip the idle routines that would take the camera from it.
	const bool streaming = CameraStream::streaming();
	auto usesCamera = [](RoutineFactory r){ return r == &makeRoutine<ObserveRoutine> || r == &makeRoutine<PersonRoutine>; };

	size_t weightSum = 0;
	for(const RandomRoutineDef& routineDef : RandomRoutines){
		if(streaming && usesCamera(routineDef.routine)) continue;
		weightSum += routineDef.weight;
	}

	const size_t random = rand() % (weightSum + 1);

	size_t sum = 0;
	for(const RandomRoutineDef& routineDef : RandomRoutines){
		if(routineDef.weight == 0 || (streaming && usesCamera(routineDef.routine))){
			continue;
		}

		sum += routineDef.weight;

		if(random <= sum){
			return routineDef.routine;
		}
	}

	return nullptr;
}

void IdleState::rescheduleRandomRoutine(){
	nextRandomRoutineTime = millis() + (rand() % (MaxRandomDelay - MinRandomDelay) + MinRandomDelay);
}

int64_t IdleState::getDynamicTickInterval() const{
	if(eventBag == nullptr){
		CMF_LOG(IdleState, LogLevel::Error, "Event Bag is nullptr.");
		return -1;
	}

	if(eventBag->probe()){
		return 0;
	}

	// A gateway push that arrived while another state was active (e.g. "find me" during a conversation,
	// a timer due mid-listen) only raised OnPush then; tick now so startPush() picks it up.
	if(sm->getActiveRoutine() == nullptr){
		VoiceLink* voiceLink = ApplicationStatics::getApplication()->getService<VoiceLink>();
		if(voiceLink != nullptr && voiceLink->pushPending()){
			return 0;
		}
	}

	if(sm->getActiveRoutine() != nullptr){
		if(lastTickingState == Routine::TickingState::Continue){
			return 0;
		}

		// Block: sleep until the awaited event wakes the tick thread.
		if(lastTickingState == Routine::TickingState::Block){
			return portMAX_DELAY;
		}
	}

	if(millis() >= nextRandomRoutineTime){
		return 0;
	}

	return nextRandomRoutineTime - millis();
}

void IdleState::onWake(){
	if(sm->isTransitionPending()){
		return;
	}
	sm->transitionTo<ListenState>(); // same as a short button press
}

void IdleState::onPush(){
	if(sm->isTransitionPending() || sm->getActiveRoutine() != nullptr){
		return; // picked up by tick() once the running routine finishes
	}
	startPush();
}

bool IdleState::startPush(){
	VoiceLink* voiceLink = ApplicationStatics::getApplication()->getService<VoiceLink>();
	VoiceLink::Push push;
	if(voiceLink == nullptr || !ServiceLocator::ScenarioRoutineServiceInstance || !voiceLink->takePush(push)){
		return false;
	}

	if(push.routine == "summon"){
		// "Find me": the controller's Summon routine. It runs here in Idle (like the controller's
		// command does), because it relies on the MotionService IdleState enables.
		EventBag::EventData data{};
		data.type = EventBag::EventType::Summon;
		startEventRoutine(data);
		// tick() returns right after a push starts, so the new routine runs on the next tick - make
		// sure there is one: the leftover state of the previous routine may be Block (sleep forever).
		lastTickingState = Routine::TickingState::Continue;
		return true;
	}

	RoutineFactory factory = nullptr;
	if(push.alarm){
		AlarmRoutine::setText(push.say, push.song);
		factory = &makeRoutine<AlarmRoutine>;
	}else if(push.scenario >= 0){
		const ScenarioActivation& activation = activations[push.scenario];
		factory = routineForScenario({ activation.scenario, activation.scenarioData });
	}else{
		SpeakReplyRoutine::setText(push.say);
		factory = &makeRoutine<SpeakReplyRoutine>;
	}
	if(factory == nullptr){
		return false;
	}

	ServiceLocator::ScenarioRoutineServiceInstance->setRoutineFactory(factory);
	sm->transitionTo<ScenarioState>();
	return true;
}

void IdleState::onCommand(Ctrl::Command cmd){
	if(cmd == Ctrl::Command::Listen){
		sm->transitionTo<ListenState>();
		return;
	}

	if(cmd == Ctrl::Command::Poke && millis() >= instantEventCutoff && canStartInstantRoutine()){
		sm->startRoutine<PokeEventRoutine>();
	}
}

void IdleState::onMotion(MotionType type){
	// Events are queued to this thread; one that arrives after the floor came back is stale.
	MotionService* motion = ApplicationStatics::getApplication()->getService<MotionService>();
	if(motion != nullptr && !motion->isLifted()) return;
	if(millis() >= instantEventCutoff && canStartInstantRoutine()){
		sm->startRoutine<MotionRoutine>(type);
	}
}

bool IdleState::canStartInstantRoutine() const{
	return sm->getActiveRoutine() == nullptr && !sm->isTransitionPending();
}

void IdleState::onRCData(const RCData& data){
	if(ServiceLocator::ControllerStateInstance){
		ServiceLocator::ControllerStateInstance->updateRCData(data);
	}

	sm->transitionTo<RCState>();
	return;
}

void IdleState::onButtonEvent(int button, ISRButtonInput::Action action){
	if(button != static_cast<int>(Button::Power)){
		return;
	}

	if(action == ISRButtonInput::Action::Press){
		buttonHeld = true;
		lastBtnPressTime = millis();
		return;
	}

	buttonHeld = false;

	if(millis() - lastBtnPressTime > MaximumBtnHoldForTransition){
		return;
	}

	sm->transitionTo<ListenState>();
}
