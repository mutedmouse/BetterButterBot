#ifndef CMF_MOTORS_H
#define CMF_MOTORS_H

#include "Entity/AsyncEntity.h"
#include "Drivers/Interface/OutputDriver.h"
#include "Drivers/Output/OutputPWM.h"
#include <cmath>

template<typename Motor>
struct MotorDef {
	Motor motor;
	OutputPin digitalPin;
	OutputPin analogPin;
};

DEFINE_LOG(Motors)

template<typename Motor>
class Motors : public Object {
	GENERATED_BODY(Motors, Object, void)
	TEMPLATE_ATTRIBUTES(Motor)

public:
	Motors() = default;

	Motors(const std::vector<MotorDef<Motor>>& motors) : Super(), motorDefs(motors){}

	void reg(MotorDef<Motor> motor){
		std::lock_guard guard(accessMutex);

		motor.digitalPin.driver->write(motor.digitalPin.port, false);
		motor.analogPin.driver->write(motor.analogPin.port, false);

		motorPins[motor.motor] = { motor.digitalPin, motor.analogPin };
		currentValues[motor.motor] = 0;
	}

	/**
	 * Sets the specified motor to val
	 * @param motor
	 * @param val [-100, 100]
	 */
	void set(Motor motor, float val){
		std::lock_guard guard(accessMutex);

		val = std::clamp(val, -100.f, 100.f);
		if(currentValues[static_cast<int>(motor)] == val) {
			return;
		}

		currentValues[static_cast<int>(motor)] = val;

		const MotorPins& pins = motorPins[static_cast<int>(motor)];

		if(val == 0.0f){
			pins.digitalPin.driver->write(pins.digitalPin.port, true);
			pins.analogPin.driver->write(pins.analogPin.port, true);
			return;
		}

		// motor driver is a basic H bridge and has two input lines (A and B) for each motor
		// Ab - forward, aB - backward, AB - brake, ab - no power
		// going forward: pull B low, drive A with PWM
		// going backward: put B high, drive A with reverse PWM value

		// value is [-100, 100], duty is [0, 100]
		const bool reverse = (val < 0);
		float duty = abs(val);

		if(reverse){
			pins.digitalPin.driver->write(pins.digitalPin.port, true);
			duty = 100.f - duty;
		}else{
			pins.digitalPin.driver->write(pins.digitalPin.port, false);
		}

		pins.analogPin.driver->write(pins.analogPin.port, duty / 100.f);
	}

	float get(Motor motor){
		std::lock_guard guard(accessMutex);

		return currentValues[motor];
	}

	void dereg(Motor motor){
		std::lock_guard guard(accessMutex);

		if(!motorPins.count(motor)){
			CMF_LOG(Motors, LogLevel::Warning, "dereg called for motor %d, but motor is not registered", static_cast<int>(motor));
			return;
		}

		motorPins[motor].digitalPin.driver->write(motorPins[motor].digitalPin.port, false);
		motorPins[motor].analogPin.driver->write(motorPins[motor].analogPin.port, false);

		motorPins.erase(motor);
		currentValues.erase(motor);
	}

protected:
	void postInitProperties() noexcept override{
		Super::postInitProperties();

		for(const auto& motor: motorDefs){
			reg(motor);
		}
	}

private:
	const std::vector<MotorDef<Motor>> motorDefs;

	/**
	 * Map of current motor values.
	 * key = Motor[enum, int] , value = state[float]
	 */
	std::map<int, float> currentValues;

	struct MotorPins {
		OutputPin digitalPin;
		OutputPin analogPin;
	};

	std::map<int, MotorPins> motorPins;

	std::mutex accessMutex;
};

#endif //CMF_MOTORS_H