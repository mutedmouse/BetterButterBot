#ifndef BUTTERBOT_FIRMWARE_SUMMONEVENTROUTINE_H
#define BUTTERBOT_FIRMWARE_SUMMONEVENTROUTINE_H

#include <cstdint>
#include <glm.hpp>
#include <Phrases.h>
#include <BBData.h>
#include "Routines/EventRoutine.h"
#include "Services/BaseBoard.h"
#include "Services/MotionService.h"

class FaceDet;

/**
 * Summon routine, triggered by the controller's Summon command (routed through EventBag). Behaves
 * like PersonRoutine: enables face detection and, while no face is visible, rotates 45 degrees in a
 * fixed random direction up to eight times (a full 360). Face detection is paused while rotating, so
 * faces are only picked up during the stationary scan windows. On detecting a face it centers on it,
 * drives towards it briefly, then announces success. Any face - stored owner or not - is treated the
 * same. Announces the search on start, and announces failure if no face is ever seen.
 *
 * Preconditions checked at start (skipped otherwise): not charging, and upright on the ground.
 * MotionService is enabled by IdleState, which this routine relies on for the upright/ground check.
 */
class SummonEventRoutine : public EventRoutine {
public:
	using EventRoutine::EventRoutine;

	~SummonEventRoutine() override;
	TickingState tick(float deltaTime) override;

private:
	void onDetect(int faceCount, bool known, glm::vec2 pos, glm::vec2 size);
	void onProximityReading(ProxState front, ProxState bottom);
	void onProximityChange(ProxSensor sensor, bool state);
	void onMotion(MotionType type);
	/** A face was seen: drive if it's roughly ahead, otherwise turn toward it by its offset and look again. */
	void aimOrDrive();
	void startDrive();
	void speak(Phrase phrase, SummonData::Phase dataPhase);
	/** One line to the gateway log ("robot: find me: ..."), so a run can be followed without USB. */
	void note(const char* fmt, ...) const __attribute__((format(printf, 2, 3)));
	void beginLook();
	void reportLook() const;

	enum class Phase : uint8_t { Init, ProxCheck, Scanning, Rotating, Centering, Driving, Settling };

	// After each turn: wait for the wobble from stopping to die down (blurry, tilted frames missed faces),
	// then look for longer (was 700 ms, about 4 detection frames).
	static constexpr uint32_t SettleMs = 400;
	static constexpr uint32_t ScanWindowMs = 2000; // long-range frames take 4 detector runs each
	// Turning at full speed made the robot rock on every start and stop (blurred, tilted frames). At 70%
	// speed, 30-degree steps (at 100%: 45 degrees ~ 1000 ms); smaller steps suit the narrower crops.
	static constexpr uint32_t RotateStepMs = 955;
	static constexpr uint8_t MaxRotations = 12;  // 12 x 30 = full 360
	static constexpr int8_t RotateSpeed = 70;
	// Aiming: continuous turning while waiting for the face to reach the middle overshot badly - long-range
	// frames take ~0.5 s and nothing is detected while turning, so the last position went stale. Instead,
	// turn by the face's offset (x fraction x horizontal field of view) with a timed turn, stop, look again.
	static constexpr float CenterDeadband = 0.12f;      // within +-12% of the middle counts as facing
	static constexpr float CameraFovDeg = 60.0f;        // horizontal field of view (estimate)
	static constexpr float AimMsPerDeg = RotateStepMs / 30.0f;   // same speed as the search steps
	static constexpr uint8_t MaxAims = 3;
	static constexpr int8_t DriveSpeed = 100;
	static constexpr uint32_t DriveMinMs = 1000;
	static constexpr uint32_t DriveMaxMs = 2000;
	static constexpr uint32_t ProxReadingTimeoutMs = 500;

	Phase phase = Phase::Init;
	bool faceDetActive = false;
	bool proxReadingReceived = false;
	uint8_t unsafeReadings = 0;    // start check: "unsafe" readings in a row
	uint64_t retryAtMs = 0;        // start check: when to ask for the next reading (0 = none)
	bool proxUnsafe = false;
	bool proxInterrupted = false;
	uint64_t proxDeadlineMs = 0;
	uint64_t settleUntilMs = 0;

	// Diagnostics for the current look and the start check.
	uint32_t lookStartFrames = 0;
	uint16_t lookDetections = 0;
	uint8_t lookMaxFaces = 0;
	int16_t lookX = -1;          // largest face centre, % of frame width
	int16_t lookW = 0;           // its width, % of frame width
	ProxState startFront = ProxState::Unknown;
	ProxState startBottom = ProxState::Unknown;
	const char* interruptCause = "";

	// Updated by onDetect on the StateMachine task, read by tick on the same task - a plain
	// member is sufficient (see PersonRoutine).
	bool detected = false;
	float faceCenterX = 0.5f;

	int8_t scanMotorLeft = 0;
	int8_t scanMotorRight = 0;
	uint8_t rotationCount = 0;

	uint64_t scanDeadlineMs = 0;
	uint64_t rotateDeadlineMs = 0;
	uint64_t centerDeadlineMs = 0;  // end of the current aiming turn
	uint8_t aimCount = 0;
	uint64_t driveEndMs = 0;

	BaseBoard* baseBoard = nullptr;
	FaceDet* faceDet = nullptr;
};

#endif //BUTTERBOT_FIRMWARE_SUMMONEVENTROUTINE_H
