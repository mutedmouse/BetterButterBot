#ifndef BUTTERBOT_FIRMWARE_TEMPHUMSCALEROUTINE_H
#define BUTTERBOT_FIRMWARE_TEMPHUMSCALEROUTINE_H

#include "Routine.h"

class TempHumCelsiusRoutine : public Routine {
    public:
	using Routine::Routine;
public:
    TickingState tick(float deltaTime) override;
};

class TempHumFahrenheitRoutine : public Routine {
    public:
	using Routine::Routine;
public:
    TickingState tick(float deltaTime) override;
};

class TempHumKelvinRoutine : public Routine {
    public:
	using Routine::Routine;
public:
    TickingState tick(float deltaTime) override;
};

#endif //BUTTERBOT_FIRMWARE_TEMPHUMSCALEROUTINE_H
