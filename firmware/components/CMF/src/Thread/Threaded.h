#ifndef CMF_THREADED_H
#define CMF_THREADED_H

#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <functional>
#include <atomic>

/**
 * @brief Thread object that runs continuously with control of its properties like stack size,
 * priority, core and interval, and an optional callback function that is called once per loop.
 * The thread DOES NOT start running immediately as constructed, instead needs to be started manually.
 */
class Threaded {
protected:
	/**
	 * @brief Constructs a thread without a callback.
	 * @param threadName Name of the thread.
	 * @param interval How often does the thread loop get called.
	 * @param threadStackSize Size of the thread stack.
	 * @param threadPriority How high of a priority the thread is, comes into effect when context switching between threads happens.
	 * @param cpuCore Which CPU code the thread should run on. If -1, it will run on any available without preference.
	 * @param internalStack If true the task stack is allocated in internal SRAM; otherwise (default) it is allocated in
	 * external PSRAM to conserve internal SRAM. Tasks that run code disabling the flash cache (NVS / SPI-flash writes)
	 * MUST set this to true, since a PSRAM stack is unreachable while the cache is disabled.
	 */
	explicit Threaded(const std::string& threadName = "", TickType_t interval = CONFIG_CMF_THREADED_INTERVAL / portTICK_PERIOD_MS, size_t threadStackSize = CONFIG_CMF_THREADED_STACK_SIZE,
		uint8_t threadPriority = CONFIG_CMF_THREADED_PRIORITY, int8_t cpuCore = CONFIG_CMF_THREADED_CPU_CORE, bool internalStack = true) noexcept;

public:
	/**
	 * @brief Constructs a thread with a callback function that is called one per loop cycle.
	 * @param fn Callback function.
	 * @param threadName Name of the thread.
	 * @param interval How often does the thread loop.
	 * @param threadStackSize Size of the thread stack.
	 * @param threadPriority How high of a priority the thread is, comes into effect when context switching between threads happens.
	 * @param cpuCore Which CPU core the thread should run on. If -1, it will run on any available without preference.
	 * @param internalStack If true the task stack is allocated in internal SRAM; otherwise (default) it is allocated in
	 * external PSRAM to conserve internal SRAM. Tasks that run code disabling the flash cache (NVS / SPI-flash writes)
	 * MUST set this to true, since a PSRAM stack is unreachable while the cache is disabled.
	 */
	Threaded(const std::function<void(void)>& fn, const std::string& threadName = "", TickType_t interval = CONFIG_CMF_THREADED_INTERVAL / portTICK_PERIOD_MS, size_t threadStackSize = CONFIG_CMF_THREADED_STACK_SIZE,
		uint8_t threadPriority = CONFIG_CMF_THREADED_PRIORITY, int8_t cpuCore = CONFIG_CMF_THREADED_CPU_CORE, bool internalStack = true) noexcept;

	/**
	 * @brief Destructor checks if the thread is stopped and deletes all semaphores that the thread uses.
	 */
	virtual ~Threaded() noexcept;

public:

	/**
	 * @brief Creates a native FreeRTOS thread.
	 */
	void start() noexcept;

	/**
	 * @brief Stops the thread, waits up to 'wait' milliseconds for the thread to stop operations.
	 * @param wait How long the function call should wait for the native thread to stop its operations before proceeding with deinitialization.
	 */
	void stop(TickType_t wait = portMAX_DELAY) noexcept;

	/**
	 * @brief Pauses the thread execution.
	 */
	void pause() noexcept;

	/**
	 * @brief Resumes a paused thread. Does nothing if the thread is not paused.
	 */
	void resume() noexcept;

	/**
	 * @brief Check if the thread is currently running its execution. A paused thread does not count as running.
	 * @return Returns if the thread is currently running execution or not.
	 */
	bool running() const noexcept;

	/**
	 * @brief Set how much time should pass between loops of the thread.
	 * @param value How many milliseconds should pass between loops of the thread.
	 */
	void setInterval(TickType_t value) noexcept;

	/**
	 * @return The interval between loop calls of the thread.
	 */
	TickType_t getInterval() const noexcept;

public:
	/**
	 * @brief Called when the thread execution is started or resumed.
	 * @return If the thread initialization should continue or abort.
	 */
	virtual bool onStart() noexcept;

	/**
	 * @brief Called when the thread execution is finished. Not called when a thread is paused.
	 * @return
	 */
	virtual bool onStop() noexcept;

	/**
	 * @brief Loops as often as possible determined by the interval of the thread.
	 * Calls the callback function every loop cycle, if any is set to the thread.
	 */
	virtual void loop() noexcept;

private:
	/**
	 * @brief The state in which the thread is at the moment.
	 * It can be either running, stopping - waiting to finish or stopped.
	 */
	enum class State : uint8_t {
		Running,
		Stopping,
		Stopped
	};

private:
	std::string name;
	std::atomic<TickType_t> loopInterval;
	size_t stackSize;
	const uint8_t priority;
	const int8_t core;
	const bool internalStack;

	State state = State::Stopped;
	bool paused = false;
	TickType_t lastLoop = 0;
	std::function<void(void)> lambdaLoop;

	TaskHandle_t task = nullptr;
	SemaphoreHandle_t stopSemaphore;
	SemaphoreHandle_t stopMutex;
	SemaphoreHandle_t pauseSemaphore;

private:
	/**
	 * @brief The function that is actually set as a bound callback to the native FreeRTOS thread.
	 */
	void threadFunction() noexcept;
};

#endif //CMF_THREADED_H