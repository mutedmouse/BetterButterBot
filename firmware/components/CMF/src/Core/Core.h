#ifndef CMF_CORE_H
#define CMF_CORE_H

#include <sdkconfig.h>

#ifndef __cplusplus
	#error "CMF: Only C++ projects are supported"
#elif __cplusplus < 202002L
	#error "CMF: Only C++20 or higher supported"
#endif

#ifndef CMF_TARGET_PROCESSOR
	#ifdef CONFIG_IDF_TARGET_ESP32
		#define CMF_TARGET_ESP32
	#elifdef CONFIG_IDF_TARGET_ESP32S2
		#define CMF_TARGET_ESP32S2
	#elifdef CONFIG_IDF_TARGET_ESP32S3
		#define CMF_TARGET_ESP32S3
	#elifdef CONFIG_IDF_TARGET_ESP32C2
		#define CMF_TARGET_ESP32C2
	#elifdef CONFIG_IDF_TARGET_ESP32C3
		#define CMF_TARGET_ESP32C3
	#elifdef CONFIG_IDF_TARGET_ESP32C6
		#define CMF_TARGET_ESP32C6
	#elifdef CONFIG_IDF_TARGET_ESP32H2
		#define CMF_TARGET_ESP32H2
	#else
		#error "CMF: Processor not supported or defined"
	#endif
#endif

#endif //CMF_CORE_H