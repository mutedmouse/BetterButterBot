#ifndef CMF_WEAKOBJECTPTR_H
#define CMF_WEAKOBJECTPTR_H

#include "ObjectPtr.h"

/**
 * @brief Weak object pointer holds a weak reference to an instance of an object.
 * This does not impact the lifetime of the object, and the pointer is invalidated when the object is destroyed.
 * @tparam T The type of object being pointed to.
 */
template<typename T>
class WeakObjectPtr : public ObjectPtr<T, false> {
public:
	/**
	 * @brief Default empty constructor.
	 */
	inline constexpr WeakObjectPtr() noexcept = default;

	/**
	 * @brief Copy constructor.
	 * @param other The weak object pointer being copied.
	 */
	inline constexpr WeakObjectPtr(const WeakObjectPtr& other) noexcept : ObjectPtr<T, false>(other) {}

	/**
	 * @brief Move constructor.
	 * @param other The weak object pointer being moved. The other pointer is invalid after.
	 */
	inline constexpr WeakObjectPtr(WeakObjectPtr&& other) noexcept : ObjectPtr<T, false>(std::move(other)) {}

	/**
	 * @brief Copy constructor of a strong object pointer type.
	 * @param other The strong object pointer being copied.
	 */
	inline constexpr WeakObjectPtr(const ObjectPtr<T, true>& other) noexcept : ObjectPtr<T, false>(other) {}

	/**
	 * @brief Move constructor of a strong object pointer type. The other pointer is invalid after.
	 * @param other The strong object pointer being moved.
	 */
	inline constexpr WeakObjectPtr(ObjectPtr<T, true>&& other) noexcept : ObjectPtr<T, false>(std::move(other)) {}

	/**
	 * @brief Copy constructor for an object pointer with different object type,
	 * and same or different lifetime management. The other pointer is invalidated.
	 * @tparam _T The object type of the object pointer being copied.
	 * @tparam _KeepAlive The lifetime management type of the object pointer being copied.
	 * @param other The object pointer being copied.
	 */
	template<typename _T, bool _KeepAlive>
	inline constexpr WeakObjectPtr(const ObjectPtr<_T, _KeepAlive>& other) noexcept : ObjectPtr<T, false>(other) {}

	/**
	 * @brief Move constructor for an object pointer with different object type,
	 * and same or different lifetime management. The other pointer is invalidated.
	 * @tparam _T The object type of the object pointer being moved.
	 * @tparam _KeepAlive The lifetime management type of the object pointer being copied.
	 * @param other The object pointer being copied.
	 */
	template<typename _T, bool _KeepAlive>
	inline constexpr WeakObjectPtr(ObjectPtr<_T, _KeepAlive>&& other) noexcept : ObjectPtr<T, false>(std::move(other)) {}

	/**
	 * @brief Constructor from a raw object pointer.
	 * @param object Object being pointed to.
	 */
	inline constexpr WeakObjectPtr(Object* object) noexcept : ObjectPtr<T, false>(object) {}

	/**
	 * @brief Constructor from a nullptr_t type.
	 * The weak object pointer is always invalid after this until another pointer is set to it.
	 */
	inline constexpr WeakObjectPtr(nullptr_t) noexcept : ObjectPtr<T, false>(nullptr) {}

	/**
	 * @brief Default destructor.
	 */
	virtual ~WeakObjectPtr() noexcept override = default;

	/**
	 * @brief Move operator =. Invalidates the other weak object pointer.
	 * @param other The weak object pointer being moved.
	 * @return Reference to this weak object pointer.
	 */
	inline constexpr WeakObjectPtr& operator = (WeakObjectPtr&& other) noexcept {
		ObjectPtr<T, false>::operator=(std::move(other));

		return *this;
	}

	/**
	 * @brief Copy operator = with a weak object pointer of a different object type.
	 * @tparam _T The other object type.
	 * @param other The weak object pointer being copied.
	 * @return Reference to this weak object pointer.
	 */
	template<typename _T>
	inline constexpr WeakObjectPtr& operator = (const WeakObjectPtr<_T>& other) noexcept {
		ObjectPtr<T, false>::template operator=<_T>(other);

		return *this;
	}

	/**
	 * @brief Move assignment operator with a weak object pointer of a different object type. Invalidates the other weak object pointer.
	 * @tparam _T The other object type.
	 * @param other The weak object pointer being moved.
	 * @return Reference to this weak object pointer.
	 */
	template<typename _T>
	inline constexpr WeakObjectPtr& operator = (WeakObjectPtr<_T>&& other) noexcept {
		ObjectPtr<T, false>::template operator=<_T>(std::move(other));

		return *this;
	}
};

#endif //CMF_WEAKOBJECTPTR_H
