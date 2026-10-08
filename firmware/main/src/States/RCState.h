#ifndef BUTTERBOT_FIRMWARE_RCSTATE_H
#define BUTTERBOT_FIRMWARE_RCSTATE_H

#include <CtrlData.h>
#include <Services/Com.h>
#include <Util/WiFiStation.h>
#include "State.h"

class RCState : public State {
public:
	RCState(BBStateMachine* sm);
	virtual ~RCState() override;
	virtual int64_t getDynamicTickInterval() const noexcept override;

private:
	StrongObjectPtr<class Feed> feed;
	StrongObjectPtr<class WiFiStation> wifiSta;
	StrongObjectPtr<class WiFi> wifi;

private:
	void onCommand(Ctrl::Command cmd);
	void playRandomSound();
	void onDriveData(DriveData driveData);
	void onBTDisconnect(Com::ConnStatus status);
	void onWifiDisconnect(WiFiStation::EventType type, bool success);
};

#endif //BUTTERBOT_FIRMWARE_RCSTATE_H