#include "ObjectManager.h"
#include "Object/Object.h"

ObjectManager* ObjectManager::get() noexcept{
	static ObjectManager managerInstance;
	return &managerInstance;
}

uint32_t ObjectManager::getReferenceCount(const Object* object) noexcept{
	if(object == nullptr){
		return 0;
	}

	std::lock_guard lock(mutex);

	if(!objectReferenceMap.contains(const_cast<Object*>(object))){
		return 0;
	}

	return objectReferenceMap[const_cast<Object*>(object)].count;
}

bool ObjectManager::isValid(const Object* object) noexcept{
	if(object == nullptr){
		return false;
	}

	{
		std::lock_guard lock(mutex);

		if(!objectReferenceMap.contains(const_cast<Object*>(object))){
			return false;
		}
	}

	return getReferenceCount(object) > 0;
}

void ObjectManager::registerReference(Object** object, bool keepAlive/* = false*/) noexcept{
	std::lock_guard lock(mutex);

	ObjectRefInfo& objectInfo = objectReferenceMap[*object];

	objectInfo.objectPointers.insert(object);

	if(keepAlive){
		objectInfo.count++;
	}
}

void ObjectManager::unregisterReference(Object** object, bool keepAlive/* = false*/) noexcept{
	std::lock_guard lock(mutex);

	if(!objectReferenceMap.contains(*object)){
		return;
	}

	ObjectRefInfo& objectInfo = objectReferenceMap[*object];

	objectInfo.objectPointers.erase(object);

	if(keepAlive){
		if(objectInfo.count > 0){
			objectInfo.count--;
		}
	}
}

void ObjectManager::forEachObject(const std::function<bool(Object*)>& fn) noexcept{
	if(fn == nullptr){
		return;
	}

	std::set<Object*> keys;

	{
		std::lock_guard lock(mutex);

		for(std::pair<Object* const, ObjectRefInfo>& pair : objectReferenceMap){
			keys.insert(pair.first);
		}
	}

	for(Object* object : keys){
		if(fn(object)){
			break;
		}
	}
}

void ObjectManager::onObjectDeleted(Object* object) noexcept{
	std::lock_guard lock(mutex);

	if(!objectReferenceMap.contains(object)){
		return;
	}

	ObjectRefInfo& objectInfo = objectReferenceMap[object];

	for(Object** ptr : objectInfo.objectPointers){
		*ptr = nullptr;
	}

	objectInfo.count = 0;

	objectReferenceMap.erase(object);
}
