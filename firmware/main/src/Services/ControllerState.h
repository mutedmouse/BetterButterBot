#ifndef BUTTERBOT_FIRMWARE_CONTROLLERSTATE_H
#define BUTTERBOT_FIRMWARE_CONTROLLERSTATE_H

#include <CtrlData.h>
#include <cstddef>

class ControllerState{
public:
	ControllerState();
	~ControllerState();

	inline const char* getSSID() const { return SSID; }

	inline const char* getPassword() const { return password; }

	static inline constexpr size_t getSSIDLength() { return sizeof(RCData::SSID); }

	static inline constexpr size_t getPasswordLength() { return sizeof(RCData::password); }

	inline void updateRCData(const RCData& data) { onRCData(data); }

private:
	char SSID[sizeof(RCData::SSID)] = {};
	char password[sizeof(RCData::password)] = {};

private:
	void onRCData(const RCData& data);
};

#endif //BUTTERBOT_FIRMWARE_CONTROLLERSTATE_H