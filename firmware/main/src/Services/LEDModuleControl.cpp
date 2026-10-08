#include "LEDModuleControl.h"
#include "Services/LED/LEDBlinkFunction.h"
#include "Services/LED/LEDFadeFunction.h"
#include <Util/stdafx.h>
#include <algorithm>
#include <esp_log.h>
#include <Util/ServiceLocator.h>

DEFINE_LOG(LEDService)

LEDModuleControl::LEDModuleControl(StrongObjectPtr<LED<MonoLED, RGBLED>> ledService)
		: ledService(std::move(ledService)) {
	if(ServiceLocator::SettingsInstance){
		const auto s = ServiceLocator::SettingsInstance->get();
		strobePeriod = s.ledStrobePeriod;
		breathePeriod = s.ledBreathePeriod;
	}
}

void LEDModuleControl::initSettings(){
	if(ServiceLocator::SettingsInstance){
		const auto cfg = ServiceLocator::SettingsInstance->get();
		strobePeriod = cfg.ledStrobePeriod;
		breathePeriod = cfg.ledBreathePeriod;
	}
}

void LEDModuleControl::turnLedOn(){
	std::lock_guard<std::mutex> lock(moduleMutex);
	modulePattern = ModulePattern::On;
	if(!ledService){
		CMF_LOG(LEDService, LogLevel::Warning, "turnLedOn: ledService is null!");
		return;
	}
	ledService->on(MonoLED::Module, ModuleLedBrightness);
}

void LEDModuleControl::turnLedOff(){
	std::lock_guard<std::mutex> lock(moduleMutex);
	modulePattern = ModulePattern::Off;
	if(!ledService){
		CMF_LOG(LEDService, LogLevel::Warning, "turnLedOff: ledService is null!");
		return;
	}
	ledService->off(MonoLED::Module);
}

void LEDModuleControl::turnLedStrobe(){
	std::lock_guard<std::mutex> lock(moduleMutex);
	modulePattern = ModulePattern::Strobe;
	applyStrobe();
}

void LEDModuleControl::turnLedBreathe(){
	std::lock_guard<std::mutex> lock(moduleMutex);
	modulePattern = ModulePattern::Breathe;
	applyBreathe();
}

void LEDModuleControl::turnLedFaster(){
	std::lock_guard<std::mutex> lock(moduleMutex);
	if(modulePattern == ModulePattern::Strobe){
		strobePeriod = std::max(StrobeMinPeriod, strobePeriod / SpeedChangeFactor);
		applyStrobe();
	}else if(modulePattern == ModulePattern::Breathe){
		breathePeriod = std::max(BreatheMinPeriod, breathePeriod / SpeedChangeFactor);
		applyBreathe();
	}else{
		return;
	}
	if(ServiceLocator::SettingsInstance){
		auto s = ServiceLocator::SettingsInstance->get();
		s.ledStrobePeriod = strobePeriod;
		s.ledBreathePeriod = breathePeriod;
		ServiceLocator::SettingsInstance->set(s);
		ServiceLocator::SettingsInstance->store();
	}
}

void LEDModuleControl::turnLedSlower(){
	std::lock_guard<std::mutex> lock(moduleMutex);
	if(modulePattern == ModulePattern::Strobe){
		strobePeriod = std::min(StrobeMaxPeriod, strobePeriod * SpeedChangeFactor);
		applyStrobe();
	}else if(modulePattern == ModulePattern::Breathe){
		breathePeriod = std::min(BreatheMaxPeriod, breathePeriod * SpeedChangeFactor);
		applyBreathe();
	}else{
		return;
	}
	if(ServiceLocator::SettingsInstance){
		auto s = ServiceLocator::SettingsInstance->get();
		s.ledStrobePeriod = strobePeriod;
		s.ledBreathePeriod = breathePeriod;
		ServiceLocator::SettingsInstance->set(s);
		ServiceLocator::SettingsInstance->store();
	}
}

bool LEDModuleControl::isLedOn() const{
	return modulePattern != ModulePattern::Off;
}

bool LEDModuleControl::isAtMinSpeed() const{
	if(modulePattern == ModulePattern::Strobe) return strobePeriod <= StrobeMinPeriod;
	if(modulePattern == ModulePattern::Breathe) return breathePeriod <= BreatheMinPeriod;
	return false;
}

bool LEDModuleControl::isAtMaxSpeed() const{
	if(modulePattern == ModulePattern::Strobe) return strobePeriod >= StrobeMaxPeriod;
	if(modulePattern == ModulePattern::Breathe) return breathePeriod >= BreatheMaxPeriod;
	return false;
}

void LEDModuleControl::applyStrobe(){
	if(!ledService){
		CMF_LOG(LEDService, LogLevel::Warning, "applyStrobe: ledService is null!");
		return;
	}
	auto func = std::make_unique<LEDBlinkFunction<MonoLED, float>>(ModuleLedBrightness, strobePeriod, StrobeOnTime, 0u);
	ledService->set(MonoLED::Module, std::move(func));
}

void LEDModuleControl::applyBreathe(){
	if(!ledService){
		CMF_LOG(LEDService, LogLevel::Warning, "applyBreathe: ledService is null!");
		return;
	}
	auto func = std::make_unique<LEDFadeFunction<MonoLED, float>>(0.0f, ModuleLedBrightness, breathePeriod, 0u);
	ledService->set(MonoLED::Module, std::move(func));
}
