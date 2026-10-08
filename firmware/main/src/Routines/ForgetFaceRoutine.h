#ifndef BUTTERBOT_FIRMWARE_FORGETFACEROUTINE_H
#define BUTTERBOT_FIRMWARE_FORGETFACEROUTINE_H

#include "Routine.h"

class ForgetFaceRoutine : public Routine {
	public:
	using Routine::Routine;

public:
	virtual Routine::TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_FORGETFACEROUTINE_H
