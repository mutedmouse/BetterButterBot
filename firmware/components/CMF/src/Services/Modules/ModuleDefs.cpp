#include "ModuleDefs.h"
#include <map>
#include "Memory/ObjectMemory.h"

#ifdef CONFIG_RM_TempHum

#include "ModuleDevices/RM_TempHumModule.h"

#endif
#ifdef CONFIG_RM_LED

#include "ModuleDevices/RM_LEDModule.h"

#endif
#ifdef CONFIG_RM_Motion

#include "ModuleDevices/RM_MotionSensor.h"

#endif
#ifdef CONFIG_RM_CO2

#include "ModuleDevices/RM_CO2Sensor.h"

#endif
#ifdef CONFIG_RM_IR

#include "ModuleDevices/RM_IRModule.h"

#endif
#ifdef CONFIG_RM_PerfBoard

#include "ModuleDevices/RM_Perfboard.h"
#include "ModuleType.h"

#endif

namespace Modules {

const std::map<Address, Type>& GetAddressMap(){
	static const std::map<Address, Type> addressMap = {
#ifdef CONFIG_RM_TempHum
			{{ 61, { .type=SubAddress::Type::RM_I2C, .I2CAddress=0x38 }}, Type::RM_TempHum },
#endif
#ifdef CONFIG_RM_LED
			{{ 16, { .type=SubAddress::Type::RM, .RMAddress=3 }}, Type::RM_LED },
#endif
#ifdef CONFIG_RM_Motion
			{{ 16, { .type=SubAddress::Type::RM, .RMAddress=0 }}, Type::RM_Motion },
#endif
#ifdef CONFIG_RM_CO2
			{{ 16, { .type=SubAddress::Type::RM, .RMAddress=1 }}, Type::RM_CO2 },
			{{ 16, { .type=SubAddress::Type::RM, .RMAddress=5 }}, Type::RM_CO2 }, // Gas module alias
#endif
#ifdef CONFIG_RM_IR
			{{ 16, { .type=SubAddress::Type::RM, .RMAddress=2 }}, Type::RM_IR },
#endif
#ifdef CONFIG_RM_PerfBoard
			{{ 16, { .type=SubAddress::Type::RM, .RMAddress=4 }}, Type::RM_PerfBoard },
#endif
	};
	return addressMap;
}

const std::map<uint8_t, std::set<SubAddress>>& GetSubAddressMap(){
	static std::map<uint8_t, std::set<SubAddress>> SubAddressMap;
	for(const auto& it : GetAddressMap()){
		const auto& address = it.first;
		SubAddressMap[address.mainAddress].insert(address.subAddress);
	}
	return SubAddressMap;
}

StrongObjectPtr<ModuleDevice> CreateModuleDevice(Object* owner, Type moduleType, const Modules::BusPins& busPins){
	switch(moduleType){
#ifdef CONFIG_RM_TempHum
		case Type::RM_TempHum:
			return newObject<RM_TempHumModule>(owner, busPins);
#endif
#ifdef CONFIG_RM_LED
		case Type::RM_LED:
			return newObject<RM_LEDModule>(owner, busPins);
#endif
#ifdef CONFIG_RM_Motion
		case Type::RM_Motion:
			return newObject<RM_MotionSensor>(owner, busPins);
#endif
#ifdef CONFIG_RM_CO2
		case Type::RM_CO2:
			return newObject<RM_CO2Sensor>(owner, busPins);
#endif
#ifdef CONFIG_RM_IR
		case Type::RM_IR:
			return newObject<RM_IRModule>(owner, busPins);
#endif
#ifdef CONFIG_RM_PerfBoard
		case Type::RM_PerfBoard:
			return newObject<RM_Perfboard>(owner, busPins);
#endif
		default:
			return nullptr;
	}
}

const std::map<Type, std::array<PinMode, 6>>& GetPinModeMap(){
	static const std::map<Type, std::array<PinMode, 6>> PinModeMap = {
#ifdef CONFIG_RM_TempHum
			{ Type::RM_TempHum, {}},
#endif
#ifdef CONFIG_RM_LED
			{ Type::RM_LED, { PinMode::Output }},
#endif
#ifdef CONFIG_RM_Motion
			{ Type::RM_Motion, { PinMode::Input, PinMode::Output }},
#endif
#ifdef CONFIG_RM_CO2
			{ Type::RM_CO2, { PinMode::None, PinMode::None, PinMode::None, PinMode::None, PinMode::None, PinMode::Input }},
#endif
#ifdef CONFIG_RM_IR
			{ Type::RM_IR, { PinMode::Input, PinMode::Output }},
#endif
#ifdef CONFIG_RM_PerfBoard
			{ Type::RM_PerfBoard, {}},
#endif
	};
	return PinModeMap;
}

}
