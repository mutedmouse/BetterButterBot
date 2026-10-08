#include "SPIFFS.h"
#include "FSFileImpl.h"
#include "Log/Log.h"
#include <esp_spiffs.h>

DEFINE_LOG(SPIFFS)

bool SPIFFS::inited = false;

bool SPIFFS::init(){
	if(inited) return true;

	//TODO - expose this config to user
	esp_vfs_spiffs_conf_t conf = {
			.base_path = Prefix,
			.partition_label = "storage",
			.max_files = 16,
			.format_if_mount_failed = false
	};

	auto ret = esp_vfs_spiffs_register(&conf);
	if(ret != ESP_OK){
		if(ret == ESP_FAIL){
			CMF_LOG(SPIFFS, LogLevel::Error, "Failed to mount or format filesystem");
		}else if(ret == ESP_ERR_NOT_FOUND){
			CMF_LOG(SPIFFS, LogLevel::Error, "Failed to find SPIFFS partition");
		}else{
			CMF_LOG(SPIFFS, LogLevel::Error, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
		}
		return false;
	}

	inited = true;
	return true;
}

File SPIFFS::open(const char* path, const char* mode){
	const std::string p = std::string(SPIFFS::Prefix) + std::string(path);
	return FSFileImpl::open(p.c_str(), mode);
}
