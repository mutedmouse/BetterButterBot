#ifndef CMF_OUTPUTDRIVER_H
#define CMF_OUTPUTDRIVER_H

#include "Object/Class.h"
#include "Misc/Enum.h"
#include "Entity/SyncEntity.h"
#include "Log/Log.h"

struct OutputPinDef {
	int port;
	bool inverted;

	bool operator==(const OutputPinDef& rhs) const noexcept{
		return port == rhs.port && inverted == rhs.inverted;
	}

	bool operator!=(const OutputPinDef& rhs) const noexcept{
		return !(rhs == *this);
	}
};

struct OutputPin {
	class OutputDriver* driver = nullptr;

	int port = -1;
};

class OutputDriver : public Object {
	GENERATED_BODY(OutputDriver, Object, CONSTRUCTOR_PACK(const std::vector<OutputPinDef>&))

public:
	void write(int port, bool value) noexcept{
		write(port, value ? 1.0f : 0.0f);
	}

	virtual void send() noexcept{}

	float getState(int port) const noexcept{
		if(!states.contains(port)){
			return 0;
		}

		return states.at(port);
	}

	void write(int port, float value) noexcept{
		states[port] = value;
		performWrite(port, getInversions()[port] ? 1.0 - value : value);
	}

	void registerOutput(const OutputPinDef& pinDef) noexcept{
		if(!inversions.contains(pinDef.port)){
			outputs.emplace_back(pinDef);
			inversions[pinDef.port] = pinDef.inverted;
		}
		performRegister(pinDef);
	}

	void removeOutput(int port) noexcept{
		auto it = std::remove_if(outputs.begin(), outputs.end(), [port](const OutputPinDef& pinDef){
			return pinDef.port == port;
		});

		for(auto i = it; i != outputs.end(); ++i){
			performDeregister(*i);
			states.erase(i->port);
			inversions.erase(i->port);
		}

		outputs.erase(it, outputs.end());
	}

protected:
	OutputDriver() noexcept = default;

	OutputDriver(const std::vector<OutputPinDef>& outputs) noexcept: outputs(outputs){}

	std::vector<OutputPinDef>& getOutputs() noexcept{
		return outputs;
	}

	std::map<int, float>& getStates() noexcept{
		return states;
	}

	std::map<int, bool>& getInversions() noexcept{
		return inversions;
	}

	virtual void performWrite(int port, float value) noexcept{}


	/**
	 * Function which extracts a copy of InputPinDef from derived structs.
	 * Useful for InputDriver specializations which also have a derived InputPinDef.
	 * @tparam Derived Type derived from InputPinDef
	 * @param derivedVec vector of derived types
	 * @return
	 */
	template<typename Derived> requires std::derived_from<Derived, OutputPinDef>
	static std::vector<OutputPinDef>
	toOutputPinDef(const std::vector<Derived>& derivedVec) {
		std::vector<OutputPinDef> tmp;
		tmp.reserve(derivedVec.size());

		for (const OutputPinDef& item : derivedVec) {
			tmp.emplace_back(static_cast<const OutputPinDef&>(item));
		}

		return tmp;
	}

private:
	void postInitProperties() noexcept override final{
		Super::postInitProperties();

		for(const auto& output : outputs){
			inversions[output.port] = output.inverted;
			performRegister(output);
		}
	}

	virtual void performRegister(const OutputPinDef& output) noexcept{}

	virtual void performDeregister(const OutputPinDef& output) noexcept{}

	std::vector<OutputPinDef> outputs;

	/**
	 * Map of cached output values.
	 * key = port[int] , value = state[float]
	 */
	std::map<int, float> states;

	/**
	 * Map of inversion settings for each port.
	 * key = port[int], value = inversion[bool]
	 */
	std::map<int, bool> inversions;

};

#endif //CMF_OUTPUTDRIVER_H
