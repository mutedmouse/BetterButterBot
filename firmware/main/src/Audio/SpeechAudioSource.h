#ifndef BUTTERBOT_FIRMWARE_SPEECHAUDIOSOURCE_H
#define BUTTERBOT_FIRMWARE_SPEECHAUDIOSOURCE_H

#include <Services/Audio/AudioSource.h>
#include "SpeechGen.h"

class SpeechAudioSource : public AudioSource{
public:
	SpeechAudioSource(SpeechGen::InputType type, std::string source);

	void open() override;

	void close() override;

	size_t getData(uint8_t* buffer, size_t bytes) override;

	operator bool() const override;

private:
	const SpeechGen::InputType type;
	const std::string source;

	bool lastSample = false;
};


#endif //BUTTERBOT_FIRMWARE_SPEECHAUDIOSOURCE_H