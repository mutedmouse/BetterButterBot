#ifndef BUTTERBOT_FIRMWARE_MOTIONROUTINE_H
#define BUTTERBOT_FIRMWARE_MOTIONROUTINE_H

#include "Routine.h"
#include "Services/MotionService.h"

class MotionRoutine : public Routine {
public:
	MotionRoutine(BBStateMachine* sm, MotionType type);

	bool isEventRoutine() const override { return true; }

private:
	TickingState tick(float deltaTime) override;

	MotionType motionType;
};


#endif //BUTTERBOT_FIRMWARE_MOTIONROUTINE_H
