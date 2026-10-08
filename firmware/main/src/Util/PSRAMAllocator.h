#ifndef BUTTERBOT_FIRMWARE_PSRAMALLOCATOR_H
#define BUTTERBOT_FIRMWARE_PSRAMALLOCATOR_H

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <string>
#include <memory>
#include <new>
#include <utility>
#include <esp_heap_caps.h>
#include <esp_rom_sys.h>

template<typename T>
class PSRAMAllocator {
public:
	using value_type = T;

	PSRAMAllocator() noexcept = default;

	template<typename U>
	constexpr PSRAMAllocator(const PSRAMAllocator<U>&) noexcept{}

	T* allocate(std::size_t n){
		void* p = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
		if(p == nullptr){
			esp_rom_printf("PSRAMAllocator: out of SPIRAM allocating %u B\n", (unsigned) (n * sizeof(T)));
			abort();
		}
		return static_cast<T*>(p);
	}

	void deallocate(T* p, std::size_t) noexcept{
		heap_caps_free(p);
	}
};

// Stateless: all instances are interchangeable, so container moves/swaps never reallocate.
template<typename A, typename B>
constexpr bool operator==(const PSRAMAllocator<A>&, const PSRAMAllocator<B>&) noexcept{ return true; }

template<typename A, typename B>
constexpr bool operator!=(const PSRAMAllocator<A>&, const PSRAMAllocator<B>&) noexcept{ return false; }

template<typename T>
using PSRAMVector = std::vector<T, PSRAMAllocator<T>>;

// Canonical byte payload buffer kept in PSRAM.
using PSRAMByteBuffer = PSRAMVector<uint8_t>;

// std::string backed by PSRAM. Converts to std::string_view like any basic_string.
using PSRAMString = std::basic_string<char, std::char_traits<char>, PSRAMAllocator<char>>;

template<typename T>
using PSRAMUnique = std::unique_ptr<T, void (*)(T*)>;

// make_unique equivalent guaranteed in PSRAM, regardless of CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL.
template<typename T, typename... Args>
PSRAMUnique<T> makePSRAM(Args&&... args){
	void* mem = heap_caps_malloc(sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
	if(mem == nullptr){
		esp_rom_printf("makePSRAM: out of SPIRAM allocating %u B\n", (unsigned) sizeof(T));
		abort();
	}
	return PSRAMUnique<T>(new (mem) T(std::forward<Args>(args)...), [](T* p){
		p->~T();
		heap_caps_free(p);
	});
}

#endif //BUTTERBOT_FIRMWARE_PSRAMALLOCATOR_H
