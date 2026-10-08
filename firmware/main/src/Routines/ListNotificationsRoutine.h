#ifndef BUTTERBOT_FIRMWARE_LISTNOTIFICATIONSROUTINE_H
#define BUTTERBOT_FIRMWARE_LISTNOTIFICATIONSROUTINE_H

#include <cstdint>
#include <vector>
#include "PhraseListeningRoutine.h"
#include "Phone/Notif.h"

class ListNotificationsRoutine : public PhraseListeningRoutine {
public:
	explicit ListNotificationsRoutine(BBStateMachine* sm) : PhraseListeningRoutine(sm) {}
	TickingState tick(float deltaTime) override;

private:
	enum class Phase : uint8_t { Init, Reading, WaitContinue, Finish };
	Phase phase = Phase::Init;

	std::vector<Notif> notifs;
	uint8_t count = 0;
	uint8_t index = 0;
};

#endif //BUTTERBOT_FIRMWARE_LISTNOTIFICATIONSROUTINE_H
