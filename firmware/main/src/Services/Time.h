#ifndef BUTTERBOT_FIRMWARE_TIME_H
#define BUTTERBOT_FIRMWARE_TIME_H

#include "Devices/BM8563.h"
#include "Entity/AsyncEntity.h"
#include "Event/EventBroadcaster.h"

class Time : public AsyncEntity {
	GENERATED_BODY(Time, AsyncEntity, CONSTRUCTOR_PACK(BM8563*))
public:
	Time(BM8563* rtc, bool internalStack = true);
	~Time() override = default;

	DECLARE_EVENT(TimeUpdateEvent, Time, tm);
	TimeUpdateEvent OnTimeUpdate = TimeUpdateEvent(this);

	tm getTime() const;
	void setTime(tm time_tm);
	void setTime(time_t time);

	// True once the RTC reports a year that could plausibly be current (>= 2024).
	// On a cold RTC with no battery backup, registers come up at 1900 — that's our "unconfigured" signal.
	bool isConfigured() const;

private:
	StrongObjectPtr<BM8563> rtc;

	static constexpr uint32_t UpdateInterval = 5000; // [ms]
	uint64_t updateTime = 0;

	time_t time;

	void tick(float deltaTime) noexcept override;

	tm updateFromRTC();

};


#endif //BUTTERBOT_FIRMWARE_TIME_H
