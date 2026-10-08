#include "FileAudioSource.h"
#include <utility>
#include "Log/Log.h"

DEFINE_LOG(FileAudioSource)

FileAudioSource::FileAudioSource(std::string path) : path(std::move(path)){
}

void FileAudioSource::open(){
	file = std::ifstream(path);
	if(!file.is_open()){
		CMF_LOG(FileAudioSource, LogLevel::Error, "Failed to open file %s", path.c_str());
		return;
	}
}

void FileAudioSource::close(){
	file.close();
}

size_t FileAudioSource::getData(uint8_t* buffer, size_t bytes){
	if(!file.is_open()){
		CMF_LOG(FileAudioSource, LogLevel::Error, "Cannot get data from unopened file");
	}

	file.read((char*)buffer, bytes);
	return file.gcount();
}

FileAudioSource::operator bool() const{
	return (bool)file;
}
