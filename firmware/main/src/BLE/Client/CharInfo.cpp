#include "../Client.h"
#include <Log/Log.h>

DEFINE_LOG(BLE_Client_CharInfo)

BLE::Client::CharInfo::CharInfo(const Client* client, uint16_t hndl) : client(client), hndl(hndl){

}

void BLE::Client::CharInfo::regForNotify(){
	CMF_LOG(BLE_Client_CharInfo, LogLevel::Verbose, "Registering for notify");
	esp_ble_gattc_register_for_notify(client->iface.hndl, (uint8_t*) client->con.addr, hndl);
}

void BLE::Client::CharInfo::writeDescr(esp_bt_uuid_t uuid, uint8_t* data, size_t len){
	CMF_LOG(BLE_Client_CharInfo, LogLevel::Verbose, "Write to descriptors");

	esp_gattc_descr_elem_t descr;
	uint16_t count = 1;
	auto ret = esp_ble_gattc_get_descr_by_char_handle(client->iface.hndl, client->con.hndl, hndl, uuid, &descr, &count);

	if(ret != ESP_GATT_OK){
		CMF_LOG(BLE_Client_CharInfo, LogLevel::Error, "Descriptor retrieve error %d", ret);
		return;
	}

	if(count == 0){
		CMF_LOG(BLE_Client_CharInfo, LogLevel::Error, "Retrieved 0 descriptors");
		return;
	}

	// TODO: this function passes the data ptr to the BT thread
	esp_ble_gattc_write_char_descr(client->iface.hndl, client->con.hndl, descr.handle, len, data, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
}

void BLE::Client::CharInfo::write(uint8_t* data, size_t len, bool needResponse){
	esp_ble_gattc_write_char(client->iface.hndl, client->con.hndl, hndl, len, data, needResponse ? ESP_GATT_WRITE_TYPE_RSP : ESP_GATT_WRITE_TYPE_NO_RSP, ESP_GATT_AUTH_REQ_NONE);
}

void BLE::Client::CharInfo::read(){
	esp_ble_gattc_read_char(client->iface.hndl, client->con.hndl, hndl, ESP_GATT_AUTH_REQ_NONE);
}
