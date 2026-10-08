#include "Threaded.h"
#include "Util/stdafx.h"
#include "Log/Log.h"
#include <esp_heap_caps.h>
#include <freertos/idf_additions.h>

Threaded::Threaded(const std::string& threadName, TickType_t interval /*= CONFIG_CMF_THREADED_INTERVAL / portTICK_PERIOD_MS*/,
	size_t threadStackSize /*= CONFIG_CMF_THREADED_STACK_SIZE*/, uint8_t threadPriority /*= CONFIG_CMF_THREADED_PRIORITY*/, int8_t cpuCore /*= CONFIG_CMF_THREADED_CPU_CORE*/, bool internalStack /*= true*/) noexcept :
						name(threadName), loopInterval(interval), stackSize(threadStackSize), priority(threadPriority), core(cpuCore), internalStack(internalStack) {
	stopSemaphore = xSemaphoreCreateBinary();
	stopMutex = xSemaphoreCreateMutex();
	pauseSemaphore = xSemaphoreCreateBinary();
}

Threaded::Threaded(const std::function<void(void)>& fn, const std::string& threadName, TickType_t interval /*= CONFIG_CMF_THREADED_INTERVAL / portTICK_PERIOD_MS*/,
	size_t threadStackSize /*= CONFIG_CMF_THREADED_STACK_SIZE*/, uint8_t threadPriority /*= CONFIG_CMF_THREADED_PRIORITY*/, int8_t cpuCore /*= CONFIG_CMF_THREADED_CPU_CORE*/, bool internalStack /*= true*/) noexcept :
						name(threadName), loopInterval(interval), stackSize(threadStackSize), priority(threadPriority), core(cpuCore), internalStack(internalStack), lambdaLoop(fn) {
	stopSemaphore = xSemaphoreCreateBinary();
	stopMutex = xSemaphoreCreateMutex();
	pauseSemaphore = xSemaphoreCreateBinary();
}

Threaded::~Threaded() noexcept{
	stop(portMAX_DELAY);

	if(state != State::Stopped){
		CMF_LOG(CMF, LogLevel::Error, "Thread was destroyed before being stopped.");
		abort();
	}

	vSemaphoreDelete(stopSemaphore);
	vSemaphoreDelete(stopMutex);
	vSemaphoreDelete(pauseSemaphore);
}

void Threaded::start() noexcept{
	if(state != State::Stopped){
		return;
	}

	if(!onStart()){
		return;
	}

	state = State::Running;

	auto function = [](void* arg) -> void {
		if(arg == nullptr){
			CMF_LOG(CMF, Error, "Thread native callback started with invalid thread pointer.");
			vTaskDeleteWithCaps(nullptr); // created WithCaps: plain vTaskDelete would leak the TCB and stack
			return;
		}

		static_cast<Threaded*>(arg)->threadFunction();
	};

	const uint32_t stackCaps = internalStack ? (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) : MALLOC_CAP_SPIRAM;

	if(core == -1){
		const auto ret = xTaskCreateWithCaps(function, name.c_str(), stackSize, this, priority, &task, stackCaps);
		if(ret != pdPASS){
			CMF_LOG(CMF, LogLevel::Error, "xTaskCreateWithCaps failed for thread '%s' (ret=%d)", name.c_str(), (int) ret);
			assert(ret == pdPASS);
		}
	}else{
		const auto ret = xTaskCreatePinnedToCoreWithCaps(function, name.c_str(), stackSize, this, priority, &task, core, stackCaps);
		if(ret != pdPASS){
			CMF_LOG(CMF, LogLevel::Error, "xTaskCreatePinnedToCoreWithCaps failed for thread '%s' on core %d (ret=%d)", name.c_str(), (int) core, (int) ret);
			assert(ret == pdPASS);
		}
	}
}

void Threaded::stop(TickType_t wait/* = portMAX_DELAY*/) noexcept{
	if(xSemaphoreTake(stopMutex, wait) == pdFALSE){
		return;
	}

	if(state != State::Running){
		xSemaphoreGive(stopMutex);
		return;
	}

	state = State::Stopping;

	xSemaphoreTake(stopSemaphore, wait);
	xSemaphoreGive(stopMutex);
}

void Threaded::pause() noexcept{
	if(paused){
		return;
	}

	xSemaphoreGive(pauseSemaphore);
}

void Threaded::resume() noexcept{
	paused = false;
	start();
}

bool Threaded::running() const noexcept{
	return state != State::Stopped;
}

void Threaded::setInterval(TickType_t value) noexcept{
	loopInterval = value;
}

TickType_t Threaded::getInterval() const noexcept{
	return loopInterval;
}

bool Threaded::onStart() noexcept{
	return true;
}

bool Threaded::onStop() noexcept{
	return true;
}

void Threaded::loop() noexcept{
	if(!lambdaLoop){
		return;
	}

	lambdaLoop();
}

void Threaded::threadFunction() noexcept{
	while(state == State::Running){
		TickType_t semaphoreWait = 0;

		if(millis() - lastLoop < loopInterval){
			semaphoreWait = loopInterval - (millis() - lastLoop);
		}

		if(xSemaphoreTake(pauseSemaphore, semaphoreWait) == pdTRUE){
			stop(0);
			paused = true;
			break;
		}

		lastLoop = millis();

		loop();
	}

	onStop();

	state = State::Stopped;
	xSemaphoreGive(stopSemaphore);

	vTaskDeleteWithCaps(nullptr); // created WithCaps: plain vTaskDelete would leak the TCB and stack
}
