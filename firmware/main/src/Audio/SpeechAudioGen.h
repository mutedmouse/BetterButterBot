#ifndef BUTTERBOT_FIRMWARE_SPEECHAUDIOGEN_H
#define BUTTERBOT_FIRMWARE_SPEECHAUDIOGEN_H

#include "Services/Audio/AudioGenerator.h"
#include "Reverb.h"
#include "Compressor.h"
/**
 * Interface between CMF Audio service and locally defined SpeechGen service.
 *
 * Basically just a passthrough of data from AudioSource, which is expected to be a SpeechAudioSource.
 */
class SpeechAudioGen : public AudioGenerator {
	using SampleType = int16_t;

public:
	SpeechAudioGen();

	void open(std::unique_ptr<AudioSource> resource) override;

	void close() override;

	size_t getData(uint8_t* buffer, size_t bytes) override;

private:
	std::unique_ptr<AudioSource> resource;
	int bytesRemaining = 0;
	Compressor compressor;
	Reverb reverb;
};


#endif //BUTTERBOT_FIRMWARE_SPEECHAUDIOGEN_H
