#include "CompressedFile.h"

#include "Log/Log.h"
#include <cstring>
#include <algorithm>

DEFINE_LOG(CompressedFile)

static constexpr uint8_t HEATSHRINK_WINDOW_SZ2 = 14;
static constexpr uint8_t HEATSHRINK_LOOKAHEAD_SZ2 = 7;

CompressedFile::CompressedFile(File source) : source(source), filePath(source ? source.name() : ""){
	if(!source){
		CMF_LOG(CompressedFile, LogLevel::Error, "Couldn't open source file: %s", source.name());
		return;
	}

	hsd = heatshrink_decoder_alloc(InputBufSize, HEATSHRINK_WINDOW_SZ2, HEATSHRINK_LOOKAHEAD_SZ2);
	if(hsd == nullptr){
		CMF_LOG(CompressedFile, LogLevel::Error, "Failed to allocate heatshrink decoder");
		return;
	}

	source.seek(0, SeekMode::SeekSet);
}

CompressedFile::~CompressedFile(){
	if(hsd != nullptr){
		heatshrink_decoder_free(hsd);
		hsd = nullptr;
	}
}

File CompressedFile::open(const File& source){
	return { std::make_shared<CompressedFile>(source) };
}

CompressedFile::operator bool(){
	return hsd != nullptr && source;
}

void CompressedFile::close(){
	source.close();
	filePath = "";
}

void CompressedFile::resetStream(){
	if(hsd != nullptr){
		heatshrink_decoder_reset(hsd);
	}
	source.seek(0, SeekMode::SeekSet);
	inPos = inLen = 0;
	outPos = outLen = 0;
	finished = false;
	decodeError = false;
	cursor = 0;
}

// Pulls the next chunk of decompressed bytes into outBuf.
// Returns false once the stream is fully drained.
bool CompressedFile::fillOut(){
	if(outPos < outLen) return true;
	outPos = outLen = 0;

	if(hsd == nullptr) return false;

	while(true){
		// Drain any output the decoder already has.
		size_t polled = 0;
		HSD_poll_res pres = heatshrink_decoder_poll(hsd, outBuf, OutputBufSize, &polled);
		if(pres < 0){ // HSDR_POLL_ERROR_*
			CMF_LOG(CompressedFile, LogLevel::Error, "Decode poll error (%d): %s", pres, filePath.c_str());
			decodeError = finished = true;
			return false;
		}
		if(polled > 0){
			outLen = polled;
			outPos = 0;
			return true;
		}
		if(pres == HSDR_POLL_MORE) continue; // more output pending; poll again

		// No output: feed the decoder. Refill our input buffer from the source if drained.
		if(inPos >= inLen){
			const size_t got = source.read(inBuf, InputBufSize);
			inLen = (got == (size_t)-1) ? 0 : got; // a closed/failed source reads as EOF, not a huge length
			inPos = 0;
		}

		if(inPos < inLen){
			size_t sunk = 0;
			HSD_sink_res sres = heatshrink_decoder_sink(hsd, inBuf + inPos, inLen - inPos, &sunk);
			if(sres < 0){ // HSDR_SINK_ERROR_*
				CMF_LOG(CompressedFile, LogLevel::Error, "Decode sink error (%d): %s", sres, filePath.c_str());
				decodeError = finished = true;
				return false;
			}
			inPos += sunk;
			continue; // loop back to poll the freshly sunk data
		}

		// Source exhausted: tell the decoder, then poll out the tail.
		if(finished) return false;
		HSD_finish_res fres = heatshrink_decoder_finish(hsd);
		if(fres == HSDR_FINISH_MORE) continue; // tail output remains; poll it on next loop
		if(fres < 0){ // HSDR_FINISH_ERROR_*
			CMF_LOG(CompressedFile, LogLevel::Error, "Decode finish error (%d): %s", fres, filePath.c_str());
			decodeError = true;
		}
		finished = true;
		return false;
	}
}

size_t CompressedFile::read(uint8_t* dest, size_t len){
	size_t written = 0;
	while(written < len){
		if(outPos >= outLen && !fillOut()) break;

		const size_t n = std::min(len - written, outLen - outPos);
		memcpy(dest + written, outBuf + outPos, n);
		outPos += n;
		written += n;
		cursor += n;
	}
	return written;
}

bool CompressedFile::seek(int pos, int whence){
	int64_t target;
	if(whence == SEEK_SET){
		target = pos;
	}else if(whence == SEEK_CUR){
		target = (int64_t)cursor + pos;
	}else{
		CMF_LOG(CompressedFile, LogLevel::Warning, "SEEK_END unsupported on compressed stream: %s", filePath.c_str());
		return false;
	}

	if(target < 0){
		CMF_LOG(CompressedFile, LogLevel::Warning, "Seek before start (%lld) on %s", target, filePath.c_str());
		return false;
	}

	const size_t tgt = (size_t)target;
	if(tgt == cursor) return true;

	if(tgt < cursor){
		const size_t back = cursor - tgt;
		if(back <= outPos){
			// Within the still-resident output window: rewind without re-decoding.
			outPos -= back;
			cursor = tgt;
			return true;
		}
		resetStream(); // beyond the buffered window — can only go back by restarting the decode
	}

	// Skip forward by decoding and discarding.
	uint8_t scratch[InputBufSize];
	while(cursor < tgt){
		const size_t want = std::min(sizeof(scratch), tgt - cursor);
		if(read(scratch, want) != want) return false; // reached end before target
	}
	return true;
}

size_t CompressedFile::size() const{
	// NOTE: forward-only stream. The total decompressed size is NOT known without decoding the
	// whole stream, so this returns bytes decoded so far not the full length.
	return cursor;
}

const char* CompressedFile::name() const{
	return filePath.c_str();
}

size_t CompressedFile::pos() const{
	return cursor;
}

size_t CompressedFile::write(const uint8_t* buf, size_t size){
	return 0;
}

void CompressedFile::flush(){
}
