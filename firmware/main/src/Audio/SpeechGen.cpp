#include "SpeechGen.h"
#include <utility>
#include "Memory/ObjectMemory.h"
#include "Util/stdafx.h"

DEFINE_LOG(SpeechGen)

extern "C" {
cst_voice* register_cmu_us_kal16(const char* voxdir);
void unregister_cmu_us_kal16(cst_voice* voice);
}

SpeechGen::SpeechGen(){
	flite_init();

	voice = register_cmu_us_kal16(nullptr);

	asi = cst_alloc(struct cst_audio_streaming_info_struct, 1);
	*asi = {
		.min_buffsize = FliteMinBufSize,
		.asc = [](const cst_wave* w, int start, int size, int last, cst_audio_streaming_info* asi) -> int {
			auto speechGen = (SpeechGen*)asi->userdata;
			return speechGen->fliteCallback(w, start, size, last, asi);
		},
		.userdata = this
	};

	feat_set(voice->features, "streaming_info", audio_streaming_info_val(asi));

	ringbuf = xRingbufferCreateWithCaps(BufferSize, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

	startSem = xSemaphoreCreateBinary();
	endSem = xSemaphoreCreateBinary();

	fliteThread = std::make_unique<Threaded>([this]() {
		fliteThreadFunc();
	}, "flite", 0, 3 * 1024, 7, 1, false);

	fliteThread->start();
}

SpeechGen::~SpeechGen(){
	checkAndAbort();
	fliteThread->stop(0);
	abort = true;
	xSemaphoreGive(startSem);
	while(fliteThread->running()){
		delayMillis(1);
	}

	delete fliteThread.get();
	vRingbufferDeleteWithCaps(ringbuf);
	vSemaphoreDelete(startSem);
	vSemaphoreDelete(endSem);
	unregister_cmu_us_kal16(voice);
}

int SpeechGen::generate(std::span<int16_t> output, bool& last, TickType_t timeout){
	// Use a non-blocking receive once synthesis is done to avoid deadlock on an
	// empty ringbuffer (nothing will ever write to it again at that point).
	const TickType_t actualTimeout = synthRunning ? timeout : 0;
	size_t size;
	auto data = (int16_t*)xRingbufferReceiveUpTo(ringbuf, &size, actualTimeout, output.size() * sizeof(int16_t));

	if(data == nullptr){
		CMF_LOG(SpeechGen, LogLevel::Debug, "Ringbuffer data is nullptr");
		last = !synthRunning;
		return 0;
	}

	CMF_LOG(SpeechGen, LogLevel::Debug, "Generate copying %zu bytes", size);

	memcpy(output.data(), data, size);
	vRingbufferReturnItem(ringbuf, data);

	last = false;

	return (int)size / 2;
}

void SpeechGen::parseText(std::string text){
	checkAndAbort();
	fliteInput = std::move(text);
	fliteInputType = InputType::Text;
	synthRunning = true;
	xSemaphoreGive(startSem);
}

void SpeechGen::parsePhonemes(const std::string& phonemes){
	checkAndAbort();
	fliteInput = CMUToFlitePhone(multinetToCMU(phonemes));
	fliteInputType = InputType::Phonemes;
	synthRunning = true;
	xSemaphoreGive(startSem);
}

void SpeechGen::parseSSML(std::string ssml){
	checkAndAbort();
	fliteInput = std::move(ssml);
	fliteInputType = InputType::SSML;
	synthRunning = true;
	xSemaphoreGive(startSem);
}

void SpeechGen::cancel(){
	checkAndAbort();
}

void SpeechGen::fliteThreadFunc(){
	xSemaphoreTake(startSem, portMAX_DELAY);

	if(abort || fliteInput.empty()){
		synthRunning = false;
		xSemaphoreGive(endSem);
		return;
	}

	CMF_LOG(SpeechGen, LogLevel::Debug, "flite start synth");

	switch(fliteInputType){
		case InputType::Text:
			flite_text_to_speech(fliteInput.c_str(), voice, "stream");
			break;
		case InputType::Phonemes:
			flite_phones_to_speech(fliteInput.c_str(), voice, "stream");
			break;
		case InputType::SSML:
			flite_ssml_text_to_speech(fliteInput.c_str(), voice, "stream");
			break;
	}

	CMF_LOG(SpeechGen, LogLevel::Debug, "flite synth done");
	synthRunning = false;
	xSemaphoreGive(endSem);
}

int SpeechGen::fliteCallback(const cst_wave* w, int start, int size, int last, cst_audio_streaming_info* asi){
	if(abort){
		CMF_LOG(SpeechGen, LogLevel::Debug, "fliteCallback abort");
		return CST_AUDIO_STREAM_STOP;
	}

	while(xRingbufferSend(ringbuf, &w->samples[start], size * sizeof(uint16_t), portMAX_DELAY) != pdTRUE);

	CMF_LOG(SpeechGen, LogLevel::Debug, "write to ringbuffer %d bytes", size * sizeof(uint16_t));

	return CST_AUDIO_STREAM_CONT;
}

void SpeechGen::checkAndAbort(){
	if(!synthRunning){
		//always take endSem just in case of double-give overflowing the underlying freertos queue
		xSemaphoreTake(endSem, 0);
		//flush any samples left over from a completed (non-aborted) synthesis
		size_t sz;
		void* p;
		while((p = xRingbufferReceiveUpTo(ringbuf, &sz, 0, BufferSize)) != nullptr){
			vRingbufferReturnItem(ringbuf, p);
		}
		CMF_LOG(SpeechGen, LogLevel::Debug, "check&abort !synthRunning");
		return;
	}

	//If semaphore is taken that means speech synthesis is running, needs to be aborted and buffer cleaned.
	CMF_LOG(SpeechGen, LogLevel::Debug, "Aborting previously ongoing synthesis");
	abort = true;

	//Drain the ringbuffer until flite acknowledges the abort
	size_t size;
	void* p;
	while(xSemaphoreTake(endSem, pdMS_TO_TICKS(10)) != pdTRUE){
		while((p = xRingbufferReceiveUpTo(ringbuf, &size, 0, BufferSize)) != nullptr){
			vRingbufferReturnItem(ringbuf, p);
		}
	}

	//consume leftover content in buffer
	while((p = xRingbufferReceiveUpTo(ringbuf, &size, 0, BufferSize)) != nullptr){
		vRingbufferReturnItem(ringbuf, p);
	}
	CMF_LOG(SpeechGen, LogLevel::Debug, "Purged leftover data in buffer. Remaining %d", xRingbufferGetCurFreeSize(ringbuf));
	//at this point synthesis is stopped and buffer is empty

	abort = false;
}


std::string SpeechGen::flitePhoneToCMU(const cst_item* seg){
	const char* ph_c = item_feat_string(seg, "name");
	if(!ph_c) return "";

	std::string ph = ph_c;

	// Find the syllable this segment belongs to (for stress)
	const cst_item* syl = path_to_item(seg, "R:SylStructure.parent");
	int stress = syl ? item_feat_int(syl, "stress") : 0;

	// Reduced vowels map directly to *0
	static const std::unordered_map<std::string, std::string> reducedVowels = {
		{ "ax", "AH0" },
		{ "ix", "IH0" },
		{ "ux", "UH0" }
	};

	auto it = reducedVowels.find(ph);
	if(it != reducedVowels.end())
		return it->second;

	// Full vowels with stress digits
	static const std::unordered_map<std::string, std::string> stressedVowels = {
		{ "ah", "AH" }, { "aa", "AA" }, { "ae", "AE" }, { "ao", "AO" },
		{ "eh", "EH" }, { "er", "ER" }, { "ey", "EY" }, { "ih", "IH" },
		{ "iy", "IY" }, { "ow", "OW" }, { "oy", "OY" }, { "uh", "UH" },
		{ "uw", "UW" }, { "aw", "AW" }, { "ay", "AY" }
	};

	auto vit = stressedVowels.find(ph);
	if(vit != stressedVowels.end())
		return vit->second + std::to_string(stress);

	// Consonants: 1:1 to uppercase ARPAbet
	std::transform(ph.begin(), ph.end(), ph.begin(), ::toupper);
	return ph;
}

std::string SpeechGen::CMUToFlitePhone(const std::string& cmu){
	if(cmu.empty()) return "";

	// Separate base symbol from final stress digit (if present)
	std::string base = cmu;
	int stress = -1;

	if(std::isdigit(cmu.back())){
		stress = cmu.back() - '0';
		base = cmu.substr(0, cmu.size() - 1);
	}

	// Reduced vowels
	static const std::unordered_map<std::string, std::string> reducedMap = {
		{ "AH", "ax" }, // AH0 → ax
		{ "IH", "ix" }, // IH0 → ix
		{ "UH", "ux" } // UH0 → ux
	};

	if(stress == 0){
		auto r = reducedMap.find(base);
		if(r != reducedMap.end())
			return "pau " + r->second + " pau";
	}

	// Normal vowels (AH1/AH2 → ah, etc.)
	static const std::unordered_map<std::string, std::string> vowelMap = {
		{ "AA", "aa" }, { "AE", "ae" }, { "AH", "ah" }, { "AO", "ao" },
		{ "AW", "aw" }, { "AY", "ay" }, { "EH", "eh" }, { "ER", "er" },
		{ "EY", "ey" }, { "IH", "ih" }, { "IY", "iy" }, { "OW", "ow" },
		{ "OY", "oy" }, { "UH", "uh" }, { "UW", "uw" }
	};

	auto v = vowelMap.find(base);
	if(v != vowelMap.end())
		return "pau " + v->second + " pau";

	// Consonants: lowercase, no other changes
	std::string lower;
	lower.reserve(base.size());
	for(char c : base)
		lower.push_back(std::tolower(static_cast<unsigned char>(c)));

	return "pau " + lower + " pau";
}

std::string SpeechGen::multinetToCMU(const std::string& input){
	// Inverted mapping: single symbol → representative CMU phoneme
	static const std::unordered_map<char, std::string> invertedAlphabet = {
		{ 'a', "AE0" },
		{ 'b', "OW0" },
		{ 'c', "AH0" },
		{ 'd', "EY0" },
		{ 'e', "AO0" },
		{ 'f', "EH0" },
		{ 'g', "IH0" },
		{ 'h', "HH" },
		{ 'i', "AY0" },
		{ 'j', "DH" },
		{ 'k', "ER0" },
		{ 'l', "NG" },
		{ 'm', "IY0" },
		{ 'n', "AA0" },
		{ 'o', "UW0" },
		{ 'p', "CH" },
		{ 'q', "JH" },
		{ 'r', "ZH" },
		{ 's', "SH" },
		{ 't', "AW0" },
		{ 'u', "OY0" },
		{ 'v', "TH" },
		{ 'w', "UH0" },
		// Direct single-letter CMU phonemes
		{ 'N', "N" },
		{ 'V', "V" },
		{ 'L', "L" },
		{ 'F', "F" },
		{ 'S', "S" },
		{ 'B', "B" },
		{ 'R', "R" },
		{ 'D', "D" },
		{ 'G', "G" },
		{ 'K', "K" },
		{ 'W', "W" },
		{ 'T', "T" },
		{ 'M', "M" },
		{ 'Z', "Z" },
		{ 'P', "P" },
		{ 'Y', "Y" },
		{ ' ', " " }
	};


	std::string result;
	result.reserve(input.size() * 4); // rough optimization

	for(char c : input){
		auto it = invertedAlphabet.find(c);
		if(it != invertedAlphabet.end()){
			if(!result.empty() && result.back() != ' ')
				result += ' ';
			result += it->second;
		} else{
			// Unknown symbol — skip or mark
			if(!result.empty() && result.back() != ' ')
				result += ' ';
			result += "<UNK>";
		}
	}

	return result;
}
