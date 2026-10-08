#ifndef CMF_FILECACHE_H
#define CMF_FILECACHE_H

#include "File.h"

class FileCache {
public:
	virtual File open(const char* path) = 0;
	virtual ~FileCache() = default;

	virtual void load() = 0;
	virtual void unload() = 0;
};

#endif //CMF_FILECACHE_H
