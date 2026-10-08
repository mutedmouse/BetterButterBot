#include "Compressor.h"
#include <cmath>
#include <algorithm>

Compressor::Compressor(){
	updateCoeffs();
}

void Compressor::process(int16_t* samples, const size_t count){
	for(size_t i = 0; i < count; i++){
		const float input = samples[i] * (1.0f / 32768.0f);
		const float mag = fabsf(input);

		// One-pole peak envelope follower: fast attack on rise, slow release on fall.
		const float coeff = (mag > envelope) ? attackCoeff : releaseCoeff;
		envelope = coeff * envelope + (1.0f - coeff) * mag;

		// Soft gain reduction above the threshold, in the linear domain.
		float reduction = 1.0f;
		if(envelope > threshold){
			reduction = powf(envelope / threshold, 1.0f / ratio - 1.0f);
		}

		const float out = input * reduction * makeup;
		samples[i] = static_cast<int16_t>(std::clamp(out * 32767.0f, -32768.0f, 32767.0f));
	}
}

void Compressor::reset(){
	envelope = 0.0f;
}

void Compressor::setThreshold(const float linear){
	threshold = std::clamp(linear, 0.0001f, 1.0f);
}

void Compressor::setRatio(const float r){
	ratio = std::max(r, 1.0f);
}

void Compressor::setMakeup(const float linear){
	makeup = linear;
}

void Compressor::setAttack(const float seconds){
	attackTime = seconds;
	updateCoeffs();
}

void Compressor::setRelease(const float seconds){
	releaseTime = seconds;
	updateCoeffs();
}

void Compressor::updateCoeffs(){
	attackCoeff = expf(-1.0f / (attackTime * SampleRate));
	releaseCoeff = expf(-1.0f / (releaseTime * SampleRate));
}
