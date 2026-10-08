#ifndef BUTTERBOT_FIRMWARE_SPEECHGEN_H
#define BUTTERBOT_FIRMWARE_SPEECHGEN_H

#include <memory>
#include <span>
#include "flite.h"
#include "freertos/ringbuf.h"
#include "Thread/Threaded.h"

/**
 * Flite thread, koji vrti TTS
 * callback puni ringbuffer/queue
 * generate vuče iz buffera/queuea, šalje dalje, odblokirava flite thread (ako je napunjen buff)
 *
 * ako je currently running TTS thread, svaki novi parse poziv prazni buffer i vraća CST_AUDIO_STREAM_STOP
 *
 *
 */
class SpeechGen {
public:
	SpeechGen();
	virtual ~SpeechGen();
	/**
	 * Retrieves speech synthesis audio.
	 * @param output std::span to be filled with speech synthesis data
	 * @param last output parameter, true = last output of current synthesis reached, false = current synthesis still ongoing
	 * @param timeout maximum time to wait for data, 0 for non-blocking call
	 * @return size of retrieved data, in samples; zero if synthesis pending or not started
	 */
	int generate(std::span<int16_t> output, bool& last, TickType_t timeout = 0);

	/**
	 * Initializes speech synthesis with given text.
	 * Aborts current synthesis and empties the buffer if synthesis is currently running.
	 * @param text US English text to be synthesized
	 */
	void parseText(std::string text);

	/**
	 * Initializes speech synthesis with given phonemes.
	 * Aborts current synthesis and empties the buffer if synthesis is currently running.
	 * @param phonemes Phonemes written in ESP-SR's Multinet format (best obtained directly from AudioFrontend's speech mode)
	 */
	void parsePhonemes(const std::string& phonemes);

	/**
	 * Initializes speech synthesis with given SSML string (https://en.wikipedia.org/wiki/Speech_Synthesis_Markup_Language)
	 * Aborts current synthesis and empties the buffer if synthesis is currently running.
	 * @param ssml SSML string
	 */
	void parseSSML(std::string ssml);

	/**
	 * Cancels ongoing synthesis and purges buffers.
	 */
	void cancel();

	enum class InputType : uint8_t {
		Text, Phonemes, SSML
	};

private:
	static constexpr size_t BufferSize = 4 * 1024;
	static constexpr size_t FliteMinBufSize = 512;

	std::unique_ptr<Threaded> fliteThread;

	void fliteThreadFunc();

	int fliteCallback(const cst_wave* w, int start, int size, int last, cst_audio_streaming_info* asi);

	SemaphoreHandle_t startSem;
	SemaphoreHandle_t endSem;
	cst_audio_streaming_info* asi;
	cst_voice* voice;

	RingbufHandle_t ringbuf;

	std::string fliteInput;

	InputType fliteInputType = InputType::Text;

	//Aborts flite's synthesis stream (flite_XXX_to_speech function calls)
	std::atomic_bool abort = false;
	std::atomic_bool synthRunning = false;

	void checkAndAbort();

	[[maybe_unused]] static std::string flitePhoneToCMU(const cst_item* seg);
	static std::string CMUToFlitePhone(const std::string& cmu);
	static std::string multinetToCMU(const std::string& input);
};


#endif //BUTTERBOT_FIRMWARE_SPEECHGEN_H
