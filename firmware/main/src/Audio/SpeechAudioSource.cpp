#include "SpeechAudioSource.h"
#include <Util/ServiceLocator.h>

SpeechAudioSource::SpeechAudioSource(SpeechGen::InputType type, std::string source) : type(type), source(std::move(source)){
}

void SpeechAudioSource::open(){
	if(!ServiceLocator::SpeechGenInstance){
		return;
	}

	switch(type){
		case SpeechGen::InputType::Text:
			ServiceLocator::SpeechGenInstance->parseText(source);
			break;
		case SpeechGen::InputType::Phonemes:
			ServiceLocator::SpeechGenInstance->parsePhonemes(source);
			break;
		case SpeechGen::InputType::SSML:
			ServiceLocator::SpeechGenInstance->parseSSML(source);
			break;
	}
}

void SpeechAudioSource::close(){
	if(!ServiceLocator::SpeechGenInstance){
		return;
	}
	ServiceLocator::SpeechGenInstance->cancel();
}

size_t SpeechAudioSource::getData(uint8_t* buffer, size_t bytes){
	if(lastSample){
		return 0;
	}

	if(!ServiceLocator::SpeechGenInstance){
		return 0;
	}

	size_t samplesGot = ServiceLocator::SpeechGenInstance->generate(std::span{ (int16_t*)buffer, bytes / 2 }, lastSample, portMAX_DELAY);

	return samplesGot * sizeof(int16_t);
}

SpeechAudioSource::operator bool() const{
	return !lastSample;
}
