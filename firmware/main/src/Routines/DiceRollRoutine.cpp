#include "DiceRollRoutine.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <span>
#include <freertos/FreeRTOS.h>
#include <esp_random.h>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Audio/AACAudioGenerator.h"
#include "Services/Audio/FileAudioSource.h"
#include "Services/Com.h"

DEFINE_LOG(DiceRollRoutine)

struct CountEntry {
	AudioFrontend::Phrase phrase;
	uint8_t value;
};

static constexpr CountEntry CountTable[] = {
	{ { "one", "WcN", "WcN" }, 1 },
	{ { "two", "To", "To" }, 2 },
	{ { "three", "vRm", "vRm" }, 3 },
	{ { "four", "FeR", "FeR" }, 4 },
	{ { "five", "FiV", "FiV" }, 5 },
	{ { "six", "SgKS", "SgKS" }, 6 },
	{ { "seven", "SfVcN", "SfVcN" }, 7 },
	{ { "eight", "dT", "dT" }, 8 },
	{ { "nine", "NiN", "NiN" }, 9 },
	{ { "ten", "TfN", "TfN", 0.34f }, 10 },
	{ { "eleven", "gLfVcN", "gLfVcN" }, 11 },
	{ { "twelve", "TWfLV", "TWfLV" }, 12 },
	{ { "thirteen", "vkTmN", "vkTmN" }, 13 },
	{ { "one hundred", "WcN hcNDRcD", "WcN hcNDRcD" }, 100 },
};

struct TypeEntry {
	AudioFrontend::Phrase phrase;
	DiceRollData::DiceType type;
};

static constexpr TypeEntry TypeTable[] = {
	// "d N" forms
	{ { "d four", "Dm FeR", "FeR" }, DiceRollData::DiceType::D4 },
	{ { "d six", "Dm SgKS", "SgKS" }, DiceRollData::DiceType::D6 },
	{ { "d eight", "Dm dT", "dT" }, DiceRollData::DiceType::D8 },
	{ { "d ten", "Dm TfN", "TfN" }, DiceRollData::DiceType::D10 },
	{ { "d twelve", "Dm TWfLV", "TWfLV" }, DiceRollData::DiceType::D12 },
	{ { "d twenty", "Dm TWfNTm", "TWfNTm", 0.3f }, DiceRollData::DiceType::D20 },
	{ { "d twenty", "Dm TWfNm", "TWfNm", 0.3f }, DiceRollData::DiceType::D20 },
	{ { "d one hundred", "Dm WcN hcNDRcD", "WcN hcNDRcD" }, DiceRollData::DiceType::D100 },

	// "N sided" forms
	{ { "four sided", "FeR SiDcD", "FeR" }, DiceRollData::DiceType::D4 },
	{ { "six sided", "SgKS SiDcD", "SgKS" }, DiceRollData::DiceType::D6 },
	{ { "eight sided", "dT SiDcD", "dT" }, DiceRollData::DiceType::D8 },
	{ { "ten sided", "TfN SiDcD", "TfN" }, DiceRollData::DiceType::D10 },
	{ { "twelve sided", "TWfLV SiDcD", "TWfLV" }, DiceRollData::DiceType::D12 },
	{ { "twenty sided", "TWfNTm SiDcD", "TWfNTm", 0.3f }, DiceRollData::DiceType::D20 },
	{ { "twenty sided", "TWfNm SiDcD", "TWfNm", 0.3f }, DiceRollData::DiceType::D20 }, // reduced "twenny"
	{ { "one hundred sided", "WcN hcNDRcD SiDcD", "WcN hcNDRcD" }, DiceRollData::DiceType::D100 },

	// bare number forms
	{ { "four", "FeR", "FeR" }, DiceRollData::DiceType::D4 },
	{ { "six", "SgKS", "SgKS" }, DiceRollData::DiceType::D6 },
	{ { "eight", "dT", "dT" }, DiceRollData::DiceType::D8 },
	{ { "ten", "TfN", "TfN", 0.34f }, DiceRollData::DiceType::D10 },
	{ { "twelve", "TWfLV", "TWfLV", 0.3f }, DiceRollData::DiceType::D12 },
	{ { "twenty", "TWfNTm", "TWfNTm", 0.3f }, DiceRollData::DiceType::D20 },
	{ { "twenty", "TWfNm", "TWfNm", 0.3f }, DiceRollData::DiceType::D20 }, // reduced "twenny"
	{ { "one hundred", "WcN hcNDRcD", "WcN hcNDRcD" }, DiceRollData::DiceType::D100 },
};

template<typename Entry>
static std::vector<AudioFrontend::Phrase> toPhraseList(std::span<const Entry> table){
	std::vector<AudioFrontend::Phrase> list;
	list.reserve(table.size());
	for(const auto& entry : table){
		list.push_back(entry.phrase);
	}
	return list;
}

static uint8_t diceTypeSides(const DiceRollData::DiceType type){
	switch(type){
		case DiceRollData::DiceType::D4: return 4;
		case DiceRollData::DiceType::D6: return 6;
		case DiceRollData::DiceType::D8: return 8;
		case DiceRollData::DiceType::D10: return 10;
		case DiceRollData::DiceType::D12: return 12;
		case DiceRollData::DiceType::D20: return 20;
		case DiceRollData::DiceType::D100: return 100;
		default: return 6;
	}
}

static const char* diceTypeName(const DiceRollData::DiceType type){
	switch(type){
		case DiceRollData::DiceType::D4: return "d four";
		case DiceRollData::DiceType::D6: return "d six";
		case DiceRollData::DiceType::D8: return "d eight";
		case DiceRollData::DiceType::D10: return "d ten";
		case DiceRollData::DiceType::D12: return "d twelve";
		case DiceRollData::DiceType::D20: return "d twenty";
		case DiceRollData::DiceType::D100: return "d one hundred";
		default: return "d six";
	}
}

static void speakText(Audio* audio, std::string text){
	auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, std::move(text));
	audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
	audio->waitEnd(portMAX_DELAY);
}

static void playPhrase(Audio* audio, Phrase phrase){
	const int16_t id = Phrases::get(phrase);
	if(id < 0){
		CMF_LOG(DiceRollRoutine, LogLevel::Warning, "No phrase output for phrase id %d", (int)phrase);
		return;
	}
	const std::string text = Phrases::map(phrase, id);
	CMF_LOG(DiceRollRoutine, LogLevel::Info, "Playing phrase %d: '%s'", (int)phrase, text.c_str());
	speakText(audio, text);
}

static void sendState(Com* com, DiceRollData::Phase phase, DiceRollData::DiceType type, uint8_t count, uint16_t result){
	DiceRollData data{};
	data.phase = phase;
	data.diceType = type;
	data.diceCount = count;
	data.diceResult = result;
	com->sendData(BB::State::Scenario, BB::Action::Scenario::DiceRoll, data);
}

static void handleNoInput(Audio* audio, const char* label, Phrase timeoutPhrase, Phrase invalidPhrase, const AudioFrontend::PhraseResult& res){
	if(res.recognized){
		if(ServiceLocator::ScenarioRoutineServiceInstance){
			if(audio != nullptr){
				audio->stop();
				audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::ListenAbort, Phrases::get(Phrase::ListenAbort))));
			}
		}

		CMF_LOG(DiceRollRoutine, LogLevel::Info, "%s: aborted by user", label);
		return;
	}

	if(res.transcript.empty()){
		CMF_LOG(DiceRollRoutine, LogLevel::Warning, "%s: timeout (no speech detected)", label);
		playPhrase(audio, timeoutPhrase);
	} else{
		CMF_LOG(DiceRollRoutine, LogLevel::Warning, "%s: unrecognized speech '%s'", label, res.transcript.c_str());
		playPhrase(audio, invalidPhrase);
	}
}

static void rollAndReport(Audio* audio, Com* com, AudioFrontend* af, AACAudioGenerator* aacGen, uint8_t count, DiceRollData::DiceType diceType){
	af->setMode(AudioFrontend::Mode::Off);
	af->unloadModel();

	const uint8_t sides = diceTypeSides(diceType);
	uint32_t total = 0;
	for(uint8_t i = 0; i < count; ++i){
		const uint8_t roll = (esp_random() % sides) + 1;
		CMF_LOG(DiceRollRoutine, LogLevel::Info, "  Die %d: %d", i + 1, roll);
		total += roll;
	}
	const uint16_t result = static_cast<uint16_t>(total);
	CMF_LOG(DiceRollRoutine, LogLevel::Info, "Roll: %dd%d = %d", count, sides, result);

	char rollText[64];
	snprintf(rollText, sizeof(rollText), "%dd%d roll", count, sides);
	speakText(audio, rollText);

	sendState(com, DiceRollData::Phase::RollAnim, diceType, count, 0);

	if(aacGen != nullptr){
		static constexpr const char* RollSounds[] = {
			"/spiffs/dice/roll_1.aac", "/spiffs/dice/roll_2.aac", "/spiffs/dice/roll_3.aac",
			"/spiffs/dice/roll_4.aac", "/spiffs/dice/roll_5.aac",
		};
		const char* path = RollSounds[esp_random() % std::size(RollSounds)];
		CMF_LOG(DiceRollRoutine, LogLevel::Info, "Playing roll sound: %s", path);

		audio->play(aacGen, std::make_unique<FileAudioSource>(path));
		audio->waitEnd(portMAX_DELAY);
	}

	sendState(com, DiceRollData::Phase::Result, diceType, count, result);

	char resultText[16];
	snprintf(resultText, sizeof(resultText), "%d", result);
	CMF_LOG(DiceRollRoutine, LogLevel::Info, "Speaking result: '%s'", resultText);
	speakText(audio, resultText);

	CMF_LOG(DiceRollRoutine, LogLevel::Info, "--- Dice roll done ---");
}

Routine::TickingState DiceRollRoutine::tick(float deltaTime){
	const Application* app = ApplicationStatics::getApplication();
	Audio* audio = app->getService<Audio>();
	Com* com = app->getService<Com>();
	AACAudioGenerator* aacGen = app->getService<AACAudioGenerator>();

	if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !af){
		CMF_LOG(DiceRollRoutine, LogLevel::Error, "Missing required service(s)");
		return TickingState::Done;
	}

	switch(phase){
		case Phase::Init: {
			CMF_LOG(DiceRollRoutine, LogLevel::Info, "--- Dice roll started ---");

			sendState(com, DiceRollData::Phase::RollCount, DiceRollData::DiceType::None, 0, 0);
			playPhrase(audio, Phrase::DiceRollAskCount);

			const std::vector<AudioFrontend::Phrase> countPhrases = toPhraseList<CountEntry>(CountTable);
			listen(countPhrases);
			phase = Phase::WaitCount;
			return TickingState::Block;
		}

		case Phase::WaitCount: {
			if(!phraseReady()){
				return TickingState::Block;
			}

			const AudioFrontend::PhraseResult& result = phraseResult();
			if(result.index == -1){
				handleNoInput(audio, "Count", Phrase::DiceRollCountTimeout, Phrase::DiceRollCountInvalid, result);
				return TickingState::Done;
			}

			count = CountTable[result.index].value;
			CMF_LOG(DiceRollRoutine, LogLevel::Info, "Count: %d (phrase index %d, transcript '%s')", count, result.index, result.transcript.c_str());

			if(knownDiceType != DiceRollData::DiceType::None){
				CMF_LOG(DiceRollRoutine, LogLevel::Info, "Type: %s (known from activation)", diceTypeName(knownDiceType));
				rollAndReport(audio, com, af, aacGen, count, knownDiceType);
				return TickingState::Done;
			}

			sendState(com, DiceRollData::Phase::DiceKind, DiceRollData::DiceType::None, count, 0);
			playPhrase(audio, Phrase::DiceRollAskType);

			const std::vector<AudioFrontend::Phrase> typePhrases = toPhraseList<TypeEntry>(TypeTable);
			listen(typePhrases);
			phase = Phase::WaitType;
			return TickingState::Block;
		}

		case Phase::WaitType: {
			if(!phraseReady()){
				return TickingState::Block;
			}

			const AudioFrontend::PhraseResult& result = phraseResult();
			if(result.index == -1){
				handleNoInput(audio, "Type", Phrase::DiceRollTypeTimeout, Phrase::DiceRollTypeInvalid, result);
				return TickingState::Done;
			}

			const DiceRollData::DiceType diceType = TypeTable[result.index].type;
			CMF_LOG(DiceRollRoutine, LogLevel::Info, "Type: %s (phrase index %d, transcript '%s')", diceTypeName(diceType), result.index, result.transcript.c_str());

			rollAndReport(audio, com, af, aacGen, count, diceType);
			return TickingState::Done;
		}
	}

	return TickingState::Done;
}

DiceRollD4Routine::DiceRollD4Routine(BBStateMachine* sm) : DiceRollRoutine(sm){
	knownDiceType = DiceRollData::DiceType::D4;
}

DiceRollD6Routine::DiceRollD6Routine(BBStateMachine* sm) : DiceRollRoutine(sm){
	knownDiceType = DiceRollData::DiceType::D6;
}

DiceRollD8Routine::DiceRollD8Routine(BBStateMachine* sm) : DiceRollRoutine(sm){
	knownDiceType = DiceRollData::DiceType::D8;
}

DiceRollD10Routine::DiceRollD10Routine(BBStateMachine* sm) : DiceRollRoutine(sm){
	knownDiceType = DiceRollData::DiceType::D10;
}

DiceRollD12Routine::DiceRollD12Routine(BBStateMachine* sm) : DiceRollRoutine(sm){
	knownDiceType = DiceRollData::DiceType::D12;
}

DiceRollD20Routine::DiceRollD20Routine(BBStateMachine* sm) : DiceRollRoutine(sm){
	knownDiceType = DiceRollData::DiceType::D20;
}

DiceRollD100Routine::DiceRollD100Routine(BBStateMachine* sm) : DiceRollRoutine(sm){
	knownDiceType = DiceRollData::DiceType::D100;
}
