#ifndef BUTTERBOT_FIRMWARE_WIFISTATION_H
#define BUTTERBOT_FIRMWARE_WIFISTATION_H

#include <CtrlData.h>
#include <esp_wifi_types_generic.h>
#include <Event/EventBroadcaster.h>
#include <Object/Object.h>
#include "Hysteresis.h"

class WiFiStation : public Object {
    GENERATED_BODY(WiFiStation, Object, CONSTRUCTOR_PACK(class WiFi*, const char*, const char*))

public:
    enum class State : uint8_t {
        Connected,
        Connecting,
        Disconnected,
        Scanning,
        ConnectionAbort
    };

    enum class EventType : uint8_t {
        Connect,
        Disconnect
    };

    enum class ConnectionStrength : uint8_t {
    	High = 0,
		Medium = 1,
    	Low = 2,
    	VeryLow = 3,
        None = 4,
    	COUNT
    };

    DECLARE_EVENT(StationEvent, WiFiStation, EventType, /*std::string,*/ bool);
    StationEvent OnStationEvent{this};

public:
    WiFiStation(WiFi* wifi, const char* ssid, const char* password) noexcept;
    ~WiFiStation() noexcept override;

    void connect() noexcept;
    void disconnect() noexcept;

    State getState() const noexcept;

    ConnectionStrength getConnectionStrength() noexcept;

private:
    Hysteresis<static_cast<size_t>(ConnectionStrength::COUNT)> hysteresis;
	StrongObjectPtr<WiFi> wifi;
	char SSID[sizeof(RCData::password)];
	char password[sizeof(RCData::password)];
    State state = State::Disconnected;
    int connectTries = 0;

    static constexpr int ConnectRetries = 2;
    static constexpr uint16_t ScanListSize = 12;

private:
    void onScanDone(uint32_t status, uint8_t number, uint8_t id) noexcept;
    void onConnect(/*std::string ssid, std::string mac,*/ uint8_t channel, wifi_auth_mode_t authMode, uint16_t aid) noexcept;
    void onDisconnect(/*std::string ssid, std::string mac,*/ uint8_t reason, int8_t rssi) noexcept;
};

#endif //BUTTERBOT_FIRMWARE_WIFISTATION_H