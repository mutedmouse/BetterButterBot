#ifndef CMF_FSFILEIMPL_H
#define CMF_FSFILEIMPL_H

#include <sys/dirent.h>
#include <sys/stat.h>
#include "FileImpl.h"
#include "File.h"

class FSFileImpl : public FileImpl {
public:
	FSFileImpl(const char* path, const char* mode);
	~FSFileImpl() override;

	operator bool() override;

	static File open(const char* path, const char* mode = "r");
	void close() final;

	size_t size() const override;
	const char* name() const override;

	size_t read(uint8_t* buf, size_t size) override;
	size_t write(const uint8_t* buf, size_t size) override;
	void flush() override;

	bool seek(int pos, int mode) override;
	size_t pos() const override;

private:
	FILE* file = nullptr;
	std::string filePath;

	mutable size_t fileSize = 0;
	mutable bool written = false;

	void stat() const;
};

#endif //CMF_FSFILEIMPL_H
