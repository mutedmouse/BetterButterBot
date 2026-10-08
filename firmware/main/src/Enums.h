#ifndef BUTTERBOT_FIRMWARE_ENUMS_H
#define BUTTERBOT_FIRMWARE_ENUMS_H

#include <cstdint>

enum class Button : uint8_t {
	Power
};

enum class MonoLED : uint8_t {
	Status,
	Module,
	PIRIndicator
};
enum class RGBLED : uint8_t {};

enum class PWMChannel : uint8_t {
	ModuleLED = 1
};

#endif //BUTTERBOT_FIRMWARE_ENUMS_H
