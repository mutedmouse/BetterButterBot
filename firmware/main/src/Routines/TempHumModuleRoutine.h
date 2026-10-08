#ifndef BUTTERBOT_FIRMWARE_TEMPHUMMODULEROUTINE_H
#define BUTTERBOT_FIRMWARE_TEMPHUMMODULEROUTINE_H

#include <string>
#include "Routine.h"
#include "Services/Settings.h"

class TempHumModuleRoutine : public Routine {
    public:
	using Routine::Routine;

public:
    static SettingsStruct::Scale currentScale;

    TickingState tick(float deltaTime) override;

    static int16_t convertTemp(int16_t celsius);
    static std::string formatTemp(int16_t converted);

private:
    bool scaleLoaded = false;
};

#endif //BUTTERBOT_FIRMWARE_TEMPHUMMODULEROUTINE_H
