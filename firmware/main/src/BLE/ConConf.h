#ifndef CLOCKSTAR_FIRMWARE_CONCONF_H
#define CLOCKSTAR_FIRMWARE_CONCONF_H

#include <esp_gap_ble_api.h>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

class ConConf {
public:

	void conf(const esp_ble_conn_update_params_t& params);
	void waitDone(TickType_t wait);
	void confDone(bool success);
	void reset();

private:
	esp_ble_conn_update_params_t current = {};
	// Per-peer pending updates. esp_ble_gap_update_conn_params accepts only one in-flight
	// update at a time, so additional conf() calls (e.g. goLowPow iterating over both peers)
	// queue here keyed by bda — a repeat for the same peer replaces its prior pending entry.
	std::vector<esp_ble_conn_update_params_t> pending;
	std::mutex confMut;

	bool hasCurrent() const;

	void send(esp_ble_conn_update_params_t params);
	void enqueuePending(const esp_ble_conn_update_params_t& params);
	bool popPending(esp_ble_conn_update_params_t& out);

	std::unordered_set<SemaphoreHandle_t> waitSems;
	std::mutex waitMut;

};


#endif //CLOCKSTAR_FIRMWARE_CONCONF_H
