#ifndef CMF_FILEIMPL_H
#define CMF_FILEIMPL_H

#include <memory>

class FileImpl;

typedef std::shared_ptr<FileImpl> FileImplPtr;

enum class SeekMode : uint8_t {
	SeekSet = SEEK_SET,
	SeekCur = SEEK_CUR,
	SeekEnd = SEEK_END
};

class FileImpl {
public:
	virtual ~FileImpl() = default;

	virtual operator bool() = 0;

	virtual void close() = 0;

	virtual size_t size() const = 0;
	virtual const char* name() const = 0;

	virtual size_t write(const uint8_t* buf, size_t size) = 0;
	virtual size_t read(uint8_t* buf, size_t size) = 0;
	virtual void flush() = 0;

	virtual bool seek(int pos, int mode) = 0;
	virtual size_t pos() const = 0;
};

#endif //CMF_FILEIMPL_H
