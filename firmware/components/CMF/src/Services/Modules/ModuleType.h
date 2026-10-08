#ifndef CMF_MODULETYPE_H
#define CMF_MODULETYPE_H

#include "Drivers/Interface/InputDriver.h"
#include "Drivers/Interface/OutputDriver.h"
#include "Periphery/I2CMaster.h"

namespace Modules {

enum class Type : uint8_t {
	Unknown = 0,
#ifdef CONFIG_RM_TempHum
	RM_TempHum,
#endif
#ifdef CONFIG_RM_LED
	RM_LED,
#endif
#ifdef CONFIG_RM_Motion
	RM_Motion,
#endif
#ifdef CONFIG_RM_CO2
	RM_CO2,
#endif
#ifdef CONFIG_RM_IR
	RM_IR,
#endif
#ifdef CONFIG_RM_PerfBoard
	RM_PerfBoard,
#endif

	COUNT
};

/**
 * Helper struct for pins which can be input and output, depending on Module type.
 */
struct IOPin {
	InputDriver* inputDriver;
	OutputDriver* outputDriver;
	int inputPort;
	int outputPort;
};
struct BusPins {
	InputPin addr[6], detPins[2];
	I2CMaster* i2c;
	IOPin subAddressPins[6];
};

}

#endif //CMF_MODULETYPE_H
