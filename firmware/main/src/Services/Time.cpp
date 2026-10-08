#include "Time.h"
#include "Util/stdafx.h"

DEFINE_LOG(Time)

//Note - task stack size can be lowered to 2kB if log level is set below INFO
Time::Time(BM8563* rtc, bool internalStack) : AsyncEntity(UpdateInterval, 2*1024, 5, -1, internalStack), rtc(rtc){
	updateFromRTC();
}

std::tm Time::getTime() const{
	time_t currentTime = time + (millis() - updateTime) / 1000;
	tm ret = {};
	gmtime_r(&currentTime, &ret);
	return ret;
}

void Time::setTime(tm time_tm){
	CMF_LOG(Time, LogLevel::Info, "Updating time by tm");

	updateTime = millis();

	rtc->setTime(time_tm);
	time = mktime(&time_tm);

	OnTimeUpdate.broadcast(time_tm);
}

void Time::setTime(time_t time){
	CMF_LOG(Time, LogLevel::Info, "Updating time by time_t");

	updateTime = millis();

	tm time_tm = {};
	gmtime_r(&time, &time_tm);

	rtc->setTime(time_tm);
	Time::time = time;

	OnTimeUpdate.broadcast(time_tm);
}

bool Time::isConfigured() const{
	return getTime().tm_year >= 124;
}

void Time::tick(float deltaTime) noexcept{
	CMF_LOG(Time, LogLevel::Debug, "Scheduled RTC sync");
	auto time_tm = updateFromRTC();
	OnTimeUpdate.broadcast(time_tm);
}

tm Time::updateFromRTC(){
	updateTime = millis();

	tm time_tm = rtc->getTime();
	time = mktime(&time_tm);
	return time_tm;
}
