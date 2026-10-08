#include "ScenarioState.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include "BBStateMachine.h"
#include <Util/ServiceLocator.h>
#include "IdleState.h"
#include "ListenState.h"
#include "RCState.h"
#include "Services/ScenarioRoutineService.h"

DEFINE_LOG(ScenarioState)

ScenarioState::ScenarioState(BBStateMachine* sm) : State(sm){
    const Application* app = ApplicationStatics::getApplication();
    if(!ServiceLocator::ScenarioRoutineServiceInstance){
        CMF_LOG(ScenarioState, LogLevel::Error, "ScenarioRoutineService missing in constructor");
        return;
    }

    RoutineFactory factory = ServiceLocator::ScenarioRoutineServiceInstance->getRoutineFactory();
    if(factory == nullptr) {
        CMF_LOG(ScenarioState, LogLevel::Warning, "ScenarioRoutineService has no routine set");
        return;
    }

    sm->startRoutine(factory);
}

ScenarioState::~ScenarioState(){
    if(!ServiceLocator::ScenarioRoutineServiceInstance){
        CMF_LOG(ScenarioState, LogLevel::Warning, "ScenarioRoutineService missing in destructor");
        return;
    }

    ServiceLocator::ScenarioRoutineServiceInstance->reset();
}

void ScenarioState::tick(float deltaTime) {
    Routine* routine = sm->getActiveRoutine();
    if(routine == nullptr){
        sm->transitionTo<IdleState>();
        return;
    }

    lastTickingState = routine->tick(deltaTime);
    if(lastTickingState == Routine::TickingState::Done){
        if(ListenState::followUpPending()){
            sm->transitionTo<ListenState>(); // conversation follow-up window
        }else{
            sm->transitionTo<IdleState>();
        }
        return;
    }
}

int64_t ScenarioState::getDynamicTickInterval() const{
    switch(lastTickingState){
        case Routine::TickingState::Continue:
            return 0;
        case Routine::TickingState::Block:
            return portMAX_DELAY;
        default:
            break;
    }

    return -1;
}

void ScenarioState::onRC(){
    sm->transitionTo<RCState>();
}
