#ifndef BUTTERBOT_FIRMWARE_EIGHTBALLROUTINE_H
#define BUTTERBOT_FIRMWARE_EIGHTBALLROUTINE_H
#include "Routine.h"
#include <atomic>
#include <cstdint>

class EightBallRoutine : public Routine {
public:
	explicit EightBallRoutine(BBStateMachine* sm);
	~EightBallRoutine() override;

	TickingState tick(float deltaTime) override;


private:
	void handleVAD(bool active);

	enum class Phase : uint8_t {
		Init,
		Listening,
		WaitingForSpeechEnd,
	};

	static constexpr uint32_t MaxSpeechDurationMs = 30000;

	Phase phase = Phase::Init;
	uint64_t listenStartMs = 0;
	uint64_t speechStartMs = 0;
	std::atomic<bool> speechDetected{ false };
	std::atomic<bool> speechEnded{ false };
};

#endif //BUTTERBOT_FIRMWARE_EIGHTBALLROUTINE_H