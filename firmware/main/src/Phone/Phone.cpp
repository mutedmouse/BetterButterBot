#include "Phone.h"
#include <functional>
#include <esp_log.h>

Phone::Phone(BLE::Server* server, BLE::Client* client){
	ancs = newObject<ANCS::Client>(this, client);
	ams = newObject<AMS::Client>(this, client);
	cTime = newObject<CurrentTime>(this, client);
	android = newObject<Android>(this, server);

	auto reg = [this](NotifSource* src){
		src->setOnConnect([this, src](){ onConnect(src); });
		src->setOnDisconnect([this, src](){ onDisconnect(src); });
		src->setOnNotifAdd([this](Notif notif){ onAdd(std::move(notif)); });
		src->setOnNotifModify([this](Notif notif){ onModify(std::move(notif)); });
		src->setOnNotifRemove([this](uint32_t id){ onRemove(id); });
	};

	reg(*ancs);
	reg(*android);

	auto mreg = [this](MediaSource* src){
		src->setOnConnect([this, src](){ onMediaConnect(src); });
		src->setOnDisconnect([this, src](){ onMediaDisconnect(src); });
		src->setOnMediaInfo([this](const MediaInfo& media){ onMediaInfo(media); });
		src->setOnMediaState([this](MediaState state){ onMediaState(state); });
	};

	mreg(*android);
	mreg(*ams);
}

bool Phone::isConnected(){
	return current != nullptr;
}

Phone::PhoneType Phone::getPhoneType(){
	if(current == *ancs) return PhoneType::IPhone;
	else if(current == *android) return PhoneType::Android;
	else return PhoneType::None;
}

auto Phone::findNotif(uint32_t id){
	return std::find_if(notifs.begin(), notifs.end(), [id](const auto& notif){ return notif.uid == id; });
}

Notif Phone::getNotif(uint32_t uid){
	auto notif = findNotif(uid);
	if(notif == notifs.end()) return {};
	return *notif;
}

std::vector<Notif> Phone::getNotifs(){
	return std::vector<Notif>(notifs.cbegin(), notifs.cend());
}

uint32_t Phone::getNotifsCount() const{
	return notifs.size();
}

Notif Phone::getCall(){
	for(const auto& notif : notifs){
		if(notif.category == Notif::Category::IncomingCall){
			return notif;
		}
	}

	return {};
}

void Phone::callIgnore(uint32_t uid){
	auto notif = findNotif(uid);
	if(notif == notifs.end()) return;
	notifs.erase(notif);
}

void Phone::callReject(uint32_t uid){
	auto notif = findNotif(uid);
	if(notif == notifs.end()) return;
	notifs.erase(notif);
	if(current == *android){
		current->actionNeg(uid);
	}
}

const MediaInfo& Phone::getMedia() const{
	return currentMedia;
}

MediaState Phone::getMediaState(){
	return currentMediaState;
}

void Phone::doPos(uint32_t id){
	if(current == nullptr || findNotif(id) == notifs.end()) return;
	current->actionPos(id);
}

void Phone::doNeg(uint32_t id){
	if(current == nullptr || findNotif(id) == notifs.end()) return;
	current->actionNeg(id);
}

void Phone::doMediaPlay(){
	if(mediaCurrent == nullptr) return;
	mediaCurrent->mediaPlay();
}

void Phone::doMediaPause(){
	if(mediaCurrent == nullptr) return;
	mediaCurrent->mediaPause();
}

void Phone::doMediaNext(){
	if(mediaCurrent == nullptr) return;
	mediaCurrent->mediaNext();
}

void Phone::doMediaPrev(){
	if(mediaCurrent == nullptr) return;
	mediaCurrent->mediaPrev();
}

void Phone::onConnect(NotifSource* src){
	current = src;
	onConnEvent.broadcast(ConnEvent::Connected);

	if(!notifs.empty()){
		notifs.clear();
		onNotifEvent.broadcast(NotifEvent { .action = NotifEvent::Cleared });
	}

	currentMediaState = MediaState::Stopped;
	currentMedia = {};
}

void Phone::onDisconnect(NotifSource* src){
	if(current != src) return;
	onConnEvent.broadcast(ConnEvent::Disconnected);
	current = nullptr;

	if(!notifs.empty()){
		notifs.clear();
		onNotifEvent.broadcast(NotifEvent { .action = NotifEvent::Cleared });
	}
}

void Phone::onMediaConnect(MediaSource* src){
	mediaCurrent = src;
	onMediaConnEvent.broadcast(ConnEvent::Connected);
}

void Phone::onMediaDisconnect(MediaSource* src){
	if(mediaCurrent != src) return;
	mediaCurrent = nullptr;
	onMediaConnEvent.broadcast(ConnEvent::Disconnected);

	currentMediaState = MediaState::Stopped;
	currentMedia = {};
}

void Phone::onMediaInfo(const MediaInfo& media){
	currentMedia = media;
	onMediaInfoEvent.broadcast(media);
}

void Phone::onMediaState(MediaState state){
	currentMediaState = state;
	onMediaStateEvent.broadcast(state);
}

void Phone::onAdd(Notif notif){
	if(notif.title.empty() && notif.message.empty()) return;

	if(findNotif(notif.uid) != notifs.end()){
		onModify(std::move(notif));
		return;
	}

	while(notifs.size() >= MaxNotifs){
		const auto notif = notifs.front();
		notifs.pop_front();
		onNotifEvent.broadcast(NotifEvent{ .action = NotifEvent::Removed, .id = notif.uid });
	}

	notifs.push_back(notif); // TODO: send whole notification, otherwise (by using a mutex) all newly unlocked tasks will rush after getNotif, and promptly get locked again by the mutex
	onNotifEvent.broadcast(NotifEvent{ .action = NotifEvent::Added, .id = notif.uid });
}

void Phone::onModify(Notif notif){
	auto saved = findNotif(notif.uid);
	if(saved == notifs.end()){
		onAdd(std::move(notif));
		return;
	}

	*saved = notif;
	onNotifEvent.broadcast(NotifEvent{ .action = NotifEvent::Changed, .id = notif.uid });
}

void Phone::onRemove(uint32_t id){
	auto notif = findNotif(id);
	if(notif == notifs.end()) return;

	notifs.erase(notif);
	onNotifEvent.broadcast(NotifEvent{ .action = NotifEvent::Removed, .id = id });
}

void Phone::findPhoneStart(){
	if(current != *android) return;
	android->findPhoneStart();
}

void Phone::findPhoneStop(){
	if(current != *android) return;
	android->findPhoneStop();
}

bool Phone::findPhoneActive(){
	if(current != *android) return false;
	return android->findPhoneActive();
}
