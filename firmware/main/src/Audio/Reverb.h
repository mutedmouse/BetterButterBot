#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

// Schroeder-Moorer reverb: 4 parallel feedback comb filters into 2 serial allpass filters.
// Delay lengths tuned for 16 kHz.
class Reverb {
public:
	Reverb();

	void process(int16_t* samples, size_t count);

	// roomSize: 0.0–1.0 — maps feedback to the 0.70–0.90 range
	void setRoomSize(float roomSize);

	// damping: 0.0 = bright/metallic, 1.0 = dark/absorptive
	void setDamping(float damping);

	// wet: 0.0–1.0 reverb mix level; dry is set to (1 - wet)
	void setWet(float wet);

private:
	static constexpr int NumCombs = 4;
	static constexpr int NumAllpass = 2;
	static constexpr int CombLengths[NumCombs] = {401, 431, 461, 487};
	static constexpr int AllpassLengths[NumAllpass] = {199, 157};
	static constexpr int MaxCombLength = 487;
	static constexpr int MaxAllpassLength = 199;

	struct CombFilter {
		std::array<float, MaxCombLength> buf = {};
		int len = 0;
		int pos = 0;
		float filterstore = 0.0f;
		float feedback = 0.84f;
		float damp = 0.2f;

		float process(float input);
	};

	struct AllpassFilter {
		std::array<float, MaxAllpassLength> buf = {};
		int len = 0;
		int pos = 0;

		float process(float input);
	};

	CombFilter combs[NumCombs];
	AllpassFilter allpasses[NumAllpass];

	float wet = 0.25f;
	float dry = 0.75f;
};
