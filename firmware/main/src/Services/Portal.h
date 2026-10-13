#ifndef BUTTERBOT_FIRMWARE_PORTAL_H
#define BUTTERBOT_FIRMWARE_PORTAL_H

#include <atomic>
#include <memory>
#include "Object/Object.h"
#include "Event/EventBroadcaster.h"
#include "Thread/Threaded.h"

class VoiceLink;

/**
 * @brief The gateway's robot portal (a web page): Remote Control mode and the robot's status.
 *
 * VC (voice control, the default at every boot) is the normal robot. In RC the portal's driver is in
 * control: IdleState ignores the wake word, the button's listen, the controller and its idle and event
 * routines, and only the portal's joystick moves the robot. The floor and front sensors are switched
 * off for the whole of RC (the driver watches the camera) and back on when RC ends, so the routines
 * use them as before. Charging doesn't restrict RC either (the robot must be able to leave a Qi dock).
 *
 * Driving: {"type":"drive","l","r"} about ten times a second while the stick is held. If none arrives
 * for DriveTimeoutMs the motors stop. The motors, sensors and that watchdog run on Portal's own task,
 * the only caller of the motors while RC is on, so a stalled network can't keep the robot driving.
 *
 * Status: {"type":"status","mode","speaking","listening","blocked"} on every change and after each
 * gateway hello, so the portal can lock Listen while the robot talks and say why it won't move.
 *
 * Battery and Wi-Fi: {"type":"battery","percent","charging","rssi"} after each hello, every minute
 * (every 5 s in RC) and when the charger is plugged in or out.
 */
class Portal : public Object {
	GENERATED_BODY(Portal, Object, CONSTRUCTOR_PACK(VoiceLink*));
public:
	explicit Portal(VoiceLink* link);
	~Portal() override;

	static Portal* get(){ return instance; }
	static bool rcActive(){ return instance != nullptr && instance->rc; }

	/** From the gateway (VoiceLink's websocket task). */
	void setRc(bool on, const char* why);
	void drive(int left, int right);

	/** The gateway link dropped: back to VC (the robot always reconnects in VC). */
	void linkLost();
	/** The gateway accepted our hello: tell it where we are. */
	void linkReady(){ dirty = true; batteryDue = true; }

	/** Set by ScenarioState / ListenState (state machine thread). */
	void setSpeaking(bool on){ if(speaking.exchange(on) != on) dirty = true; }
	void setListening(bool on){ if(listening.exchange(on) != on) dirty = true; }

	/** VoiceLink's task, every ~20 ms: sends the status when it changed. */
	void tick();

	/** bool rc: RC switched on or off (IdleState drops its routine / resumes). */
	DECLARE_EVENT(ModeEvent, Portal, bool);
	ModeEvent OnMode{ this };

	static constexpr uint32_t DriveTimeoutMs = 400;   // no stick message this long: stop

private:
	inline static Portal* instance = nullptr;
	VoiceLink* link;

	std::atomic<bool> rc{ false };
	std::atomic<bool> speaking{ false };
	std::atomic<bool> listening{ false };
	std::atomic<bool> dirty{ false };
	std::atomic<bool> stopNow{ false };          // leave RC / link lost: stop the motors on the next tick

	std::atomic<int> wantLeft{ 0 }, wantRight{ 0 };
	std::atomic<uint32_t> lastDriveMs{ 0 };
	int appliedLeft = 0, appliedRight = 0;     // motor task only
	std::atomic<const char*> blocked{ "" };     // set by the motor task, reported by tick()
	uint32_t lastMotorBeat = 0, lastNetBeat = 0;
	std::unique_ptr<Threaded> motorTask;
	void motorLoop();

	void applyMotors(int left, int right, bool force = false);
	void setSensors(bool on);
	void sendStatus();
	void sendBattery();
	std::atomic<bool> batteryDue{ false };
	uint32_t lastBatteryMs = 0;   // tick() only
	int parkedPercent = -1;       // tick() only: last level measured with the motors off
	std::atomic<uint32_t> lastMovingMs{ 0 };   // motor task: when the tracks last turned
};

#endif //BUTTERBOT_FIRMWARE_PORTAL_H
