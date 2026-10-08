#ifndef CMF_FILE_H
#define CMF_FILE_H

#include "FileImpl.h"
#include <fstream>
#include <utility>

class File {
public:
	File(FileImplPtr p = FileImplPtr());

	operator bool() const;
	bool operator ==(const File& other) const;
	bool open() const;

	void close();

	size_t size() const;
	const char* name() const;

	size_t read(uint8_t* buf, size_t size);
	int read(); // Reads 1 byte. Returns -1 on fail. Not good
	size_t write(const uint8_t* buf, size_t size);
	size_t write(uint8_t);
	void flush();

	bool seek(int pos, SeekMode mode = SeekMode::SeekSet);
	size_t position() const;

	int available();
	int peek();

protected:
	FileImplPtr impl;
};

#endif //CMF_FILE_H
