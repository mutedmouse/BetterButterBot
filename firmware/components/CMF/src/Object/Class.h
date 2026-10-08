#ifndef CMF_CLASS_H
#define CMF_CLASS_H

#include <cinttypes>
#include <map>
#include <string>
#include <tuple>
#include "Memory/SmartPtr/StrongObjectPtr.h"

class Object;
class Class;

/**
 * @brief Class registry is used internally by CMF to track all existing object classes by their ID.
 */
class ClassRegistry {
public:
	/**
	 * @brief Deleted default constructor.
	 */
	virtual ~ClassRegistry() noexcept = default;

	/**
	 * @param ID ID of the wanted class.
	 * @return The class matching the given ID. If no class matches it, returns nullptr.
	 */
	const Class* getClass(uint64_t ID) const noexcept;

	/**
	 * @brief Registers a class, called by each class when constructed.
	 * Each class is constructed only once statically for each Object type.
	 * @param cls 
	 */
	void registerClass(const Class* cls) noexcept;

private:
	std::map<uint64_t, const Class*> classes;
};

/**
 * @brief Class distinguishes the Objects.
 * Each object has a unique class which is implemented uniquely depending on the object.
 * Classes are mainly used for Object casting and construction.
 */
class Class {
	friend class Object;

public:
	/**
	 * @brief Default destructor.
	 */
	virtual ~Class() = default;

	/**
	 * @return An object represented by this class.
	 */
	template<typename ...Args>
	StrongObjectPtr<Object> createObject(Args&&... args) const noexcept {
		std::tuple<Args...> arguments = std::make_tuple(std::forward<Args>(args)...);
		return __createObject(&arguments);
	}

	/**
	 * @return The ID of the class. Each class has a unique ID
	 * depending on the name of the object it represents and the templates types of that object.
	 */
	uint64_t getID() const noexcept;

	/**
	 * @tparam Type The type of interface that the object represented by this class implements.
	 * @return True if the represented object implements the template interface.
	 */
	template<typename Type>
	static inline constexpr bool implements() noexcept {
		return false;
	}

	/**
	 * @brief Checks if this class is of type given in the parameter, or derived from it.
	 * @param other The type class being compared to.
	 * @return True if same type or derived from it, false otherwise.
	 */
	virtual bool isA(const Class* other) const noexcept;

	/**
	 * @brief Checks if this class is of type given in the template, or derived from it.
	 * @tparam __T The type of Object being compared to. Has to have a static implementation of 'staticClass()' function.
	 * @return True if same type or derived from it, false otherwise.
	 */
	template<typename __T>
	inline bool isA() const noexcept{
		return isA(__T::staticClass());
	}

	/**
	 * @param ID ID of the given class.
	 * @return The class matching the ID from the ClassRegistry.
	 */
	static inline const Class* getClasByID(uint64_t ID) noexcept {
		if(registry == nullptr){
			return nullptr;
		}

		return registry->getClass(ID);
	}

	/**
	 * @return The name of the object type the class represents.
	 */
	inline virtual constexpr std::string getName() const noexcept{
		return "Object";
	}

protected:
	static inline ClassRegistry* registry = nullptr;

protected:
	/**
	 * @brief Constructor of the class with given ID. The ID is generated partially at compile time,
	 * and partially at the beginning of the runtime statically.
	 * @param ID The generated ID of the class.
	 */
	explicit Class(uint64_t ID) noexcept;

	/**
	 * @return An object represented by this class.
	 */
	virtual StrongObjectPtr<Object> __createObject(void* arguments) const noexcept;

private:
	uint64_t classID;
};

#endif //CMF_CLASS_H