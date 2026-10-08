#ifndef CLOCKSTAR_FIRMWARE_PHONE_H
#define CLOCKSTAR_FIRMWARE_PHONE_H

#include "Android.h"
#include "ANCS/Client.h"
#include "AMS/Client.h"
#include "CurrentTime.h"
#include "NotifSource.h"
#include "MediaSource.h"
#include <deque>
#include <cstdint>
#include <optional>
#include <Event/EventBroadcaster.h>

class Phone : public Object {
	GENERATED_BODY(Phone, Object, CONSTRUCTOR_PACK(BLE::Server*, BLE::Client*))
public:

	enum class PhoneType {
		None, Android, IPhone
	};

	struct Event {
		enum { Connected, Disconnected, Added, Changed, Removed, Cleared,
		      MediaConnected, MediaDisconnected, MediaState, MediaInfo } action;
		union {
			struct { uint32_t id; } addChgRem;
			PhoneType phoneType;
			::MediaState mediaState;
		} data;
	};

	static constexpr size_t MaxNotifs = 20;

	enum class ConnEvent { Connected, Disconnected };
	DECLARE_EVENT(OnConnEvent, Phone, ConnEvent);
	OnConnEvent onConnEvent{ this };

	DECLARE_EVENT(OnMediaConnEvent, Phone, ConnEvent);
	OnMediaConnEvent onMediaConnEvent{ this };

	struct NotifEvent {
		enum { Added, Changed, Removed, Cleared } action;
		uint32_t id;
		PhoneType phoneType;
	};
	DECLARE_EVENT(OnNotifEvent, Phone, NotifEvent);
	OnNotifEvent onNotifEvent{ this };

	DECLARE_EVENT(OnMediaStateEvent, Phone, MediaState);
	OnMediaStateEvent onMediaStateEvent{ this };

	DECLARE_EVENT(OnMediaInfoEvent, Phone, MediaInfo);
	OnMediaInfoEvent onMediaInfoEvent{ this };

	Phone(BLE::Server* server, BLE::Client* client);

	bool isConnected();
	PhoneType getPhoneType();

	Notif getNotif(uint32_t uid);
	std::vector<Notif> getNotifs();
	uint32_t getNotifsCount() const;

	Notif getCall();
	void callIgnore(uint32_t uid);
	void callReject(uint32_t uid);

	void doPos(uint32_t id);
	void doNeg(uint32_t id);

	// Media controls
	void doMediaPlay();
	void doMediaPause();
	void doMediaNext();
	void doMediaPrev();

	const MediaInfo& getMedia() const;
	MediaState getMediaState();

	void findPhoneStart();
	void findPhoneStop();
	bool findPhoneActive();

private:
	StrongObjectPtr<ANCS::Client> ancs;
	StrongObjectPtr<AMS::Client> ams;
	StrongObjectPtr<CurrentTime> cTime;
	StrongObjectPtr<Android> android;

	NotifSource* current = nullptr;
	MediaSource* mediaCurrent = nullptr;

	MediaInfo currentMedia;
	MediaState currentMediaState = MediaState::Stopped;

	void onConnect(NotifSource* src);
	void onDisconnect(NotifSource* src);

	void onMediaConnect(MediaSource* src);
	void onMediaDisconnect(MediaSource* src);

	void onMediaInfo(const MediaInfo& media);
	void onMediaState(MediaState state);

	void onAdd(Notif notif);
	void onModify(Notif notif);
	void onRemove(uint32_t id);

	// TODO: const size backed dequeue
	// notifs is mutated by NotifSource callbacks (ANCS worker task and BLE/BTC
	// task via Android) and read by UI threads via getNotif*/getCall. Not
	// currently synchronized — covered by a separate ticket.
	std::deque<Notif> notifs;

	auto findNotif(uint32_t id);
};


#endif //CLOCKSTAR_FIRMWARE_PHONE_H
