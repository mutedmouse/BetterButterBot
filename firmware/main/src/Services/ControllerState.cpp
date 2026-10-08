#include "ControllerState.h"
#include <Core/Application.h>
#include "Com.h"

DEFINE_LOG(ControllerState);

ControllerState::ControllerState(){
	memset(SSID, '\0', sizeof(RCData::SSID));
	memset(password, '\0', sizeof(RCData::password));

	Application* app = Application::getApp();

	Com* com = app->getService<Com>();
	if(com == nullptr){
		CMF_LOG(ControllerState, Warning, "Com is nullptr in postInitProperties.");
		return;
	}

	com->OnRCData.bind(app, [this](const RCData& data) { onRCData(data); });
}

ControllerState::~ControllerState(){
	Application* app = Application::getApp();
	if(app == nullptr){
		return;
	}

	if(Com* com = app->getService<Com>()){
		com->OnRCData.unbind(app);
	}
}

void ControllerState::onRCData(const RCData& data){
	memcpy(SSID, data.SSID, sizeof(RCData::SSID));
	memcpy(password, data.password, sizeof(RCData::password));
}