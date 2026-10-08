#ifndef CLOCKSTAR_FIRMWARE_CURRENTTIME_H
#define CLOCKSTAR_FIRMWARE_CURRENTTIME_H

#include "BLE/Client.h"
#include "Entity/AsyncEntity.h"

class CurrentTime : public Object {
	GENERATED_BODY(CurrentTime, Object, CONSTRUCTOR_PACK(BLE::Client*))

public:
	CurrentTime(BLE::Client* client);
	~CurrentTime() override;

private:
	std::shared_ptr<BLE::Client::Service> service;
	std::shared_ptr<BLE::Client::Char> chr;

	TaskHandle_t timeThread = nullptr;

	void loop() noexcept;

	void setTime(const PSRAMByteBuffer& data);

	static constexpr esp_bt_uuid_t ServiceUUID = { .len = ESP_UUID_LEN_16, .uuid = { .uuid16 = 0x1805 }};
	static constexpr esp_bt_uuid_t CharUUID = { .len = ESP_UUID_LEN_16, .uuid = { .uuid16 = 0x2A2B }};

};


#endif //CLOCKSTAR_FIRMWARE_CURRENTTIME_H
