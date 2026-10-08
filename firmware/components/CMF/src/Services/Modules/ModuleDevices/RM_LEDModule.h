#ifndef CMF_RM_LEDMODULE_H
#define CMF_RM_LEDMODULE_H

#include "Object/Class.h"
#include "Services/Modules/ModuleDevice.h"

class RM_LEDModule : public ModuleDevice {
	GENERATED_BODY(RM_LEDModule, ModuleDevice, CONSTRUCTOR_PACK(const Modules::BusPins&))

public:
	RM_LEDModule(const Modules::BusPins& busPins = {});

	void set(bool state);
	void setLevel(float level);
};


#endif //CMF_RM_LEDMODULE_H
