#ifndef CMF_MODULEDEFS_H
#define CMF_MODULEDEFS_H

#include "ModuleType.h"
#include "ModuleDevice.h"
#include <map>

namespace Modules {
/**
 * Model of Module sub-address.
 * Necessary since sub-addressing follows various types and methods of addressing.
 */
struct SubAddress {
	enum class Type : uint8_t {
		None, RM, RM_I2C, Token, Rover_I2C
	} type;
	union {
		uint8_t RMAddress; //Rick&Morty Butterbot non-I2C modules
		uint8_t I2CAddress; //Rick&Morty Butterbot, Rover I2C modules
		uint8_t tokenAddress; //STEM adventure tokens (Bit)
	};

	bool operator<(const SubAddress& rhs) const{
		if(type < rhs.type)
			return true;
		if(rhs.type < type)
			return false;
		if(RMAddress < rhs.RMAddress)
			return true;
		if(rhs.RMAddress < RMAddress)
			return false;
		if(I2CAddress < rhs.I2CAddress)
			return true;
		if(rhs.I2CAddress < I2CAddress)
			return false;
		return tokenAddress < rhs.tokenAddress;
	}

	bool operator>(const SubAddress& rhs) const{
		return rhs < *this;
	}

	bool operator<=(const SubAddress& rhs) const{
		return !(rhs < *this);
	}

	bool operator>=(const SubAddress& rhs) const{
		return !(*this < rhs);
	}
};

/**
 * Unique representation of a module, through a main and sub address.
 */
struct Address {
	uint8_t mainAddress;
	SubAddress subAddress;

	bool operator<(const Address& rhs) const{
		if(mainAddress < rhs.mainAddress)
			return true;
		if(rhs.mainAddress < mainAddress)
			return false;
		return subAddress < rhs.subAddress;
	}

	bool operator>(const Address& rhs) const{
		return rhs < *this;
	}

	bool operator<=(const Address& rhs) const{
		return !(rhs < *this);
	}

	bool operator>=(const Address& rhs) const{
		return !(*this < rhs);
	}
};

/**
 * Enumeration of pin modes for specific UMAX modules bus pin.
 *
 * If pin is unused, leave it at default (None)
 */
enum class PinMode {
	None, Input, Output
};

/**
 * Mapping address to a known Module type
 *
 * Useful for recognizing a module from an acquired address.
 */
const std::map<Address, Type>& GetAddressMap();

/**
 * Factory method for creating a new instance of a specific Module
 */
StrongObjectPtr<ModuleDevice> CreateModuleDevice(Object* owner, Type moduleType, const Modules::BusPins& busPins);
const std::map<uint8_t, std::set<SubAddress>>& GetSubAddressMap();
const std::map<Type, std::array<PinMode, 6>>& GetPinModeMap();

}

#endif //CMF_MODULEDEFS_H
