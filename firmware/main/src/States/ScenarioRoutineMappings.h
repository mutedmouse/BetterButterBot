#ifndef BUTTERBOT_FIRMWARE_SCENARIOROUTINEMAPPINGS_H
#define BUTTERBOT_FIRMWARE_SCENARIOROUTINEMAPPINGS_H

#include <utility>
#include "BBData.h"
#include "Scenarios.h"
#include "StateRoutineFactory.h"

RoutineFactory routineForScenario(const std::pair<BB::Action::Scenario, ScenarioData>& scenario);

#endif //BUTTERBOT_FIRMWARE_SCENARIOROUTINEMAPPINGS_H