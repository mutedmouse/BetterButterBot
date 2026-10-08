#ifndef BUTTERBOTCTRL_FIRMWARE_COMPRESSEDFILE_H
#define BUTTERBOTCTRL_FIRMWARE_COMPRESSEDFILE_H

#include <cstdint>
#include <string>
#include "FileImpl.h"
#include "File.h"

extern "C" {
#include <heatshrink_decoder.h>
}

/**
 * @brief A forward-streaming heatshrink decompressor exposed as a File.
 *
 * Wraps a compressed source File and decodes it on demand as read() is called, holding only a
 * small decoder window + I/O buffers (never the whole decompressed archive). This keeps the
 * decompression concern entirely inside this class: consumers such as ArchiveCache read through
 * the plain File interface and don't know (or care) whether the bytes are compressed.
 *
 * Reads are sequential; seek() supports rewinding (cheap within the current output buffer, otherwise
 * re-decodes from the start) and forward skips. This is NOT a random-access/sized File: size() reports
 * bytes decoded so far (not the total length), so available() is always 0 — read it to completion
 * rather than wrapping it in something that pre-allocates from size() (e.g. RamFile).
 */
class CompressedFile : public FileImpl {
public:
	explicit CompressedFile(File source);
	~CompressedFile() override;

	operator bool() override;

	static File open(const File& source);
	void close() override;

	size_t size() const override;
	const char* name() const override;

	size_t read(uint8_t* dest, size_t len) override;
	size_t write(const uint8_t* buf, size_t size) override;
	void flush() override;

	bool seek(int pos, int whence) override;
	size_t pos() const override;

private:
	static constexpr size_t InputBufSize = 256;
	static constexpr size_t OutputBufSize = 1024;

	File source;
	heatshrink_decoder* hsd = nullptr;
	std::string filePath;

	uint8_t inBuf[InputBufSize];
	size_t inPos = 0, inLen = 0;  // compressed bytes pending sink

	uint8_t outBuf[OutputBufSize];
	size_t outPos = 0, outLen = 0; // decoded bytes pending consumption

	// Is decoder done
	bool finished = false;
	// Decoder reported an error (HSDR_*_ERROR_*); the stream is truncated/garbage
	bool decodeError = false;
	// Position in stream
	size_t cursor = 0;

	void resetStream();
	// Refill outBuf with decoded bytes; false at end of stream
	bool fillOut();
};

#endif //BUTTERBOTCTRL_FIRMWARE_COMPRESSEDFILE_H
