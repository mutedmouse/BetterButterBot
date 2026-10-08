#include "Battery.h"
#include "Pins.hpp"
#include "Memory/ObjectMemory.h"
#include <Util/stdafx.h>
#include <driver/gpio.h>
#include <algorithm>

DEFINE_LOG(Battery)

SemaphoreHandle_t Battery::sem;

Battery::Battery(OutputPin refSwitch) : refSwitch(refSwitch) {

	sem = xSemaphoreCreateBinary();

	// Unfiltered reader: returns the calibrated voltage [mV] at the divider tap. Smoothing and the
	// conversion to a percentage are done in computePercent().
	readerBatt = newObject<ADCReader>(this, (gpio_num_t)PIN_BATT, cfg, true);

	calibrate();

	BaseBoard* baseBoard = getApp()->getService<BaseBoard>();
	auto currentChargingState = baseBoard->getChargingState();
	if(currentChargingState != lastCharging){
		lastCharging = currentChargingState;
		OnChargeStatus.broadcast(lastCharging);
	}
	baseBoard->onChargeChanged.bind(this, &Battery::chargeChanged);

	timer = newObject<Timer>(this, ShortMeasureIntverval, timerFunc, nullptr, "battery");

	sample(true);

	//psram
	xTaskCreateWithCaps([](void* arg) {
		auto _this = (Battery*)arg;
		while(1){
			_this->tick();
		}
	}, "Battery", 3 * 1024, this, 5, &batThread, MALLOC_CAP_SPIRAM);

	timer->start();
}

Battery::~Battery(){
	timer->stop();
	abortFlag = true;
	xSemaphoreGive(sem);
	vTaskDeleteWithCaps(batThread);
	delete timer.get();
	vSemaphoreDelete(sem);
}

bool Battery::isShutdown() const{
	return shutdown;
}

float Battery::getPerc() const{
	return percent;
}

Battery::Level Battery::getLevel() const{
	return (Level)hysteresis.get();
}

ChargingState Battery::getChargingState() const{
	return lastCharging;
}

void Battery::setSleep(bool sleep){
	if(!sleep){
		CMF_LOG(Battery, Info, "Battery reconfigure on wake\n");
		inSleepReconfigure();
	}

	timer->stop();
	std::lock_guard lock(mut);

	emaFactor = sleep ? EmaA_sleep : EmaA;

	this->sleep = sleep;
	xSemaphoreGive(sem);
}

void Battery::calibrate(){
	refSwitch.driver->write(refSwitch.port, true);

	delayMillis(100);
	for(int i = 0; i < CalReads; i++){
		readerBatt->sample();
		delayMillis(10);
	}

	float total = 0;
	for(int i = 0; i < CalReads; i++){
		// readerBatt is unfiltered, so apply the divider factor here to match the previous reference reading.
		total += readerBatt->sample() * Factor;
		delayMillis(10);
	}

	const float reading = total / (float)CalReads;
	const float offset = CalExpected - reading;
	calibOffset += offset;

	refSwitch.driver->write(refSwitch.port, false);
	delayMillis(100);

	CMF_LOG(Battery, LogLevel::Info, "Calibration: Read %.02f mV, expected %.02f mV. Applying %.02f mV offset.\n", reading, CalExpected, offset);
}

void Battery::sample(bool fresh){
	if(isShutdown()) return;
	if(getChargingState() != ChargingState::Unplugged && !fresh) return;

	auto oldLevel = getLevel();

	const float value = computePercent(readerBatt->sample(), fresh);
	if(fresh){
		hysteresis.reset(value);
	} else{
		hysteresis.update(value);
	}

	if(oldLevel != getLevel() || fresh){
		OnLevelChanged.broadcast(getLevel());
	}

	if(getLevel() == Level::Critical){
		shutdown = true;
		return;
	}
}

float Battery::computePercent(float rawMillivolts, bool fresh){
	if(fresh || emaValue < 0.0f){
		// Seed the filter with the raw reading instead of smoothing toward it.
		emaValue = rawMillivolts;
	} else{
		emaValue = emaFactor * rawMillivolts + (1.0f - emaFactor) * emaValue;
	}

	const float voltage = emaValue * Factor + calibOffset;
	const float clamped = std::clamp(voltage, VoltEmpty, VoltFull);
	percent = (clamped - VoltEmpty) / (VoltFull - VoltEmpty) * 100.0f;

	return percent;
}

void Battery::tick(){
	while(!xSemaphoreTake(sem, portMAX_DELAY)){
		timer->stop();
		startTimer();
	}
	timer->stop();

	if(abortFlag || shutdown) return;

	std::lock_guard lock(mut);

	if(sleep){
		CMF_LOG(Battery, Info, "InSleepReconfigure");
		inSleepReconfigure();
		sample(true);
	} else{
		sample();
	}

	startTimer();
}

void Battery::startTimer(){
	timer->stop();
	if(shutdown) return;

	if((getChargingState() != ChargingState::Unplugged) || !sleep){
		timer->setPeriod(ShortMeasureIntverval);
	} else{
		timer->setPeriod(LongMeasureIntverval);
	}
	timer->start();
}

void Battery::chargeChanged(ChargingState newState){
	if(newState != lastCharging){
		lastCharging = newState;
		sample(true);
		OnChargeStatus.broadcast(lastCharging);
	}
}

void Battery::timerFunc(void*){
	xSemaphoreGive(sem);
}

void Battery::inSleepReconfigure(){
	delete readerBatt.get();
	ADCUnit::getADCUnit((gpio_num_t)PIN_BATT)->reinit();

	readerBatt = newObject<ADCReader>(this, (gpio_num_t)PIN_BATT, cfg, true);
}
