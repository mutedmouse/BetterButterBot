#ifndef BUTTERBOT_FIRMWARE_SHUTDOWNSERVICE_H
#define BUTTERBOT_FIRMWARE_SHUTDOWNSERVICE_H

#include <BBData.h>

typedef ShutdownData::ShutdownReason ShutdownReason;

class ShutdownService {
public:
	static void Shutdown(ShutdownReason reason);

	static void PowerOff();
};

#endif //BUTTERBOT_FIRMWARE_SHUTDOWNSERVICE_H
