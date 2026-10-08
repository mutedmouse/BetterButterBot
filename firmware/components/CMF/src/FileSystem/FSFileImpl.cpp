#include "FSFileImpl.h"
#include "Log/Log.h"
#include <cstring>

DEFINE_LOG(FSFile)

FSFileImpl::FSFileImpl(const char* path, const char* mode) : filePath(path){
	file = fopen(path, mode);

	if(file == nullptr){
		CMF_LOG(FSFile, LogLevel::Error, "Can't open file %s", path);
		return;
	}

	stat();
}

FSFileImpl::~FSFileImpl(){
	close();
}

FSFileImpl::operator bool(){
	return file != nullptr;
}

File FSFileImpl::open(const char* path, const char* mode){
	auto file = std::make_shared<FSFileImpl>(path, mode);
	return { file };
}

void FSFileImpl::close(){
	fclose(file);
	file = nullptr;
}

size_t FSFileImpl::size() const{
	if(written){
		stat();
	}

	return fileSize;
}

const char* FSFileImpl::name() const{
	return filePath.c_str() + 7;
}

size_t FSFileImpl::read(uint8_t* buf, size_t size){
	return fread(buf, 1, size, file);
}

size_t FSFileImpl::write(const uint8_t* buf, size_t size){
	written = true;
	return fwrite(buf, 1, size, file);
}

void FSFileImpl::flush(){
	fflush(file);
	// workaround for https://github.com/espressif/arduino-esp32/issues/1293
	fsync(fileno(file));
}

bool FSFileImpl::seek(int pos, int mode){
	return fseek(file, pos, mode) == 0;
}

size_t FSFileImpl::pos() const{
	return ftell(file);
}

void FSFileImpl::stat() const{
	if(filePath.empty()) return;

	struct stat res = {};
	if(!::stat(filePath.c_str(), &res)){
		written = false;
		fileSize = res.st_size;
	}
}
