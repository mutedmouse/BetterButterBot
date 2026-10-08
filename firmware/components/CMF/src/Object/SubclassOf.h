#ifndef CMF_SUBCLASSOF_H
#define CMF_SUBCLASSOF_H

#include <concepts>
#include "Object.h"

/**
 * @brief SubclassOf is used to mark a Object type that is of template type T (or derives template type T).
 * @tparam T The object type that the underlying class instance has to be of, or has to extend.
 */
template<typename T, typename = std::enable_if_t<std::derived_from<T, Object>, T>>
class SubclassOf {
public:
	/**
	 * @brief Constructs a default SubclassOf which will be set to null.
	 */
	inline constexpr SubclassOf() noexcept = default;

	/**
	 * @brief Constructs a SubclassOf from the given object, if object is of class T or derives class T.
	 * If not, the constructed SubclassOf is null.
	 * @param object Object from which the SubclassOf will be constructed.
	 */
	explicit inline constexpr SubclassOf(const Object* object) noexcept {
		if(object == nullptr){
			return;
		}

		if(object->isA<T>()){
			subclass = object->getStaticClass();
		}
	}

	/**
	 * @brief Constructs a SubclassOf from a given class, if that class is class of T, or extended object type.
	 * @param cls Class used for construction.
	 */
	inline constexpr SubclassOf(const Class* cls) noexcept {
		if(cls == nullptr){
			return;
		}

		if(cls->isA<T>()) {
			subclass = cls;
		}
	}

	/**
	 * @brief Constructs a SubclassOf with a nullptr_t parameter type that is always null.
	 */
	inline constexpr SubclassOf(nullptr_t) noexcept {}

	/**
	 * @brief Default constructor.
	 */
	virtual constexpr ~SubclassOf() noexcept = default;

	/**
	 * @return The pointer to the class used for construction of the SubclassOf instance.
	 */
	inline constexpr const Class* operator *() const noexcept {
		return subclass;
	}

	/**
	 * @return The pointer to the class used for construction of the SubclassOf instance.
	 */
	inline constexpr const Class* operator ->() const noexcept {
		return subclass;
	}

	/**
	 * @param other Compared SubclassOf.
	 * @return True if both SubclassOf instances are the same (if they point to the same Class instance).
	 */
	inline constexpr bool operator == (const SubclassOf& other) const noexcept {
		return subclass == other.subclass;
	}

	/**
	 * @param other Compared class.
	 * @return True if this instance has the same underlying class as the given comparison parameter.
	 */
	inline constexpr bool operator == (const Class* other) const noexcept {
		return subclass == other;
	}

	/**
	 * @return True if this SubclassOf is null.
	 */
	inline constexpr bool operator == (nullptr_t) const noexcept {
		return subclass == nullptr;
	}

private:
	const Class* subclass = nullptr;
};

#endif //CMF_SUBCLASSOF_H