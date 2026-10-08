#ifndef CMF_TEMPLATE_MICINPUT_H
#define CMF_TEMPLATE_MICINPUT_H

#include "Periphery/I2S.h"

class MicInput {
public:
	/**
	 * @param i2s - read-capable I2S instance
	 */
	MicInput(I2S* i2s = nullptr);

	/**
	 * Reads and returns a stereo audio buffer
	 * @param output buffer
	 * @return amount of read samples, or ESP_FAIL
	 */
	int32_t getStereo(std::span<int16_t> output);

	/**
	 * Reads and flattens audio into a mono buffer
	 * @param output buffer
	 * @return amount of read samples, or ESP_FAIL
	 */
	int32_t getMono(std::span<int16_t> output);

private:
	I2S* i2s;

	static constexpr uint8_t BytesPerSample = sizeof(int16_t);

	static constexpr size_t BufferSize = 8 * 1024; //bytes, not samples

	std::array<int16_t, BufferSize/2> buf{};

};


#endif //CMF_TEMPLATE_MICINPUT_H
