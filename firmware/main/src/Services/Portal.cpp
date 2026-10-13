#include "Portal.h"
#include <algorithm>
#include <cstdio>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include "Log/Log.h"
#include "BaseBoard.h"
#include "VoiceLink.h"
#include "Battery/Battery.h"

DEFINE_LOG(Portal)

namespace {
	uint32_t nowMs(){
		return (uint32_t) (esp_timer_get_time() / 1000);
	}

	constexpr uint32_t StatusSendTimeoutMs = 500;   // a status report never holds up the link for long
	constexpr uint32_t HeartbeatMs = 2000;          // RC: each task logs that it is still running
	// Battery level for the portal: every minute (60 bytes against ~20 KB/s of camera), after each connect,
	// and at once when the charger is plugged in or out.
	constexpr uint32_t BatteryReportMs = 60 * 1000;
	// With Wi-Fi signal strength in the same report: every 5 s while driving (RC), to see it fade.
	constexpr uint32_t RcReportMs = 5 * 1000;
	// The motors pull the battery voltage down, so the level only counts once the robot has been still
	// this long; while driving, the last parked reading is reported.
	constexpr uint32_t BatterySettleMs = 5000;
}

Portal::Portal(VoiceLink* link) : link(link){
	instance = this;
	// The motors, the floor/front sensors and the stick watchdog run on their own task, so a stalled
	// network (Wi-Fi fading as the robot drives away, a slow send) can never keep the motors running.
	motorTask = std::make_unique<Threaded>([this](){ motorLoop(); }, "Portal", 20 / portTICK_PERIOD_MS, 3 * 1024, 6, 1, false);
	motorTask->start();

	if(Battery* battery = ApplicationStatics::getApplication()->getService<Battery>()){
		battery->OnChargeStatus.bind(this, [this](ChargingState){ batteryDue = true; });
	}
}

Portal::~Portal(){
	motorTask->stop(portMAX_DELAY);
}

void Portal::setRc(bool on, const char* why){
	wantLeft = wantRight = 0;
	if(rc.exchange(on) == on){
		dirty = true;   // the gateway may have missed our status: send it again
		return;
	}
	stopNow = true;   // entering: stop whatever moved before; leaving: stop the stick
	dirty = true;
	CMF_LOG(Portal, LogLevel::Info, "%s (%s)", on ? "Remote control" : "Voice control", why);
	OnMode.broadcast(on);
}

void Portal::drive(int left, int right){
	if(!rc) return;
	wantLeft = std::clamp(left, -100, 100);
	wantRight = std::clamp(right, -100, 100);
	lastDriveMs = nowMs();
}

void Portal::linkLost(){
	speaking = listening = false;
	setRc(false, "gateway link lost");
}

void Portal::motorLoop(){
	if(stopNow.exchange(false)){
		applyMotors(0, 0, true);
		blocked = "";
		setSensors(!rc);
	}
	if(!rc) return;

	int left = wantLeft, right = wantRight;
	const uint32_t age = nowMs() - lastDriveMs;
	if(age > DriveTimeoutMs){
		if(appliedLeft != 0 || appliedRight != 0){
			CMF_LOG(Portal, LogLevel::Info, "Stick watchdog: no drive for %lu ms, stopping", (unsigned long) age);
		}
		left = right = 0;      // stick messages stopped (page closed, Wi-Fi fading): stop
	}

	// Nothing on the robot restricts RC driving: not the floor or front sensors (off in RC), and not the
	// charger (a Qi dock charges too, and the robot has to be able to drive off it). The driver decides.

	if((left == 0 && right == 0) != (appliedLeft == 0 && appliedRight == 0)){
		CMF_LOG(Portal, LogLevel::Info, "Drive %s L%d R%d (stick age %lu ms)", (left || right) ? "start" : "stop",
				left, right, (unsigned long) age);
	}
	applyMotors(left, right);
	if(appliedLeft != 0 || appliedRight != 0){
		lastMovingMs = nowMs();
	}

	if(nowMs() - lastMotorBeat > HeartbeatMs){
		lastMotorBeat = nowMs();
		CMF_LOG(Portal, LogLevel::Info, "motor task alive: L%d R%d, stick age %lu ms", appliedLeft, appliedRight, (unsigned long) age);
	}
}

void Portal::tick(){
	if(batteryDue.exchange(false) || nowMs() - lastBatteryMs > (rc ? RcReportMs : BatteryReportMs)){
		sendBattery();
	}
	if(rc && nowMs() - lastNetBeat > HeartbeatMs){
		lastNetBeat = nowMs();
		VoiceLink::logHeap("rc link task alive");   // also logs Wi-Fi signal strength (RSSI)
	}
	if(dirty.exchange(false)){
		sendStatus();
	}
}

void Portal::setSensors(bool on){
	// The base board's floor and front sensors: off for the whole of RC so nothing on the robot cuts the
	// driver's motors (the base board may act on them itself while scanning). Back on for voice control,
	// with a fresh reading so the routines and MotionService start from the real state.
	BaseBoard* baseBoard = ApplicationStatics::getApplication()->getService<BaseBoard>();
	if(baseBoard == nullptr) return;
	baseBoard->setProximityScanning(on);
	if(on){
		baseBoard->requestProximityState();
	}
	CMF_LOG(Portal, LogLevel::Info, "Floor/front sensors %s", on ? "on" : "off (remote control)");
}

void Portal::applyMotors(int left, int right, bool force){
	if(!force && left == appliedLeft && right == appliedRight) return;
	BaseBoard* baseBoard = ApplicationStatics::getApplication()->getService<BaseBoard>();
	if(baseBoard == nullptr) return;
	baseBoard->setMotors((int8_t) left, (int8_t) right);
	appliedLeft = left;
	appliedRight = right;
}

void Portal::sendBattery(){
	Battery* battery = ApplicationStatics::getApplication()->getService<Battery>();
	if(battery == nullptr) return;
	const ChargingState state = battery->getChargingState();
	const char* charging = state == ChargingState::Charging ? "charging" : state == ChargingState::Full ? "full" : "unplugged";
	int rssi = 0;
	esp_wifi_sta_get_rssi(&rssi);
	// Driving (or just stopped): keep the last parked level instead of the load-dipped one.
	int percent = (int) (battery->getPerc() + 0.5f);
	if(nowMs() - lastMovingMs < BatterySettleMs && parkedPercent >= 0){
		percent = parkedPercent;
	}else{
		parkedPercent = percent;
	}
	char msg[96];
	snprintf(msg, sizeof(msg), "{\"type\":\"battery\",\"percent\":%d,\"charging\":\"%s\",\"rssi\":%d}", percent, charging, rssi);
	if(link->sendText(msg, StatusSendTimeoutMs)){
		lastBatteryMs = nowMs();
	}else{
		lastBatteryMs = nowMs() - BatteryReportMs + 5000;   // not connected: try again in 5 s
	}
}

void Portal::sendStatus(){
	char msg[112];
	snprintf(msg, sizeof(msg), "{\"type\":\"status\",\"mode\":\"%s\",\"speaking\":%s,\"listening\":%s,\"blocked\":\"%s\"}",
			 rc ? "rc" : "vc", speaking ? "true" : "false", listening ? "true" : "false", blocked.load());
	if(!link->sendText(msg, StatusSendTimeoutMs)){
		dirty = true;   // not connected or the link is busy: try again on a later tick (and after the next hello)
	}
}
