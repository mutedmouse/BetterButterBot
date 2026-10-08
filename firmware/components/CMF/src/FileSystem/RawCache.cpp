#include "RawCache.h"
#include "Log/Log.h"
#include "SPIFFS.h"
#include "RamFile.h"

DEFINE_LOG(RawCache)

RawCache::RawCache(FileCache* archiveCache) : archiveCache(archiveCache){ }

RawCache::RawCache(const std::vector<std::string>& paths) : paths(paths){
	files.reserve(paths.size());
}

void RawCache::setPaths(const std::vector<std::string>& filePaths){
	if(loaded) return;

	paths = filePaths;

	if(files.bucket_count() < filePaths.size()){
		files.reserve(filePaths.size());
	}
}

void RawCache::load(){
	if(loaded) return;
	loaded = true;

	for(const auto& path : paths){
		// If using ArchiveCache, find file in it instead of spiffs
		File file = archiveCache ? archiveCache->open(path.c_str()) : SPIFFS::open(path.c_str());
		if(!file){
			CMF_LOG(RawCache, LogLevel::Warning, "Can't open file %s", path.c_str());
			continue;
		}

		File cFile = RamFile::open(file);

		files.emplace(path, cFile);
	}
}

void RawCache::unload(){
	if(!loaded) return;
	loaded = false;

	files.clear();
}

File RawCache::open(const char* path){
	const auto entry = files.find(path);
	if(entry == files.end()) return { };
	return entry->second;
}
