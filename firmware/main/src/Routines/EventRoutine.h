#ifndef BUTTERBOT_FIRMWARE_EVENTROUTINE_H
#define BUTTERBOT_FIRMWARE_EVENTROUTINE_H

#include <utility>
#include "Routine.h"
#include "EventBag.h"

class EventRoutine : public Routine {
public:
    EventRoutine(BBStateMachine* sm, EventBag::EventData data) : Routine(sm), data(std::move(data)) {}
    EventRoutine() = delete;

    bool isEventRoutine() const override { return true; }

protected:
    EventBag::EventData data;
};

#endif //BUTTERBOT_FIRMWARE_EVENTROUTINE_H
