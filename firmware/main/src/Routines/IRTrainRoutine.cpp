#include "IRTrainRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Services/Modules/ModuleService.h>
#include <Services/Modules/ModuleDevices/RM_IRModule.h>
#include <BBData.h>
#include <Phrases.h>
#include <cstring>
#include <vector>
#include <algorithm>
#include <memory>
#include <Util/ServiceLocator.h>
#include <Util/IRSignalProc.h>
#include <Util/PSRAMAllocator.h>
#include <esp_timer.h>
#include "Audio/AudioFrontend.h"
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/IRStorage.h"
#include "States/ScenarioRoutineMappings.h"

DEFINE_LOG(IRTrainRoutine)

static constexpr uint32_t ScanTimeoutMs = 6000;


int IRTrainRoutine::levenshtein(const char* a, const char* b){
    const int la = static_cast<int>(strlen(a));
    const int lb = static_cast<int>(strlen(b));
    if(la == 0) return lb;
    if(lb == 0) return la;
    std::vector<int> row(lb + 1);
    for(int j = 0; j <= lb; ++j) row[j] = j;
    for(int i = 1; i <= la; ++i){
        int prev = row[0];
        row[0] = i;
        for(int j = 1; j <= lb; ++j){
            const int tmp = row[j];
            row[j] = (a[i - 1] == b[j - 1]) ? prev : 1 + std::min({prev, row[j], row[j - 1]});
            prev = tmp;
        }
    }
    return row[lb];
}

bool IRTrainRoutine::tooSimilar(const char* newPhonemes){
    const int lenNew = static_cast<int>(strlen(newPhonemes));
    for(uint8_t i = 0; i < ServiceLocator::IRStorageInstance->getCount(); ++i){
        const char* stored = ServiceLocator::IRStorageInstance->getCommand(i).phonemes;
        const int lenStored = static_cast<int>(strlen(stored));
        const int maxLen = std::max(lenNew, lenStored);
        if(maxLen == 0) continue;
        const int dist = levenshtein(newPhonemes, stored);
        if((float)dist / (float)maxLen < 0.25f){
            return true;
        }
    }
    return false;
}

Routine::TickingState IRTrainRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();
    AudioFrontend* af = app->getService<AudioFrontend>();
    ModuleService* modules = app->getService<ModuleService>();

    if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !af || !modules || !ServiceLocator::IRStorageInstance){
        CMF_LOG(IRTrainRoutine, LogLevel::Error, "Missing required service(s)");
        return TickingState::Done;
    }

    auto playPhrase = [&](Phrase phrase, bool wait = true){
        const int16_t id = Phrases::get(phrase);
        if(id < 0) return;
        const std::string text = Phrases::map(phrase, id);
        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, text);
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        if(wait){
            audio->waitEnd(portMAX_DELAY);
        }
    };

    auto playModuleMissing = [&](){
        const int16_t id = Phrases::get(Phrase::IRModuleMissing);
        if(id < 0) return;

        IR_trainData data{};
        data.id = static_cast<uint8_t>(id);
        data.missing = true;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_train, data);

        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::IRModuleMissing, id));
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
    };

    if(modules->getInserted() != Modules::Type::RM_IR){
        playModuleMissing();
        return TickingState::Done;
    }

    RM_IRModule* irModule = cast<RM_IRModule>(modules->getDevice().get());
    if(!irModule){
        playModuleMissing();
        return TickingState::Done;
    }

    if(ServiceLocator::IRStorageInstance->isFull()){
        IR_trainData data{};
        data.state = IR_trainData::State::MaxCapacity;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_train, data);
        playPhrase(Phrase::IRTrainFull);
        return TickingState::Done;
    }

    {
        IR_trainData data{};
        data.state = IR_trainData::State::Scanning;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_train, data);
    }
    playPhrase(Phrase::IRTrainScanning);

    auto processed = makePSRAM<ProcessedIR>();
    {
        auto capture = makePSRAM<IRCapture>();

        // Unusable signals are discarded and capture retried until the overall deadline.
        const int64_t deadlineUs = esp_timer_get_time() + ScanTimeoutMs * 1000LL;
        bool captured = false;
        while(!captured){
            const int64_t remainingMs = (deadlineUs - esp_timer_get_time()) / 1000;
            if(remainingMs <= 0 || !irModule->captureBurst(*capture, static_cast<uint32_t>(remainingMs), 2000)){
                break;
            }
            captured = processCapture(*capture, *processed);
        }

        if(!captured){
            playPhrase(Phrase::IRTrainScanTimeout);
            return TickingState::Done;
        }
    }

    {
        IR_trainData data{};
        data.state = IR_trainData::State::ScanDone;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_train, data);
    }
    playPhrase(Phrase::IRTrainScanSuccess);

    {
        IR_trainData data{};
        data.state = IR_trainData::State::Listening;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_train, data);
    }
    playPhrase(Phrase::IRTrainAskPhrase, false);

    std::vector<AudioFrontend::Phrase> phrases;
    phrases.reserve(activations.size() + ServiceLocator::IRStorageInstance->getCount());
    for(const ScenarioActivation& activation : activations){
        const RoutineFactory factory = routineForScenario({ activation.scenario, activation.scenarioData });
        phrases.push_back({ activation.string, activation.phonemes, activation.fuzzyCore, activation.fuzzyThreshold, reinterpret_cast<const void*>(factory) });
    }
    const int topLevelCount = static_cast<int>(phrases.size());

    for(uint8_t i = 0; i < ServiceLocator::IRStorageInstance->getCount(); ++i){
        const IRCommand& cmd = ServiceLocator::IRStorageInstance->getCommand(i);
        phrases.push_back({ cmd.phonemes, cmd.phonemes, "", 0.35f });
    }

    af->setSpeechPhrases(phrases);
    audio->waitEnd(portMAX_DELAY);

    while(true){
        const AudioFrontend::PhraseResult result = af->waitForPhrase();
    	TRACE_LOG("%s", result.transcript.c_str());

        if(result.index == -1 && result.recognized){
            return TickingState::Done;
        }

        if(!result.recognized){
            if(result.transcript.empty()){
                playPhrase(Phrase::IRTrainPhraseTimeout);
                return TickingState::Done;
            }
            const char* newPhonemes = result.transcript.c_str();
            if(tooSimilar(newPhonemes)){
                playPhrase(Phrase::IRTrainTooSimilar);
                continue;
            }
            if(!ServiceLocator::IRStorageInstance->add(newPhonemes, *processed)){
                CMF_LOG(IRTrainRoutine, LogLevel::Error, "Failed to add IR command");
                return TickingState::Done;
            }
            ServiceLocator::IRStorageInstance->store();
            {
                IR_trainData data{};
                data.state = IR_trainData::State::SaveDone;
                com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_train, data);
            }
            playPhrase(Phrase::IRTrainSaved);
            return TickingState::Done;
        }

        if(result.index < topLevelCount){
            playPhrase(Phrase::IRActionReserved);
            continue;
        }

        const int irIndex = result.index - topLevelCount;
        const char* stored = ServiceLocator::IRStorageInstance->getCommand(static_cast<uint8_t>(irIndex)).phonemes;
        const int lenNew = static_cast<int>(result.transcript.size());
        const int lenStored = static_cast<int>(strlen(stored));
        const int maxLen = std::max(lenNew, lenStored);
        const bool similar = maxLen > 0 && (float)levenshtein(result.transcript.c_str(), stored) / (float)maxLen < 0.25f;

        if(similar){
            playPhrase(Phrase::IRTrainTooSimilar);
        } else {
            playPhrase(Phrase::IRActionReserved);
        }
    }
}
