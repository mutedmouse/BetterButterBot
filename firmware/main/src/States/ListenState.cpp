#include "ListenState.h"
#include "BBStateMachine.h"
#include "Audio/AudioFrontend.h"
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include <Phrases.h>
#include <Services/Audio/Audio.h>
#include <Services/Audio/AACAudioGenerator.h>
#include <Services/Audio/FileAudioSource.h>
#include "Audio/MemoryAudioSource.h"
#include "Util/PSRAMAllocator.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <span>
#include <vector>
#include <Util/ServiceLocator.h>
#include "BBData.h"
#include "Enums.h"
#include "IdleState.h"
#include "RCState.h"
#include "ScenarioState.h"
#include "Services/Com.h"
#include "Services/ScenarioRoutineService.h"
#include "Services/IRStorage.h"
#include "Routines/IRActionRoutine.h"
#include "Routines/SpeakReplyRoutine.h"
#include "ScenarioRoutineMappings.h"
#include "Services/VoiceLink.h"
#include "Services/WakeWord.h"
#include "Services/Portal.h"
#include <esp_heap_caps.h>

DEFINE_LOG(ListenState)

namespace {
    constexpr size_t StreamBufferSize = 64 * 1024; // ~4 s of 16 kHz mu-law, rides out Wi-Fi stalls
    constexpr size_t MicChunkFrames = 512;          // 32 ms per I2S read
    constexpr uint32_t MaxUtteranceMs = 9000;       // gateway endpoints first; this is the backstop
    constexpr uint32_t ResultTimeoutMs = 12000;    // transcription + an LLM answer for questions
    constexpr uint32_t ReconnectGraceMs = 60000;   // gateway seen / Wi-Fi started this recently: wait, don't fall back
    constexpr uint32_t GatewayWaitMs = 5000;       // how long a listen waits for a link that is coming up
    constexpr const char* FollowUpChirp = "/spiffs/listen/lasers1/phaseJump4.aac";
    constexpr const char* ThinkingCue = "/spiffs/listen/robot/robot_1.aac";
}

static void playPhraseBlocking(const Application* app, Phrase phrase){
    if(app == nullptr){
        return;
    }

    Audio* audio = app->getService<Audio>();
    if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance){
        return;
    }

    const int16_t id = Phrases::get(phrase);
    if(id < 0){
        return;
    }

    const std::string text = Phrases::map(phrase, id);
    if(text.empty()){
        return;
    }

    auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, text);
    audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
    audio->waitEnd(portMAX_DELAY);
}

static const char* TAG = "ListenState";

namespace {
    constexpr uint32_t IntroWindowMs = 2500;

    struct SoundEffect { const char* path; uint16_t durationMs; };
    struct StartPrompt { const char* text; uint16_t durationMs; };

    constexpr StartPrompt StartPrompts[] = {
        { "Yes?",                  753 },
        { "I'm listening",         978 },
        { "How can I help?",      1303 },
        { "Another task?",        1010 },
        { "How may I assist?",    1417 },
        { "You have my attention", 1531 },
        { "Awaiting input",       1144 },
        { "State your request",   1511 },
    };

    constexpr SoundEffect Lasers1[] = {
        { "/spiffs/listen/lasers1/phaseJump1.aac",  384 },
        { "/spiffs/listen/lasers1/phaseJump2.aac",  384 },
        { "/spiffs/listen/lasers1/phaseJump3.aac",  320 },
        { "/spiffs/listen/lasers1/phaseJump4.aac",  256 },
        { "/spiffs/listen/lasers1/phaseJump5.aac",  384 },
        { "/spiffs/listen/lasers1/phaserDown1.aac", 384 },
        { "/spiffs/listen/lasers1/phaserDown2.aac", 320 },
        { "/spiffs/listen/lasers1/phaserDown3.aac", 384 },
    };

    constexpr SoundEffect Lasers2[] = {
        { "/spiffs/listen/lasers2/laser1.aac", 384 },
        { "/spiffs/listen/lasers2/laser2.aac", 384 },
        { "/spiffs/listen/lasers2/laser3.aac", 384 },
        { "/spiffs/listen/lasers2/laser4.aac", 384 },
        { "/spiffs/listen/lasers2/laser5.aac", 384 },
        { "/spiffs/listen/lasers2/laser6.aac", 320 },
        { "/spiffs/listen/lasers2/laser7.aac", 384 },
        { "/spiffs/listen/lasers2/laser8.aac", 384 },
        { "/spiffs/listen/lasers2/laser9.aac", 384 },
    };

    constexpr SoundEffect Robot[] = {
        { "/spiffs/listen/robot/robot_1.aac", 256 },
        { "/spiffs/listen/robot/robot_2.aac", 384 },
        { "/spiffs/listen/robot/robot_3.aac", 320 },
        { "/spiffs/listen/robot/robot_4.aac", 256 },
    };

    struct SfxGroup { const char* name; std::span<const SoundEffect> clips; };
    constexpr SfxGroup SfxGroups[] = {
        { "lasers1", Lasers1 },
        { "lasers2", Lasers2 },
        { "robot",   Robot   },
    };

    PSRAMByteBuffer readFile(const char* path){
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if(!f){
            return {};
        }
        const std::streamsize size = f.tellg();
        if(size <= 0){
            return {};
        }
        f.seekg(0);
        PSRAMByteBuffer buf(static_cast<size_t>(size));
        f.read(reinterpret_cast<char*>(buf.data()), size);
        buf.resize(static_cast<size_t>(f.gcount()));
        return buf;
    }

    void buildAndPlayIntro(const Application* app){
        Audio* audio = app->getService<Audio>();
        AACAudioGenerator* aacGen = app->getService<AACAudioGenerator>();
        if(audio == nullptr || aacGen == nullptr || ServiceLocator::SpeechAudioGenInstance == nullptr || ServiceLocator::SpeechGenInstance == nullptr){
            return;
        }

        const StartPrompt& prompt = StartPrompts[rand() % std::size(StartPrompts)];
        const uint32_t budget = prompt.durationMs >= IntroWindowMs ? 0 : IntroWindowMs - prompt.durationMs;

        const SfxGroup& group = SfxGroups[rand() % std::size(SfxGroups)];

        const uint32_t preloadStart = millis();

        const size_t maxAttempts = group.clips.size() * 4;

        std::vector<Audio::GenSourcePair> batch;
        batch.reserve(maxAttempts + 1); // + 1 for the spoken prompt source

        std::vector<size_t> pool;
        pool.reserve(group.clips.size());
        uint32_t accumulated = 0;
        size_t attempts = 0;
        while(accumulated < budget && attempts < maxAttempts){
            ++attempts;
            if(pool.empty()){
                for(size_t i = 0; i < group.clips.size(); ++i){
                    pool.push_back(i);
                }
            }
            const size_t poolIdx = rand() % pool.size();
            const SoundEffect& sfx = group.clips[pool[poolIdx]];
            pool.erase(pool.begin() + poolIdx);

            // Preload the clip into PSRAM now (before loadModel() hammers the flash),
            // so the audio task can play it without blocking on the flash lock.
            PSRAMByteBuffer bytes = readFile(sfx.path);
            if(bytes.empty()){
                continue;
            }

            batch.emplace_back(aacGen, std::make_unique<MemoryAudioSource>(std::move(bytes)));
            accumulated += sfx.durationMs;
        }

        batch.emplace_back(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, prompt.text));

        audio->play(std::span<Audio::GenSourcePair>(batch));
    }

}

std::vector<AudioFrontend::Phrase> ListenState::phrases;

void ListenState::preallocate(){
	phrases.reserve(activations.size() * 2);
}

ListenState::ListenState(BBStateMachine* sm) : State(sm){
	phrases.clear();

    const Application* app = ApplicationStatics::getApplication();

    chat = chatFlag.exchange(false);
    followUp = followUpFlag.exchange(false) && !chat;
    const bool quickIntro = quickIntroFlag.exchange(false);
    if(Portal* portal = Portal::get()){
        portal->setListening(true);
    }
    if(followUp || chat){
        playClip(FollowUpChirp); // short "still listening" cue instead of the full intro
    }else if(quickIntro){
        // Re-woken mid-conversation: a plain "Yes?" instead of up to 2.5 s of intro effects.
        if(Audio* audio = app->getService<Audio>(); audio != nullptr && ServiceLocator::SpeechAudioGenInstance){
            audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, "Yes?"));
        }
    }else{
        buildAndPlayIntro(app);
    }

    auto buttonInput = app->getService<ISRButtonInput>();
    if(buttonInput == nullptr){
        ESP_LOGE(TAG, "ButtonInput service missing!");
        return;
    }

    sm->bindState(buttonInput->OnButtonEvent, this, &ListenState::onButtonEvent);

    if(!ServiceLocator::ScenarioRoutineServiceInstance){
        CMF_LOG(ListenState, LogLevel::Error, "ScenarioRoutineService missing in constructor");
        return;
    }

    ServiceLocator::ScenarioRoutineServiceInstance->reset();

    com = ApplicationStatics::getApplication()->getService<Com>();
    if(com == nullptr){
        ESP_LOGE(TAG, "Com service missing!");
        return;
    }

    sm->bindState(com->OnCommand, this, &ListenState::onCommand);
    ListenData listenData{.phase = ListenData::Phase::Listening};
    com->sendData(BB::State::Listen, BB::Action::Listen{}, listenData);


    AudioFrontend* audioFrontend = app->getService<AudioFrontend>();
    if(audioFrontend == nullptr){
        CMF_LOG(ListenState, LogLevel::Error, "AudioFrontend service missing in constructor");
        return;
    }

    voiceLink = app->getService<VoiceLink>();
    cloud = voiceLink != nullptr && voiceLink->ready();

    // Right after boot (or a reconnect) the gateway may be seconds away: wait for it while the
    // intro plays rather than falling back to on-device recognition, which also turns Wi-Fi off
    // and so keeps the gateway from ever coming up.
    if(!cloud && voiceLink != nullptr && voiceLink->comingUp(ReconnectGraceMs)){
        const uint32_t until = millis() + GatewayWaitMs;
        while(!voiceLink->ready() && millis() < until){
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        cloud = voiceLink->ready();
    }

    if(cloud && startCloud()){
        return;
    }

    cloud = false;
    if(chat){
        // The portal's Listen only makes sense through the gateway; never fall back to on-device.
        CMF_LOG(ListenState, LogLevel::Warning, "Chat listen: gateway not ready");
        sm->transitionTo<IdleState>();
        return;
    }
    if(followUp){
        // Follow-ups only make sense with the gateway; otherwise just go back to idle quietly.
        sm->transitionTo<IdleState>();
        return;
    }

    if(voiceLink != nullptr && voiceLink->comingUp(ReconnectGraceMs)){
        // The link is only (re)connecting. Falling back would tear Wi-Fi down and restart the
        // connection, so every retry would fall back again; ask the user to wait instead.
        if(Audio* audio = app->getService<Audio>()){
            audio->waitEnd(portMAX_DELAY);
            audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, "Still connecting to the network. Try again in a moment."));
            audio->waitEnd(portMAX_DELAY);
        }
        sm->transitionTo<IdleState>();
        return;
    }

    if(voiceLink != nullptr){
        // On-device recognition needs the internal RAM the Wi-Fi stack holds; release it while listening.
        voiceLink->pause();
        linkPaused = true;
    }
    startLocal(audioFrontend);
}

bool ListenState::startCloud(){
    const Application* app = ApplicationStatics::getApplication();

    // Raw mic straight to the gateway: no AFE, so the Wi-Fi stack keeps its internal RAM.
    // The gateway decides when the utterance has ended.
    streamBuffer = xRingbufferCreateWithCaps(StreamBufferSize, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
    micBuffer = static_cast<int16_t*>(heap_caps_malloc(MicChunkFrames * 2 * sizeof(int16_t), MALLOC_CAP_SPIRAM));
    if(streamBuffer == nullptr || micBuffer == nullptr){
        CMF_LOG(ListenState, LogLevel::Error, "Stream buffer alloc failed");
        releaseStream();
        return false;
    }

    if(Audio* audio = app->getService<Audio>()){
        audio->waitEnd(portMAX_DELAY);
    }

    if(!voiceLink->beginUtterance(streamBuffer, followUp, chat)){
        CMF_LOG(ListenState, LogLevel::Warning, "Gateway unavailable, using on-device recognition");
        releaseStream();
        return false;
    }

    sm->bindState(voiceLink->OnResult, this, &ListenState::onCloudResult);
    sm->bindState(voiceLink->OnEndpoint, this, &ListenState::onCloudEndpoint);

    if(followUp){
        if(WakeWord* wakeWord = app->getService<WakeWord>(); wakeWord != nullptr && wakeWord->available()){
            wakeWord->beginExternal();
            sm->bindState(wakeWord->OnWake, this, &ListenState::onFollowUpWake);
            followUpWake = wakeWord;
        }
    }

    streaming = true;
    capturing = true;
    streamStartMs = millis();
    // 6 KB: the follow-up window also runs the wake-word model (TFLM Invoke) on this task.
    micTask = std::make_unique<Threaded>([this](){ micLoop(); }, "ListenMic", 0, 6 * 1024, 6, 1, false);
    micTask->start();

    if(ServiceLocator::LEDControlInstance){
        ServiceLocator::LEDControlInstance->startListen();
    }

    CMF_LOG(ListenState, LogLevel::Info, "Streaming to gateway");
    return true;
}

void ListenState::micLoop(){
    if(!capturing){
        vTaskDelay(pdMS_TO_TICKS(10));
        return;
    }

    const int32_t samples = ServiceLocator::MicInputInstance->getStereo({ micBuffer, MicChunkFrames * 2 });
    if(samples <= 0){
        return;
    }

    // Average L/R in place (write index never overtakes the read index).
    const size_t frames = (size_t) samples / 2;
    for(size_t i = 0; i < frames; ++i){
        micBuffer[i] = (int16_t) (((int32_t) micBuffer[2 * i] + micBuffer[2 * i + 1]) / 2);
    }

    // Follow-up window: the wake-word service is paused (we own the mic), so let it listen to
    // the same audio - "butterbot" then starts a fresh listen right away.
    if(followUpWake != nullptr){
        followUpWake->feedExternal(micBuffer, frames);
    }

    // mu-law encode in place: byte i overwrites part of int16 slot i/2, already consumed.
    uint8_t* out = reinterpret_cast<uint8_t*>(micBuffer);
    for(size_t i = 0; i < frames; ++i){
        out[i] = VoiceLink::mulaw(micBuffer[i]);
    }

    // If Wi-Fi stalls longer than the buffer holds, newest audio is dropped rather than blocking.
    xRingbufferSend(streamBuffer, out, frames, 0);
}

void ListenState::stopCapture(){
    if(!capturing){
        return;
    }
    capturing = false;
    if(ServiceLocator::LEDControlInstance){
        ServiceLocator::LEDControlInstance->stopListen();
    }
}

void ListenState::releaseStream(){
    if(micTask){
        capturing = false;
        micTask->stop(portMAX_DELAY);
        micTask.reset();
    }
    if(streamBuffer){
        vRingbufferDeleteWithCaps(streamBuffer);
        streamBuffer = nullptr;
    }
    if(micBuffer){
        heap_caps_free(micBuffer);
        micBuffer = nullptr;
    }
}

void ListenState::startLocal(AudioFrontend* audioFrontend){
    const Application* app = ApplicationStatics::getApplication();

	audioFrontend->loadModel();

    audioFrontend->setMode(AudioFrontend::Mode::Off);

    if(!activations.empty()){
        for(const ScenarioActivation& activation : activations){
            const RoutineFactory factory = routineForScenario({ activation.scenario, activation.scenarioData });
            phrases.push_back({ activation.string, activation.phonemes, activation.fuzzyCore, activation.fuzzyThreshold, reinterpret_cast<const void*>(factory) });
        }

        if(ServiceLocator::IRStorageInstance){
            for(uint8_t i = 0; i < ServiceLocator::IRStorageInstance->getCount(); ++i){
                const IRCommand& cmd = ServiceLocator::IRStorageInstance->getCommand(i);
                phrases.push_back({ cmd.phonemes, cmd.phonemes });
            }
        }

        audioFrontend->setSpeechPhrases(phrases);
    }

    if(Audio* audio = app->getService<Audio>()){
        audio->waitEnd(portMAX_DELAY);

        sm->bindState(audioFrontend->onPhrase, this, &ListenState::onPhrase);
        audioFrontend->setMode(AudioFrontend::Mode::Speech);
    }
}

ListenState::~ListenState(){
    // Event bindings are dropped by BBStateMachine::unbindStateEvents() after this dtor runs.
    const Application* app = ApplicationStatics::getApplication();

    if(cloud){
        stopCapture();
        voiceLink->cancelUtterance();
        releaseStream();
    }else if(AudioFrontend* audioFrontend = app->getService<AudioFrontend>()){
        audioFrontend->setMode(AudioFrontend::Mode::Off);
		audioFrontend->unloadModel();
    }

    if(linkPaused){
        voiceLink->resume();
    }

    if(Portal* portal = Portal::get()){
        portal->setListening(false);
    }

    if(timedOut){
        playPhraseBlocking(app, Phrase::ListeningTimeout);
    }
}

int64_t ListenState::getDynamicTickInterval() const{
    return (streaming || awaitingResult) ? 100 : -1;
}

void ListenState::tick(float deltaTime){
    if(!cloud || resultHandled){
        return;
    }

    const uint32_t now = millis();
    if((streaming || awaitingResult) && !voiceLink->ready()){
        CMF_LOG(ListenState, LogLevel::Warning, "Gateway link lost while listening");
        onCloudResult(false, -1, "", "");
    }else if(streaming && now - streamStartMs > MaxUtteranceMs){
        finishStream("maxlen");
    }else if(awaitingResult && now - endSentMs > ResultTimeoutMs){
        CMF_LOG(ListenState, LogLevel::Warning, "Gateway result timed out");
        onCloudResult(false, -1, "", "");
    }
}

void ListenState::finishStream(const char* reason){
    if(!streaming){
        return;
    }

    streaming = false;
    stopCapture();
    voiceLink->endUtterance(reason);
    awaitingResult = true;
    endSentMs = millis();
    CMF_LOG(ListenState, LogLevel::Info, "Utterance ended (%s) after %lu ms", reason, (unsigned long) (endSentMs - streamStartMs));
}

void ListenState::onFollowUpWake(){
    if(resultHandled){
        return;
    }
    CMF_LOG(ListenState, LogLevel::Info, "Wake word during follow-up window: starting a fresh listen");
    resultHandled = true;
    streaming = false;
    stopCapture();
    quickIntroFlag = true;
    sm->transitionTo<ListenState>();
}

void ListenState::onCloudEndpoint(bool heardSpeech){
    if(!streaming){
        return;
    }
    // Same as a local end-of-utterance, minus the "end" message the gateway no longer needs.
    streaming = false;
    stopCapture();
    voiceLink->cancelUtterance();
    awaitingResult = true;
    endSentMs = millis();
    if(heardSpeech){
        playClip(ThinkingCue); // "got it" while the gateway transcribes / thinks
    }
    CMF_LOG(ListenState, LogLevel::Info, "Gateway endpoint after %lu ms", (unsigned long) (endSentMs - streamStartMs));
}

void ListenState::playClip(const char* path){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    AACAudioGenerator* aacGen = app->getService<AACAudioGenerator>();
    if(audio == nullptr || aacGen == nullptr){
        return;
    }
    audio->play(aacGen, std::make_unique<FileAudioSource>(path));
}

void ListenState::onCloudResult(bool recognized, int index, std::string transcript, std::string reply){
    if(resultHandled){
        return;
    }
    resultHandled = true;
    awaitingResult = false;

    // The gateway ends the utterance itself, so the result usually arrives while still streaming.
    if(streaming){
        streaming = false;
        stopCapture();
        voiceLink->cancelUtterance();
    }
    CMF_LOG(ListenState, LogLevel::Info, "Result after %lu ms of listening", (unsigned long) (millis() - streamStartMs));

    // The portal's Listen button: the gateway put the words in the chat; nothing to run or say.
    if(chat){
        sm->transitionTo<IdleState>();
        return;
    }

    // The gateway only knows the scenario table; IR phrases stay on the on-device path for now.
    if(index >= static_cast<int>(activations.size())){
        recognized = false;
    }

    // Just "butterbot" (typically said during a follow-up window): start over with a fresh listen,
    // exactly as if the on-device wake word had fired.
    if(index == VoiceLink::RewakeIndex){
        quickIntroFlag = true;
        sm->transitionTo<ListenState>();
        return;
    }

    // Follow-up window that heard nothing: drop back to idle without "I didn't hear you".
    if(followUp && !recognized && reply.empty()){
        sm->transitionTo<IdleState>();
        return;
    }

    // Not a command, but the gateway's LLM answered it: speak the answer, then listen for a follow-up.
    if(!recognized && !reply.empty() && ServiceLocator::ScenarioRoutineServiceInstance){
        SpeakReplyRoutine::setText(reply, true);
        ServiceLocator::ScenarioRoutineServiceInstance->setRoutineFactory(&makeRoutine<SpeakReplyRoutine>);
        sm->transitionTo<ScenarioState>();
        return;
    }

    onPhrase(recognized, index, transcript, 1.0f);
}

void ListenState::onRC(){
    sm->transitionTo<RCState>();
}

void ListenState::onPhrase(bool recognized, int index, const std::string& transcript, float confidence){
	// Runs on the AFE proc task. Do NOT stop the AFE or play audio here
    if(!recognized){
    	CMF_LOG(ListenState, LogLevel::Info, "Listen result: recognized=%d, index=%d, transcript='%s', confidence=%.2f",
			recognized, index, transcript.c_str(), confidence);

        ListenData listenData{.phase = ListenData::Phase::NotUnderstand};
        com->sendData(BB::State::Listen, BB::Action::Listen{}, listenData);
        timedOut = true;
        sm->transitionTo<IdleState>();
        return;
    }

	if(index < 0){
        // Abort phrase
        aborted();
        sm->transitionTo<IdleState>();
        return;
    }

    if(!ServiceLocator::ScenarioRoutineServiceInstance){
        CMF_LOG(ListenState, LogLevel::Error, "ScenarioRoutineService missing in onPhrase");
        return;
    }

    if(index >= static_cast<int>(activations.size())){
        const int irIndex = index - static_cast<int>(activations.size());
        if(ServiceLocator::IRStorageInstance){
            ServiceLocator::IRStorageInstance->setSelectedIndex(static_cast<uint8_t>(irIndex));
        }
        ServiceLocator::ScenarioRoutineServiceInstance->setRoutineFactory(&makeRoutine<IRActionRoutine>);
        sm->transitionTo<ScenarioState>();
        return;
    }

    const ScenarioActivation& activation = activations[index];
    const RoutineFactory factory = routineForScenario({ activation.scenario, activation.scenarioData });
    if(factory == nullptr){
        CMF_LOG(ListenState, LogLevel::Error, "No routine mapped for scenario %d", static_cast<int>(activation.scenario));
        sm->transitionTo<IdleState>();
        return;
    }

    ServiceLocator::ScenarioRoutineServiceInstance->setRoutineFactory(factory);
    sm->transitionTo<ScenarioState>();
}

void ListenState::onButtonEvent(int button, ISRButtonInput::Action action){
    if(action != ISRButtonInput::Action::Press || button != (int)Button::Power){
        return;
    }
    aborted();
    sm->transitionTo<IdleState>();
}

void ListenState::onCommand(Ctrl::Command cmd) {
    if(cmd == Ctrl::Command::Listen || cmd == Ctrl::ShutUp){
        aborted();
        sm->transitionTo<IdleState>();
    	return;
    }
}

void ListenState::aborted(){
	if(ServiceLocator::ScenarioRoutineServiceInstance){
		if(Audio* audio = ApplicationStatics::getApplication()->getService<Audio>()){
			audio->stop();
			audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::ListenAbort, Phrases::get(Phrase::ListenAbort))));
		}
	}

    if(com == nullptr){
        return;
    }

    ListenData listenData{.phase = ListenData::Phase::Abort};
    com->sendData(BB::State::Listen, BB::Action::Listen{}, listenData);
}
