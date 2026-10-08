#ifndef CMF_TIMER_H
#define CMF_TIMER_H

#include "Object/Object.h"
#include "Object/Class.h"
#include <esp_timer.h>

/**
 * Abstraction for ESP's hardware timers (esp_timer).
 *
 * By default the underlying esp_timer uses ESP_TIMER_TASK dispatch: the user callback
 * runs in the esp_timer task (priority ESP_TASK_PRIO_MAX - 3) and may use normal
 * FreeRTOS calls.
 *
 * Passing ESP_TIMER_ISR as the dispatch method requires
 * CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD in the application's sdkconfig (the enum
 * member does not exist without it). The callback then runs directly in ISR context with
 * the SPI flash cache potentially disabled:
 *   - The callback function must be marked IRAM_ATTR.
 *   - Any constants the callback reads must live in DRAM (DRAM_ATTR / static const).
 *   - The callback must not call into flash-resident code or perform blocking work.
 *   - Only ISR-safe FreeRTOS primitives (the *FromISR variants) may be used.
 *
 * ESP Timer HAL takes care of HW allocation when Timers are created (no need to specify
 * timerID and timerGroup). For more control, use GPTimer instead.
 */


class Timer : public Object {
	GENERATED_BODY(Timer, Object, CONSTRUCTOR_PACK(uint32_t, void(*)(void*), void*, const char*, esp_timer_dispatch_t))

public:
	using Callback = void (*)(void* arg);

	/**
	 * @param period Time between periodic callback invocations [ms]
	 * @param callback Callback to be invoked when the period elapses. Runs in the esp_timer task,
	 * unless dispatch is ESP_TIMER_ISR — then it runs in ISR context and must be IRAM_ATTR.
	 * @param arg Opaque pointer forwarded to callback on every invocation.
	 * @param name Name for internal ESP timer HAL (optional, for debugging purposes)
	 * @param dispatch Dispatch method for the callback (task by default).
	 */
	Timer(uint32_t period, Callback callback, void* arg, const char* name = "", esp_timer_dispatch_t dispatch = ESP_TIMER_TASK);
	virtual ~Timer();

	void start();
	void stop();
	void reset();

	/**
	 * Creates a one-time timer, instead of a periodically called one.
	 * @param delay Delay before the callback is invoked [ms]
	 * @param callback Callback to be invoked when the delay elapses. Runs in the esp_timer task.
	 * @param arg Opaque pointer forwarded to callback when it fires.
	 */
	static void single(uint32_t delay, Callback callback, void* arg);

	void setPeriod(uint32_t period);

private:
	esp_timer_handle_t timer;
	uint64_t period;
	char timerName[32];
};


#endif //CMF_TIMER_H
