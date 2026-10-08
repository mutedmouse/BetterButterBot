#ifndef BUTTERBOTCTRL_FIRMWARE_COMPOSITECACHE_H
#define BUTTERBOTCTRL_FIRMWARE_COMPOSITECACHE_H

#include "FileCache.h"

template<size_t N>
class CompositeCache : public FileCache {
public:
	explicit CompositeCache(const std::array<FileCache*, N>& caches) : caches(caches){}

	void load() final{
		for(FileCache* cache : caches){
			cache->load();
		}
	}

	void unload() final{
		for(FileCache* cache : caches){
			cache->unload();
		}
	}

	File open(const char* path) final{
		for(FileCache* cache : caches){
			File file = cache->open(path);
			if(file){
				return file;
			}
		}
		return File();
	}

private:
	std::array<FileCache*, N> caches;
};

#endif //BUTTERBOTCTRL_FIRMWARE_COMPOSITECACHE_H
