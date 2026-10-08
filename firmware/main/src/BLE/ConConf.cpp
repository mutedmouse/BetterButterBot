#include <freertos/FreeRTOS.h>
#include "ConConf.h"
#include <cstring>

// TODO: add timout for configuration

void ConConf::conf(const esp_ble_conn_update_params_t& params){
	std::lock_guard lock(confMut);

	if(hasCurrent()){
		enqueuePending(params);
		return;
	}

	send(params);
}

void ConConf::confDone(bool success){
	std::lock_guard lockConf(confMut);

	esp_ble_conn_update_params_t next;
	if(popPending(next)){
		send(next);
		return;
	}

	if(!success && hasCurrent()){
		send(current);
		return;
	}

	current = {};

	std::lock_guard lockWait(waitMut);
	for(const auto& sem : waitSems){
		xSemaphoreGive(sem);
		vSemaphoreDelete(sem);
	}
	waitSems.clear();
}

void ConConf::send(esp_ble_conn_update_params_t params){
	current = params;
	auto status = esp_ble_gap_update_conn_params(&current);
	int tries = 1;
	while(status != ESP_BT_STATUS_SUCCESS && tries < 3){
		vTaskDelay(10);
		status = esp_ble_gap_update_conn_params(&current);
		tries++;
	}

	if(status != ESP_BT_STATUS_SUCCESS){
		current = {};
	}
}

void ConConf::reset(){
	std::lock_guard lockConf(confMut);
	current = {};
	pending.clear();

	std::lock_guard lockWait(waitMut);
	for(const auto& sem : waitSems){
		xSemaphoreGive(sem);
		vSemaphoreDelete(sem);
	}
	waitSems.clear();
}

void ConConf::waitDone(TickType_t wait){
	SemaphoreHandle_t sem = xSemaphoreCreateBinary();

	waitMut.lock();
	waitSems.insert(sem);
	waitMut.unlock();

	xSemaphoreTake(sem, wait);
}

bool ConConf::hasCurrent() const{
	return current.timeout != 0;
}

void ConConf::enqueuePending(const esp_ble_conn_update_params_t& params){
	for(auto& p : pending){
		if(memcmp(p.bda, params.bda, 6) == 0){
			p = params;
			return;
		}
	}
	pending.push_back(params);
}

bool ConConf::popPending(esp_ble_conn_update_params_t& out){
	if(pending.empty()) return false;
	out = pending.front();
	pending.erase(pending.begin());
	return true;
}
