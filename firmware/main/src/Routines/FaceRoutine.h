#ifndef BUTTERBOT_FIRMWARE_FACEROUTINE_H
#define BUTTERBOT_FIRMWARE_FACEROUTINE_H

#include <glm.hpp>
#include "Routine.h"
#include <Phrases.h>

class FaceDet;

class FaceRoutine : public Routine {
	public:
	using Routine::Routine;

public:
	~FaceRoutine() noexcept override;

	virtual Routine::TickingState tick(float deltaTime) override final;

protected:
	// Invoked while the model is loaded (recognizer available) but before the camera scan.
	// Return true to continue with detection, false to skip (subclass has already reported its outcome).
	virtual bool prepare(FaceDet* faceDet){
		return true;
	}

	// Speak the activation phrase and notify Com of the scanning phase.
	virtual void onScanStart(){}

	// Kick off the actual scan. Default starts a recognition scan (start(true));
	// subclasses that want enrollment override this to call learnFace().
	virtual void startScan(FaceDet* faceDet);

	// Face was detected within the timeout window. `known` is true if it matches the stored owner.
	virtual void onFaceDetected(FaceDet* faceDet, bool known){}

	// Scan window expired without a detection.
	virtual void onScanTimeout(){}

	// Plays a single phrase output and blocks the routine's tick until playback completes.
	// Safe to call from tick() — waitEnd blocks on a semaphore given by Audio's own thread.
	void speak(Phrase phrase, int16_t id) const;

private:
	// Routine is ticked by ScenarioState which runs on the StateMachine task — the same task that
	// dispatches CMF event callbacks via scanEvents(). The routine therefore must never block waiting
	// for OnDetect (or any other event), or the callback can never run. State is preserved across ticks
	// and OnDetect just sets a flag the next tick observes.
	enum class Phase : uint8_t {
		Init,
		Scanning,
	};

	static constexpr uint32_t DetectionTimeoutMs = 8000;

	void onDetect(int faceCount, bool known, glm::vec2 pos, glm::vec2 size);

	Phase phase = Phase::Init;
	uint64_t scanStartMs = 0;
	bool detected = false;
	bool detectedKnown = false;

	bool faceDetActive = false;
};

#endif //BUTTERBOT_FIRMWARE_FACEROUTINE_H
