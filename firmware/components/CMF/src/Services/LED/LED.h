#ifndef CMF_LED_H
#define CMF_LED_H

#include <Event/EventBroadcaster.h>
#include "Entity/AsyncEntity.h"
#include "Misc/Enum.h"
#include "glm.hpp"
#include "Drivers/Interface/OutputDriver.h"
#include "Log/Log.h"
#include "Memory/ObjectMemory.h"
#include "Object/SubclassOf.h"
#include "Util/stdafx.h"
#include <memory>

DEFINE_LOG(LED)

template<typename LED, typename DataT>
class LEDFunction {
public:
	virtual ~LEDFunction() = default;

	/**
	 *
	 * @return True when the LEDFunction is finished, false otherwise.
	 */
	virtual bool isDone() const noexcept{ return true; }

	/**
	 *
	 * @return The value to set the LED to at the time of the function call.
	 */
	virtual DataT getValue() const noexcept{ return {}; }

	/**
	 *
	 * @return The time until next needed change to the LED state. This directly affects the ticking period of LEDFunction as well as LED service.
	 * The smallest interval of all functions is the one taken for the LED service ticking interval.
	 */
	virtual TickType_t getInterval() const noexcept{ return portMAX_DELAY; }

	/**
	 * @brief Advances the function state. Driven by the owning LEDBase from within its tick(), under the LEDBase access mutex.
	 * @param deltaTime Time since the last tick call.
	 */
	virtual void tick(float deltaTime) noexcept{}

	/**
	 * @brief Runs begin() exactly once before the first tick.
	 */
	void ensureBegun() noexcept{
		if(begun){
			return;
		}

		begin();
		begun = true;
	}

protected:
	virtual void begin() noexcept{}

	bool hasBegun() const noexcept{ return begun; }

private:
	bool begun = false;
};

template<typename LED, typename DataT> requires (std::same_as<DataT, glm::vec3> || std::same_as<DataT, float>)
class LEDBase : public SyncEntity {
	TEMPLATE_ATTRIBUTES(LED, DataT)
	GENERATED_BODY(LEDBase, SyncEntity, void)

public:
	virtual ~LEDBase() override{
		for(auto pair : waitSemaphores){
			xSemaphoreGive(pair.second);
			vSemaphoreDelete(pair.second);
		}
	}

	/**
	 *
	 * @return The time to wait until the next tick for the LED service and all children functions. Returns the minimum interval of all current functions.
	 */
	virtual TickType_t getInterval(){
		static constexpr TickType_t defaultInterval = portMAX_DELAY;

		std::lock_guard guard(accessMutex);

		if(currentFunctions.empty() && prevFunctions.empty()){
			return defaultInterval;
		}

		TickType_t interval = defaultInterval;

		for(auto it = currentFunctions.begin(); it != currentFunctions.end(); ++it){
			const RegisteredFunction& func = it->second;

			// Should not check if the function is done and disregard it. If it does, and it is the only function,
			// it will block the service tick for portMAX_DELAY and the wairFor function will remain blocked forever in certain cases
			if(func.function == nullptr){
				continue;
			}

			const TickType_t funcInterval = func.function->getInterval();
			const TickType_t sinceLastActivation = static_cast<TickType_t>(millis() - func.lastActivation);

			if(funcInterval <= sinceLastActivation){
				return 0;
			}

			const TickType_t funcIntervalRemainder = funcInterval - sinceLastActivation;
			interval = std::min(interval, funcIntervalRemainder);
		}

		return interval / portTICK_PERIOD_MS;
	}

	/**
	 * @brief Registers an LED and its output pins.
	 * @param led LED being registered
	 * @param pins The output pin(s) for that LED
	 */
	void reg(LED led, std::array<OutputPin, sizeof(DataT) / sizeof(float)> pins) noexcept{
		accessMutex.lock();
		outputs[led] = pins;
		accessMutex.unlock();
		waitSemaphores[led] = xSemaphoreCreateBinary();
		off(led);
	}

	/**
	 * @brief Turns the LED off (sets its value to DataT()).
	 * @param led The LED to turn off.
	 */
	void off(LED led) noexcept{
		std::lock_guard guard(accessMutex);
		if(!outputs.contains(led)){
			CMF_LOG(CMF, Error, "LED %d not registered", (int) led);
			return;
		}

		internalOff(led);

		prevStates[led] = DataT(); //zero-value

		if(currentFunctions.contains(led)){
			destroyFunction(currentFunctions[led].function);
			currentFunctions.erase(led);
		}
		if(prevFunctions.contains(led)){
			destroyFunction(prevFunctions[led].function);
			prevFunctions.erase(led);
		}

		xSemaphoreGive(waitSemaphores[led]);
	}

	/**
	 * @brief Turn the LED on.
	 * @param led LED to turn on.
	 * @param level The level to set to the LED output.
	 */
	void on(LED led, DataT level) noexcept{
		std::lock_guard guard(accessMutex);
		if(!outputs.contains(led)){
			CMF_LOG(CMF, Error, "LED %d not registered", (int) led);
			return;
		}

		internalOn(led, level);

		prevStates[led] = level;

		if(currentFunctions.contains(led)){
			destroyFunction(currentFunctions[led].function);
			currentFunctions.erase(led);
		}
		if(prevFunctions.contains(led)){
			destroyFunction(prevFunctions[led].function);
			prevFunctions.erase(led);
		}

		xSemaphoreGive(waitSemaphores[led]);
	}

	/**
	 * @brief Set a function to the LED. If temporary, when the function ends, the old one will be reactivated.
	 * @param led LED to set a function to.
	 * @param function LED function for the LED
	 * @param temp If the function is temporary or not
	 */
	void set(LED led, std::unique_ptr<LEDFunction<LED, DataT>> function, bool temp = false) noexcept{
		std::lock_guard guard(accessMutex);
		if(!outputs.contains(led)){
			CMF_LOG(CMF, Error, "LED %d not registered", (int) led);
			return;
		}

		RegisteredFunction regFunc = { std::move(function), temp };

		bool alreadyLocked = false;
		if(currentFunctions.contains(led)){
			alreadyLocked = true;
			if(!currentFunctions[led].isTemporary){
				if(prevFunctions.contains(led)){
					destroyFunction(prevFunctions[led].function);
				}
				prevFunctions[led] = std::move(currentFunctions[led]);
			}else{
				destroyFunction(currentFunctions[led].function);
			}
		}

		currentFunctions[led] = std::move(regFunc);

		/**
		 * When setting a function while an existing continuous function is already playing,
		 * the waitSemaphore is already locked and will never be given, since tick is also blocked by the accessMutex.
		 * Furthermore, if the previous function is continuous the waitSemaphore will never be given.
		 *
		 * Since accessMutex is locked here, there isn't any rug-pulling on the tick() thread.
		 */
		if(alreadyLocked){
			xSemaphoreTake(waitSemaphores[led], 0);
		} else{
			xSemaphoreTake(waitSemaphores[led], portMAX_DELAY);
		}
	}

	/**
	 *
	 * @param led The LED to of the value.
	 * @return The value set to the output of the LED.
	 */
	DataT getValue(LED led) noexcept requires (std::same_as<DataT, float>){
		std::lock_guard guard(accessMutex);
		if(!outputs.contains(led)){
			CMF_LOG(CMF, Error, "LED %d not registered", (int) led);
			return DataT();
		}

		const auto& pin = outputs[led][0];
		return pin.driver->getState(pin.port);
	}

	/**
	 *
	 * @param led The LED to of the value.
	 * @return The value set to the output of the LED.
	 */
	DataT getValue(LED led) noexcept requires (std::same_as<DataT, glm::vec3>){
		std::lock_guard guard(accessMutex);
		if(!outputs.contains(led)){
			CMF_LOG(CMF, Error, "LED %d not registered", (int) led);
			return DataT();
		}

		DataT level;
		for(int i = 0; i < outputs[led].size(); i++){
			level[i] = outputs[led][i].driver->getState(outputs[led][i].port);
		}
		return level;
	}

	void tick(float deltaTime) noexcept override{
		Super::tick(deltaTime);

		std::lock_guard guard(accessMutex);

		if(currentFunctions.empty() && prevFunctions.empty()) return;

		for(auto& pair : currentFunctions){
			if(pair.second.function == nullptr) continue;
			pair.second.function->ensureBegun();
			pair.second.function->tick(deltaTime);
		}
		for(auto& pair : prevFunctions){
			if(pair.second.function == nullptr) continue;
			pair.second.function->ensureBegun();
			pair.second.function->tick(deltaTime);
		}

		if(currentFunctions.empty()) return;

		for(auto it = currentFunctions.begin(); it != currentFunctions.end();){
			RegisteredFunction& func = it->second;
			const LED& led = it->first;

			if(!func.function->isDone()){
				const DataT level = func.function->getValue();
				func.lastActivation = millis();
				internalOn(led, level);

				++it;
				continue;
			}

			xSemaphoreGive(waitSemaphores[led]);

			destroyFunction(func.function);

			if(prevFunctions.contains(led)){
				currentFunctions[led].function = std::move(prevFunctions[led].function);
				currentFunctions[led].isTemporary = prevFunctions[led].isTemporary;
				prevFunctions.erase(led);
				++it;
			}else if(prevStates.contains(led)){
				internalOn(led, prevStates[led]);
				it = currentFunctions.erase(it);
			}else{
				internalOff(led);
				it = currentFunctions.erase(it);
			}

			if(currentFunctions.contains(led)){
				xSemaphoreTake(waitSemaphores[led], portMAX_DELAY);
			}
		}
	}

	/**
	 * @brief Block the active thread for {wait} milliseconds maximum until the function on the given LED finishes.
	 * @param led LED to wait on.
	 * @param wait Wait time.
	 * @return True if successful.
	 */
	bool waitFor(LED led, TickType_t wait){
		{
			std::lock_guard guard(accessMutex);

			if(!currentFunctions.contains(led)){
				return true;
			}
		}

		if(xSemaphoreTake(waitSemaphores[led], wait) != pdTRUE){
			return false;
		}

		xSemaphoreGive(waitSemaphores[led]);
		return true;
	}

private:
	void internalOn(LED led, DataT level) noexcept requires (std::same_as<DataT, float>){
		const auto& pin = outputs[led][0];

		if(!pin.driver){
			CMF_LOG(CMF, Error, "LED %d driver not set", (int) led);
			return;
		}

		pin.driver->write(pin.port, level);
	}

	void internalOn(LED led, DataT level) noexcept requires (std::same_as<DataT, glm::vec3>){
		for(int i = 0; i < outputs[led].size(); i++){

			if(!outputs[led][i].driver) continue;

			outputs[led][i].driver->write(outputs[led][i].port, level[i]);
		}
	}

	void internalOff(LED led) noexcept{
		for(const OutputPin& pin: outputs[led]){

			if(!pin.driver) continue;

			pin.driver->write(pin.port, false);
		}
	}

	void destroyFunction(std::unique_ptr<LEDFunction<LED, DataT>>& function) noexcept{
		function.reset();
	}

	struct RegisteredFunction {
		std::unique_ptr<LEDFunction<LED, DataT>> function;
		bool isTemporary = false;
		uint64_t lastActivation = 0;
	};

	std::map<LED, std::array<OutputPin, sizeof(DataT) / sizeof(float)>> outputs;
	std::map<LED, RegisteredFunction> currentFunctions;
	std::map<LED, RegisteredFunction> prevFunctions;
	std::map<LED, DataT> prevStates;
	std::map<LED, SemaphoreHandle_t> waitSemaphores;

	std::mutex accessMutex;
};

template<typename Monos, typename RGBs>
class LED : public AsyncEntity {
	TEMPLATE_ATTRIBUTES(Monos, RGBs)
	GENERATED_BODY(LED, AsyncEntity, void)

private:
	// This event is only used internally by the LED service to unlock the wait for events semaphore and begin ticking when needed
	DECLARE_EVENT(LEDFunctionStartEvent, LED)
	LEDFunctionStartEvent OnFunctionStart{this};

public:
	explicit LED(bool internalStack = true) noexcept : Super(CONFIG_CMF_LED_TICK_INTERVAL / portTICK_PERIOD_MS, CONFIG_CMF_LED_STACK_SIZE, CONFIG_CMF_LED_THREAD_PRIORITY, CONFIG_CMF_LED_CPU_CORE, internalStack){
		monos = newObject<LEDBase<Monos, float>>(this);
		rgbs = newObject<LEDBase<RGBs, glm::vec3>>(this);

		OnFunctionStart.bind(this, &LED::onFunctionStart);
	}

	virtual TickType_t getEventScanningTime() const noexcept override{
		const TickType_t monosInterval = monos.isValid() ? monos->getInterval() : Super::getEventScanningTime();
		const TickType_t rgbsInterval = rgbs.isValid() ? rgbs->getInterval() : Super::getEventScanningTime();
		const TickType_t minInterval = std::min(monosInterval, rgbsInterval);

		if(minInterval < MinimumTickInterval){
			return MinimumTickInterval;
		}

		return minInterval;
	}

	void reg(Monos led, OutputPin pin) noexcept{
		monos->reg(led, { pin });
	}

	void reg(const std::vector<std::pair<Monos, OutputPin>>& monoRegs){
		for(const auto& entry: monoRegs){
			reg(entry.first, entry.second);
		}
	}

	void reg(RGBs led, OutputPin pin_r, OutputPin pin_g, OutputPin pin_b) noexcept{
		rgbs->reg(led, { pin_r, pin_g, pin_b });
	}

	void reg(const std::vector<std::tuple<RGBs, OutputPin, OutputPin, OutputPin>>& rgbRegs){
		for(const auto& entry: rgbRegs){
			reg(std::get<0>(entry), std::get<1>(entry), std::get<2>(entry), std::get<3>(entry));
		}
	}

	void off(Monos led) noexcept{
		monos->off(led);
	}

	void off(RGBs led) noexcept{
		rgbs->off(led);
	}

	void on(Monos led, float level) noexcept{
		monos->on(led, level);
	}

	void on(RGBs led, glm::vec3 level) noexcept{
		rgbs->on(led, level);
	}

	void set(Monos led, std::unique_ptr<LEDFunction<Monos, float>> function, bool temp = false) noexcept{
		monos->set(led, std::move(function), temp);
		OnFunctionStart.broadcast();
	}

	void set(RGBs led, std::unique_ptr<LEDFunction<RGBs, glm::vec3>> function, bool temp = false) noexcept{
		rgbs->set(led, std::move(function), temp);
		OnFunctionStart.broadcast();
	}

	float getValue(Monos led) noexcept{
		return monos->getValue(led);
	}

	glm::vec3 getValue(RGBs led) noexcept{
		return rgbs->getValue(led);
	}

	bool waitFor(Monos led, TickType_t wait = portMAX_DELAY){
		if(!monos.isValid()){
			return true;
		}

		return monos->waitFor(led, wait);
	}

	bool waitFor(RGBs led, TickType_t wait = portMAX_DELAY){
		if(!rgbs->isValid()){
			return true;
		}

		return rgbs->waitFor(led, wait);
	}

private:
	StrongObjectPtr<LEDBase<Monos, float>> monos;
	StrongObjectPtr<LEDBase<RGBs, glm::vec3>> rgbs;

	inline static constexpr TickType_t MinimumTickInterval = CONFIG_CMF_LED_MINIMAL_TICK_INTERVAL;

private:
	void onFunctionStart() noexcept{}
};

#endif //CMF_LED_H
