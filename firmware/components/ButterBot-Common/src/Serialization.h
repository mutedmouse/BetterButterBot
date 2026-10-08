#ifndef BUTTERBOT_COMMON_SERIALIZATION_H
#define BUTTERBOT_COMMON_SERIALIZATION_H

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>
#include <cstddef>
#include <concepts>
#include <algorithm>

/**
 * Length-prefixed byte (de)serialization helpers for variable-length Com payloads.
 *
 * Wire format for a string is [len:uint8_t][len bytes]. This header is shared between the
 * ButterBot firmware (encode side) and the controller firmware (decode side) so both ends
 * agree on the exact layout. Read helpers are bounds-checked against `end` and degrade to
 * empty/zero values on a short/corrupt buffer rather than reading out of bounds.
 */
namespace ComSerial {

inline void writeU8(std::vector<uint8_t>& buf, uint8_t v){
	buf.push_back(v);
}

inline void writeString(std::vector<uint8_t>& buf, std::string_view s, uint8_t maxLen){
	const uint8_t len = (uint8_t) std::min<size_t>(s.size(), maxLen);
	buf.push_back(len);
	buf.insert(buf.end(), s.begin(), s.begin() + len);
}

inline uint8_t readU8(const uint8_t*& cur, const uint8_t* end){
	if(cur >= end) return 0;
	return *cur++;
}

inline std::string readString(const uint8_t*& cur, const uint8_t* end){
	const uint8_t len = readU8(cur, end);
	const size_t avail = (cur < end) ? (size_t)(end - cur) : 0;
	const size_t n = std::min<size_t>(len, avail);
	std::string s((const char*) cur, n);
	cur += n;
	return s;
}

}

/**
 * A payload type that defines its own variable-length wire encoding via serialize().
 * Com dispatches such types through serialize() instead of a raw sizeof() memcpy.
 */
template<typename T>
concept ComSerializable = requires(const T& t){
	{ t.serialize() } -> std::same_as<std::vector<uint8_t>>;
};

#endif //BUTTERBOT_COMMON_SERIALIZATION_H
