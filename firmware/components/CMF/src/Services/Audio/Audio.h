#ifndef CMF_AUDIO_H
#define CMF_AUDIO_H

#include <optional>
#include "Object/Class.h"
#include "Entity/AsyncEntity.h"
#include "Drivers/Interface/OutputDriver.h"
#include "AudioGenerator.h"
#include "Event/EventBroadcaster.h"
#include "Periphery/I2S.h"

class Audio : public AsyncEntity {
	GENERATED_BODY(Audio, AsyncEntity, CONSTRUCTOR_PACK(StrongObjectPtr<I2S>, OutputPin))

public:
	typedef std::pair<AudioGenerator*, std::unique_ptr<AudioSource>> GenSourcePair;

	Audio(StrongObjectPtr<I2S> i2s, OutputPin enablePin, bool internalStack = true);

	Audio(StrongObjectPtr<I2S> i2s, bool internalStack = true);

	~Audio() override;

	void play(AudioGenerator* generator, std::unique_ptr<AudioSource> source);

	void play(std::span<std::pair<AudioGenerator*, std::unique_ptr<AudioSource>>> sources);

	void enqueue(AudioGenerator* generator, std::unique_ptr<AudioSource> source);

	void enqueue(std::span<std::pair<AudioGenerator*, std::unique_ptr<AudioSource>>> sources);

	void stop();

	void setEnabled(bool enabled);

	bool isEnabled() const;

	void setGain(float gain);

	bool isPlaying() const;

	/**
	 * Waits for current playback to finish, or ticks timeout elapses.
	 * @param ticks Timeout, in FreeRTOS ticks
	 * @return true - playback ended before timeout, false - timeouted
	 */
	bool waitEnd(TickType_t ticks);

	/**
	 * bool status - true - audio started playing, false - audio stopped playing
	 */
	DECLARE_EVENT(AudioStatus, Audio, bool);
	AudioStatus OnAudioStatusChanged{this};

protected:
	void tick(float deltaTime) noexcept override;

private:
	StrongObjectPtr<I2S> i2s;
	std::optional<OutputPin> enablePin = std::nullopt;
	AudioGenerator* currentGenerator;

	std::queue<GenSourcePair> sourceQueue;

	bool enabled = true;

	static constexpr size_t BufSize = 1024;
	std::array<int16_t, BufSize> dataBuf;

	std::atomic_bool playing = false;

	SemaphoreHandle_t tickSemaphore;

	SemaphoreHandle_t playSemaphore;

	float gain = 1.0f;

	void internalStop();
};

#endif //CMF_AUDIO_H
