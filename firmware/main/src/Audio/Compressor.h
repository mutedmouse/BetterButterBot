#ifndef BUTTERBOT_FIRMWARE_COMPRESSOR_H
#define BUTTERBOT_FIRMWARE_COMPRESSOR_H

#include <cstddef>
#include <cstdint>

/**
 * Single-channel feed-forward dynamic range compressor, applied in-place to
 * int16 samples.
 *
 * A one-pole peak envelope follower (attack/release) drives a soft-knee gain
 * reduction above the threshold; makeup gain then lifts the whole signal.
 * Lets the quiet body of the speech come up without driving the peaks into
 * clipping. Carries state (the envelope), so reset() must be called at the
 * start of each utterance.
 */
class Compressor {
public:
	Compressor();

	void process(int16_t* samples, size_t count);

	// Clears the envelope follower. Call at the start of each new speech.
	void reset();

	void setThreshold(float linear); // level above which gain reduction kicks in, [0, 1]
	void setRatio(float r);          // compression ratio, e.g. 4 => 4:1
	void setMakeup(float linear);    // output gain applied after compression
	void setAttack(float seconds);
	void setRelease(float seconds);

private:
	void updateCoeffs();

	static constexpr float SampleRate = 16000.0f;

	// Tunable parameters.
	float threshold = 0.2f; // ~-14 dBFS
	float ratio = 4.0f;
	float makeup = 1.5f;
	float attackTime = 0.005f;  // 5 ms
	float releaseTime = 0.080f; // 80 ms

	// Derived envelope coefficients.
	float attackCoeff = 0.0f;
	float releaseCoeff = 0.0f;

	// State.
	float envelope = 0.0f;
};


#endif //BUTTERBOT_FIRMWARE_COMPRESSOR_H
