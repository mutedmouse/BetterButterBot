#ifndef BUTTERBOT_FIRMWARE_VOICELINK_H
#define BUTTERBOT_FIRMWARE_VOICELINK_H

#include <atomic>
#include <memory>
#include <string>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_websocket_client.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include <freertos/semphr.h>
#include "Object/Object.h"
#include "Event/EventBroadcaster.h"
#include "Thread/Threaded.h"

/**
 * @brief Home Wi-Fi + WebSocket link to bb-voice-gateway (server/bb-voice-gateway).
 * Owns the whole Wi-Fi stack while running, the same way CMF's WiFi class does for RC mode,
 * so it must be paused before RCState creates its own WiFi and resumed afterwards.
 *
 * Listen audio (16 kHz s16le mono) is drained from a ring buffer by an internal sender task.
 */
class VoiceLink : public Object {
	GENERATED_BODY(VoiceLink, Object, void);
public:
	VoiceLink();
	~VoiceLink() override;

	/** Wi-Fi up, socket connected and the gateway agreed on the command vocabulary. */
	bool ready() const;

	/** True if the link was ready at some point in the last withinMs (i.e. it is probably just reconnecting). */
	bool recentlyReady(uint32_t withinMs) const;

	/**
	 * True while the link is expected to become ready soon: Wi-Fi was (re)started less than graceMs
	 * ago (boot, after RC mode or on-device listening) or it was ready less than graceMs ago.
	 */
	bool comingUp(uint32_t graceMs) const;

	/** Tear down Wi-Fi synchronously (before RC mode takes the radio). */
	void pause();

	/** Bring Wi-Fi back up once RC mode has released the radio. Non-blocking. */
	void resume();

	/**
	 * Start streaming the contents of rb to the gateway. followUp shortens the gateway's no-speech wait;
	 * chat marks the portal's Listen button (the gateway returns text only, never a command).
	 */
	bool beginUtterance(RingbufHandle_t rb, bool followUp = false, bool chat = false);

	/**
	 * Background-agent push from the gateway: something to say, or a scenario to run, whenever the
	 * robot is next idle. Held here until IdleState takes it so it never interrupts a conversation.
	 */
	struct Push {
		std::string say;
		int scenario = -1; // index into activations, -1 = none
		bool alarm = false; // sing the alarm song, then say `say`
		std::string song;   // with alarm: which song ("timer" = toast song); empty = butter song
		std::string routine; // a built-in routine with no voice command of its own ("summon" = find me)
		bool chatListen = false; // the portal's Listen button: one listen whose words go to the chat only
	};
	bool takePush(Push& out);
	bool pushPending() const;

	DECLARE_EVENT(PushEvent, VoiceLink);
	PushEvent OnPush{ this };

	/** Flush what is left in the ring buffer, then ask the gateway for a result. */
	void endUtterance(const char* reason);

	/** Stop streaming without asking for a result. Safe to call when not streaming. */
	void cancelUtterance();

	/** Milliseconds since audio last left successfully (meaningful while streaming). */
	uint32_t sendStallMs() const;

	/**
	 * One camera JPEG for the gateway: a {"type":"frame"} header, then the bytes as a binary message.
	 * Skipped (Busy) while listen audio is streaming, so voice always has the radio.
	 */
	enum class FrameResult : uint8_t { Sent, Busy, Failed };
	FrameResult sendFrame(const uint8_t* data, size_t len);

	/** Send a small JSON text message (status reports); timeoutMs 0 = the link's usual send timeout. */
	bool sendText(const char* json, uint32_t timeoutMs = 0);

	/** Set once at startup; {"type":"camera"} messages from the gateway are forwarded to it. */
	void setCameraStream(class CameraStream* stream){ cameraStream = stream; }

	/** Set once at startup; RC mode, the joystick and status reports (see Portal.h). Ticked from this task. */
	void setPortal(class Portal* p){ portal = p; }

	/** OnResult index meaning "only the wake word was heard: start a fresh listen". */
	static constexpr int RewakeIndex = -2;

	/**
	 * bool recognized, int index (into activations, -1 = abort, RewakeIndex), std::string transcript,
	 * std::string reply (spoken answer for non-commands; empty if none)
	 */
	DECLARE_EVENT(ResultEvent, VoiceLink, bool, int, std::string, std::string);
	ResultEvent OnResult{ this };

	/** bool heardSpeech - the gateway ended the utterance (false: nobody spoke); the result follows. */
	DECLARE_EVENT(EndpointEvent, VoiceLink, bool);
	EndpointEvent OnEndpoint{ this };

	/** bool ready - true once the gateway accepts the vocabulary, false when a ready link drops. */
	DECLARE_EVENT(ReadyEvent, VoiceLink, bool);
	ReadyEvent OnReady{ this };

	static uint32_t vocabHash();

	/** Logs free / largest-block / minimum-ever internal RAM and Wi-Fi RSSI under the given label. */
	static void logHeap(const char* where);

	/** G.711 mu-law encode; the stream is sent as 8-bit mu-law (announced in the "start" message). */
	static uint8_t mulaw(int16_t sample);

private:
	enum class Want : uint8_t { Run, Pause };
	std::atomic<Want> want{ Want::Run };
	std::atomic<bool> netUp{ false };
	std::atomic<bool> wsConnected{ false };
	std::atomic<bool> helloOk{ false };
	std::atomic<uint32_t> lastReadyMs{ 0 };
	std::atomic<bool> quietReconnect{ false };
	std::atomic<uint32_t> lastSendOkMs{ 0 };
	std::atomic<bool> rebuildSocket{ false };      // set on GOT_IP; the loop (re)creates the client
	uint32_t socketDownSince = 0;                  // loop-only: when the gateway was last seen down
	static constexpr uint32_t SocketRebuildMs = 20000;

	esp_netif_t* netif = nullptr;
	esp_event_handler_instance_t wifiHandler = nullptr;
	esp_event_handler_instance_t ipHandler = nullptr;
	esp_websocket_client_handle_t ws = nullptr;
	bool stackOwned = false;
	uint32_t lastStartAttempt = 0;

	SemaphoreHandle_t stackMutex = nullptr;  // guards bring-up / tear-down
	SemaphoreHandle_t streamMutex = nullptr; // guards rb / streaming flags vs. sender task

	SemaphoreHandle_t pushMutex = nullptr;
	Push pending;
	class CameraStream* cameraStream = nullptr;
	class Portal* portal = nullptr;
	bool hasPending = false;

	RingbufHandle_t rb = nullptr;
	bool streaming = false;
	bool closing = false; // guarded by streamMutex: the client is being destroyed, don't send
	bool endRequested = false;
	std::string endReason;

	std::unique_ptr<Threaded> task;
	void loop();

	bool startStack();
	void stopStack();
	void startSocket();
	void stopSocket();
	void drain();

	static void onWifiEvent(void* arg, esp_event_base_t base, int32_t id, void* data);
	static void onWsEvent(void* arg, esp_event_base_t base, int32_t id, void* data);
	void handleMessage(const char* data, int len);
	void onLinkDropped();
};

#endif //BUTTERBOT_FIRMWARE_VOICELINK_H
