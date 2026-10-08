#include "Class.h"
#include "Object.h"
#include "Log/Log.h"
#include "Memory/SmartPtr/StrongObjectPtr.h"

const Class* ClassRegistry::getClass(uint64_t ID) const noexcept{
	if(!classes.contains(ID)){
		return nullptr;
	}

	return classes.at(ID);
}

void ClassRegistry::registerClass(const Class* cls) noexcept{
	if(classes.contains(cls->getID())){
		return;
	}

	classes[cls->getID()] = cls;
}

uint64_t Class::getID() const noexcept{
	return classID;
}

bool Class::isA(const Class *other) const noexcept {
	if(other == nullptr) {
		return false;
	}

	return other->getID() == getID();
}

Class::Class(uint64_t ID) noexcept : classID(ID) {
	if(registry == nullptr){
		registry = new ClassRegistry();
	}

	registry->registerClass(this);
}

StrongObjectPtr<Object> Class::__createObject(void* arguments) const noexcept {
	void* temp = operator new(sizeof(Object));
	if(temp == nullptr){
		CMF_LOG(CMF, LogLevel::Error, "Class::__createObject: out of memory allocating Object (%zu B)", sizeof(Object));
		return nullptr;
	}

	memset(temp, 0, sizeof(Object));

	// This is used to make sure the new object is valid in its constructor
	StrongObjectPtr<Object> tempPtr = static_cast<Object*>(temp);

	return new(temp) Object();
}
