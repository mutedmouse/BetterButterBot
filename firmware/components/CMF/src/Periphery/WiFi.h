#ifndef WIFI_H
#define WIFI_H

#include <esp_netif_types.h>
#include <semaphore>
#include <esp_wifi_types_generic.h>
#include "Object/Object.h"
#include "Event/EventBroadcaster.h"

enum class WiFiType : uint8_t {
    None,
    AccessPoint,
    Station,
    AccessPointStation,
    NeighbourhoodAreaNetwork,
    Count
};

class WiFi : public Object{
    GENERATED_BODY(WiFi, Object, void)

public:
    // STATION EVENTS
#ifdef CONFIG_CMF_WIFI_STA_SUPPORT
    DECLARE_EVENT(ScanDoneEvent, WiFi, uint32_t, uint8_t, uint8_t);
    ScanDoneEvent OnScanDone{this};

    DECLARE_EVENT(StationStartEvent, WiFi);
    StationStartEvent OnStationStart{this};

    DECLARE_EVENT(StationStopEvent, WiFi);
    StationStopEvent OnStationStop{this};

    DECLARE_EVENT(StationConnectedEvent, WiFi, /*std::string, std::string,*/ uint8_t, wifi_auth_mode_t, uint16_t);
    StationConnectedEvent OnStationConnected{this};

    DECLARE_EVENT(StationDisconnectedEvent, WiFi, /*std::string, std::string,*/ uint8_t, int8_t);
    StationDisconnectedEvent OnStationDisconnected{this};

    DECLARE_EVENT(StationAuthModeChangedEvent, WiFi, wifi_auth_mode_t, wifi_auth_mode_t);
    StationAuthModeChangedEvent OnStationAuthModeChanged{this};
#endif

    // ACCESS POINT EVENTS
#ifdef CONFIG_CMF_WIFI_AP_SUPPORT
    DECLARE_EVENT(AccessPointStartEvent, WiFi);
    AccessPointStartEvent OnAccessPointStart{this};

    DECLARE_EVENT(AccessPointStopEvent, WiFi);
    AccessPointStopEvent OnAccessPointStop{this};

    DECLARE_EVENT(AccessPointConnectionEvent, WiFi, /*std::string,*/ uint8_t, bool);
    AccessPointConnectionEvent OnAccessPointConnection{this};

    DECLARE_EVENT(AccessPointDisconnectionEvent, WiFi, /*std::string,*/ uint8_t, bool, uint8_t);
    AccessPointDisconnectionEvent OnAccessPointDisconnection{this};
#endif

public:
    WiFi() noexcept;
	virtual ~WiFi() override;

    // ACCESS POINT FUNCTIONS
#ifdef CONFIG_CMF_WIFI_AP_SUPPORT
	void startAccessPoint(const std::string& name, const std::string& password, uint8_t channel = 1, uint8_t maxConnections = 1) noexcept;

    bool isHidden() const noexcept;
    void setHidden(bool hidden) const noexcept;
    void setNetworkParameters(const std::string& name, const std::string& password) const noexcept;
#endif

    // STATION FUNCTIONS
#ifdef CONFIG_CMF_WIFI_STA_SUPPORT
	void startStation() noexcept;

    void resetIPInfo() const noexcept;
    void connect() const noexcept;
    void startScanning(wifi_scan_type_t scanType = WIFI_SCAN_TYPE_PASSIVE, wifi_scan_time_t scanTime = {.active = {.min = 0, .max = 1500}, .passive = 1500}) const noexcept;
    void stopScanning() const noexcept;
    void disconnect() const noexcept;
    int getConnectionRSSI() const noexcept;
    void setTargetParameters(const std::string& ssid, const std::string& password) const noexcept;
    std::vector<wifi_ap_record_t> getAPRecords(size_t maxScanSize) const noexcept;
#endif

private:
    WiFiType type = WiFiType::None;
    esp_event_handler_instance_t nativeEventHandler;
    std::binary_semaphore initSemaphore { 0 };
	esp_netif_t* netif = nullptr;

private:
    void onNativeEvent(uint32_t id, void* data) noexcept;
    void createNetif() noexcept;
};

#endif //WIFI_H