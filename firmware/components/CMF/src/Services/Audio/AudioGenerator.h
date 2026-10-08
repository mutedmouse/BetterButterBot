#ifndef CMF_AUDIOGENERATOR_H
#define CMF_AUDIOGENERATOR_H

#include "AudioSource.h"
#include <memory>

class AudioGenerator {
public:
	virtual ~AudioGenerator() = default;

	virtual void open(std::unique_ptr<AudioSource> resource) = 0;

	virtual void close() = 0;

	/**
	 * Pull data from decoder.
	 * @param buffer output, to be filled with decoded data
	 * @param bytes number of requested bytes
	 * @return number of bytes actually written to provided buffer
	 */
	virtual size_t getData(uint8_t* buffer, size_t bytes) = 0;
};

#endif //CMF_AUDIOGENERATOR_H
