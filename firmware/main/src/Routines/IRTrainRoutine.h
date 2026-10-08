#ifndef BUTTERBOT_FIRMWARE_IRTRAINROUTINE_H
#define BUTTERBOT_FIRMWARE_IRTRAINROUTINE_H

#include "PhraseListeningRoutine.h"

class IRTrainRoutine : public PhraseListeningRoutine {
public:
	explicit IRTrainRoutine(BBStateMachine* sm) : PhraseListeningRoutine(sm) {}
	TickingState tick(float deltaTime) override;

private:
    static int levenshtein(const char* a, const char* b);
    static bool tooSimilar(const char* phonemes);
};

#endif //BUTTERBOT_FIRMWARE_IRTRAINROUTINE_H
