#ifndef BUTTERBOT_FIRMWARE_SCENARIOROUTINE_H
#define BUTTERBOT_FIRMWARE_SCENARIOROUTINE_H

#include "States/StateRoutineFactory.h"

// This service is needed because at the moment there is no other way to pass information or parameters from one state to another (in this case listen state to scenario state)
class ScenarioRoutineService {
public:
    void setRoutineFactory(RoutineFactory value){
        routine = value;
    }

    RoutineFactory getRoutineFactory() const{
        return routine;
    }

    void reset(){
        routine = nullptr;
    }

private:
    RoutineFactory routine = nullptr;
};

#endif //BUTTERBOT_FIRMWARE_SCENARIOROUTINE_H