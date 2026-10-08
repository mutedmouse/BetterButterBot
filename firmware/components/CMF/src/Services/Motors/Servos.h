#ifndef CMF_SERVOS_H
#define CMF_SERVOS_H

#include "Drivers/Interface/OutputDriver.h"
#include "Entity/AsyncEntity.h"
#include "Drivers/Output/OutputMCPWM.h"
#include "Drivers/Output/OutputPWM.h"

DEFINE_LOG(Servos)

template<typename Servo>
struct ServoDef {
	Servo motor;

	/**
	 * Minimum and maximum limit of servo range. (Default 0 - 1.0)
	 * All values sent to this servo will be mapped to this range before pushing it to hardware.
	 */
	std::pair<float, float> allowedRange = { 0, 1 };
	OutputPin pin;
};

template<typename Servo>
class Servos : public Object {
	GENERATED_BODY(Servos, Object, void)
	TEMPLATE_ATTRIBUTES(Servo)

public:
	Servos() = default;

	Servos(const std::vector<ServoDef<Servo>>& servos) noexcept : Super(), servoDefs(servos){}

	void reg(Servo servo, std::pair<float, float> allowedRange, OutputPin pin){
		pins[servo] = pin;
		allowedRanges[servo] = allowedRange;
		currentValues[servo] = 0.5f;

		pin.driver->write(pin.port, mapToServo(0.5f, servo));
	}

	/**
	 * Set servo value.
	 * @param servo
	 * @param val [0 - 1.0]
	 */
	void set(Servo servo, float val){
		if(!currentValues.count(servo)){
			CMF_LOG(Servos, LogLevel::Warning, "!currentValues.count(servo)");
			return;
		}

		val = std::clamp(val, 0.f, 1.f);

		if(currentValues[servo] == val){
			return;
		}

		currentValues[servo] = val;

		const OutputPin& pin = pins[servo];

		pin.driver->write(pin.port, mapToServo(val, servo));
	}

	float get(Servo servo){
		if(!currentValues.count(servo)){
			CMF_LOG(Servos, LogLevel::Warning, "get called for unregistered servo");
			return 0;
		}

		return currentValues[servo];
	}

	void enable(Servo servo){
		if(!pins.count(servo)){
			CMF_LOG(Servos, LogLevel::Warning, "Servo not registered");
			return;
		}

		const auto& pin = pins[servo];

		if(pin.driver->isA(OutputPWM::staticClass())){
			cast<OutputPWM>(pin.driver)->attach(pin.port);
		}else if(pin.driver->isA(OutputMCPWM::staticClass())){
			cast<OutputMCPWM>(pin.driver)->enable(pin.port);
		}else{
			CMF_LOG(Servos, LogLevel::Warning, "Underlying output driver doesn't support enable/disable");
		}
	}

	void disable(Servo servo){
		if(!pins.count(servo)){
			CMF_LOG(Servos, LogLevel::Warning, "Servo not registered");
			return;
		}

		const auto& pin = pins[servo];

		if(pin.driver->isA(OutputPWM::staticClass())){
			cast<OutputPWM>(pin.driver)->detach(pin.port);
		}else if(pin.driver->isA(OutputMCPWM::staticClass())){
			cast<OutputMCPWM>(pin.driver)->disable(pin.port);
		}else{
			CMF_LOG(Servos, LogLevel::Warning, "Underlying output driver doesn't support enable/disable");
		}
	}

protected:
	void postInitProperties() noexcept override{
		Super::postInitProperties();

		for(const auto& motor: servoDefs){
			reg(motor.motor, motor.allowedRange, motor.pin);
		}
	}

private:
	const std::vector<ServoDef<Servo>> servoDefs;

	/**
	 * Map of current motor values.
	 * key = Motor[enum, int] , value = state[float]
	 */
	std::map<Servo, float> currentValues;

	std::map<Servo, OutputPin> pins;

	std::map<Servo, std::pair<float, float>> allowedRanges;

	/**
	 * Map value from 0 - 1 to servo's configured allowed range.
	 */
	float mapToServo(float val, Servo servo) const{
		if(!allowedRanges.contains(servo)) return 0;

		const std::pair<float, float>& range = allowedRanges.at(servo);

		return val * (range.second - range.first) + range.first;
	}
};

#endif //CMF_SERVOS_H