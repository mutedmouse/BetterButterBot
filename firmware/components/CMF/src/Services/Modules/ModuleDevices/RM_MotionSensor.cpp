#include "RM_MotionSensor.h"
#include "Memory/ObjectMemory.h"
#include "Services/Modules/ModuleService.h"

DEFINE_LOG(RM_MotionSensor);

RM_MotionSensor::RM_MotionSensor(const Modules::BusPins& busPins) : Super(Modules::Type::RM_Motion, busPins){
	pin = static_cast<gpio_num_t>(busPins.subAddressPins[0].inputPort);
	active = gpio_get_level(pin);

	sem = xSemaphoreCreateBinary();
	thread = std::make_unique<Threaded>([this]() {
		if(!debouncing){
			if(xSemaphoreTake(sem, pdMS_TO_TICKS(IdleWaitMs)) != pdTRUE){
				return; // periodic wake so a pending stop() can join
			}
			debouncing = true;
			return;
		}

		if(xSemaphoreTake(sem, pdMS_TO_TICKS(DebounceMs)) == pdTRUE){
			CMF_LOG(RM_MotionSensor, Debug, "ignored bounce");
			return;
		}

		debouncing = false;
		bool newActive = gpio_get_level(pin);
		if(newActive == active.load()) return;

		active = newActive;

		ModuleService* parent = cast<ModuleService>(getOwner());
		if(parent == nullptr){
			CMF_LOG(RM_MotionSensor, Error, "Parent isn't ModuleService!");
			return;
		}

		parent->OnRM_Motion.broadcast(active.load());
	}, "RM_MotionSensor", 0, 3 * 1024, CONFIG_CMF_THREADED_PRIORITY, CONFIG_CMF_THREADED_CPU_CORE, false);


	const gpio_config_t io_conf = {
		.pin_bit_mask = 1ULL << pin,
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_ENABLE,
		.intr_type = GPIO_INTR_ANYEDGE
	};

	gpio_config(&io_conf);
	gpio_isr_handler_add(pin, isr, this);
	gpio_intr_enable(pin);

	setLEDs(false);

	thread->start();
}

RM_MotionSensor::~RM_MotionSensor() noexcept{
	gpio_intr_disable(pin);
	gpio_isr_handler_remove(pin);

	thread->stop();
	vSemaphoreDelete(sem);
}

void RM_MotionSensor::setLEDs(bool state){
	if(pins.subAddressPins[1].outputDriver){
		pins.subAddressPins[1].outputDriver->write(pins.subAddressPins[1].outputPort, state);
	}
}

bool RM_MotionSensor::getState() const{
	return active;
}

void IRAM_ATTR RM_MotionSensor::isr(void* arg){
	if(arg == nullptr){
		return;
	}

	RM_MotionSensor* motion = static_cast<RM_MotionSensor*>(arg);
	BaseType_t higherPrioTask = pdFALSE;
	xSemaphoreGiveFromISR(motion->sem, &higherPrioTask);
	portYIELD_FROM_ISR(higherPrioTask);
}
