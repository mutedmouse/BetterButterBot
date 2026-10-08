#ifndef CMF_OBJECTMEMORY_H
#define CMF_OBJECTMEMORY_H

#include <concepts>
#include <span>
#include "Cast.h"
#include "Object/Object.h"
#include "Memory/SmartPtr/StrongObjectPtr.h"
#include "Statics/ApplicationStatics.h"
#include "ObjectManager.h"
#include "Object/Class.h"
#include "Object/Interface.h"

/**
 * @brief Initialization function, used to set the owner of the object, as well as call the postInitProperties of the object.
 * This function is called after an object is created.
 * @param object The object to initialize.
 * @param owner The owner to set to the object.
 */
void initObject(Object* object, Object* owner) noexcept;

/**
 * @param object Object to check for validity.
 * @return Is object valid or not. If object was marked for deletion, deleted or nullptr, it is not valid.
 */
bool isValid(const Object* object) noexcept;

/**
 * @tparam T Object type in the object smart pointer.
 * @tparam KeepAlive Marks the type of smart pointer, set automatically depending on the type of object pointer passed through the argument.
 * @param object The object to check for validity.
 * @return Is object valid or not. If object was marked for deletion, deleted or nullptr, it is not valid.
 */
template<typename T, bool KeepAlive>
inline bool isValid(const ObjectPtr<T, KeepAlive>& object) noexcept {
	if(object == nullptr){
		return false;
	}

	return ObjectManager::get()->isValid(object.get());
}

/**
 * @tparam T Type of interface being checked for validity.
 * @param interface The interface to check for validity.
 * @return True if interface is not null, and if the object of the interface is valid.
 */
template<typename T>
inline bool isValid(const Interface<T>& interface) noexcept {
	if(!interface){
		return false;
	}

	return isValid(interface.getObject());
}

/**
 * @brief Creates a object of type T with given arguments passed to the constructor.
 * @tparam T Type of object to create.
 * @tparam Args Types of arguments that need to be passed to the object constructor.
 * @param owner The owner to set.
 * @param args Arguments that are passed to the object constructor.
 * @return A strong object pointer to the newly created object.
 */
template<typename T, typename ...Args, typename = std::enable_if<std::derived_from<T, Object>, T>::type>
inline StrongObjectPtr<T> newObject(Object* owner = nullptr, Args&&... args) noexcept {
	// TODO change this to better support with API
	void* temp = operator new(sizeof(T));
	memset(temp, 0, sizeof(T));

	// This is used to make sure the new object is valid in its constructor
	StrongObjectPtr<T> tempPtr = static_cast<T*>(temp);

	StrongObjectPtr<T> newObject = new(temp) T(std::forward<Args>(args)...);

	initObject(cast<Object>(newObject.get()), owner);

	return newObject;
}

/**
 * @brief Creates a new object from a give class.
 * Using types that are not compatible with the constructor of the actual object of the class type will still compile but could result in unexpected behavior.
 * @tparam T Type of object being created.
 * @param cls Class of which the object is created.
 * @param owner Owner of the new object.
 * @param args Constructor arguments
 * @return Strong object pointer to the newly created object.
 */
template<typename T, typename ...Args, typename = std::enable_if<std::derived_from<T, Object>, T>::type>
inline StrongObjectPtr<T> newObject(const Class* cls, Object* owner = nullptr, Args... args) noexcept {
	if(cls == nullptr){
		return nullptr;
	}

	StrongObjectPtr<T> newObjectPtr = cast<T>(cls->createObject(std::forward<Args>(args)...).get());
	if(!newObjectPtr.isValid()){
		return nullptr;
	}

	initObject(cast<Object>(newObjectPtr.get()), owner);

	return newObjectPtr;
}

/**
 * @brief Specialization of the newObject function for empty constructors.
 * @tparam T Type of object being created.
 * @tparam Args Types of arguments being passed to the object, which are always empty in the case of this function.
 * @param owner The owner of the newly created object.
 * @param args Arguments being passed to the object, which are always empty in the case of this function.
 * @return Strong object pointer to the new object instance.
 */
template<typename T, typename ...Args, typename = std::enable_if<std::derived_from<T, Object>, T>::type>
inline StrongObjectPtr<T> newObject(Object* owner = nullptr, Args&&... args) noexcept requires (sizeof...(Args) == 0){
	// TODO change this to better support with API
	void* temp = operator new(sizeof(T));
	memset(temp, 0, sizeof(T));

	// This is used to make sure the new object is valid in its constructor
	StrongObjectPtr<T> tempPtr = static_cast<T*>(temp);

	StrongObjectPtr<T> newObject = new(temp) T();

	initObject(cast<Object>(newObject.get()), owner);

	return newObject;
}

/**
 * @brief De-serializes an object of type T from a byte array with a given owner.
 * @tparam T Type of object being de-serialized.
 * @param data Byte array containing information about the object.
 * @param owner Owner being set to the created object.
 * @return The newly created object.
 */
template<typename T, typename = std::enable_if<std::derived_from<T, Object>, T>::type>
inline StrongObjectPtr<T> objectFromByteArray(std::span<const uint8_t> data, Object* owner = nullptr) noexcept {
	OutArchive archive(data);
	uint64_t classID = 0;
	archive << classID;

	StrongObjectPtr<T> object = newObject<T>(Class::getClasByID(classID), owner);
	if(!object.isValid()){
		return nullptr;
	}

	object->serialize(archive);

	return object;
}

/**
 * @brief De-serializes an object of type T from a byte array with a given owner.
 * @tparam T Type of object being de-serialized.
 * @param data Byte array containing information about the object.
 * @param owner Owner being set to the created object.
 * @return True if successful, false if given object is invalid or of wrong type
 */
template<typename T, typename = std::enable_if<std::derived_from<T, Object>, T>::type>
inline bool objectFromByteArray(T* object, std::span<const uint8_t> data) noexcept {
	if(!isValid(object)) {
		return false;
	}

	OutArchive archive(data);
	uint64_t classID = 0;
	archive << classID;

	const Class* cls = Class::getClasByID(classID);
	if(cls == nullptr) {
		return false;
	}

	if(!object->isA(cls)) {
		return false;
	}

	object->serialize(archive);

	return true;
}

/**
 * @brief Serializes the given object to a byte array by using archives internally.
 * @param object The object being serialized.
 * @param data Data buffer being set.
 */
inline void byteArrayFromObject(Object* object, std::vector<uint8_t>& data) noexcept {
	if(object == nullptr){
		return;
	}

	InArchive archive;
	archive << object->getStaticClass()->getID();

	object->serialize(archive);

	archive.toByteArray(data);
}

#endif //CMF_OBJECTMEMORY_H