#include "SpeechAudioGen.h"
#include "Core/Application.h"

DEFINE_LOG(SpeechAudioGen)

SpeechAudioGen::SpeechAudioGen(){
	reverb.setRoomSize(0.2f); // 0 = metalan/sjajan, 1 = taman/prigusen
	reverb.setDamping(0.8f); // duzina odjeka
	reverb.setWet(0.05f); // Koliko reverba cujes
}

void SpeechAudioGen::open(std::unique_ptr<AudioSource> resource){
	resource->open();
	this->resource = std::move(resource);
	compressor.reset();
}

void SpeechAudioGen::close(){
	resource->close();
	resource.reset();
}

size_t SpeechAudioGen::getData(uint8_t* buffer, size_t bytes){
	if(resource == nullptr){
		CMF_LOG(SpeechAudioGen, LogLevel::Debug, "early return, no resource");
		return 0;
	}

	size_t bytesTransferred = 0;
	while(bytesTransferred < bytes && (bool)*resource){
		auto i = resource->getData(buffer + bytesTransferred, bytes - bytesTransferred);
		if(i == 0) break;
		bytesTransferred += i;
	}
	const size_t sampleCount = bytesTransferred / sizeof(int16_t);
	compressor.process(reinterpret_cast<int16_t*>(buffer), sampleCount);
	reverb.process(reinterpret_cast<int16_t*>(buffer), sampleCount);
	return bytesTransferred;
}
