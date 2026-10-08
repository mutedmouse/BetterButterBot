#include "WiFiStation.h"
#include <Periphery/WiFi.h>

DEFINE_LOG(WiFiStation)

WiFiStation::WiFiStation(WiFi* wifi, const char* ssid, const char* password) noexcept : hysteresis({0, 20, 40, 60, 80}, 1), wifi(wifi){
	memcpy(SSID, ssid, sizeof(RCData::SSID));
	memcpy(this->password, password, sizeof(RCData::password));

    if(wifi == nullptr){
    	CMF_LOG(WiFiStation, Warning, "Wifi is nullptr in constructor.");
        return;
    }

    wifi->startStation();

    wifi->OnScanDone.bind(this, &WiFiStation::onScanDone);
    wifi->OnStationConnected.bind(this, &WiFiStation::onConnect);
    wifi->OnStationDisconnected.bind(this, &WiFiStation::onDisconnect);
}

WiFiStation::~WiFiStation() noexcept{
    if(wifi == nullptr){
        return;
    }

    wifi->OnScanDone.unbind(this);
    wifi->OnStationConnected.unbind(this);
    wifi->OnStationDisconnected.unbind(this);
}

void WiFiStation::connect() noexcept{
    if(state != State::Disconnected && state != State::Connected){
        return;
    }

    if(wifi == nullptr){
    	CMF_LOG(WiFiStation, Warning, "Wifi is nullptr in connect.");
        return;
    }

    state = State::Scanning;
    wifi->startScanning(WIFI_SCAN_TYPE_PASSIVE, {.active = {.min = 0, .max = 0}, .passive = 0});
}

void WiFiStation::disconnect() noexcept{
    if(state == State::Disconnected){
        return;
    }

    if(wifi == nullptr){
    	CMF_LOG(WiFiStation, Warning, "Wifi is nullptr in disconnect.");
        return;
    }

    switch (state){
        case State::Connected:{
            wifi->disconnect();
            break;
        }
        case State::Connecting:{
            state = State::Disconnected;
            wifi->disconnect();
            break;
        }
        case State::Disconnected:{
            break;
        }
        case State::Scanning:{
            state = State::ConnectionAbort;
            wifi->stopScanning();
            break;
        }
        default:{
            break;
        }
    }
}

WiFiStation::State WiFiStation::getState() const noexcept{
    return state;
}

WiFiStation::ConnectionStrength WiFiStation::getConnectionStrength() noexcept{
    if(wifi == nullptr){
        return ConnectionStrength::None;
    }

    hysteresis.update(-wifi->getConnectionRSSI());
    return static_cast<ConnectionStrength>(hysteresis.get());
}

void WiFiStation::onScanDone(uint32_t status, uint8_t number, uint8_t id) noexcept{
    if(state == State::ConnectionAbort){
        state = State::Disconnected;
        OnStationEvent.broadcast(EventType::Connect/*, ""*/, false);
        return;
    }

    if(wifi == nullptr){
        return;
    }

    state = State::Connecting;
    connectTries = 0;

	CMF_LOG(WiFiStation, LogLevel::Info, "Attempting connect on SSID: %s\n", SSID);
    wifi->setTargetParameters(SSID, password);
    wifi->connect();
}

void WiFiStation::onConnect(/*std::string ssid, std::string mac,*/ uint8_t channel, wifi_auth_mode_t authMode, uint16_t aid) noexcept{
	CMF_LOG(WiFiStation, LogLevel::Info, "onConnected event");
    if(wifi == nullptr){
        return;
    }

    wifi->resetIPInfo();

    if(state == State::Connected){
		CMF_LOG(WiFiStation, LogLevel::Info, "early return, already connected");
	    OnStationEvent.broadcast(EventType::Connect/*, mac*/, false);
        return;
    }

    state = State::Connected;

    OnStationEvent.broadcast(EventType::Connect/*, mac*/, true);
}

void WiFiStation::onDisconnect(/*std::string ssid, std::string mac,*/ uint8_t reason, int8_t rssi) noexcept{
	CMF_LOG(WiFiStation, LogLevel::Info, "onDisconnect event");
    if(wifi == nullptr){
        return;
    }

    if(state == State::Connecting){
        if(++connectTries <= ConnectRetries){
            wifi->connect();
        }else{
            state = State::Disconnected;

            OnStationEvent.broadcast(EventType::Connect/*, mac*/, false);
        }

        return;
    }

    if(state == State::ConnectionAbort){
        state = State::Disconnected;
        OnStationEvent.broadcast(EventType::Connect/*, mac*/, false);
        return;
    }

    state = State::Disconnected;

    OnStationEvent.broadcast(EventType::Disconnect/*, mac*/, false);
}
