#ifndef CMF_STRONGOBJECTPTR_H
#define CMF_STRONGOBJECTPTR_H

#include "ObjectPtr.h"

#include <Log/Log.h>

/**
 * @brief Strong object pointer holds a strong reference to an instance of an object,
 * which works towards keeping the object from being destroyed.
 * @tparam T The type of object being pointer to.
 */
template<typename T>
class StrongObjectPtr : public ObjectPtr<T, true> {
public:
	/**
	 * @brief Default empty constructor.
	 */
	inline constexpr StrongObjectPtr() noexcept = default;

	/**
	 * @brief Copy constructor.
	 * @param other The strong object pointer being copied.
	 */
	inline constexpr StrongObjectPtr(const StrongObjectPtr& other) noexcept : ObjectPtr<T, true>(other) {}

	/**
	 * @brief Move constructor.
	 * @param other The strong object pointer being moved. The other pointer is invalid after.
	 */
	inline constexpr StrongObjectPtr(StrongObjectPtr&& other) noexcept : ObjectPtr<T, true>(std::move(other)) {}

	/**
	 * @brief Copy constructor of a weak object pointer type.
	 * @param other The weak object pointer being copied.
	 */
	inline constexpr StrongObjectPtr(const ObjectPtr<T, false>& other) noexcept : ObjectPtr<T, true>(other) {}

	/**
	 * @brief Move constructor of a weak object pointer type. The other pointer is invalid after.
	 * @param other The weak object pointer being moved.
	 */
	inline constexpr StrongObjectPtr(ObjectPtr<T, false>&& other) noexcept : ObjectPtr<T, true>(std::move(other)) {}

	/**
	 * @brief Copy constructor for an object pointer with different object type,
	 * and same or different lifetime management. The other pointer is invalidated.
	 * @tparam _T The object type of the object pointer being copied.
	 * @tparam _KeepAlive The lifetime management type of the object pointer being copied.
	 * @param other The object pointer being copied.
	 */
	template<typename _T, bool _KeepAlive>
	inline constexpr StrongObjectPtr(const ObjectPtr<_T, _KeepAlive>& other) noexcept : ObjectPtr<T, true>(other) {}

	/**
	 * @brief Move constructor for an object pointer with different object type,
	 * and same or different lifetime management. The other pointer is invalidated.
	 * @tparam _T The object type of the object pointer being moved.
	 * @tparam _KeepAlive The lifetime management type of the object pointer being copied.
	 * @param other The object pointer being copied.
	 */
	template<typename _T, bool _KeepAlive>
	inline constexpr StrongObjectPtr(ObjectPtr<_T, _KeepAlive>&& other) noexcept : ObjectPtr<T, true>(std::move(other)) {}

	/**
	 * @brief Constructor from a raw object pointer.
	 * @param object Object being pointed to.
	 */
	inline constexpr StrongObjectPtr(Object* object) noexcept : ObjectPtr<T, true>(object) {}

	/**
	 * @brief Constructor from a nullptr_t type. The strong object pointer is always invalid until another pointer is set to it.
	 */
	inline constexpr StrongObjectPtr(nullptr_t) noexcept : ObjectPtr<T, true>(nullptr) {}

	/**
	 * @brief Default destructor.
	 */
	virtual ~StrongObjectPtr() noexcept override = default;

	/**
	 * @brief Move operator =. Invalidates the other strong object pointer.
	 * @param other The strong object pointer being moved.
	 * @return Reference to this strong object pointer.
	 */
	inline constexpr StrongObjectPtr& operator = (StrongObjectPtr&& other) noexcept {
		ObjectPtr<T, true>::operator=(std::move(other));

		return *this;
	}

	/**
	 * @brief Copy operator = with a strong object pointer of a different object type.
	 * @tparam _T The other object type.
	 * @param other The strong object pointer being copied.
	 * @return Reference to this strong object pointer.
	 */
	template<typename _T>
	inline constexpr StrongObjectPtr& operator = (const StrongObjectPtr<_T>& other) noexcept {
		ObjectPtr<T, true>::template operator=<_T>(other);

		return *this;
	}

	/**
	 * @brief Move operator = with a strong object pointer of a different object type.
	 * Invalidates the other strong object pointer.
	 * @tparam _T The other object type.
	 * @param other The strong object pointer being moved.
	 * @return Reference to this strong object pointer.
	 */
	template<typename _T>
	inline constexpr StrongObjectPtr& operator = (StrongObjectPtr<_T>&& other) noexcept {
		ObjectPtr<T, true>::template operator=<_T>(std::move(other));

		return *this;
	}
};

#endif //CMF_STRONGOBJECTPTR_H