#include "Timer.h"
#include "Log/Log.h"

DEFINE_LOG(Timer)

Timer::Timer(uint32_t period, Callback callback, void* arg, const char* name, esp_timer_dispatch_t dispatch) : period(period * 1000){
	snprintf(timerName, sizeof(timerName), "Tmr-%s", name);

	esp_timer_create_args_t args = {
			.callback = callback,
			.arg = arg,
			.dispatch_method = dispatch,
			.name = timerName,
			.skip_unhandled_events = true
	};
	ESP_ERROR_CHECK(esp_timer_create(&args, &timer));
}

Timer::~Timer(){
	stop();
	esp_timer_delete(timer);
}

void Timer::start(){
	esp_timer_start_once(timer, period);
}

void Timer::stop(){
	esp_timer_stop(timer);
}

void Timer::reset(){
	esp_timer_stop(timer);
	esp_timer_start_once(timer, period);
}

namespace {
	struct SingleShotCtx {
		Timer::Callback cb;
		void* arg;
		esp_timer_handle_t handle;
	};

	void singleShotTrampoline(void* arg){
		auto* ctx = static_cast<SingleShotCtx*>(arg);
		if(ctx == nullptr) return;
		if(ctx->cb) ctx->cb(ctx->arg);
		esp_timer_delete(ctx->handle);
		delete ctx;
	}
}

void Timer::single(uint32_t delay, Callback callback, void* arg){
	auto* ctx = new SingleShotCtx{ callback, arg, nullptr };

	const esp_timer_create_args_t args = {
			.callback = singleShotTrampoline,
			.arg = ctx,
			.dispatch_method = ESP_TIMER_TASK, // task dispatch so the trampoline can self-delete
			.name = "Single",
			.skip_unhandled_events = true
	};

	if(esp_timer_create(&args, &ctx->handle) != ESP_OK){
		delete ctx;
		return;
	}

	esp_timer_start_once(ctx->handle, (uint64_t) delay * 1000);
}

void Timer::setPeriod(uint32_t period){
	if(esp_timer_is_active(timer)){
		CMF_LOG(Timer, LogLevel::Error, "setPeriod called while timer is running");
		return;
	}

	this->period = period*1000;
}
