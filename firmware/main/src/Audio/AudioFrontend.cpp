#include "AudioFrontend.h"
#include <algorithm>
#include <cstring>
#include <limits>
#include <span>
#include <Util/ServiceLocator.h>
#include <Core/Application.h>
#include "Services/VoiceLink.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_models.h"
#include "esp_mn_speech_commands.h"

DEFINE_LOG(AudioFrontend)

namespace {
	constexpr float FuzzyMargin = 0.1f;
	constexpr float ShortWordWeight = 0.5f;
	constexpr size_t ShortWordMaxPhonemes = 4;
	constexpr float CoreWordWeight = 2.0f;
	constexpr float RepeatPhonemeCost = 0.1f;
	constexpr float RepeatSimilarityMax = 0.3f;
	constexpr float TailPhonemeCost = 0.3f;
	constexpr float HeadPhonemeCost = 0.3f;
	constexpr size_t EdgeDiscountMinPhrase = 6;
	constexpr size_t EdgeDiscountMinTranscript = 4;
	constexpr float NasalIntrusionCost = 0.3f;
	constexpr float CoreMatchMax = 0.4f;

	enum class PhonemeClass : uint8_t {
		Vowel, Plosive, Fricative, Affricate, Nasal, Glide, Other
	};

	PhonemeClass phonemeClass(char phoneme){
		switch(phoneme){
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'i':
			case 'k': case 'm': case 'n': case 'o': case 't': case 'u': case 'w':
				return PhonemeClass::Vowel;
			case 'P': case 'B': case 'T': case 'D': case 'K': case 'G':
				return PhonemeClass::Plosive;
			case 'F': case 'V': case 'S': case 'Z': case 's': case 'r': case 'v': case 'j': case 'h':
				return PhonemeClass::Fricative;
			case 'p': case 'q':
				return PhonemeClass::Affricate;
			case 'M': case 'N': case 'l':
				return PhonemeClass::Nasal;
			case 'L': case 'R': case 'W': case 'Y':
				return PhonemeClass::Glide;
			default:
				return PhonemeClass::Other;
		}
	}

	bool isPair(char a, char b, std::span<const char* const> pairs){
		for(const char* pair : pairs){
			if((a == pair[0] && b == pair[1]) || (a == pair[1] && b == pair[0])){
				return true;
			}
		}
		return false;
	}

	float indelCost(char phoneme){
		if(phoneme == 'c'){
			return 0.4f;
		}

		if(phoneme == 'L' || phoneme == 'R' || phoneme == 'h' || phoneme == 'j' || phoneme == 'v'){
			return 0.5f;
		}

		return 1.0f;
	}

	float substitutionCost(char a, char b, bool weakCap = true){
		if(a == b){
			return 0.0f;
		}

		constexpr const char* VoicingPairs[] = { "PB", "TD", "KG", "FV", "SZ", "sr", "vj", "pq" };
		constexpr const char* NearVowelPairs[] = { "gm", "fa", "cn", "cf", "en", "eb", "wo", "df", "kc", "fg", "ec", "cg", "ci", "cm", "ca" };
		constexpr const char* AccentPairs[] = { "vT", "vF", "vS", "jD", "jZ", "WV", "ps", "qY", "KT", "GD", "DB" };

		if(isPair(a, b, VoicingPairs) || isPair(a, b, NearVowelPairs)){
			return 0.3f;
		}

		if(isPair(a, b, AccentPairs)){
			return 0.4f;
		}

		float cost = 1.0f;
		const PhonemeClass classA = phonemeClass(a);
		const PhonemeClass classB = phonemeClass(b);
		if(classA == classB){
			cost = classA == PhonemeClass::Vowel ? 0.5f : 0.6f;
		}

		if(weakCap && cost > 0.5f && (indelCost(a) < 1.0f || indelCost(b) < 1.0f)){
			cost = 0.5f;
		}

		return cost;
	}

	float weightedIndelCost(char phoneme, float weight){
		const float cost = indelCost(phoneme);
		if(cost < 1.0f){
			return cost;
		}

		return cost * weight;
	}

	std::string stripSpaces(const char* phonemes){
		std::string out;
		out.reserve(strlen(phonemes));
		for(const char* c = phonemes; *c != '\0'; ++c){
			if(*c != ' '){
				out.push_back(*c);
			}
		}
		return out;
	}

	struct WeightedPhonemes {
		std::string chars;
		std::vector<float> weights;
	};

	bool isCoreWord(const char* word, size_t len, const char* core){
		if(core == nullptr){
			return false;
		}

		const char* c = core;
		while(*c != '\0'){
			while(*c == ' '){
				++c;
			}

			const char* start = c;
			while(*c != '\0' && *c != ' '){
				++c;
			}

			if((size_t)(c - start) == len && strncmp(start, word, len) == 0){
				return true;
			}
		}

		return false;
	}

	WeightedPhonemes weighPhonemes(const char* phonemes, const char* core){
		WeightedPhonemes out;
		const size_t len = strlen(phonemes);
		out.chars.reserve(len);
		out.weights.reserve(len);

		size_t i = 0;
		while(i < len){
			if(phonemes[i] == ' '){
				++i;
				continue;
			}

			size_t end = i;
			while(end < len && phonemes[end] != ' '){
				++end;
			}

			const size_t wordLen = end - i;
			float weight = 1.0f;
			if(isCoreWord(phonemes + i, wordLen, core)){
				weight = CoreWordWeight;
			}else if(wordLen <= ShortWordMaxPhonemes){
				weight = ShortWordWeight;
			}

			for(size_t j = i; j < end; ++j){
				out.chars.push_back(phonemes[j]);
				out.weights.push_back(weight);
			}

			i = end;
		}

		return out;
	}

	float phraseDistance(const std::string& transcript, const WeightedPhonemes& phrase){
		const size_t n = transcript.length();
		const size_t m = phrase.chars.length();

		std::vector<float> prev(m + 1);
		std::vector<float> curr(m + 1);

		const bool tailConsonant = m >= EdgeDiscountMinPhrase && phonemeClass(phrase.chars[m - 1]) != PhonemeClass::Vowel;
		const bool headConsonant = m >= EdgeDiscountMinPhrase && phonemeClass(phrase.chars[0]) != PhonemeClass::Vowel;

		prev[0] = 0.0f;
		for(size_t j = 1; j <= m; ++j){
			const bool phraseRepeat = j >= 2 && phrase.chars[j - 1] == phrase.chars[j - 2];
			const bool phraseTail = j == m && tailConsonant;
			const bool phraseHead = j == 1 && headConsonant;
			prev[j] = prev[j - 1] + (phraseRepeat ? RepeatPhonemeCost : (phraseTail ? TailPhonemeCost : (phraseHead ? HeadPhonemeCost : weightedIndelCost(phrase.chars[j - 1], phrase.weights[j - 1]))));
		}

		for(size_t i = 1; i <= n; ++i){
			const bool transcriptRepeat = i >= 2 && substitutionCost(transcript[i - 1], transcript[i - 2]) <= RepeatSimilarityMax;
			const bool transcriptHead = i == 1 && n >= EdgeDiscountMinTranscript;
			const bool transcriptTail = i == n && n >= EdgeDiscountMinTranscript;
			const bool nasalIntrusion = i < n && phonemeClass(transcript[i - 1]) == PhonemeClass::Nasal && phonemeClass(transcript[i]) == PhonemeClass::Plosive;

			curr[0] = prev[0] + (transcriptRepeat ? RepeatPhonemeCost : (transcriptHead ? std::min(indelCost(transcript[i - 1]), HeadPhonemeCost) : indelCost(transcript[i - 1])));
			for(size_t j = 1; j <= m; ++j){
				const float weight = phrase.weights[j - 1];
				const bool phraseRepeat = j >= 2 && phrase.chars[j - 1] == phrase.chars[j - 2];
				const bool phraseTail = j == m && tailConsonant;
				const bool phraseHead = j == 1 && headConsonant;

				float delCost = transcriptRepeat ? RepeatPhonemeCost : weightedIndelCost(transcript[i - 1], weight);
				if(nasalIntrusion){
					delCost = std::min(delCost, NasalIntrusionCost);
				}

				float subCost = substitutionCost(transcript[i - 1], phrase.chars[j - 1]) * weight;
				if(transcriptHead){
					delCost = std::min(delCost, HeadPhonemeCost);
					subCost = std::min(subCost, HeadPhonemeCost);
				}

				if(transcriptTail){
					delCost = std::min(delCost, TailPhonemeCost);
					subCost = std::min(subCost, TailPhonemeCost);
				}

				const float del = prev[j] + delCost;
				const float ins = curr[j - 1] + (phraseRepeat ? RepeatPhonemeCost : (phraseTail ? TailPhonemeCost : (phraseHead ? HeadPhonemeCost : weightedIndelCost(phrase.chars[j - 1], weight))));
				const float sub = prev[j - 1] + subCost;
				curr[j] = std::min(std::min(del, ins), sub);
			}
			std::swap(prev, curr);
		}

		return prev[m] / (float)std::max(n, m);
	}

	bool sameAction(const AudioFrontend::Phrase& a, const AudioFrontend::Phrase& b, size_t indexA, size_t indexB){
		if(a.action != nullptr && b.action != nullptr){
			return a.action == b.action;
		}
		return indexA == indexB;
	}

	float coreWordFit(const std::string& transcript, const char* word, size_t len){
		const size_t n = transcript.length();
		if(n == 0){
			return 1.0f;
		}

		std::vector<float> prev(n + 1, 0.0f);
		std::vector<float> curr(n + 1);

		for(size_t j = 1; j <= len; ++j){
			curr[0] = prev[0] + indelCost(word[j - 1]);
			for(size_t i = 1; i <= n; ++i){
				const bool transcriptRepeat = i >= 2 && substitutionCost(transcript[i - 1], transcript[i - 2]) <= RepeatSimilarityMax;
				const bool nasalIntrusion = i < n && phonemeClass(transcript[i - 1]) == PhonemeClass::Nasal && phonemeClass(transcript[i]) == PhonemeClass::Plosive;

				float extraCost = transcriptRepeat ? RepeatPhonemeCost : indelCost(transcript[i - 1]);
				if(nasalIntrusion){
					extraCost = std::min(extraCost, NasalIntrusionCost);
				}

				const float sub = prev[i - 1] + substitutionCost(word[j - 1], transcript[i - 1], false);
				const float skip = prev[i] + indelCost(word[j - 1]);
				const float extra = curr[i - 1] + extraCost;
				curr[i] = std::min(std::min(sub, skip), extra);
			}
			std::swap(prev, curr);
		}

		float best = prev[0];
		for(size_t i = 1; i <= n; ++i){
			best = std::min(best, prev[i]);
		}

		return best / (float)len;
	}

	float coreFit(const std::string& transcript, const char* core){
		if(core == nullptr){
			return 0.0f;
		}

		float worst = 0.0f;
		const char* c = core;
		while(*c != '\0'){
			while(*c == ' '){
				++c;
			}

			const char* start = c;
			while(*c != '\0' && *c != ' '){
				++c;
			}

			const size_t len = (size_t)(c - start);
			if(len == 0){
				continue;
			}

			worst = std::max(worst, coreWordFit(transcript, start, len));
		}

		return worst;
	}
}

AudioFrontend::AudioFrontend(){
	if(!ServiceLocator::MicInputInstance){
		CMF_LOG(AudioFrontend, LogLevel::Error, "MicInput is null!");
		abort();
	}

	feedBuffer.resize(FeedBufferSize); // PSRAM-backed; must exist before feedTask starts

	phraseSem = xSemaphoreCreateBinary();

	taskGate = xEventGroupCreate();
	// RunBit starts cleared so the tasks block at their gate as soon as they start.

	// Stacks in PSRAM: these tasks only read the mic and run models (no flash/NVS writes), and they exist
	// even while idle, so keeping 6 KB of internal RAM for them starves Wi-Fi.
	feedTask = std::make_unique<Threaded>([this](){ feedTaskFunc(); }, "AFE_feed", 1, 3 * 1024, 6, 1, false);
	procTask = std::make_unique<Threaded>([this](){ procTaskFunc(); }, "AFE_proc", 1, 3 * 1024, 5, 1, false);

	feedTask->start();
	procTask->start();

	// Wait for both tasks to reach their gate so any subsequent reconfiguration is safe.
	xEventGroupWaitBits(taskGate, FeedAtGate | ProcAtGate, pdFALSE, pdTRUE, portMAX_DELAY);
}

AudioFrontend::~AudioFrontend(){
	// Set RunBit so any task blocked at the gate wakes up before stop() takes effect;
	// without this, stop() would block waiting for a task that's parked on the event group.
	xEventGroupSetBits(taskGate, RunBit);

	feedTask->stop(portMAX_DELAY);
	procTask->stop(portMAX_DELAY);

	vSemaphoreDelete(phraseSem);

	if(taskGate){
		vEventGroupDelete(taskGate);
	}

	currentMode = Mode::Off;
	unloadModel();
}

void AudioFrontend::setMode(Mode mode){
	const Mode previousMode = currentMode.load();
	if(mode == previousMode) return;

	if(mode != Mode::Off && !afe_handle){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Model not loaded!");
		return;
	}

	if((previousMode == Mode::Speech || previousMode == Mode::Audio) && mode != Mode::Speech && mode != Mode::Audio){
		if(ServiceLocator::LEDControlInstance){
			ServiceLocator::LEDControlInstance->stopListen();
		}
	}

	switch(mode){
		case Mode::Audio:
			if(!outputBuffer){
				CMF_LOG(AudioFrontend, LogLevel::Warning, "No audio output buffer defined!");
			}

			if(ServiceLocator::LEDControlInstance){
				ServiceLocator::LEDControlInstance->startListen();
			}

			lastVADState.store(0); // only report the first speech onset, not the initial silence
			break;

		case Mode::Wake:
			CMF_LOG(AudioFrontend, LogLevel::Error, "Wake word mode is not supported: the wakeword partition and model have been removed");
			abort();

		case Mode::Speech:
			if(ServiceLocator::LEDControlInstance){
				ServiceLocator::LEDControlInstance->startListen();
			}

			lastRawTranscript.clear();
			break;

		case Mode::VAD:
			// Force the first fetch in VAD mode to broadcast the current state.
			lastVADState.store(-1);
			break;

		case Mode::Off:
			// Park the tasks at their gate. Tasks observe the cleared RunBit at the start of
			// their next loop iteration; once both AtGate bits are set, no work is in flight.
			xEventGroupClearBits(taskGate, RunBit);
			xEventGroupWaitBits(taskGate, FeedAtGate | ProcAtGate, pdFALSE, pdTRUE, portMAX_DELAY);
			break;
	}

	// Publish the new mode before releasing the gate so the tasks observe it immediately.
	currentMode = mode;

	if(mode != Mode::Off && previousMode == Mode::Off){
		xEventGroupSetBits(taskGate, RunBit);
	}
}

void AudioFrontend::setSpeechPhrases(std::span<Phrase> phrases){
	if(currentMode == Mode::Speech){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Cannot set speech phrases while speech detection is running!");
		return;
	}

	if(!multinet_handle){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Model not loaded!");
		return;
	}


	if(phrases.empty()){
		CMF_LOG(AudioFrontend, LogLevel::Warning, "Got empty phrases list, adding only abort phrases");
	}

	// speechPhrases mirrors MultiNet registration order: aborts first, then caller phrases.
	speechPhrases.assign(AbortPhrases, AbortPhrases + AbortPhrasesNum);
	speechPhrases.insert(speechPhrases.end(), phrases.begin(), phrases.end());

	ESP_ERROR_CHECK(esp_mn_commands_clear());

	for(uint8_t i = 1; i <= AbortPhrasesNum; ++i){
		ESP_ERROR_CHECK(esp_mn_commands_phoneme_add(i, AbortPhrases[i-1].string, AbortPhrases[i-1].phonemes));
	}

	for(size_t i = 1; i <= phrases.size(); ++i){
		ESP_ERROR_CHECK(esp_mn_commands_phoneme_add((int)i + AbortPhrasesNum, phrases[i-1].string, phrases[i-1].phonemes));

		// Periodically yield so this loop doesn't saturate the flash bus
		if((i % 8) == 0){
			vTaskDelay(1);
		}
	}
	auto err = esp_mn_commands_update();

	if(err != nullptr){
		CMF_LOG(AudioFrontend, Error, "%d phrases with errors", err->num);
	}
}

AudioFrontend::PhraseResult AudioFrontend::waitForPhrase(){
	phraseWaiting.store(true);
	setMode(Mode::Speech);
	xSemaphoreTake(phraseSem, portMAX_DELAY);
	setMode(Mode::Off);
	phraseWaiting.store(false);
	return phraseWaitResult;
}

void AudioFrontend::setOutputBuffer(RingbufHandle_t rb){
	if(currentMode == Mode::Audio){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Cannot set output buffer while audio mode is active!");
		return;
	}

	outputBuffer = rb;
}

void AudioFrontend::setVADParams(vad_mode_t vad_mode, int min_speech_ms, int min_noise_ms, int vad_delay_ms){
	if(!afe_handle || !afe_config){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Model not loaded!");
		return;
	}

	if(currentMode != Mode::Off){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Cannot set VAD params while AFE is running!");
		return;
	}

	afe_handle->destroy(afe_data);

	afe_config->vad_mode = vad_mode;
	afe_config->vad_min_speech_ms = min_speech_ms;
	afe_config->vad_min_noise_ms = min_noise_ms;
	afe_config->vad_delay_ms = vad_delay_ms;

	afe_handle = esp_afe_handle_from_config(afe_config);
	afe_data = afe_handle->create_from_config(afe_config);
}

void AudioFrontend::waitAtGate(EventBits_t atGateBit){
	if((xEventGroupGetBits(taskGate) & RunBit) != 0) return;

	xEventGroupSetBits(taskGate, atGateBit);
	xEventGroupWaitBits(taskGate, RunBit, pdFALSE, pdTRUE, portMAX_DELAY);
	xEventGroupClearBits(taskGate, atGateBit);
}

void AudioFrontend::loadModel(bool withMultinet){
	if(afe_handle){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Model already loaded!");
		return;
	}

	if(currentMode != Mode::Off){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Cannot load model while AFE is running!");
		return;
	}

	uint32_t startTime = millis();

	// AFE + MultiNet need the internal RAM the Wi-Fi stack holds (they ran it down to ~0 and
	// crashed when both were up), so on-device recognition borrows it for as long as it's loaded.
	if(withMultinet){
		if(VoiceLink* voiceLink = Application::getApp()->getService<VoiceLink>()){
			voiceLink->pause();
			pausedVoiceLink = true;
		}
	}

	models = esp_srmodel_init("model"); // partition label defined in partitions.csv

	afe_config = afe_config_init("MM", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
	afe_config->wakenet_init = false;
	afe_config->afe_ringbuf_size = 50;
	afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

	//Acoustic echo cancellation
	afe_config->aec_init = false;

	//Voice activity detection
	afe_config->vad_init = true;
	afe_config->vad_mode = VAD_MODE_0;

	afe_config->vad_min_speech_ms = 64;
	afe_config->vad_delay_ms = 384;
	if(!withMultinet){
		// Streaming ends the utterance on VAD silence, so don't wait the default 1 s to call it.
		afe_config->vad_min_noise_ms = 600;
	}

	//Speech enhancement
	afe_config->se_init = false;

	//Noise suppression, turned off since:
	//W (978) AFE_CONFIG: Noise Supression may reduce the accuracy of speech recognition. It is not recommended to turn it on.
	afe_config->ns_init = false;

	//Automatic gain control
	afe_config->agc_init = true;
	afe_config->agc_mode = AFE_AGC_MODE_WEBRTC; //Can't be MODE_WAKENET since it's not used
	afe_config->agc_compression_gain_db = 9; //Default
	afe_config->agc_target_level_dbfs = -3; //Default

	afe_handle = esp_afe_handle_from_config(afe_config);
	afe_data = afe_handle->create_from_config(afe_config);

	if(!withMultinet){
		CMF_LOG(AudioFrontend, LogLevel::Debug, "AFE-only time: %d ms", (int)millis() - (int)startTime);
		return;
	}

	//Multinet (speech recognition)
	char* mn_name = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);
	CMF_LOG(AudioFrontend, LogLevel::Info, "multinet:%s", mn_name);
	multinet_handle = esp_mn_handle_from_name(mn_name);

	multinet_data = multinet_handle->create(mn_name, MultinetDuration);
	multinet_handle->set_det_threshold(multinet_data, MultinetDetThreshold);

	ESP_ERROR_CHECK(esp_mn_commands_alloc(multinet_handle, multinet_data));
	ESP_ERROR_CHECK(esp_mn_commands_clear());
	auto err = esp_mn_commands_update();
	if(err != nullptr){
		CMF_LOG(AudioFrontend, Error, "%d phrases with errors", err->num);
	}

	CMF_LOG(AudioFrontend, LogLevel::Debug, "AFE time: %d ms", (int)millis() - (int)startTime);

	const size_t feedBufferLength = (size_t)afe_handle->get_feed_chunksize(afe_data) * afe_handle->get_channel_num(afe_data);
	CMF_LOG(AudioFrontend, LogLevel::Debug, "feedBuffer size: %zu (FeedBufferSize: %zu)", feedBufferLength, FeedBufferSize);
	if(feedBufferLength != FeedBufferSize){
		CMF_LOG(AudioFrontend, LogLevel::Warning, "AFE feed chunk size mismatch: expected %zu, got %zu", FeedBufferSize, feedBufferLength);
	}
}

void AudioFrontend::unloadModel(){
	if(!afe_handle){
		return;
	}

	if(currentMode != Mode::Off){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Cannot unload model while AFE is running!");
		return;
	}

	uint32_t startTime = millis();

	if(multinet_handle && multinet_data){
		ESP_ERROR_CHECK(esp_mn_commands_free());
		multinet_handle->destroy(multinet_data);
		multinet_handle = nullptr;
		multinet_data = nullptr;
	}

	if(afe_handle && afe_data){
		afe_handle->destroy(afe_data);
		afe_handle = nullptr;
		afe_data = nullptr;
	}

	if(afe_config){
		afe_config_free(afe_config);
		afe_config = nullptr;
	}

	if(models){
		esp_srmodel_deinit(models);
		models = nullptr;
	}

	if(pausedVoiceLink){
		pausedVoiceLink = false;
		if(VoiceLink* voiceLink = Application::getApp()->getService<VoiceLink>()){
			voiceLink->resume();
		}
	}

	CMF_LOG(AudioFrontend, LogLevel::Debug, "unload time: %d ms", (int)millis() - (int)startTime);
}

void AudioFrontend::feedTaskFunc(){
	waitAtGate(FeedAtGate);

	if(!afe_handle){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Model not loaded in feed task");
		return;
	}

	auto ret = ServiceLocator::MicInputInstance->getStereo({ feedBuffer });

	if(ret != feedBuffer.size()){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Error reading from microphone");
		return;
	}

	afe_handle->feed(afe_data, feedBuffer.data());
}

void AudioFrontend::procTaskFunc(){
	waitAtGate(ProcAtGate);

	if(!afe_handle){
		CMF_LOG(AudioFrontend, LogLevel::Error, "Model not loaded in proc task");
		return;
	}

	afe_fetch_result_t* res = afe_handle->fetch(afe_data);

	if(!res || res->ret_value == ESP_FAIL){
		CMF_LOG(AudioFrontend, LogLevel::Error, "AFE fetch error");
		return;
	}

	switch(currentMode){
		case Mode::Audio:
			processAudio(res);
			break;

		case Mode::Wake:
			CMF_LOG(AudioFrontend, LogLevel::Error, "Wake word mode is not supported: the wakeword partition and model have been removed");
			abort();

		case Mode::Speech:
			processSpeech(res);
			break;

		case Mode::VAD:
			processVAD(res);
			break;

		case Mode::Off:
			CMF_LOG(AudioFrontend, LogLevel::Warning, "Processing task running while mode is Off");
			break;
		default:
			break;
	}
}

void AudioFrontend::processAudio(afe_fetch_result_t* res){
	if(!outputBuffer){
		CMF_LOG(AudioFrontend, LogLevel::Error, "No output buffer for Audio samples");
		return;
	}

	// Processed (AGC'd) mono output; the stream is continuous, so the VAD cache is not needed.
	const size_t freeSize = xRingbufferGetCurFreeSize(outputBuffer);
	const size_t toSend = std::min((size_t) res->data_size, freeSize);

	if(toSend == 0 || xRingbufferSend(outputBuffer, (void*)res->data, toSend, 0) != pdTRUE){
		CMF_LOG(AudioFrontend, LogLevel::Warning, "No space in output buffer");
		onAudio.broadcast(0);
	}

	const int8_t current = (res->vad_state == VAD_SPEECH) ? 1 : 0;
	if(lastVADState.exchange(current) != current){
		onVAD.broadcast(current == 1);
	}
}

void AudioFrontend::processSpeech(afe_fetch_result_t* res){
	if(res->vad_cache_size > 0 && res->data_size > 0){
		int16_t* cache = res->vad_cache;
		int remaining = res->vad_cache_size;
		while(remaining >= res->data_size){
			multinet_handle->detect(multinet_data, cache);
			cache += res->data_size / (int)sizeof(int16_t);
			remaining -= res->data_size;
		}
	}

	esp_mn_state_t mn_state = multinet_handle->detect(multinet_data, res->data);

	if(mn_state == ESP_MN_STATE_DETECTING){
		esp_mn_results_t* partial = multinet_handle->get_results(multinet_data);
		if(partial != nullptr && partial->raw_string[0] != '\0' && lastRawTranscript != partial->raw_string){
			lastRawTranscript = partial->raw_string;
		}
		return;
	}

	esp_mn_results_t* mn_result = multinet_handle->get_results(multinet_data);
	if(mn_state == ESP_MN_STATE_DETECTED){
		for(int i = 0; i < mn_result->num; i++){
			CMF_LOG(AudioFrontend, LogLevel::Info, "TOP %d, command_id: %d, phrase_id: %d, string: %s, prob: %f",
			        i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->string, mn_result->prob[i]);
		}

		const auto commandId = mn_result->command_id[0];
		const std::string transcript = mn_result->string;
		const float confidence = mn_result->prob[0];

		if(commandId <= AbortPhrasesNum){
			reportPhrase(true, -1, transcript, confidence);
		} else{
			reportPhrase(true, commandId - AbortPhrasesNum - 1, transcript, confidence);
		}

		multinet_handle->clean(multinet_data);
	}

	if(mn_state == ESP_MN_STATE_TIMEOUT){
		if(lastRawTranscript.empty()){
			lastRawTranscript = mn_result->raw_string;
		}
		CMF_LOG(AudioFrontend, LogLevel::Info, "timeout, broadcasting transcript: '%s', len: %d", lastRawTranscript.c_str(), (int)lastRawTranscript.length());

		const int fuzzyIndex = fuzzyMatch(lastRawTranscript);
		if(fuzzyIndex < 0){
			reportPhrase(false, -1, lastRawTranscript, 0.0f);
		}else if(fuzzyIndex < AbortPhrasesNum){
			reportPhrase(true, -1, lastRawTranscript, 0.0f);
		}else{
			reportPhrase(true, fuzzyIndex - AbortPhrasesNum, lastRawTranscript, 0.0f);
		}

		multinet_handle->clean(multinet_data);
	}
}

void AudioFrontend::reportPhrase(bool recognized, int index, const std::string& transcript, float confidence){
	onPhrase.broadcast(recognized, index, transcript, confidence);
	if(phraseWaiting.load()){
		phraseWaitResult = { recognized, index, transcript, confidence };
		xSemaphoreGive(phraseSem);
	}
}

int AudioFrontend::fuzzyMatch(const std::string& transcript){
	const std::string stripped = stripSpaces(transcript.c_str());
	if(stripped.empty() || speechPhrases.empty()){
		return -1;
	}

	std::vector<float> scores;
	std::vector<float> coreFits;
	scores.reserve(speechPhrases.size());
	coreFits.reserve(speechPhrases.size());
	for(const Phrase& phrase : speechPhrases){
		scores.push_back(phraseDistance(stripped, weighPhonemes(phrase.phonemes, phrase.fuzzyCore)));
		coreFits.push_back(coreFit(stripped, phrase.fuzzyCore));
	}

	size_t rawBest = 0;
	int best = -1;
	for(size_t i = 0; i < scores.size(); ++i){
		if(scores[i] < scores[rawBest]){
			rawBest = i;
		}

		if(coreFits[i] <= CoreMatchMax && scores[i] < speechPhrases[i].fuzzyThreshold && (best < 0 || scores[i] < scores[best])){
			best = (int)i;
		}
	}

	if(best < 0){
		CMF_LOG(AudioFrontend, LogLevel::Info, "Fuzzy match rejected: transcript '%s', best '%s' [%s], core '%s', score %.3f, threshold %.3f, core fit %.3f (max %.3f)",
			transcript.c_str(), speechPhrases[rawBest].string, speechPhrases[rawBest].phonemes,
			speechPhrases[rawBest].fuzzyCore ? speechPhrases[rawBest].fuzzyCore : "",
			scores[rawBest], speechPhrases[rawBest].fuzzyThreshold, coreFits[rawBest], CoreMatchMax);
		return -1;
	}

	float bestOtherAction = std::numeric_limits<float>::max();
	for(size_t i = 0; i < scores.size(); ++i){
		if(sameAction(speechPhrases[i], speechPhrases[(size_t)best], i, (size_t)best)){
			continue;
		}

		if(coreFits[i] > CoreMatchMax || scores[i] >= speechPhrases[i].fuzzyThreshold){
			continue;
		}

		if(scores[i] < bestOtherAction){
			bestOtherAction = scores[i];
		}
	}

	if(bestOtherAction - scores[best] < FuzzyMargin){
		CMF_LOG(AudioFrontend, LogLevel::Info, "Fuzzy match ambiguous: best '%s' score %.3f, competing action score %.3f",
			speechPhrases[best].string, scores[best], bestOtherAction);
	}

	const float bestThreshold = speechPhrases[best].fuzzyThreshold;
	if(scores[best] >= bestThreshold){
		CMF_LOG(AudioFrontend, LogLevel::Info, "Fuzzy match rejected: transcript '%s', best '%s' [%s], core '%s', score %.3f above phrase threshold %.3f",
			transcript.c_str(), speechPhrases[best].string, speechPhrases[best].phonemes,
			speechPhrases[best].fuzzyCore ? speechPhrases[best].fuzzyCore : "",
			scores[best], bestThreshold);
		return -1;
	}

	CMF_LOG(AudioFrontend, LogLevel::Info, "Fuzzy matched: transcript '%s', phrase '%s' [%s], core '%s', score %.3f, threshold %.3f, core fit %.3f (max %.3f)",
		transcript.c_str(), speechPhrases[best].string, speechPhrases[best].phonemes,
		speechPhrases[best].fuzzyCore ? speechPhrases[best].fuzzyCore : "",
		scores[best], bestThreshold, coreFits[best], CoreMatchMax);
	return best;
}

void AudioFrontend::processVAD(afe_fetch_result_t* res){
	const int8_t current = (res->vad_state == VAD_SPEECH) ? 1 : 0;
	if(lastVADState.exchange(current) == current) return;

	onVAD.broadcast(current == 1);
}
