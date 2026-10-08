#ifndef CMF_OBJECTPTR_H
#define CMF_OBJECTPTR_H

#include <type_traits>
#include "Memory/Cast.h"
#include "Memory/ObjectManager.h"

/**
 * @brief Base object pointer class used for common functionality between the weak object pointer and the strong object pointer.
 * @tparam T Type of object being pointed to.
 * @tparam KeepAlive Determines if the pointer is working towards preventing the object from being destroyed or not.
 */
template<typename T, bool KeepAlive>
class ObjectPtr : public std::integral_constant<bool, KeepAlive> {
public:
	/**
	 * @brief Default empty constructor.
	 */
	inline constexpr ObjectPtr() noexcept = default;

	/**
	 * @brief Copy constructor.
	 * @param other The object pointer being copied.
	 */
	inline constexpr ObjectPtr(const ObjectPtr& other) noexcept {
		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());
	}

	/**
	 * @brief Move constructor. The other object pointer is invalidated.
	 * @param other The object pointer being moved.
	 */
	inline constexpr ObjectPtr(ObjectPtr&& other) noexcept {
		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());
		other = nullptr;
	}

	/**
	 * @brief Copy constructor for a different object lifetime management pointer.
	 * @param other The object pointer being copied.
	 */
	inline constexpr ObjectPtr(const ObjectPtr<T, !KeepAlive>& other) noexcept {
		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());
	}

	/**
	 * @brief Move constructor for a different object lifetime management pointer. The other pointer is invalidated.
	 * @param other The object pointer being moved.
	 */
	inline constexpr ObjectPtr(ObjectPtr<T, !KeepAlive>&& other) noexcept {
		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());
		other = nullptr;
	}

	/**
	 * @brief Copy constructor for an object pointer with different object type, and same or different lifetime management.
	 * @tparam _T The object type of the object pointer being copied.
	 * @tparam _KeepAlive The lifetime management type of the object pointer being copied.
	 * @param other The object pointer being copied.
	 */
	template<typename _T, bool _KeepAlive>
	inline constexpr ObjectPtr(const ObjectPtr<_T, _KeepAlive>& other) noexcept {
		if(cast<T>(*other) == nullptr){
			return;
		}

		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());
	}

	/**
	 * @brief Move constructor for an object pointer with different object type,
	 * and same or different lifetime management. The other pointer is invalidated.
	 * @tparam _T The object type of the object pointer being moved.
	 * @tparam _KeepAlive The lifetime management type of the object pointer being copied.
	 * @param other The object pointer being copied.
	 */
	template<typename _T, bool _KeepAlive>
	inline constexpr ObjectPtr(ObjectPtr<_T, _KeepAlive>&& other) noexcept {
		if(cast<T>(*other) == nullptr){
			return;
		}

		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());
		other = nullptr;
	}

	/**
	 * @brief Constructor from a raw object pointer.
	 * @param object Object being pointer to.
	 */
	inline constexpr ObjectPtr(Object* object) noexcept {
		ptr = object;
		ObjectManager::get()->registerReference(&ptr, (*this)());
	}

	/**
	 * @brief Constructor from a nullptr_t type. The object pointer will always be invalid until another pointer is set.
	 */
	inline constexpr ObjectPtr(nullptr_t) noexcept {
		if(ptr != nullptr){
			ObjectManager::get()->unregisterReference(&ptr, (*this)());
		}

		ptr = nullptr;
	}

	/**
	 * @brief Destructor which removes the registered reference of this object pointer from the object manager.
	 */
	virtual ~ObjectPtr() noexcept {
		if(ptr != nullptr){
			ObjectManager::get()->unregisterReference(&ptr, (*this)());
		}

		ptr = nullptr;
	}

	/**
	 * @brief Copy operator =.
	 * @param other The object pointer being copied.
	 * @return Reference to this object pointer.
	 */
	inline constexpr ObjectPtr& operator = (const ObjectPtr& other) noexcept {
		if(&other == this){
			return *this;
		}

		if(ptr != nullptr){
			ObjectManager::get()->unregisterReference(&ptr, (*this)());
		}

		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());

		return *this;
	}

	/**
	 * @brief Move operator =. The other object pointer is invalidated.
	 * @param other The object pointer being moved.
	 * @return Reference to this object pointer.
	 */
	inline constexpr ObjectPtr& operator = (ObjectPtr&& other) noexcept {
		if(&other == this){
			return *this;
		}

		if(ptr != nullptr){
			ObjectManager::get()->unregisterReference(&ptr, (*this)());
		}

		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());

		other = nullptr;

		return *this;
	}

	/**
	 * @brief Copy operator = with a different lifetime management pointer.
	 * @param other The object pointer being copied.
	 * @return Reference to this object pointer.
	 */
	inline constexpr ObjectPtr& operator = (const ObjectPtr<T, !KeepAlive>& other) noexcept {
		if(ptr != nullptr){
			ObjectManager::get()->unregisterReference(&ptr, (*this)());
		}

		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());

		return *this;
	}

	/**
	 * @brief Move operator = with a different lifetime management pointer. The other pointer is invalidated.
	 * @param other The object pointer being moved.
	 * @return Reference to this object pointer.
	 */
	inline constexpr ObjectPtr& operator = (ObjectPtr<T, !KeepAlive>&& other) noexcept {
		if(ptr != nullptr){
			ObjectManager::get()->unregisterReference(&ptr, (*this)());
		}

		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());

		other = nullptr;

		return *this;
	}

	/**
	 * @brief Copy operator = with an different type object pointer, with same or different lifetime management.
	 * @tparam _T The other object type.
	 * @tparam _KeepAlive The other pointer lifetime management.
	 * @param other The object pointer being copied.
	 * @return Reference to this object pointer.
	 */
	template<typename _T, bool _KeepAlive>
	inline constexpr ObjectPtr& operator = (const ObjectPtr<_T, _KeepAlive>& other) noexcept {
		if(cast<T>(*other) == nullptr){
			return *this;
		}

		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());

		return *this;
	}

	/**
	 * @brief Move operator = with a different type object pointer,
	 * with same or different lifetime management. The other object pointer is invalidated.
	 * @tparam _T The other object type.
	 * @tparam _KeepAlive The other pointer lifetime management.
	 * @param other The object pointer being moved.
	 * @return Reference to this object pointer.
	 */
	template<typename _T, bool _KeepAlive>
	inline constexpr ObjectPtr& operator = (ObjectPtr<_T, _KeepAlive>&& other) noexcept {
		if(cast<T>(*other) == nullptr){
			return *this;
		}

		ptr = *other;
		ObjectManager::get()->registerReference(&ptr, (*this)());

		other = nullptr;

		return *this;
	}

	/**
	 * @brief Operator = with a raw object pointer.
	 * @param object Object being pointed to.
	 * @return Reference to this object pointer.
	 */
	inline constexpr ObjectPtr& operator = (Object* object) noexcept {
		if(ptr != nullptr){
			ObjectManager::get()->unregisterReference(&ptr, (*this)());
		}

		ptr = object;
		ObjectManager::get()->registerReference(&ptr, (*this)());

		return *this;
	}

	/**
	 * @brief Operator = with a nullptr_t type. Invalidates this object pointer.
	 * @return Reference to this object pointer.
	 */
	inline constexpr ObjectPtr& operator = (nullptr_t) noexcept {
		if(ptr != nullptr){
			ObjectManager::get()->unregisterReference(&ptr, (*this)());
		}

		ptr = nullptr;

		return *this;
	}

	/**
	 * @brief Comparison operator with another object pointer.
	 * @param other The object pointer being compared with.
	 * @return True if both are invalid, or if both point to the same object.
	 */
	inline constexpr bool operator == (const ObjectPtr& other) const noexcept {
		if(!other.isValid()){
			return !isValid();
		}

		return ptr == other.ptr;
	}

	/**
	 * @brief Comparison operator with another object pointer with a different lifetime management.
	 * @param other The object pointer being compared with.
	 * @return True if both are invalid or if both point to the same object.
	 */
	inline constexpr bool operator == (const ObjectPtr<T, !KeepAlive>& other) const noexcept {
		if(!other.isValid()){
			return !isValid();
		}

		return ptr == other.ptr;
	}

	/**
	 * @brief Comparison operator with a raw object pointer.
	 * @param object The raw object pointer being compared with.
	 * @return True if both are invalid or if both point to the same object.
	 */
	inline constexpr bool operator == (const Object* object) const noexcept {
		if(!ObjectManager::get()->isValid(object)){
			return !isValid();
		}

		return ptr == object;
	}

	/**
	 * @brief Comparison operator with a nullptr_t type.
	 * @return True if invalid.
	 */
	inline constexpr bool operator == (nullptr_t) const noexcept {
		return !isValid();
	}

	/**
	 * @brief Operator > with another object pointer.
	 * @param other The object pointer being compared with.
	 * @return Same as raw pointer > operator.
	 */
	inline constexpr bool operator > (const ObjectPtr& other) const noexcept {
		return ptr > *other;
	}

	/**
	 * @brief Operator > with another object pointer with different lifetime management.
	 * @param other The object pointer being compared with.
	 * @return Same as raw pointer > operator.
	 */
	inline constexpr bool operator > (const ObjectPtr<T, !KeepAlive>& other) const noexcept {
		return ptr > *other;
	}

	/**
	 * @brief Operator > with another object raw pointer.
	 * @param object The pointer being compared with.
	 * @return Same as raw pointer > operator.
	 */
	inline constexpr bool operator > (const Object* object) const noexcept {
		return ptr > object;
	}

	/**
	 * @brief Operator < with another object pointer.
	 * @param other The object pointer being compared with.
	 * @return Same as raw pointer < operator.
	 */
	inline constexpr bool operator < (const ObjectPtr& other) const noexcept {
		return ptr < *other;
	}

	/**
	 * @brief Operator < with another object pointer with different lifetime management.
	 * @param other The object pointer being compared with.
	 * @return Same as raw pointer < operator.
	 */
	inline constexpr bool operator < (const ObjectPtr<T, !KeepAlive>& other) const noexcept {
		return ptr < *other;
	}

	/**
	 * @brief Operator < with another object raw pointer.
	 * @param object The pointer being compared with.
	 * @return Same as raw pointer < operator.
	 */
	inline constexpr bool operator < (const Object* object) const noexcept {
		return ptr < object;
	}

	/**
	 * @return The object being pointed to.
	 */
	inline constexpr T* operator * () const noexcept {
		return get();
	}

	/**
	 * @return The object being pointed to.
	 */
	inline constexpr T* operator -> () const noexcept {
		return get();
	}

	/**
	 * @return True if this object pointer is valid.
	 */
	inline constexpr operator bool() const noexcept {
		return isValid();
	}

	/**
	 * @return The raw pointer to the referenced object.
	 */
	inline constexpr T* get() const noexcept {
		return cast<T>(ptr);
	}

	/**
	 * @return True if object is valid, meaning not nullptr and valid in the object manager.
	 */
	inline constexpr bool isValid() const noexcept {
		if(!ObjectManager::get()->isValid(ptr)){
			return false;
		}

		return get() != nullptr;
	}

private:
	Object* ptr = nullptr;
};

#endif //CMF_OBJECTPTR_H