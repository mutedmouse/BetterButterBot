#ifndef CMF_SPIFFS_H
#define CMF_SPIFFS_H

#include "File.h"

class SPIFFS {
public:
	static bool init();

	static File open(const char* path, const char* mode = "r");

private:
	static constexpr const char* Prefix = "/spiffs";

	static bool inited;
};

#endif //CMF_SPIFFS_H
