#ifndef CMF_LOG_H
#define CMF_LOG_H

#include <cstdio>
#include <string>
#include <esp_log.h>
#include <source_location>

/**
 * @brief LogLevel enum marks the severity of the log being used.
 * This is consistent with ESP logging to ensure both systems run on the same settings from the sdkconfig.
 */
enum LogLevel {
	None = (uint8_t) ESP_LOG_NONE,
	Error = (uint8_t) ESP_LOG_ERROR,
	Warning = (uint8_t) ESP_LOG_WARN,
	Info = (uint8_t) ESP_LOG_INFO,
	Debug = (uint8_t) ESP_LOG_DEBUG,
	Verbose = (uint8_t) ESP_LOG_VERBOSE
};

/**
 * @brief The type of log, a predefined can be used, or a custom one can be defined.
 * Changes the tag of the log so that each is unique on the serial output.
 */
struct LogType {
	const std::string Tag;
};

/**
 * @brief Macro used to define a custom log.
 * @param name The name of the custom log.
 */
#define DEFINE_LOG(name) inline static const LogType name##_log = {#name};
DEFINE_LOG(CMF)

/**
 * @brief Log used to trace location of the code to the serial output.
 * @param format The format of the log used for extra parameter output.
 */
#define TRACE_LOG(format, ...) printf(std::string("%s(%d:%d) '%s' ").append(format).append("\n").c_str(), std::source_location::current().file_name(),std::source_location::current().line(), std::source_location::current().column(), std::source_location::current().function_name() __VA_OPT__(,) __VA_ARGS__)

/**
 * @brief Normal log using a defined log for a tag,
 * level for severity and format for extra argument output.
 * @param log The log tag used.
 * @param level The severity level.
 * @param format The format of extra arguments.
 */
#define CMF_LOG(log, level, format, ...)																		\
	do {																										\
		if(level == Error){																						\
			ESP_LOGE(log##_log.Tag.c_str(), format __VA_OPT__(,) __VA_ARGS__);										\
		}else if(level == LogLevel::Warning){																	\
			ESP_LOGW(log##_log.Tag.c_str(), format __VA_OPT__(,) __VA_ARGS__);										\
		}else if(level == Info){																				\
			ESP_LOGI(log##_log.Tag.c_str(), format __VA_OPT__(,) __VA_ARGS__);										\
		}else if(level == Debug){																				\
			ESP_LOGD(log##_log.Tag.c_str(), format __VA_OPT__(,) __VA_ARGS__);										\
		}else if(level == Verbose){																				\
			ESP_LOGV(log##_log.Tag.c_str(), format __VA_OPT__(,) __VA_ARGS__);										\
		}else{}																									\
    } while(false)																								\

#endif //CMF_LOG_H