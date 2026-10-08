#include "Archive.h"

Archive::Archive(std::span<const uint8_t> queue) noexcept {
	for(uint8_t value : queue) {
		pushDataPoint(value);
	}
}

Archive::Archive(const std::queue<uint8_t>& queue) noexcept : byteBuffer(queue) {}

Archive::Archive(std::queue<uint8_t>&& queue) noexcept : byteBuffer(std::move(queue)) {}

void Archive::toByteArray(std::vector<uint8_t>& buffer) const noexcept {
	buffer.clear();

	if(buffer.capacity() < byteBuffer.size()) {
		buffer.reserve(byteBuffer.size());
	}

	std::queue<uint8_t> bufferCopy = byteBuffer;

	while(!bufferCopy.empty()){
		buffer.emplace_back(bufferCopy.front());
		bufferCopy.pop();
	}
}

void Archive::pushDataPoint(uint8_t data) noexcept {
	byteBuffer.push(data);
}

bool Archive::popDataPoint(uint8_t& data) noexcept {
	if(byteBuffer.empty()){
		return false;
	}

	const uint8_t value = byteBuffer.front();
	data = value;
	byteBuffer.pop();

	return true;
}

InArchive::InArchive(std::span<const uint8_t> queue) noexcept : Archive(queue) {}

InArchive::InArchive(const std::queue<uint8_t>& queue) noexcept : Archive(queue) {}

InArchive::InArchive(std::queue<uint8_t>&& queue) noexcept : Archive(queue) {}

Archive& InArchive::operator << (bool& data) noexcept {
	pushData<bool>(data);
	return *this;
}

Archive& InArchive::operator << (wchar_t& data) noexcept {
	pushData<wchar_t>(data);
	return *this;
}

Archive& InArchive::operator << (uint8_t& data) noexcept {
	pushData<uint8_t>(data);
	return *this;
}

Archive& InArchive::operator << (uint16_t& data) noexcept {
	pushData<uint16_t>(data);
	return *this;
}

Archive& InArchive::operator << (uint32_t& data) noexcept {
	pushData<uint32_t>(data);
	return *this;
}

Archive& InArchive::operator << (uint64_t& data) noexcept {
	pushData<uint64_t>(data);
	return *this;
}

Archive& InArchive::operator << (int8_t& data) noexcept {
	pushData<int8_t>(data);
	return *this;
}

Archive& InArchive::operator << (int16_t& data) noexcept {
	pushData<int16_t>(data);
	return *this;
}

Archive& InArchive::operator << (int32_t& data) noexcept {
	pushData<int32_t>(data);
	return *this;
}

Archive& InArchive::operator << (int64_t& data) noexcept {
	pushData<int64_t>(data);
	return *this;
}

Archive& InArchive::operator << (float& data) noexcept {
	pushData<float>(data);
	return *this;
}

Archive& InArchive::operator << (double& data) noexcept {
	pushData<double>(data);
	return *this;
}

Archive& InArchive::operator << (long double& data) noexcept {
	pushData<long double>(data);
	return *this;
}
Archive& InArchive::operator << (std::string& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<char>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator<<(std::wstring& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<wchar_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<bool>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<bool>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<wchar_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<wchar_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<uint8_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<uint8_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<uint16_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<uint16_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<uint32_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<uint32_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<uint64_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<uint64_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<int8_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<int8_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<int16_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<int16_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<int32_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<int32_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<int64_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<int64_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<float>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<float>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<double>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<double>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<long double>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<long double>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (std::vector<std::string>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		operator<<(data[i]);
	}

	return *this;
}

Archive& InArchive::operator<<(std::vector<std::wstring>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		operator<<(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (bool data) noexcept {
	pushData<bool>(data);
	return *this;
}

Archive& InArchive::operator << (wchar_t data) noexcept {
	pushData<wchar_t>(data);
	return *this;
}

Archive& InArchive::operator << (uint8_t data) noexcept {
	pushData<uint8_t>(data);
	return *this;
}

Archive& InArchive::operator << (uint16_t data) noexcept {
	pushData<uint16_t>(data);
	return *this;
}

Archive& InArchive::operator << (uint32_t data) noexcept {
	pushData<uint32_t>(data);
	return *this;
}

Archive& InArchive::operator << (uint64_t data) noexcept {
	pushData<uint64_t>(data);
	return *this;
}

Archive& InArchive::operator << (int8_t data) noexcept {
	pushData<int8_t>(data);
	return *this;
}

Archive& InArchive::operator << (int16_t data) noexcept {
	pushData<int16_t>(data);
	return *this;
}

Archive& InArchive::operator << (int32_t data) noexcept {
	pushData<int32_t>(data);
	return *this;
}

Archive& InArchive::operator << (int64_t data) noexcept {
	pushData<int64_t>(data);
	return *this;
}

Archive& InArchive::operator << (float data) noexcept {
	pushData<float>(data);
	return *this;
}

Archive& InArchive::operator << (double data) noexcept {
	pushData<double>(data);
	return *this;
}

Archive& InArchive::operator << (long double data) noexcept {
	pushData<long double>(data);
	return *this;
}

Archive& InArchive::operator << (const std::string& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<char>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::wstring& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<wchar_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<bool>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<bool>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<wchar_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<wchar_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<uint8_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<uint8_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<uint16_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<uint16_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<uint32_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<uint32_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<uint64_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<uint64_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<int8_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<int8_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<int16_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<int16_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<int32_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<int32_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<int64_t>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<int64_t>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<float>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<float>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<double>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<double>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<long double>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		pushData<long double>(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<std::string>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		operator<<(data[i]);
	}

	return *this;
}

Archive& InArchive::operator << (const std::vector<std::wstring>& data) noexcept {
	pushData<size_t>(data.size());

	for(size_t i = 0; i < data.size(); ++i){
		operator<<(data[i]);
	}

	return *this;
}

OutArchive::OutArchive(std::span<const uint8_t> queue) noexcept : Archive(queue) {}

OutArchive::OutArchive(const std::queue<uint8_t>& queue) noexcept : Archive(queue) {}

OutArchive::OutArchive(std::queue<uint8_t>&& queue) noexcept : Archive(queue) {}

Archive& OutArchive::operator << (bool& data) noexcept {
	popData<bool>(data);
	return *this;
}

Archive& OutArchive::operator << (wchar_t& data) noexcept {
	popData<wchar_t>(data);
	return *this;
}

Archive& OutArchive::operator << (uint8_t& data) noexcept {
	popData<uint8_t>(data);
	return *this;
}

Archive& OutArchive::operator << (uint16_t& data) noexcept {
	popData<uint16_t>(data);
	return *this;
}

Archive& OutArchive::operator << (uint32_t& data) noexcept {
	popData<uint32_t>(data);
	return *this;
}

Archive& OutArchive::operator << (uint64_t& data) noexcept {
	popData<uint64_t>(data);
	return *this;
}

Archive& OutArchive::operator << (int8_t& data) noexcept {
	popData<int8_t>(data);
	return *this;
}

Archive& OutArchive::operator << (int16_t& data) noexcept {
	popData<int16_t>(data);
	return *this;
}

Archive& OutArchive::operator << (int32_t& data) noexcept {
	popData<int32_t>(data);
	return *this;
}

Archive& OutArchive::operator << (int64_t& data) noexcept {
	popData<int64_t>(data);
	return *this;
}

Archive& OutArchive::operator << (float& data) noexcept {
	popData<float>(data);
	return *this;
}

Archive& OutArchive::operator << (double& data) noexcept {
	popData<double>(data);
	return *this;
}

Archive& OutArchive::operator << (long double& data) noexcept {
	popData<long double>(data);
	return *this;
}

Archive& OutArchive::operator << (std::string& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);

	for(size_t i = 0; i < size; ++i){
		char c;
		if(popData<char>(c)){
			data += c;
		}
	}

	return *this;
}

Archive& OutArchive::operator<<(std::wstring& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);

	for(size_t i = 0; i < size; ++i){
		wchar_t c;
		popData<wchar_t>(c);
		data += c;
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<bool>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		bool val = false;
		popData<bool>(val);
		data[i] = val;
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<wchar_t>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<wchar_t>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<uint8_t>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<uint8_t>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<uint16_t>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<uint16_t>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<uint32_t>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<uint32_t>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<uint64_t>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<uint64_t>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<int8_t>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<int8_t>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<int16_t>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<int16_t>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<int32_t>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<int32_t>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<int64_t>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<int64_t>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<float>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<float>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<double>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<double>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<long double>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		popData<long double>(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator << (std::vector<std::string>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		operator<<(data[i]);
	}

	return *this;
}

Archive& OutArchive::operator<<(std::vector<std::wstring>& data) noexcept {
	size_t size = 0;
	popData<size_t>(size);
	data.resize(size);

	for(size_t i = 0; i < size; ++i){
		operator<<(data[i]);
	}

	return *this;
}