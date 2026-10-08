#include "ArchiveCache.h"
#include <vector>
#include <esp_heap_caps.h>
#include "Log/Log.h"
#include "FileSystem/RamFile.h"

DEFINE_LOG(ArchiveCache)

ArchiveCache::ArchiveCache(const File& file) : archiveFile(file){
	if(!file){
		CMF_LOG(ArchiveCache, LogLevel::Error, "File not opened: %s", file.name());
	}
	ArchiveCache::load();
}

ArchiveCache::ArchiveCache(const bool useExternalRam) : useExternalRam(useExternalRam){}

ArchiveCache::~ArchiveCache(){
	if(ownsData){
		free(data);
	}
	data = nullptr;
}

void ArchiveCache::setArchiveFile(const File& file){
	if(!file){
		CMF_LOG(ArchiveCache, LogLevel::Error, "New file not opened: %s", file.name());
	}
	archiveFile = file;
}

void ArchiveCache::setBuffer(uint8_t* buffer, const size_t bufferCapacity){
	data = buffer;
	capacity = bufferCapacity;
	ownsData = false;
}

void ArchiveCache::load(){
	if(loaded) return;

	if(!archiveFile){
		CMF_LOG(ArchiveCache, LogLevel::Error, "No archive file to load");
		return;
	}

	loaded = true;

	// Reads exactly n bytes or reports failure (short read / closed source -> (size_t)-1).
	auto readExact = [&](void* dst, const size_t n) {
		return archiveFile.read((uint8_t*)dst, n) == n;
	};

	// A corrupt/truncated archive is a build- or flash-time fault, not a recoverable runtime
	// condition: fail loudly rather than silently serving stale shared-buffer bytes.
	auto fail = [&](const char* why) {
		CMF_LOG(ArchiveCache, LogLevel::Error, "Corrupt archive: %s", why);
		abort();
	};

	archiveFile.seek(0);
	uint32_t count = 0;
	if(!readExact(&count, 4)) return fail("truncated header");

	std::vector<Entry> tmp;
	tmp.reserve(count);

	size_t totalSize = 0;

	for(uint32_t i = 0; i < count; i++){
		std::string name;
		name.reserve(32);
		char c = 0;
		while(archiveFile.read((uint8_t*)&c, 1) == 1 && c != 0){
			if(name.length() >= 32) return fail("filename too long");
			name.append(1, c);
		}

		size_t size = 0;
		if(!readExact(&size, 4)) return fail("truncated directory");
		totalSize += size;

		tmp.emplace_back(Entry{ name, size, 0 });
	}

	char terminator = 0;
	if(!readExact(&terminator, 1)) return fail("missing directory terminator");

	if(ownsData){
		// No shared buffer was provided: allocate one sized exactly to this archive.
		data = (uint8_t*)(useExternalRam ? heap_caps_malloc(totalSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) : malloc(totalSize));
		if(data == nullptr){
			CMF_LOG(ArchiveCache, LogLevel::Warning, "Failed allocating data buffer of %zu B", totalSize);
			loaded = false;
			return;
		}
		capacity = totalSize;
	}else if(data == nullptr){
		CMF_LOG(ArchiveCache, LogLevel::Error, "No buffer set for archive load");
		loaded = false;
		return;
	}else if(totalSize > capacity){
		CMF_LOG(ArchiveCache, LogLevel::Error, "Archive (%zu B) exceeds shared buffer (%zu B)", totalSize, capacity);
		loaded = false;
		return;
	}

	size_t offset = 0;

	for(auto& entry : tmp){
		if(archiveFile.read(data + offset, entry.size) != entry.size){
			return fail("truncated payload");
		}
		entry.offset = offset;

		offset += entry.size;

		entries.insert(std::make_pair(entry.name, std::move(entry)));
	}

	archiveFile = File();
}

void ArchiveCache::unload(){
	if(!loaded) return;
	loaded = false;

	entries.clear();
	if(ownsData){
		// Shared buffers are kept for reuse; only free a buffer we allocated ourselves.
		free(data);
		data = nullptr;
	}
	archiveFile = File();
}

File ArchiveCache::open(const char* path){
	const auto entry = entries.find(path);
	if(entry == entries.end()) return File();

	const auto ramFile = std::make_shared<RamFile>(data + entry->second.offset, entry->second.size, path);
	return File(ramFile);
}
