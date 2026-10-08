#ifndef CMF_AUDIOSOURCE_H
#define CMF_AUDIOSOURCE_H

#include <cstdint>
#include <cstddef>

class AudioSource {
public:
	virtual ~AudioSource() = default;

	virtual void open() = 0;

	virtual void close() = 0;

	/**
	 * Pull data from the source.
	 * @param buffer output, to be filled with raw data
	 * @param bytes number of requested bytes
	 * @return number of bytes actually written to provided buffer
	 */
	virtual size_t getData(uint8_t* buffer, size_t bytes) = 0;

	virtual operator bool() const = 0;
};

#endif //CMF_AUDIOSOURCE_H
