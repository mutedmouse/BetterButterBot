#include "../Client.h"
#include <Log/Log.h>

DEFINE_LOG(BLE_Client_Char)

BLE::Client::Char::Char(esp_bt_uuid_t uuid, esp_gatt_char_prop_t props) : uuid(uuid), props(props), notifQueue((props & ESP_GATT_CHAR_PROP_BIT_NOTIFY) ? 12 : 1){

}

std::unique_ptr<BLE::Client::Char::Notif> BLE::Client::Char::getNextNotif(TickType_t wait){
	if(!(props & ESP_GATT_CHAR_PROP_BIT_NOTIFY) || !(remoteProps & ESP_GATT_CHAR_PROP_BIT_NOTIFY)){
		CMF_LOG(BLE_Client_Char, LogLevel::Warning,"Requesting notify, but NOTIFY property bit isn't");
		return nullptr;
	}

	return notifQueue.get(wait);
}

void BLE::Client::Char::setOnConnectedCb(ConnectedCB cb){
	onConnectedCB = cb;
}

bool BLE::Client::Char::established(){
	return chr != nullptr;
}

bool BLE::Client::Char::connected(){
	return configsDone.size() == (int) Config::COUNT;
}

void BLE::Client::Char::establish(std::unique_ptr<CharInfo> info, esp_gatt_char_prop_t rProps){
	CMF_LOG(BLE_Client_Char, LogLevel::Info,"Established");
	chr = std::move(info);

	remoteProps = rProps;

	if(props & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
		if(remoteProps & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
			chr->regForNotify();
		}else{
			CMF_LOG(BLE_Client_Char, LogLevel::Warning,"Have NOTIFY bit set, but remote characteristic doesn't");
		}
	}else{
		configDone(Config::Notify);
	}

	if(props & ESP_GATT_CHAR_PROP_BIT_WRITE){
		if(!(remoteProps & ESP_GATT_CHAR_PROP_BIT_WRITE)){
			CMF_LOG(BLE_Client_Char, LogLevel::Warning,"Have WRITE bit set, but remote characteristic doesn't");
		}
	}
}

void BLE::Client::Char::close(){
	if(!chr) return;
	CMF_LOG(BLE_Client_Char, LogLevel::Info,"Closed");

	chr.reset();
	remoteProps = 0;
	configsDone.clear();

	while(notifQueue.get(0) != nullptr){}
}

void BLE::Client::Char::configDone(Char::Config config){
	configsDone.insert(config);
	if(configsDone.size() == (int) Config::COUNT){
		if(onConnectedCB){
			onConnectedCB();
		}
	}
}

void BLE::Client::Char::onRegNotify(const esp_ble_gattc_cb_param_t::gattc_reg_for_notify_evt_param* param){
	if(param->status != ESP_GATT_OK){
		CMF_LOG(BLE_Client_Char, LogLevel::Error,"reg for notify failed, error status = 0x%x", param->status);
		return;
	}

	CMF_LOG(BLE_Client_Char, LogLevel::Info,"Registered for notify");
	configDone(Config::Notify);
}

void BLE::Client::Char::onNotify(const esp_ble_gattc_cb_param_t::gattc_notify_evt_param* param){
	if(notifQueue.size != 1){
		notifQueue.post(std::make_unique<Notif>(PSRAMByteBuffer(param->value, param->value + param->value_len), !param->is_notify), 0);
	}
}

void BLE::Client::Char::onWriteResp(esp_gattc_cb_event_t evt, const esp_ble_gattc_cb_param_t::gattc_write_evt_param* param){
	if(param->status != ESP_GATT_OK){
		CMF_LOG(BLE_Client_Char, LogLevel::Error,"write failed, error status = 0x%x", param->status);
		return;
	}
}

void BLE::Client::Char::writeDescr(uint16_t uuid, const std::vector<uint8_t>& data){
	if(!connected()) return;

	esp_bt_uuid_t id = { .len = 2, .uuid = { .uuid16 = uuid } };
	chr->writeDescr(id, (uint8_t*) data.data(), data.size());
}

bool BLE::Client::Char::write(const std::vector<uint8_t>& data){
	if(!connected()) return false;

	if(!(props & ESP_GATT_CHAR_PROP_BIT_WRITE) || !(remoteProps & ESP_GATT_CHAR_PROP_BIT_WRITE)){
		CMF_LOG(BLE_Client_Char, LogLevel::Warning,"Requesting writy, but WRITE property bit isn't");
		return false;
	}

	chr->write((uint8_t*) data.data(), data.size(), true);
	return true;
}

void BLE::Client::Char::read(){
	if(!connected()) return;

	if(!(props & ESP_GATT_CHAR_PROP_BIT_READ) || !(remoteProps & ESP_GATT_CHAR_PROP_BIT_READ)){
		CMF_LOG(BLE_Client_Char, LogLevel::Warning,"Requesting read, but READ property bit isn't");
		return;
	}

	chr->read();
}
