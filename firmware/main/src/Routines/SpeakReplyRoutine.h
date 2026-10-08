#ifndef BUTTERBOT_FIRMWARE_SPEAKREPLYROUTINE_H
#define BUTTERBOT_FIRMWARE_SPEAKREPLYROUTINE_H

#include <string>
#include "Routine.h"

/**
 * Speaks a free-form answer from the voice gateway's LLM (anything that wasn't a command).
 * Routines are built by factory function pointer, so the text is handed over via setText().
 */
class SpeakReplyRoutine : public Routine {
public:
	using Routine::Routine;

	/** followUp: open a short follow-up listening window after speaking (conversation answers). */
	static void setText(const std::string& text, bool followUp = false);

	virtual TickingState tick(float deltaTime) override;

private:
	static std::string pending;
	static bool followUpAfter;
};

#endif //BUTTERBOT_FIRMWARE_SPEAKREPLYROUTINE_H
