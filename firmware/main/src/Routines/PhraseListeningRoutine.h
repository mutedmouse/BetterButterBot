#ifndef BUTTERBOT_FIRMWARE_PHRASELISTENINGROUTINE_H
#define BUTTERBOT_FIRMWARE_PHRASELISTENINGROUTINE_H

#include <span>
#include "Routine.h"
#include "Audio/AudioFrontend.h"

/**
 * @brief Base for routines that recognize a spoken phrase without blocking the tick thread.
 *
 * Instead of calling AudioFrontend::waitForPhrase() (which parks the whole state machine), a
 * derived routine arms recognition with listen(), remembers its flow phase, and returns
 * TickingState::Block. The tick thread then sleeps (portMAX_DELAY) until onPhrase wakes it. The
 * AFE always fires onPhrase - on a detection or on its own recognition-window timeout
 * (recognized == false) - so the wait is guaranteed to end. On the next tick the routine consumes
 * phraseReady()/phraseResult() and advances.
 *
 * The onPhrase callback runs on the state machine tick thread (same thread as tick()), so the
 * result members need no synchronisation.
 */
class PhraseListeningRoutine : public Routine {
public:
	explicit PhraseListeningRoutine(BBStateMachine* sm);
	~PhraseListeningRoutine() override;

protected:
	/**
	 * @brief Arm speech recognition for the given phrases and request the machine to block until a
	 * result arrives. The caller must return TickingState::Block right after calling this.
	 */
	void listen(std::span<const AudioFrontend::Phrase> phrases);

	/** @return True once onPhrase has delivered a result for the current listen. */
	bool phraseReady() const { return resultReady; }
	const AudioFrontend::PhraseResult& phraseResult() const { return result; }

	AudioFrontend* af = nullptr;

private:
	void onPhrase(bool recognized, int index, std::string transcript, float confidence);

	AudioFrontend::PhraseResult result{};
	bool resultReady = false;
	bool bound = false;
};

#endif //BUTTERBOT_FIRMWARE_PHRASELISTENINGROUTINE_H
