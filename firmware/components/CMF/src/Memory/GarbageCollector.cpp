#include "GarbageCollector.h"
#include "ObjectManager.h"
#include "Util/stdafx.h"

GarbageCollector::GarbageCollector() noexcept : lastCollection(millis()) {}

void GarbageCollector::tick(float deltaTime) noexcept{
	if(millis() - lastCollection < Interval){
		return;
	}

	lastCollection = millis();

	std::set<Object*> objectsToDelete;

	ObjectManager::get()->forEachObject([&objectsToDelete](Object* object) -> bool {
		if(ObjectManager::get()->isValid(object)){
			objectsToDelete.insert(object);
		}

		return false;
	});

	for(Object* object : objectsToDelete){
		if(object == nullptr){
			continue;
		}

		if(ObjectManager::get()->getReferenceCount(object) == 0){
			CMF_LOG(CMF, Warning, "Object was deleted via garbage collector.");
			delete object;
		}
	}
}
