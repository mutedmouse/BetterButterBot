#ifndef BUTTERBOT_FIRMWARE_WANDERROUTINE_H
#define BUTTERBOT_FIRMWARE_WANDERROUTINE_H

#include <cstdint>
#include "Routine.h"
#include "Services/BaseBoard.h"

class WanderRoutine : public Routine {
public:
	using Routine::Routine;

	virtual ~WanderRoutine() override;
	TickingState tick(float deltaTime) override;

private:
	void onProximityReading(ProxState front, ProxState bottom);
	void onProximityChange(ProxSensor sensor, bool state);

	enum class Phase : uint8_t { Init, ProxCheck, Moving, Done };

	struct Action {
		int8_t left;
		int8_t right;
		uint32_t durationMs;
	};

	static constexpr uint16_t MinRotationDeg = 90;
	static constexpr uint16_t MaxRotationDeg = 360;

	static constexpr uint32_t MinForwardMs = 500;
	static constexpr uint32_t MaxForwardMs = 2500;

	// Rotation timing reference, taken from VoiceTurn180Routine: 180° at ±100 takes 4000 ms.
	static constexpr uint16_t Rotation180Deg = 180;
	static constexpr uint32_t Rotation180Ms = 4000;

	static constexpr int8_t MotorSpeed = 100;
	static constexpr uint32_t ProxReadingTimeoutMs = 500;

	Phase phase = Phase::Init;
	Action actions[2] = {};
	uint8_t actionIndex = 0;
	uint64_t actionEndMs = 0;
	uint64_t proxDeadlineMs = 0;
	bool proxReadingReceived = false;
	bool proxUnsafe = false;
	bool proxInterrupted = false;

	BaseBoard* baseBoard = nullptr;

	static Action makeRotation();
	static Action makeForward();
	void startAction(const Action& action);
};

#endif //BUTTERBOT_FIRMWARE_WANDERROUTINE_H
