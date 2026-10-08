#ifndef CMF_OBJECTMANAGER_H
#define CMF_OBJECTMANAGER_H

#include <functional>
#include <map>
#include <set>
#include <mutex>

class Object;

/**
 * @brief Object manager is a class that keeps all valid references to objects, invalidates them when needed,
 * checks validity of object pointers and check if an object is ready for garbage collection.
 */
class ObjectManager {
public:
	/**
	 * @return The static instance of the object manager.
	 */
	static ObjectManager* get() noexcept;

public:
	/**
	 * @brief Default destructor.
	 */
	virtual ~ObjectManager() noexcept = default;

	/**
	 * @param object Object of which the reference count is returned. Only strong object pointers influence it.
	 * @return The reference count of the given object.
	 */
	uint32_t getReferenceCount(const Object* object) noexcept;

	/**
	 * @brief Checks if the object is valid. Object is considered valid if it is not nullptr,
	 * contained in the object reference tree with more than 0 strong references and not marked for destroy.
	 * @param object Object being checked for validity.
	 * @return True if given object is valid.
	 */
	bool isValid(const Object* object) noexcept;

	/**
	 * @brief Registers a weak or strong object reference to the given pointer.
	 * @param object Pointer to the object pointer being managed.
	 * @param keepAlive Determines if the pointer is weak or strong type.
	 */
	void registerReference(Object** object, bool keepAlive = false) noexcept;

	/**
	 * @brief Unregisters a weak or strong object reference to the given pointer.
	 * Usually happens when a smart object pointer is destroyed.
	 * @param object Pointer to the object pointer being unregistered.
	 * @param keepAlive Determines if the pointer is weak or strong type.
	 */
	void unregisterReference(Object** object, bool keepAlive = false) noexcept;

	/**
	 * @brief Iterated through all managed objects and calls the given callback function for each until the callback returns true.
	 * @param fn The callback function being executed for each object until true is returned.
	 */
	void forEachObject(const std::function<bool(Object*)>& fn) noexcept;

	/**
	 * @brief Called when an object is deleted from memory by the garbage collector.
	 * Removes internal components needed for managing that object.
	 * @param object The object being deleted.
	 */
	void onObjectDeleted(Object* object) noexcept;

private:
	/**
	 * @brief Contains all pointers that reference a single object, both weak and strong.
	 * Only the strong pointer influence the count, which is what keeps the object alive while > 0.
	 */
	struct ObjectRefInfo {
		uint32_t count = 0;
		std::set<Object**> objectPointers;
	};

private:
	std::mutex mutex;
	std::map<Object*, ObjectRefInfo> objectReferenceMap;
};

#endif //CMF_OBJECTMANAGER_H