#include "VoiceLink.h"
#include <cstring>
#include <cJSON.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_wifi.h>
#include <Scenarios.h>
#include "Log/Log.h"
#include "BLE/ConMan.h"
#include "CameraStream.h"
#include "Portal.h"

namespace {
	// Wi-Fi gets the shared radio to itself while the gateway link is up (see BB_BLE_QUIET_ON_LAN).
	void bleQuiet(bool quiet){
#ifdef CONFIG_BB_BLE_QUIET_ON_LAN
		ConMan.setQuiet(quiet);
#endif
	}
}

DEFINE_LOG(VoiceLink)

namespace {
	constexpr uint32_t RestartDelayMs = 3000;
	constexpr size_t SendChunk = 2048; // 128 ms of 16 kHz mu-law audio
	// The websocket client drops the connection when a write times out, and Wi-Fi modem sleep
	// (mandatory alongside BLE) can delay ACKs by a few hundred ms, so be generous.
	// The websocket client drops the connection when a write times out, and Wi-Fi modem sleep
	// (mandatory alongside BLE) can delay ACKs by a few hundred ms, so be generous.
	constexpr TickType_t SendTimeout = pdMS_TO_TICKS(10000);
constexpr TickType_t FrameTimeout = pdMS_TO_TICKS(1500); // camera frames: drop rather than wait

	uint32_t nowMs(){
		return (uint32_t) (esp_timer_get_time() / 1000);
	}

	struct Lock {
		explicit Lock(SemaphoreHandle_t m) : m(m){ xSemaphoreTake(m, portMAX_DELAY); }
		~Lock(){ xSemaphoreGive(m); }
		SemaphoreHandle_t m;
	};
}

uint32_t VoiceLink::vocabHash(){
	// FNV-1a over every activation string + '\n'; must match vocab.vocab_hash() in the gateway.
	uint32_t h = 0x811C9DC5u;
	for(const ScenarioActivation& a : activations){
		for(const char* c = a.string; ; ++c){
			const uint8_t b = (*c == '\0') ? '\n' : (uint8_t) *c;
			h ^= b;
			h *= 0x01000193u;
			if(*c == '\0') break;
		}
	}
	return h;
}

void VoiceLink::logHeap(const char* where){
	int rssi = 0;
	esp_wifi_sta_get_rssi(&rssi);
	CMF_LOG(VoiceLink, LogLevel::Info, "heap[%s]: internal free %u, largest %u, min ever %u; psram free %u; rssi %d", where,
			(unsigned) heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
			(unsigned) heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
			(unsigned) heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
			(unsigned) heap_caps_get_free_size(MALLOC_CAP_SPIRAM), rssi);
}

uint8_t VoiceLink::mulaw(int16_t sample){
	// ITU-T G.711 mu-law: 8 bits per sample halves the stream for the same speech quality Whisper needs.
	constexpr int Bias = 0x84;
	constexpr int Clip = 32635;
	int s = sample;
	const uint8_t sign = (s < 0) ? 0x80 : 0;
	if(s < 0) s = -s;
	if(s > Clip) s = Clip;
	s += Bias;
	int exponent = 7;
	for(int mask = 0x4000; (s & mask) == 0 && exponent > 0; mask >>= 1){
		--exponent;
	}
	const int mantissa = (s >> (exponent + 3)) & 0x0F;
	return (uint8_t) ~(sign | (exponent << 4) | mantissa);
}

VoiceLink::VoiceLink(){
	stackMutex = xSemaphoreCreateMutex();
	streamMutex = xSemaphoreCreateMutex();
	pushMutex = xSemaphoreCreateMutex();

	if(strlen(CONFIG_BB_VOICELINK_SSID) == 0){
		CMF_LOG(VoiceLink, LogLevel::Warning, "No SSID configured, VoiceLink disabled");
		want = Want::Pause;
	}

	// Network code touches NVS (Wi-Fi calibration data), so the stack must be internal.
	task = std::make_unique<Threaded>([this](){ loop(); }, "VoiceLink", 20 / portTICK_PERIOD_MS, 4 * 1024, 5, 0, true);
	task->start();
}

VoiceLink::~VoiceLink(){
	want = Want::Pause;
	task->stop(portMAX_DELAY);
	stopStack();
	vSemaphoreDelete(stackMutex);
	vSemaphoreDelete(streamMutex);
	vSemaphoreDelete(pushMutex);
}

bool VoiceLink::ready() const{
	return netUp && wsConnected && helloOk;
}

void VoiceLink::pause(){
	quietReconnect = true; // a requested pause: don't announce the reconnect afterwards
	want = Want::Pause;
	cancelUtterance();
	stopStack();
}

void VoiceLink::resume(){
	if(strlen(CONFIG_BB_VOICELINK_SSID) == 0) return;
	lastStartAttempt = nowMs(); // let RC mode's WiFi finish tearing down first
	want = Want::Run;
}

bool VoiceLink::pushPending() const{
	Lock lock(pushMutex);
	return hasPending;
}

bool VoiceLink::takePush(Push& out){
	Lock lock(pushMutex);
	if(!hasPending) return false;
	out = std::move(pending);
	pending = {};
	hasPending = false;
	return true;
}

bool VoiceLink::beginUtterance(RingbufHandle_t buffer, bool followUp, bool chat){
	if(!ready()) return false;

	Lock lock(streamMutex);
	const char* start = chat ? "{\"type\":\"start\",\"codec\":\"mulaw\",\"chat\":true}"
					  : followUp ? "{\"type\":\"start\",\"codec\":\"mulaw\",\"followup\":true}"
					  : "{\"type\":\"start\",\"codec\":\"mulaw\"}";
	if(esp_websocket_client_send_text(ws, start, strlen(start), SendTimeout) < 0){
		CMF_LOG(VoiceLink, LogLevel::Warning, "Failed to send start");
		return false;
	}

	rb = buffer;
	streaming = true;
	endRequested = false;
	lastSendOkMs = nowMs();
	return true;
}

void VoiceLink::endUtterance(const char* reason){
	Lock lock(streamMutex);
	if(!streaming) return;
	endReason = reason;
	endRequested = true;
}

void VoiceLink::cancelUtterance(){
	Lock lock(streamMutex);
	streaming = false;
	endRequested = false;
	rb = nullptr;
}

uint32_t VoiceLink::sendStallMs() const{
	return nowMs() - lastSendOkMs;
}

VoiceLink::FrameResult VoiceLink::sendFrame(const uint8_t* data, size_t len){
	// Under streamMutex: keeps the header and its JPEG adjacent (audio is sent under the same lock),
	// and keeps stopSocket() from destroying the client mid-send.
	Lock lock(streamMutex);
	if(streaming) return FrameResult::Busy;
	if(ws == nullptr || closing || !ready()) return FrameResult::Failed;

	char header[48];
	snprintf(header, sizeof(header), "{\"type\":\"frame\",\"len\":%u}", (unsigned) len);
	if(esp_websocket_client_send_text(ws, header, strlen(header), FrameTimeout) < 0 ||
	   esp_websocket_client_send_bin(ws, (const char*) data, (int) len, FrameTimeout) < 0){
		CMF_LOG(VoiceLink, LogLevel::Warning, "Camera frame send failed (%u bytes)", (unsigned) len);
		return FrameResult::Failed;
	}
	return FrameResult::Sent;
}

bool VoiceLink::sendText(const char* json, uint32_t timeoutMs){
	Lock lock(streamMutex);
	if(ws == nullptr || closing || !wsConnected) return false;
	const TickType_t timeout = timeoutMs ? pdMS_TO_TICKS(timeoutMs) : SendTimeout;
	return esp_websocket_client_send_text(ws, json, strlen(json), timeout) >= 0;
}

void VoiceLink::loop(){
	if(portal != nullptr){
		portal->tick();   // joystick, its watchdog and status reports (RC mode)
	}

	if(want == Want::Run && !stackOwned && nowMs() - lastStartAttempt >= RestartDelayMs){
		lastStartAttempt = nowMs();
		startStack();
	}

	// Reconnect watchdog. The websocket client was seen to stop retrying after a drop (8+ minutes,
	// no attempts, no errors) and a Wi-Fi re-join left the stuck client in place. If Wi-Fi has an
	// IP but the gateway hasn't been connected for SocketRebuildMs, or the IP was re-acquired,
	// throw the client away and start a fresh one. Only acts while disconnected.
	{
		Lock lock(stackMutex);
		const bool rejoined = rebuildSocket.exchange(false);
		if(stackOwned && netUp && !wsConnected){
			if(socketDownSince == 0){
				socketDownSince = nowMs();
			}
			if(rejoined || nowMs() - socketDownSince > SocketRebuildMs){
				CMF_LOG(VoiceLink, LogLevel::Warning, "Gateway unreachable for %lu ms%s, restarting websocket client",
						(unsigned long) (nowMs() - socketDownSince), rejoined ? " (Wi-Fi re-joined)" : "");
				stopSocket();
				startSocket();
				socketDownSince = nowMs();
			}
		}else{
			socketDownSince = 0;
		}
	}

	static uint32_t lastHeapLog = 0;
	if(nowMs() - lastHeapLog > 60000){
		lastHeapLog = nowMs();
		logHeap("minute");
	}

	Lock lock(streamMutex);
	if(!streaming || rb == nullptr) return;

	if(!wsConnected){
		// Socket dropped mid-utterance: give up, ListenState's timeout reports "not understood".
		streaming = false;
		rb = nullptr;
		return;
	}

	drain();

	if(endRequested){
		char msg[64];
		snprintf(msg, sizeof(msg), "{\"type\":\"end\",\"reason\":\"%s\"}", endReason.c_str());
		esp_websocket_client_send_text(ws, msg, strlen(msg), SendTimeout);
		streaming = false;
		endRequested = false;
		rb = nullptr;
	}
}

void VoiceLink::drain(){
	// Send everything buffered so far; the AFE refills at 32 ms per frame, this loop runs every 20 ms.
	while(true){
		size_t size = 0;
		void* data = xRingbufferReceiveUpTo(rb, &size, 0, SendChunk);
		if(data == nullptr) return;

		if(esp_websocket_client_send_bin(ws, (const char*) data, (int) size, SendTimeout) < 0){
			CMF_LOG(VoiceLink, LogLevel::Warning, "Audio send failed (%u bytes)", (unsigned) size);
		}else{
			lastSendOkMs = nowMs();
		}
		vRingbufferReturnItem(rb, data);
	}
}

bool VoiceLink::startStack(){
	Lock lock(stackMutex);
	if(stackOwned) return true;

	// CMF's WiFi (RC mode) creates and deletes the default loop itself; if it still exists
	// RC mode has not finished releasing the radio yet, so try again later.
	if(esp_event_loop_create_default() != ESP_OK){
		CMF_LOG(VoiceLink, LogLevel::Info, "Default event loop busy, retrying later");
		return false;
	}

	ESP_ERROR_CHECK(esp_netif_init());
	netif = esp_netif_create_default_wifi_sta();

	const wifi_init_config_t initCfg = WIFI_INIT_CONFIG_DEFAULT();
	if(esp_wifi_init(&initCfg) != ESP_OK){
		CMF_LOG(VoiceLink, LogLevel::Error, "esp_wifi_init failed");
		esp_netif_destroy_default_wifi(netif);
		netif = nullptr;
		esp_event_loop_delete_default();
		return false;
	}

	esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &VoiceLink::onWifiEvent, this, &wifiHandler);
	esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &VoiceLink::onWifiEvent, this, &ipHandler);

	wifi_config_t cfg = {};
	strlcpy((char*) cfg.sta.ssid, CONFIG_BB_VOICELINK_SSID, sizeof(cfg.sta.ssid));
	strlcpy((char*) cfg.sta.password, CONFIG_BB_VOICELINK_PASSWORD, sizeof(cfg.sta.password));
	cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

	esp_wifi_set_storage(WIFI_STORAGE_RAM);
	esp_wifi_set_mode(WIFI_MODE_STA);

	// The IDF default only scans channels 1-11; the lab AP sits on 13. AUTO still adopts the AP's
	// country IE when it advertises one.
	const wifi_country_t country = { .cc = "01", .schan = 1, .nchan = 13, .max_tx_power = 20, .policy = WIFI_COUNTRY_POLICY_AUTO };
	esp_wifi_set_country(&country);
	esp_wifi_set_config(WIFI_IF_STA, &cfg);
	esp_wifi_set_ps(WIFI_PS_MIN_MODEM); // required alongside BLE
	ESP_ERROR_CHECK(esp_wifi_start());

	stackOwned = true;
	CMF_LOG(VoiceLink, LogLevel::Info, "Wi-Fi started, joining %s", CONFIG_BB_VOICELINK_SSID);
	return true;
}

void VoiceLink::stopStack(){
	Lock lock(stackMutex);
	if(!stackOwned) return;

	stopSocket();
	netUp = false;

	esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifiHandler);
	esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ipHandler);
	wifiHandler = ipHandler = nullptr;

	esp_wifi_disconnect();
	esp_wifi_stop();
	esp_wifi_deinit();

	if(netif){
		esp_netif_destroy_default_wifi(netif);
		netif = nullptr;
	}

	esp_event_loop_delete_default();
	stackOwned = false;
	CMF_LOG(VoiceLink, LogLevel::Info, "Wi-Fi stopped");
}

void VoiceLink::startSocket(){
	if(ws) return;

	esp_websocket_client_config_t cfg = {};
	cfg.uri = CONFIG_BB_VOICELINK_URL;
	// Patience, not latency: audio still goes out as soon as it's captured. Wi-Fi modem sleep
	// (mandatory with BLE) produces multi-second stalls that a 5 s write timeout turned into a
	// torn-down connection while idle; ride those out, and reconnect quickly after a real drop.
	cfg.reconnect_timeout_ms = 1000;
	cfg.network_timeout_ms = 15000;
	cfg.pingpong_timeout_sec = 60;
	cfg.ping_interval_sec = 10;
	cfg.task_stack = 4 * 1024;
	cfg.buffer_size = 1024;

	ws = esp_websocket_client_init(&cfg);
	{
		Lock lock(streamMutex);
		closing = false;
	}
	esp_websocket_register_events(ws, WEBSOCKET_EVENT_ANY, &VoiceLink::onWsEvent, this);
	esp_websocket_client_start(ws);
}

void VoiceLink::stopSocket(){
	if(!ws) return;

	{
		// sendFrame()/sendText() run on other tasks and use ws under this lock: once `closing` is set
		// here, none is in flight and none will start, so the client can be destroyed outside the lock
		// (the websocket task may call back into code that takes streamMutex while it shuts down).
		Lock lock(streamMutex);
		streaming = false;
		rb = nullptr;
		closing = true;
	}

	esp_websocket_client_stop(ws);
	esp_websocket_client_destroy(ws);
	ws = nullptr;
	wsConnected = false;
	helloOk = false;
	bleQuiet(false); // paused for RC mode / on-device listening: the controller needs to find us
}

void VoiceLink::onWifiEvent(void* arg, esp_event_base_t base, int32_t id, void* data){
	auto self = static_cast<VoiceLink*>(arg);

	if(base == WIFI_EVENT && id == WIFI_EVENT_STA_START){
		esp_wifi_connect();
	}else if(base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED){
		auto ev = static_cast<wifi_event_sta_disconnected_t*>(data);
		CMF_LOG(VoiceLink, LogLevel::Warning, "Wi-Fi disconnected, reason %d, rssi %d", ev->reason, ev->rssi);
		self->netUp = false;
		if(self->want == Want::Run){
			esp_wifi_connect(); // driver paces its own retries
		}
	}else if(base == IP_EVENT && id == IP_EVENT_STA_GOT_IP){
		auto ev = static_cast<ip_event_got_ip_t*>(data);
		CMF_LOG(VoiceLink, LogLevel::Info, "Got IP " IPSTR, IP2STR(&ev->ip_info.ip));
		self->netUp = true;
		// The VoiceLink loop (re)creates the websocket client; on a re-join it replaces any client
		// left over from before the drop.
		self->rebuildSocket = true;
	}
}

void VoiceLink::onWsEvent(void* arg, esp_event_base_t base, int32_t id, void* data){
	auto self = static_cast<VoiceLink*>(arg);
	auto ev = static_cast<esp_websocket_event_data_t*>(data);

	switch(id){
		case WEBSOCKET_EVENT_CONNECTED: {
			self->wsConnected = true;
			self->helloOk = false;
			// "songs": the alarm songs this firmware can sing; the gateway falls back to speech for others.
			// "routines": routines the gateway may start with {"type":"routine","name":...}.
			// "camera": the robot can stream its camera through the gateway ({"type":"camera"}).
			// "portal": RC mode, the joystick, chat listening and status reports (Portal.h).
			char hello[208];
			snprintf(hello, sizeof(hello), "{\"type\":\"hello\",\"vocab\":\"%08lx\",\"n\":%d,\"songs\":[\"alarm\",\"timer\"],\"routines\":[\"summon\"],\"camera\":true,\"portal\":true}",
					 (unsigned long) vocabHash(), (int) activations.size());
			esp_websocket_client_send_text(self->ws, hello, strlen(hello), SendTimeout);
			CMF_LOG(VoiceLink, LogLevel::Info, "Gateway connected");
			break;
		}
		case WEBSOCKET_EVENT_DISCONNECTED:
		case WEBSOCKET_EVENT_CLOSED:
			if(self->wsConnected){
				CMF_LOG(VoiceLink, LogLevel::Info, "Gateway disconnected");
			}
			self->wsConnected = false;
			self->onLinkDropped();
			break;
		case WEBSOCKET_EVENT_DATA:
			// Text frames only; results are tiny so they always arrive unfragmented.
			if(ev->op_code == 0x1 && ev->payload_offset == 0 && ev->data_len == ev->payload_len){
				self->handleMessage(ev->data_ptr, ev->data_len);
			}
			break;
		default:
			break;
	}
}

bool VoiceLink::recentlyReady(uint32_t withinMs) const{
	return lastReadyMs != 0 && nowMs() - lastReadyMs < withinMs;
}

bool VoiceLink::comingUp(uint32_t graceMs) const{
	if(want != Want::Run){
		return false;
	}
	// Recently ready (reconnecting), or still within graceMs of (re)starting the Wi-Fi stack.
	return recentlyReady(graceMs) || nowMs() - lastStartAttempt < graceMs;
}

void VoiceLink::onLinkDropped(){
	bleQuiet(false);
	if(portal != nullptr){
		portal->linkLost();   // the robot always reconnects in VC, motors stopped
	}
	if(helloOk.exchange(false)){
		lastReadyMs = nowMs(); // "ready until just now"
		OnReady.broadcast(false);
	}
}

void VoiceLink::handleMessage(const char* data, int len){
	cJSON* root = cJSON_ParseWithLength(data, len);
	if(root == nullptr) return;

	const cJSON* type = cJSON_GetObjectItem(root, "type");
	if(cJSON_IsString(type) && strcmp(type->valuestring, "hello") == 0){
		helloOk = cJSON_IsTrue(cJSON_GetObjectItem(root, "ok"));
		if(helloOk){
			lastReadyMs = nowMs();
			CMF_LOG(VoiceLink, LogLevel::Info, "Gateway ready");
			bleQuiet(true);
			if(portal != nullptr){
				portal->linkReady();
			}
			if(!quietReconnect.exchange(false)){
				OnReady.broadcast(true);
			}
		}else{
			CMF_LOG(VoiceLink, LogLevel::Warning, "Gateway vocabulary mismatch, staying on-device");
		}
	}else if(cJSON_IsString(type) && strcmp(type->valuestring, "result") == 0){
		const bool recognized = cJSON_IsTrue(cJSON_GetObjectItem(root, "recognized"));
		const cJSON* index = cJSON_GetObjectItem(root, "index");
		const cJSON* text = cJSON_GetObjectItem(root, "text");
		const cJSON* replyItem = cJSON_GetObjectItem(root, "reply");
		const std::string transcript = cJSON_IsString(text) ? text->valuestring : "";
		const std::string reply = cJSON_IsString(replyItem) ? replyItem->valuestring : "";
		// "rewake": the user only said the wake word (e.g. during a follow-up window).
		const int idx = cJSON_IsTrue(cJSON_GetObjectItem(root, "rewake")) ? RewakeIndex
					  : cJSON_IsNumber(index) ? index->valueint : -1;

		CMF_LOG(VoiceLink, LogLevel::Info, "Result: rec=%d idx=%d '%s'%s", recognized, idx, transcript.c_str(), reply.empty() ? "" : " (+reply)");
		OnResult.broadcast(recognized, idx, transcript, reply);
	}else if(cJSON_IsString(type) && strcmp(type->valuestring, "camera") == 0){
		const cJSON* on = cJSON_GetObjectItem(root, "on");
		if(cameraStream != nullptr){
			if(cJSON_IsTrue(on)){
				cameraStream->start();   // stays on until the gateway says off
			}else{
				cameraStream->stop();
			}
		}
		CMF_LOG(VoiceLink, LogLevel::Info, "Camera stream %s by gateway", cJSON_IsTrue(on) ? "requested" : "stop requested");
	}else if(cJSON_IsString(type) && strcmp(type->valuestring, "drive") == 0){
		// The portal's joystick, ~10 a second while held (Portal stops the motors if they stop coming).
		const cJSON* l = cJSON_GetObjectItem(root, "l");
		const cJSON* r = cJSON_GetObjectItem(root, "r");
		if(portal != nullptr && cJSON_IsNumber(l) && cJSON_IsNumber(r)){
			portal->drive(l->valueint, r->valueint);
		}
	}else if(cJSON_IsString(type) && strcmp(type->valuestring, "mode") == 0){
		if(portal != nullptr){
			portal->setRc(cJSON_IsTrue(cJSON_GetObjectItem(root, "rc")), "portal");
		}
	}else if(cJSON_IsString(type) && strcmp(type->valuestring, "listen") == 0){
		// The portal's Listen button: picked up by IdleState like the other pushes.
		{
			Lock lock(pushMutex);
			pending = {};
			pending.chatListen = true;
			hasPending = true;
		}
		CMF_LOG(VoiceLink, LogLevel::Info, "Chat listen requested");
		OnPush.broadcast();
	}else if(cJSON_IsString(type) && strcmp(type->valuestring, "endpoint") == 0){
		const cJSON* reason = cJSON_GetObjectItem(root, "reason");
		OnEndpoint.broadcast(!(cJSON_IsString(reason) && strcmp(reason->valuestring, "nospeech") == 0));
	}else if(cJSON_IsString(type) && (strcmp(type->valuestring, "say") == 0 || strcmp(type->valuestring, "action") == 0 ||
										   strcmp(type->valuestring, "alarm") == 0 || strcmp(type->valuestring, "routine") == 0)){
		const cJSON* text = cJSON_GetObjectItem(root, "text");
		const cJSON* index = cJSON_GetObjectItem(root, "index");
		const cJSON* song = cJSON_GetObjectItem(root, "song");
		const cJSON* name = cJSON_GetObjectItem(root, "name");
		const bool routine = strcmp(type->valuestring, "routine") == 0;
		{
			Lock lock(pushMutex);
			pending.say = cJSON_IsString(text) ? text->valuestring : "";
			pending.scenario = cJSON_IsNumber(index) && index->valueint < (int) activations.size() ? index->valueint : -1;
			pending.alarm = strcmp(type->valuestring, "alarm") == 0;
			pending.song = cJSON_IsString(song) ? song->valuestring : "";
			pending.routine = routine && cJSON_IsString(name) ? name->valuestring : "";
			pending.chatListen = false;
			hasPending = !pending.say.empty() || pending.scenario >= 0 || pending.alarm || !pending.routine.empty();
		}
		CMF_LOG(VoiceLink, LogLevel::Info, "Push from gateway: say=%u chars, scenario=%d%s%s%s%s", (unsigned) pending.say.size(), pending.scenario,
				pending.alarm ? ", alarm " : "", pending.song.c_str(), pending.routine.empty() ? "" : ", routine ", pending.routine.c_str());
		OnPush.broadcast();
	}

	cJSON_Delete(root);
}
