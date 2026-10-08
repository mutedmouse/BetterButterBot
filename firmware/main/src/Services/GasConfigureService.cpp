#include "GasConfigureService.h"
#include <Util/ServiceLocator.h>
#include "Settings.h"

GasConfigureService::~GasConfigureService(){
	// postInitProperties() binds `this` to ModuleService::ModulesEvent and nothing unbinds it.
	// Without this, destroying the service leaves a handle owning a freed object in ModulesEvent.
	if(const Application* app = getApp()){
		if(ModuleService* moduleService = app->getService<ModuleService>()){
			moduleService->ModulesEvent.unbind(this);
		}
	}
}

void GasConfigureService::postInitProperties() noexcept{
	const Application* app = getApp();

	if(ServiceLocator::SettingsInstance){
		if(ServiceLocator::SettingsInstance->get().gasModuleConfigured){
			isConfigured = true;
			done = true;
			return;
		}
	}

	if(ModuleService* moduleService = app->getService<ModuleService>()){
		moduleService->ModulesEvent.bind(this, &GasConfigureService::onModule);
	}
}

void GasConfigureService::tick(float deltaTime) noexcept{
	if(isConfigured && !done){
		if(ServiceLocator::SettingsInstance){
			SettingsStruct settingsStruct = ServiceLocator::SettingsInstance->get();
			settingsStruct.gasModuleConfigured = true;
			ServiceLocator::SettingsInstance->set(settingsStruct);
			ServiceLocator::SettingsInstance->store();
		}

		OnGasConfigureDone.broadcast();

		done = true;
	}
}

void GasConfigureService::onModule(uint8_t bus, Modules::Type type, ModuleService::Action action){
	if(isConfigured){
		return;
	}

	if(type != Modules::Type::RM_CO2){
		return;
	}

	// In this case we do not want the event to go off, this is the case where you remove the gas module, we do not want the user to get the 'configured' message on remove within the config time, but only the 'remove' message
	if(action == ModuleService::Action::Remove){
		isConfigured = true;

		if(ServiceLocator::SettingsInstance){
			SettingsStruct settingsStruct = ServiceLocator::SettingsInstance->get();
			settingsStruct.gasModuleConfigured = true;
			ServiceLocator::SettingsInstance->set(settingsStruct);
			ServiceLocator::SettingsInstance->store();
		}

		return;
	}

	Timer::single(GasConfigureWaitTime, timerFunc, this);
}

void GasConfigureService::timerFunc(void* arg){
	GasConfigureService* service = static_cast<GasConfigureService*>(arg);
	if(service == nullptr){
		return;
	}

	service->isConfigured = true;
}
