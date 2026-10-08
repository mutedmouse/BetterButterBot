#include "ConMan.h"
#include <cstring>
#include <Log/Log.h>

DEFINE_LOG(BLE_ConMan)

ConManager ConMan;

void ConManager::confDone(bool success){
	conConf.confDone(success);
}

void ConManager::start(){
	startAdv();
}

void ConManager::connect(const esp_bd_addr_t addr){
	// Bluedroid implicitly stops advertising on a connection and does not always fire
	// ADV_STOP_COMPLETE_EVT for it. Sync the host-side hint here so the startAdv() /
	// stopAdv() short-circuits below match the real radio state.
	advertising = false;

	Peer* p = findPeer(addr);
	if(!p){
		for(auto& slot : peers){
			if(!slot.used){
				slot.used = true;
				memcpy(slot.addr, addr, 6);
				p = &slot;
				break;
			}
		}
	}
	if(!p){
		CMF_LOG(BLE_ConMan, LogLevel::Warning, "connect: capacity exceeded; bluedroid accepted more than MaxPeers");
		return;
	}

	setCon(p->addr);

	// Keep advertising until both slots are filled; the controller and phone can connect
	// in either order, and the second peer is the one that finally stops the advert.
	if(peerCount() >= MaxPeers){
		stopAdv();
	}else{
		startAdv();
	}
}

void ConManager::disconnect(const esp_bd_addr_t addr){
	if(Peer* p = findPeer(addr)){
		p->used = false;
		memset(p->addr, 0, 6);
	}

	// Only wipe in-flight conn-param state when the last peer has gone; otherwise the
	// remaining peer's pending conf would be cleared along with the disconnecting one's.
	if(peerCount() == 0){
		conConf.reset();
	}

	// Decide based on current peer count rather than assuming the disconnect freed a slot:
	// if `addr` wasn't recorded (e.g. a connect that hit the capacity-exceeded path) the
	// slots are still full and we must not re-start advertising.
	if(peerCount() < MaxPeers){
		startAdv();
	}
}

void ConManager::goLowPow(){
	lowPow = true;
	for(const auto& p : peers){
		if(p.used) setCon(p.addr);
	}
	if(peerCount() < MaxPeers){
		startAdv();
	}
}

void ConManager::goHiPow(){
	lowPow = false;
	for(const auto& p : peers){
		if(p.used) setCon(p.addr);
	}
	if(peerCount() < MaxPeers){
		startAdv();
	}
}

void ConManager::setQuiet(bool value){
	if(quiet.exchange(value) == value) return;
	CMF_LOG(BLE_ConMan, LogLevel::Info, "BLE advertising %s (LAN voice %s)", value ? "paused" : "resumed", value ? "up" : "down");
	if(value){
		esp_ble_gap_stop_advertising();   // onAdvStopComplete sees `quiet` and doesn't restart
	}else if(wantAdv){
		esp_ble_gap_start_advertising((esp_ble_adv_params_t*) (lowPow ? &AdvLowPow : &AdvHiPow));
	}
}

void ConManager::startAdv(){
	wantAdv = true;
	if(quiet) return;   // remembered in wantAdv; setQuiet(false) resumes
	if(advertising) return;
	auto err = esp_ble_gap_start_advertising((esp_ble_adv_params_t*) (lowPow ? &AdvLowPow : &AdvHiPow));
	if(err != ESP_OK){
		CMF_LOG(BLE_ConMan, LogLevel::Warning, "esp_ble_gap_start_advertising failed: 0x%x", err);
	}
}

void ConManager::stopAdv(){
	wantAdv = false;
	if(!advertising) return;
	auto err = esp_ble_gap_stop_advertising();
	if(err != ESP_OK){
		CMF_LOG(BLE_ConMan, LogLevel::Warning, "esp_ble_gap_stop_advertising failed: 0x%x", err);
	}
}

void ConManager::onAdvStartComplete(bool success){
	if(!success){
		advertising = false;
		// Honest divergence: GAP rejected our start. Don't auto-retry here — the caller's next
		// state change (next connect/disconnect/goLowPow/goHiPow) will issue another start.
		return;
	}
	advertising = true;

	// If our intent has flipped to "stop" (or LAN quiet began) between issuing the start and the
	// event arriving, pull the trigger now to converge.
	if(!wantAdv || quiet){
		auto err = esp_ble_gap_stop_advertising();
		if(err != ESP_OK){
			CMF_LOG(BLE_ConMan, LogLevel::Warning, "esp_ble_gap_stop_advertising failed: 0x%x", err);
		}
	}
}

void ConManager::onAdvStopComplete(bool success){
	if(success){
		advertising = false;
	}

	// If our intent flipped to "start" between issuing the stop and the event arriving, resync.
	if(wantAdv && !advertising && !quiet){
		startAdv();
	}
}

void ConManager::setCon(const esp_bd_addr_t addr){
	/* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
	esp_ble_conn_update_params_t params = {};
	memcpy(&params, lowPow ? &ConLowPow : &ConHiPow, sizeof(esp_ble_conn_update_params_t));
	memcpy(params.bda, addr, 6);
	conConf.conf(params);
}

size_t ConManager::peerCount() const{
	size_t n = 0;
	for(const auto& p : peers){
		if(p.used) ++n;
	}
	return n;
}

ConManager::Peer* ConManager::findPeer(const esp_bd_addr_t addr){
	for(auto& p : peers){
		if(p.used && memcmp(p.addr, addr, 6) == 0) return &p;
	}
	return nullptr;
}
