#ifndef CMF_AACSOURCE_H
#define CMF_AACSOURCE_H

#include "Object/Object.h"
#include "AudioGenerator.h"
#include <aacdec.h>
#include <memory>

class AACAudioGenerator : public AudioGenerator, public Object {
	GENERATED_BODY(AACAudioGenerator, Object, void)

	using SampleType = int16_t;

public:
	AACAudioGenerator();

	void open(std::unique_ptr<AudioSource> resource) override;

	void close() override;

	size_t getData(uint8_t* buffer, size_t bytes) override;

private:
	HAACDecoder decoder;

	std::unique_ptr<AudioSource> resource;
	int bytesRemaining = 0;

	std::vector<char> fillBuffer;
	std::vector<char> dataBuffer;

	static constexpr size_t FileReadThreshold = 1024;
	static constexpr size_t FileReadChunkSize = 1024;
	static constexpr size_t ChannelNumber = 1;
	static constexpr size_t SampleSize = sizeof(SampleType);
	static constexpr size_t DecodeOutBufferSize = ChannelNumber * 1024 * SampleSize; // Size of decoded data from one decode pass is 1024 samples per channel
};

#endif //CMF_AACSOURCE_H