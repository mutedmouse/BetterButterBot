#ifndef BUTTERBOT_FIRMWARE_AUDIOFRONTEND_H
#define BUTTERBOT_FIRMWARE_AUDIOFRONTEND_H

#include "Object/Object.h"
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/ringbuf.h>
#include <freertos/semphr.h>
#include <atomic>
#include <vector>
#include "esp_vad.h"
#include "esp_mn_iface.h"
#include "MicInput.h"
#include "Util/PSRAMAllocator.h"
#include "Thread/Threaded.h"
#include "Event/EventBroadcaster.h"
#include "esp_afe_sr_iface.h"
#include "esp_mn_iface.h"
#include "model_path.h"
#include "Services/LEDControl.h"

class AudioFrontend : public Object {
	GENERATED_BODY(AudioFrontend, Object, void);
public:
	AudioFrontend();

	~AudioFrontend() override;

	enum class Mode : uint8_t {
		Audio, Wake, Speech, VAD, Off
	};

	void setMode(Mode mode);

	static constexpr float DefaultFuzzyThreshold = 0.2f;

	struct Phrase {
		const char* string; //Phrase string (graphemes)
		const char* phonemes; //Phrase phonemes, generated using multinet_g2p.py[esp-sr/tool/multinet_g2p.py]
		const char* fuzzyCore = nullptr;
		float fuzzyThreshold = DefaultFuzzyThreshold;
		const void* action = nullptr;
	};

	struct PhraseResult {
		bool recognized;
		int index;
		std::string transcript;
		float confidence;
	};

	/**
	 * Clears previous phrases and replaces them with specified phrases.
	 */
	void setSpeechPhrases(std::span<Phrase> phrases);

	/**
	 * Blocks until a phrase is detected or the recognition window times out.
	 * Mode must be Off before calling. Sets mode to Speech internally and restores it to Off on return.
	 */
	PhraseResult waitForPhrase();

	void setOutputBuffer(RingbufHandle_t rb);

	/**
	 * Adjust voice activity detection parameters.
	 * Can only be called when AudioFrontend's Mode is set to Off
	 * @param vad_mode The value can be: VAD_MODE_0 to VAD_MODE_4, ranging from least to most aggressive detection rate
	 * @param min_speech_ms The minimum duration of speech in ms. It should be bigger than 32 ms, default: 128 ms
	 * @param min_noise_ms The minimum duration of noise or silence in ms. It should be bigger than 64 ms, default: 1000 ms
	 * @param vad_delay_ms The delay of the first speech frame in ms, default: 128 ms
	 */
	void setVADParams(vad_mode_t vad_mode, int min_speech_ms, int min_noise_ms, int vad_delay_ms);


	/**
	 * Mode::Audio writes processed mono audio to the output buffer and broadcasts this with 0 when the
	 * buffer is full. Mode::Audio also broadcasts onVAD on voice activity changes.
	 */
	DECLARE_EVENT(AudioEvent, AudioFrontend, size_t)
	AudioEvent onAudio = AudioEvent(this);

	/**
	 * bool recognized - true if a registered phrase (or abort phrase) was detected, false if timeout - also invalidates all values except transcript
	 * int index - index of detected phrase, from previously set phrases with setSpeechPhrases(), -1 for abort phrases
	 * std::string transcript - phonemic transcript of detected phrase
	 * float confidence - confidence of detection
	 */
	DECLARE_EVENT(PhraseEvent, AudioFrontend, bool, int, std::string, float)
	PhraseEvent onPhrase = PhraseEvent(this);

	/**
	 * bool active - true when voice activity is ongoing, false when no voice activity.
	 * Fires only on changes; the first fetch after entering Mode::VAD always fires
	 * to publish the initial state.
	 */
	DECLARE_EVENT(VADEvent, AudioFrontend, bool)
	VADEvent onVAD = VADEvent(this);

	/**
	 * @param withMultinet false loads only the AFE (AGC + VAD) for streaming audio out in Mode::Audio,
	 * which saves the MultiNet model's memory while Wi-Fi is up.
	 */
	void loadModel(bool withMultinet = true);
	void unloadModel();

private:
	void feedTaskFunc();

	std::unique_ptr<Threaded> feedTask;

	void procTaskFunc();

	std::unique_ptr<Threaded> procTask;

	/**
	 * Event group that coordinates whether the feed/proc tasks should be doing work.
	 * When RunBit is clear, each task blocks at the start of its loop on its respective gate
	 * until RunBit is set again. AtGate bits let setMode/the constructor wait until the tasks
	 * have actually reached the blocked state before mutating shared state.
	 */
	EventGroupHandle_t taskGate = nullptr;
	static constexpr EventBits_t RunBit = BIT0;
	static constexpr EventBits_t FeedAtGate = BIT1;
	static constexpr EventBits_t ProcAtGate = BIT2;

	void waitAtGate(EventBits_t atGateBit);

	void processAudio(afe_fetch_result_t* res);

	void processSpeech(afe_fetch_result_t* res);

	void processVAD(afe_fetch_result_t* res);

	// Last vad_state observed in Mode::VAD that was broadcast on onVAD.
	// -1 is a sentinel meaning "nothing fired yet"; used to force the first
	// fetch after entering VAD mode to publish the initial state.
	std::atomic<int8_t> lastVADState{ -1 };

	std::atomic<Mode> currentMode = Mode::Off;

	RingbufHandle_t outputBuffer = nullptr;

	bool pausedVoiceLink = false; // Wi-Fi released while MultiNet is loaded

	srmodel_list_t* models = nullptr;

	afe_config_t* afe_config = nullptr;
	const esp_afe_sr_iface_t* afe_handle = nullptr;
	esp_afe_sr_data_t* afe_data = nullptr;

	esp_mn_iface_t* multinet_handle = nullptr;
	model_iface_data_t* multinet_data = nullptr;

	//MultiNet speech recognition buffer size
	static constexpr int32_t MultinetDuration = 4000; //[ms]

	static constexpr float MultinetDetThreshold = 0.35f;

	/*
	 * FeedBuffer size is inferred from chunksize in runtime, since it can depend on available AFE features and config.
	 * This would change if the underlying AudioFrontend's configuration is modified.
	 */
	static constexpr size_t FeedBufferSize = 1024;
	PSRAMVector<int16_t> feedBuffer; //Buffer used by feedTask; sized to FeedBufferSize in ctor, backed by PSRAM

	SemaphoreHandle_t phraseSem = nullptr;
	PhraseResult phraseWaitResult{};
	std::atomic<bool> phraseWaiting{ false };

	std::string lastRawTranscript;

	std::vector<Phrase> speechPhrases;

	int fuzzyMatch(const std::string& transcript);

	void reportPhrase(bool recognized, int index, const std::string& transcript, float confidence);

	static constexpr int AbortAction{};

	static constexpr Phrase AbortPhrases[] = {
		{ "cancel", "KaNScL", "KaNScL", DefaultFuzzyThreshold, &AbortAction },
		{ "nevermind", "NfVkMiND", "NfVkMiND", DefaultFuzzyThreshold, &AbortAction }
	};
	static constexpr uint8_t AbortPhrasesNum = 2;
};


#endif //BUTTERBOT_FIRMWARE_AUDIOFRONTEND_H
