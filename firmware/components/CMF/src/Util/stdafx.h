#ifndef CMF_STDAFX_H
#define CMF_STDAFX_H

#include <cstdint>
#include <utility>

/**
 * @brief Function maps the given value [fromLow, fromHigh] to [toLow, toHigh].
 * @tparam T Type being mapped.
 * @param val Value being mapped.
 * @param fromLow Low value from which the mapping is done.
 * @param fromHigh High value from which the mapping is done.
 * @param toLow Low value to which the mapping is done.
 * @param toHigh High value to which the mapping is done.
 * @return Mapped value.
 */
template<typename T> constexpr
T map(T val, decltype(val) fromLow, decltype(val) fromHigh, decltype(val) toLow, decltype(val) toHigh){
    if(fromLow > fromHigh){
        std::swap(fromLow, fromHigh);
    }

    if(toLow > toHigh){
        std::swap(toLow, toHigh);
    }

    return (val - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

/**
 * @return How many milliseconds have passed since the start of the firmware on the device.
 */
uint64_t millis();

/**
 * @return How many microseconds have passed since the start of the firmware on the device.
 */
uint64_t micros();

/**
 * @brief Delays the thread from which the function is called by at least 'micros' microseconds.
 * @param micros How many microseconds is the thread delayed for (at least).
 */
void delayMicros(uint32_t micros);

/**
 * @brief Delays the thread from which the function is called by at least 'millis' milliseconds.
 * @param millis How many milliseconds is the thread delayed for (at least).
 */
void delayMillis(uint32_t millis);

/**
 * @brief Prints out how much of internal RAM heap is free, and how big is the biggest block.
 */
void ramReport();

/**
 * @brief Prints out how much stack was used at the highest usage point (the minimal amount of stack that the thread needs).
 * @param where Used to distinguish multiple function calls in the serial output, only gets printed out.
 */
void stackRep(const char* where = nullptr);

/**
 * @brief Prints out how much 32B and 8B heap is free, and how big is the biggest free block on them.
 * @param where Used to distinguish multiple function calls in the serial output, only gets printed out.
 */
void heapRep(const char* where = nullptr);

#endif //CMF_STDAFX_H