#ifndef BUTTERBOT_FIRMWARE_LISTENSTATE_H
#define BUTTERBOT_FIRMWARE_LISTENSTATE_H

#include "State.h"
#include "CtrlData.h"
#include "Audio/AudioFrontend.h"
#include "Services/Com.h"
#include "Services/ISRButtonInput.h"
#include "Thread/Threaded.h"
#include <atomic>
#include <memory>

class VoiceLink;

class ListenState : public State {
public:
    explicit ListenState(BBStateMachine* sm);
    virtual ~ListenState() override;

    static void preallocate();

    /** Conversation follow-up: the next ListenState skips the intro and quietly gives up on silence. */
    static void requestFollowUp(){ followUpFlag = true; }
    static void cancelFollowUp(){ followUpFlag = false; }
    static bool followUpPending(){ return followUpFlag; }

    /** The portal's Listen button: the next ListenState streams once for the chat (text only, no command). */
    static void requestChat(){ chatFlag = true; }

    void tick(float deltaTime) override;
    int64_t getDynamicTickInterval() const override;

private:
    Com* com;

    bool timedOut = false;

    // LAN Whisper path (VoiceLink); falls back to on-device MultiNet when the gateway isn't ready.
    VoiceLink* voiceLink = nullptr;
    RingbufHandle_t streamBuffer = nullptr;
    int16_t* micBuffer = nullptr;
    class WakeWord* followUpWake = nullptr; // set during follow-up windows: shares our mic audio
    std::unique_ptr<Threaded> micTask;
    std::atomic<bool> capturing{ false };
    inline static std::atomic<bool> followUpFlag{ false };
    inline static std::atomic<bool> quickIntroFlag{ false }; // next listen: just "Yes?" (re-wake)
    inline static std::atomic<bool> chatFlag{ false };
    bool followUp = false;
    bool chat = false;
    bool cloud = false;
    bool linkPaused = false;
    bool streaming = false;
    bool awaitingResult = false;
    bool resultHandled = false;
    uint32_t streamStartMs = 0;
    uint32_t endSentMs = 0;

    bool startCloud();
    void startLocal(AudioFrontend* audioFrontend);
    void micLoop();
    void stopCapture();
    void releaseStream();
    void finishStream(const char* reason);
    void onCloudEndpoint(bool heardSpeech);
    void onFollowUpWake();
    static void playClip(const char* path);
    void onCloudResult(bool recognized, int index, std::string transcript, std::string reply);

    void onRC();
    void onPhrase(bool recognized, int index, const std::string& transcript, float confidence);
    void onButtonEvent(int button, ISRButtonInput::Action action);
    void onCommand(Ctrl::Command cmd);

    void aborted();

	static std::vector<AudioFrontend::Phrase> phrases;
};

#endif //BUTTERBOT_FIRMWARE_LISTENSTATE_H
