#include "ModuleDevice.h"
#include "ModuleType.h"

ModuleDevice::ModuleDevice(Modules::Type moduleType, const Modules::BusPins& busPins) : moduleType(moduleType), pins(busPins){

}

Modules::Type ModuleDevice::getType() const{
	return moduleType;
}
