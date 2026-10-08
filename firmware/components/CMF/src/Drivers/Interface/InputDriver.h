#ifndef CMF_INPUTDRIVER_H
#define CMF_INPUTDRIVER_H

#include "Object/Class.h"
#include "Misc/Enum.h"
#include "Entity/SyncEntity.h"
#include "Log/Log.h"

struct InputPinDef {
	int port;
	bool inverted;

	bool operator==(const InputPinDef& rhs) const noexcept{
		return port == rhs.port && inverted == rhs.inverted;
	}

	bool operator!=(const InputPinDef& rhs) const noexcept{
		return !(rhs == *this);
	}
};

struct InputPin {
	class InputDriver* driver;

	int port;
};

class InputDriver : public Object {
	GENERATED_BODY(InputDriver, Object, const std::vector<InputPinDef>&)

public:
	virtual void scan() noexcept{}

	float read(int port) const noexcept{
		if(!states.contains(port)){
			CMF_LOG(CMF, Error, "Input port %d not registered", port);
			return 0;
		}

		return states.at(port) ^ inversions.at(port);
	}

	void registerInput(const InputPinDef& pinDef) noexcept{
		if(!inversions.contains(pinDef.port)){
			inputs.emplace_back(std::cref(pinDef));
			inversions[pinDef.port] = pinDef.inverted;
		}
		performRegister(pinDef);
	}

	void removeInput(int port) noexcept{
		auto it = std::remove_if(inputs.begin(), inputs.end(), [port](const InputPinDef& pinDef){
			return pinDef.port == port;
		});

		for(auto i = it; i != inputs.end(); ++i){
			performDeregister(*i);
			states.erase(i->port);
			inversions.erase(i->port);
		}

		inputs.erase(it, inputs.end());
	}


protected:
	InputDriver() noexcept = default;

	InputDriver(const std::vector<InputPinDef>& inputs) noexcept: inputs(inputs){}

	void forEachInput(const std::function<void(const InputPinDef&)>& func) const noexcept{
		for(const auto& input : inputs){
			func(input);
		}
	}

	std::vector<InputPinDef>& getInputs() noexcept{
		return inputs;
	}

	std::unordered_map<int, bool>& getStates() noexcept{
		return states;
	}

	std::unordered_map<int, bool>& getInversions() noexcept{
		return inversions;
	}

	/**
	 * Function which extracts a copy of InputPinDef from derived structs.
	 * Useful for InputDriver specializations which also have a derived InputPinDef.
	 * @tparam Derived Type derived from InputPinDef
	 * @param derivedVec vector of derived types
	 * @return
	 */
	template<typename Derived>
	requires std::derived_from<Derived, InputPinDef>
	static std::vector<InputPinDef> toInputPinDef(const std::vector<Derived>& derivedVec){
		std::vector<InputPinDef> tmp;
		tmp.reserve(derivedVec.size());

		for(const auto& item : derivedVec){
			tmp.emplace_back(static_cast<const InputPinDef&>(item));
		}

		return tmp;
	}

private:
	virtual void postInitProperties() noexcept override final{
		Super::postInitProperties();

		for(const auto& input : inputs){
			inversions[input.port] = input.inverted;
			performRegister(input);
		}
	}

	virtual void performRegister(const InputPinDef& input) noexcept{}

	virtual void performDeregister(InputPinDef input) noexcept{}

	std::vector<InputPinDef> inputs;

	/**
	 * Map of cached input values.
	 * key = port[int] , value = state[bool]
	 */
	std::unordered_map<int, bool> states;

	/**
	 * Map of inversion settings for each port.
	 * key = port[int], value = inversion[bool]
	 */
	std::unordered_map<int, bool> inversions;
};

#endif //CMF_INPUTDRIVER_H
