#include "Reverb.h"
#include <algorithm>

Reverb::Reverb() {
	for(int i = 0; i < NumCombs; i++){
		combs[i].len = CombLengths[i];
	}
	for(int i = 0; i < NumAllpass; i++){
		allpasses[i].len = AllpassLengths[i];
	}
}

float Reverb::CombFilter::process(const float input){
	const float output = buf[pos];
	filterstore = output * (1.0f - damp) + filterstore * damp;
	buf[pos] = input + filterstore * feedback;
	if (++pos >= len){
		pos = 0;
	}

	return output;
}

float Reverb::AllpassFilter::process(const float input){
	const float bufout = buf[pos];
	const float output = -input + bufout;
	buf[pos] = input + bufout * 0.5f;
	if(++pos >= len){
		pos = 0;
	}

	return output;
}

void Reverb::process(int16_t* samples, const size_t count){
	for(size_t i = 0; i < count; i++){
		const float input = samples[i] * (1.0f / 32768.0f);

		float sum = 0.0f;
		for(auto& comb : combs){
			sum += comb.process(input);
		}

		float reverbOut = sum * (1.0f / NumCombs);
		for(auto& ap : allpasses){
			reverbOut = ap.process(reverbOut);
		}

		const float out = input * dry + reverbOut * wet;
		samples[i] = static_cast<int16_t>(std::clamp(out * 32768.0f, -32768.0f, 32767.0f));
	}
}

void Reverb::setRoomSize(const float roomSize){
	const float feedback = 0.70f + roomSize * 0.20f;
	for(auto& comb : combs){
		comb.feedback = feedback;
	}
}

void Reverb::setDamping(const float damping){
	for(auto& comb : combs){
		comb.damp = damping;
	}
}

void Reverb::setWet(float w){
	wet = std::clamp(w, 0.0f, 1.0f);
	dry = 1.0f - wet;
}
