#ifndef CMF_ARCHIVE_H
#define CMF_ARCHIVE_H

#include <cstdint>
#include <queue>
#include <span>
#include <string>
#include <vector>

/**
 * @brief The base archive class, functioning as the backbone and implementation of internal works for the in-only and out-only archives.
 */
class Archive {
public:
	/**
	 * @brief Default constructor.
	 */
	Archive() noexcept = default;

	/**
	 * @brief Constructor from a given byte array.
	 * @param queue Span of bytes containing the archive data.
	 */
	Archive(std::span<const uint8_t> queue) noexcept;

	/**
	 * @brief Constructor from a give byte queue.
	 * @param queue Queue containing the archive data.
	 */
	Archive(const std::queue<uint8_t>& queue) noexcept;

	/**
	 * @brief Move constructor from a give byte queue.
	 * @param queue Queue containing the archive data. Queue is empty after constructor finishes execution.
	 */
	Archive(std::queue<uint8_t>&& queue) noexcept;

	inline size_t size() const noexcept { return byteBuffer.size(); }

	/**
	 * @brief Default destructor.
	 */
	virtual ~Archive() noexcept = default;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (bool& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (wchar_t& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (uint8_t& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (uint16_t& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (uint32_t& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (uint64_t& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (int8_t& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (int16_t& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (int32_t& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (int64_t& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (float& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (double& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (long double& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::string& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::wstring& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<bool>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<wchar_t>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint8_t>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint16_t>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint32_t>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint64_t>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int8_t>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int16_t>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int32_t>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int64_t>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<float>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<double>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<long double>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<std::string>& data) noexcept = 0;

	/**
	 * @brief The in/out operator that reads from internal data buffer, or adds data to the internal data buffer.
	 * @param data The data being put in / given out.
	 * @return Reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<std::wstring>& data) noexcept = 0;

	/**
	 * @brief Converts the archive to a byte array.
	 * @param buffer The byte vector the data is put in.
	 */
	void toByteArray(std::vector<uint8_t>& buffer) const noexcept;

protected:
	/**
	 * @brief Pushes a byte of data to the internal data buffer.
	 * @param data The byte being pushed in the internal buffer.
	 */
	void pushDataPoint(uint8_t data) noexcept;

	/**
	 * @brief Pops a byte of data from the internal buffer.
	 * @param data The data variable being set to the retrieved value from the internal buffer.
	 * @return True if successful, false otherwise.
	 */
	bool popDataPoint(uint8_t& data) noexcept;

	/**
	 * @brief Default deleted template for pushing templated data type to the internal buffer.
	 * @tparam T The type of data being pushed to the internal buffer.
	 * @param data The data being pushed to the internal buffer.
	 */
	template<typename T>
	void pushData(T data) noexcept = delete;

	/**
	 * @brief Implementation of the template function for pushing type specific data to the internal buffer.
	 * The function is limited only to primitive data types.
	 * @tparam T The type of data being pushed to the internal buffer.
	 * @param data The data being pushed to the internal buffer.
	 */
	template<typename T>
	void pushData(T data) noexcept requires (std::is_fundamental<T>::value) {
		uint8_t* dataAsBuffer = reinterpret_cast<uint8_t*>(&data);
		if(dataAsBuffer == nullptr){
			return;
		}

		for(size_t i = 0; i < sizeof(T); ++i){
			pushDataPoint(dataAsBuffer[i]);
		}
	}

	/**
	 * @brief Deleted template function for popping type specific data from the internal buffer.
	 * @tparam T The type of data being popped from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return True if successful, false otherwise.
	 */
	template<typename T>
	bool popData(T& data) noexcept = delete;

	/**
	 * @brief Implementation of the template function for popping type specific data from the internal buffer.
	 * @tparam T The type of data being popped from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return True if successful, false otherwise.
	 */
	template<typename T>
	bool popData(T& data) noexcept requires (std::is_fundamental<T>::value) {
		uint8_t* dataAsBuffer = reinterpret_cast<uint8_t*>(&data);
		if(dataAsBuffer == nullptr){
			return false;
		}

		for(size_t i = 0; i < sizeof(T); ++i){
			if(!popDataPoint(dataAsBuffer[i])){
				return false;
			}
		}

		return true;
	}

private:
	std::queue<uint8_t> byteBuffer;
};

/**
 * @brief The in-only implementation of the Archive class. Only allows data to flow in,
 * needs to be converted to an out-only archive for the data to be retrieved.
 */
class InArchive : public Archive {
public:
	/**
	 * @brief Default empty constructor.
	 */
	InArchive() noexcept = default;

	/**
	 * @brief Constructor from a span of byte data treated as a queue.
	 * @param queue The span of byte data.
	 */
	InArchive(std::span<const uint8_t> queue) noexcept;

	/**
	 * @brief Constructor from a queue of byte data.
	 * @param queue The queue of byte data.
	 */
	InArchive(const std::queue<uint8_t>& queue) noexcept;

	/**
	 * @brief Move constructor from a queue of byte data.
	 * @param queue The queuue of byte data. The queue is empty after the constructor finishes execution.
	 */
	InArchive(std::queue<uint8_t>&& queue) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (bool& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (wchar_t& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint8_t& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint16_t& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint32_t& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint64_t& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int8_t& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int16_t& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int32_t& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int64_t& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (float& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (double& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (long double& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::string& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::wstring& data) noexcept override;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<bool>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<wchar_t>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint8_t>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint16_t>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint32_t>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint64_t>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int8_t>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int16_t>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int32_t>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int64_t>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<float>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<double>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<long double>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<std::string>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<std::wstring>& data) noexcept override final;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (bool data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (wchar_t data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint8_t data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint16_t data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint32_t data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint64_t data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int8_t data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int16_t data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int32_t data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int64_t data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (float data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (double data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (long double data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::string& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::wstring& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<bool>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<wchar_t>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<uint8_t>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<uint16_t>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<uint32_t>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<uint64_t>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<int8_t>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<int16_t>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<int32_t>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<int64_t>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<float>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<double>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<long double>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<std::string>& data) noexcept;

	/**
	 * @brief The in operator that adds given data to the internal buffer.
	 * @param data The data flowing into the archive.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (const std::vector<std::wstring>& data) noexcept;
};

/**
 * @brief The out-only implementation of the Archive class. Only allows data to flow out,
 * needs to be converted to an in-only archive for the data to be set.
 */
class OutArchive : public Archive {
public:
	/**
	 * @brief The default empty constructor.
	 */
	OutArchive() noexcept = default;

	/**
	 * @brief The constructor from a span of byte data treated as a queue.
	 * @param queue The span of byte data.
	 */
	OutArchive(std::span<const uint8_t> queue) noexcept;

	/**
	 * @brief The constructor from a queue of byte data.
	 * @param queue The queue of byte data.
	 */
	OutArchive(const std::queue<uint8_t>& queue) noexcept;

	/**
	 * @brief The move constructor from a queue of byte data.
	 * @param queue The queue of byte data. The queue remains empty after the constructor finished execution.
	 */
	OutArchive(std::queue<uint8_t>&& queue) noexcept;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (bool& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (wchar_t& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint8_t& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint16_t& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint32_t& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (uint64_t& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int8_t& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int16_t& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int32_t& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (int64_t& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (float& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (double& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (long double& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::string& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::wstring& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<bool>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<wchar_t>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint8_t>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint16_t>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint32_t>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<uint64_t>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int8_t>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int16_t>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int32_t>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<int64_t>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<float>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<double>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<long double>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<std::string>& data) noexcept override final;

	/**
	 * @brief The out operator that retrieves given data from the internal buffer.
	 * @param data The variable being set to the data in the internal buffer.
	 * @return The reference to the archive called on.
	 */
	virtual Archive& operator << (std::vector<std::wstring>& data) noexcept override final;
};

#endif //CMF_ARCHIVE_H