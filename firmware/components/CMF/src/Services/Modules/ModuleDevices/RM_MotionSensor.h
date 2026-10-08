#ifndef CMF_RM_MOTIONSENSOR_H
#define CMF_RM_MOTIONSENSOR_H

#include "Object/Class.h"
#include "Services/Modules/ModuleDevice.h"
#include "Thread/Threaded.h"

class RM_MotionSensor : public ModuleDevice {
	GENERATED_BODY(RM_MotionSensor, ModuleDevice, CONSTRUCTOR_PACK(const Modules::BusPins&))

public:
	RM_MotionSensor(const Modules::BusPins& busPins = {});
	~RM_MotionSensor() noexcept override;
	void setLEDs(bool state);

	bool getState() const;

private:
	IRAM_ATTR static void isr(void* arg);

	static constexpr uint32_t IdleWaitMs = 100;
	static constexpr uint32_t DebounceMs = 200;

	std::atomic_bool active = false;
	gpio_num_t pin;
	bool debouncing = false;

	std::unique_ptr<Threaded> thread;
	SemaphoreHandle_t sem;
};


#endif //CMF_RM_MOTIONSENSOR_H
