#include "MicInput.h"

DEFINE_LOG(MicInput)

MicInput::MicInput(I2S* i2s) : i2s(i2s){
	if(!i2s){
		CMF_LOG(MicInput, LogLevel::Error, "No I2S peripheral provided!");
		abort();
	}
}

int32_t MicInput::getMono(std::span<int16_t> output){
	size_t totalSamplesRead = 0;

	while(totalSamplesRead < output.size()){
		const auto readSamples = i2s->read((uint8_t*) buf.data(), BufferSize) / BytesPerSample;

		if(!readSamples){
			break;
		}

		for(size_t i = 0; i < (output.size() - totalSamplesRead) && i < (readSamples / 2); i++){
			output[totalSamplesRead + i] = (int16_t) ((buf[i * 2] + buf[i * 2 + 1]) / 2);
		}

		totalSamplesRead += readSamples / 2;
	}

	return (int32_t) totalSamplesRead;
}

int32_t MicInput::getStereo(std::span<int16_t> output){
	const auto readSamples = i2s->read((uint8_t*) output.data(), output.size_bytes()) / BytesPerSample;

	if(!readSamples){
		return ESP_FAIL;
	}

	return (int32_t) readSamples;
}
