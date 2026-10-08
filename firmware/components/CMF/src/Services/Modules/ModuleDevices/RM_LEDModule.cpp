#include "RM_LEDModule.h"
#include "Log/Log.h"

DEFINE_LOG(RM_LEDModule)

RM_LEDModule::RM_LEDModule(const Modules::BusPins& busPins) : Super(Modules::Type::RM_LED, busPins){
	set(false);
}

void RM_LEDModule::set(bool state){
	setLevel(state ? 1.0f : 0.0f);
}

void RM_LEDModule::setLevel(float level){
	CMF_LOG(RM_LEDModule, LogLevel::Info, "Setting LED level to %.2f", level);
	if(pins.subAddressPins[0].outputDriver){
		pins.subAddressPins[0].outputDriver->write(pins.subAddressPins[0].outputPort, level);
	}else{
		CMF_LOG(RM_LEDModule, LogLevel::Warning, "No output driver assigned for LED module pin");
	}
}
