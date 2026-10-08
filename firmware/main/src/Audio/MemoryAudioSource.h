#ifndef BUTTERBOT_FIRMWARE_MEMORYAUDIOSOURCE_H
#define BUTTERBOT_FIRMWARE_MEMORYAUDIOSOURCE_H

#include <Services/Audio/AudioSource.h>
#include <cstdint>
#include "Util/PSRAMAllocator.h"

/**
 * In-RAM audio source. Plays from a PSRAM buffer that was preloaded from flash, so
 * the audio task never touches the SPI flash during playback. Used to play SFX while
 * a flash-heavy operation (e.g. esp-sr model loading) is running on another task -
 * a FileAudioSource would otherwise block on the flash lock until that finishes.
 */
class MemoryAudioSource : public AudioSource {
public:
	explicit MemoryAudioSource(PSRAMByteBuffer data);

	void open() override;

	void close() override;

	size_t getData(uint8_t* buffer, size_t bytes) override;

	operator bool() const override;

private:
	PSRAMByteBuffer data;
	size_t pos = 0;
};

#endif //BUTTERBOT_FIRMWARE_MEMORYAUDIOSOURCE_H
