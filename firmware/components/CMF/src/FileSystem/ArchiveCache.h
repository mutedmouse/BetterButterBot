#ifndef CMF_ARCHIVECACHE_H
#define CMF_ARCHIVECACHE_H

#include "FileCache.h"
#include <unordered_map>
#include <string>
#include "FileSystem/File.h"

class ArchiveCache : public FileCache {
public:
	/**
	 * @brief Construct and immediately load from the given archive file.
	 */
	ArchiveCache(const File& file);

	/**
	 * @brief Construct without an archive file; load is deferred until setArchiveFile() + load().
	 * @param useExternalRam Allocate the decompressed archive buffer in SPIRAM.
	 */
	explicit ArchiveCache(bool useExternalRam = false);
	~ArchiveCache() override;

	void load() override;
	void unload() override;

	File open(const char* path) override;

	void setArchiveFile(const File& file);

	/**
	 * @brief Decompress into a caller-owned buffer instead of allocating one per load.
	 *        Lets several caches share a single, permanently reserved buffer.
	 * @param buffer   Destination buffer
	 * @param capacity Buffer size in bytes
	 */
	void setBuffer(uint8_t* buffer, size_t capacity);

private:
	struct Entry {
		const std::string name;
		size_t size;
		size_t offset;
	};

	std::unordered_map<std::string, const Entry> entries;

	bool loaded = false;
	bool useExternalRam = false;
	uint8_t* data = nullptr;
	size_t capacity = 0;
	bool ownsData = true; // false when `data` is a caller-provided shared buffer
	File archiveFile;
};

#endif //CMF_ARCHIVECACHE_H
