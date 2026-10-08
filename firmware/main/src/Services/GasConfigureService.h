#ifndef BUTTERBOT_FIRMWARE_GASCONFIGURESERVICE_H
#define BUTTERBOT_FIRMWARE_GASCONFIGURESERVICE_H

#include <Devices/Timer.h>
#include <Entity/SyncEntity.h>
#include <Event/EventBroadcaster.h>
#include <Services/Modules/ModuleService.h>

class GasConfigureService : public SyncEntity {
	GENERATED_BODY(GasConfigureService, SyncEntity, void)

public:
	DECLARE_EVENT(GasConfigureDoneEvent, GasConfigureService);
	GasConfigureDoneEvent OnGasConfigureDone{ this };

public:
	~GasConfigureService() override;

	virtual void postInitProperties() noexcept override;
	virtual void tick(float deltaTime) noexcept override;

private:
	std::atomic<bool> isConfigured;
	bool done;

	inline static constexpr uint32_t GasConfigureWaitTime = 10 * 60 * 1000; // [10 min]

private:
	void onModule(uint8_t bus, Modules::Type type, ModuleService::Action action);
	static void timerFunc(void* arg);
};

#endif //BUTTERBOT_FIRMWARE_GASCONFIGURESERVICE_H