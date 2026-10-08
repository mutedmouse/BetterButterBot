#include "AACAudioGenerator.h"

DEFINE_LOG(AACSource)

AACAudioGenerator::AACAudioGenerator(){
	decoder = AACInitDecoder();

	if(decoder == nullptr){
		CMF_LOG(AACSource, LogLevel::Error, "Libhelix AAC decoder failed to initialize.");
		return;
	}

	fillBuffer.reserve(FileReadThreshold + FileReadChunkSize);
	dataBuffer.reserve(2 * DecodeOutBufferSize);
}

void AACAudioGenerator::open(std::unique_ptr<AudioSource> resource){
	AACFlushCodec(decoder);

	fillBuffer.clear();
	dataBuffer.clear();
	bytesRemaining = 0;

	resource->open();
	this->resource = std::move(resource);
}

void AACAudioGenerator::close(){
	if(resource != nullptr){
		resource->close();
		resource.reset();
	}

	AACFlushCodec(decoder);

	fillBuffer.clear();
	dataBuffer.clear();
	bytesRemaining = 0;
}

size_t AACAudioGenerator::getData(uint8_t* buffer, size_t bytes){
	if(resource == nullptr){
		CMF_LOG(AACSource, LogLevel::Debug, "early return, no resource");
		return 0;
	}
	if(decoder == nullptr){
		CMF_LOG(AACSource, LogLevel::Debug, "early return, decoder == nullptr");
		return 0;
	}

	if(buffer == nullptr){
		CMF_LOG(AACSource, LogLevel::Debug, "early return, buffer == nullptr");
		return 0;
	}

	if(bytes == 0){
		CMF_LOG(AACSource, LogLevel::Debug, "early return, bytes == 0");
		return 0;
	}


	size_t bytesTransferred = 0;

	if(!dataBuffer.empty()){
		const size_t toCopy = std::min(dataBuffer.size(), bytes);
		memcpy(buffer, dataBuffer.data(), toCopy);
		bytesTransferred += toCopy;
		// Erase only what was copied; keep any remainder for the next call.
		dataBuffer.erase(dataBuffer.begin(), dataBuffer.begin() + toCopy);
	}

	while(bytesTransferred < bytes){
		CMF_LOG(AACSource, LogLevel::Debug, "bytesRemaining: %d", bytesRemaining);

		if(fillBuffer.size() < FileReadThreshold && (bool)*resource){
			const size_t oldSize = fillBuffer.size();
			fillBuffer.resize(oldSize + FileReadChunkSize);
			const size_t read = resource->getData((uint8_t*)fillBuffer.data() + oldSize, FileReadChunkSize);
			fillBuffer.resize(oldSize + read);
			bytesRemaining += read;
			CMF_LOG(AACSource, LogLevel::Debug, "fillBuffer resized, bytesRemaining: %d", bytesRemaining);
		}

		if(bytesRemaining <= 0){
			break;
		}

		unsigned char* inBuffer = reinterpret_cast<unsigned char*>(fillBuffer.data());

		const int bytesRemainingBefore = bytesRemaining;

		dataBuffer.resize(dataBuffer.size() + DecodeOutBufferSize, 0);

		if(const int ret = AACDecode(decoder, &inBuffer, &bytesRemaining, reinterpret_cast<SampleType*>(dataBuffer.data() + dataBuffer.size() - DecodeOutBufferSize))){
			CMF_LOG(AACSource, LogLevel::Error, "AAC decoding error %d", ret);
			return 0;
		}

		const int bytesDecoded = bytesRemainingBefore - bytesRemaining;
		CMF_LOG(AACSource, LogLevel::Debug, "decoded: %d", bytesDecoded);

		fillBuffer.erase(fillBuffer.begin(), fillBuffer.begin() + bytesDecoded);

		const int bytesToTransfer = std::min(dataBuffer.size(), bytes - bytesTransferred);
		memcpy((buffer + bytesTransferred / SampleSize), dataBuffer.data(), bytesToTransfer);
		dataBuffer.erase(dataBuffer.begin(), dataBuffer.begin() + bytesToTransfer);

		bytesTransferred += bytesToTransfer;
		CMF_LOG(AACSource, LogLevel::Debug, "bytesTransferred: %d", bytesTransferred);
	}

	return bytesTransferred;
}
