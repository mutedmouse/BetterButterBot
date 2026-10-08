#include "MemoryAudioSource.h"
#include <algorithm>
#include <cstring>

MemoryAudioSource::MemoryAudioSource(PSRAMByteBuffer data) : data(std::move(data)){
}

void MemoryAudioSource::open(){
	pos = 0;
}

void MemoryAudioSource::close(){
	pos = data.size();
}

size_t MemoryAudioSource::getData(uint8_t* buffer, size_t bytes){
	const size_t remaining = data.size() - pos;
	const size_t n = std::min(bytes, remaining);
	if(n > 0){
		std::memcpy(buffer, data.data() + pos, n);
		pos += n;
	}
	return n;
}

MemoryAudioSource::operator bool() const{
	return pos < data.size();
}
