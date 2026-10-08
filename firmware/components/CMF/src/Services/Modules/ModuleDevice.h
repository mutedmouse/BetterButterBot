#ifndef CMF_MODULEDEVICE_H
#define CMF_MODULEDEVICE_H

#include "Object/Class.h"
#include "Object/Object.h"
#include "ModuleType.h"
#include "Drivers/Interface/InputDriver.h"
#include "Drivers/Interface/OutputDriver.h"

class ModuleDevice : public Object {
	GENERATED_BODY(ModuleDevice, Object, CONSTRUCTOR_PACK(Modules::Type, const Modules::BusPins&))

public:
	ModuleDevice(Modules::Type Type = Modules::Type::Unknown, const Modules::BusPins& busPins={});

	Modules::Type getType() const;

protected:
	const Modules::Type moduleType;

	Modules::BusPins pins;
};


#endif //CMF_MODULEDEVICE_H
