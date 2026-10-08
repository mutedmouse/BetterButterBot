#ifndef CMF_INTERFACE_H
#define CMF_INTERFACE_H

#include <concepts>
#include "Object.h"
#include "Memory/Cast.h"
#include "Memory/SmartPtr/WeakObjectPtr.h"

/**
 * @brief Interface is used to pass pointer to an object that extends an interface,
 * that is needed both as an object, and as the interface type.
 * @tparam I The interface that the underlying object implements.
 */
template<typename I, typename = std::enable_if<!std::derived_from<I, Object>, I>::type>
class Interface {
public:
	/**
	 * @brief Constructs an Interface by using the given object.
	 * If the object does not implement the templated interface or is nullptr, the Interface instance will be null.
	 * @tparam O The type of object that the parameter is, that implements the template type I interface.
	 * @param object Object that implements the template type I interface, used to construct the internals of an Interface instance.
	 */
	template<typename O, typename = std::enable_if<std::derived_from<O, Object>, O>::type>
	explicit inline constexpr Interface(O* object) noexcept {
		if(object == nullptr){
			return;
		}

		if(!object->template implements<I>()){
			return;
		}

		this->object = cast<Object>(object);
		this->interface = (I*) object;
	}

	/**
	 * @brief Copy constructor.
	 * @param other Copied interface.
	 */
	inline constexpr Interface(const Interface& other) noexcept {
		if(!other){
			return;
		}

		object = other.getObject();
		interface = other.getInterface();
	}

	/**
	 * @brief Move constructor, invalidates the moved interface.
	 * @param other Moved interface.
	 */
	inline constexpr Interface(Interface&& other) noexcept {
		if(!other){
			return;
		}

		object = other.getObject();
		interface = other.getInterface();

		other.object = nullptr;
		other.interface = nullptr;
	}

	/**
	 * @brief Default destructor
	 */
	virtual ~Interface() noexcept = default;

	/**
	 * @return The object that extends the interface.
	 */
	inline constexpr Object* getObject() const noexcept {
		if(!object.isValid()){
			return nullptr;
		}

		return *object;
	}

	/**
	 * @return The interface extended by the object.
	 */
	inline constexpr I* getInterface() const noexcept {
		if(!object.isValid()){
			return nullptr;
		}

		return interface;
	}

	/**
	 * @return True if the interface is valid (if both the object and the interface pointers are valid).
	 */
	inline constexpr explicit operator bool() const noexcept {
		return getObject() != nullptr && getInterface() != nullptr;
	}

	/**
	 * @return The interface extended by the object.
	 */
	inline constexpr I* operator *() const noexcept {
		return getInterface();
	}

	/**
	 * @return The interface extended by the object.
	 */
	inline constexpr I* operator ->() const noexcept {
		return getInterface();
	}

private:
	WeakObjectPtr<Object> object = nullptr;
	I* interface = nullptr;
};

#endif //CMF_INTERFACE_H