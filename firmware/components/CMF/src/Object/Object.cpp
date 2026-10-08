#include "Object.h"
#include "Event/EventHandle.h"
#include "Memory/ObjectMemory.h"
#include "Thread/Threaded.h"
#include "Log/Log.h"
#include "Statics/ApplicationStatics.h"
#include "Core/Application.h"
#include "Class.h"

const Object::ClassType Object::objectStaticClass = Object::ClassType(static_cast<uint64_t>(STRING_HASH("Object")) << 32);

Object::Object() noexcept : id(ObjectIndex++){}

Object::~Object() noexcept {
	const std::set<WeakObjectPtr<Object>> children = childrenObjects;

	for(const WeakObjectPtr<Object>& child : children){
		if(!isValid(child)){
			continue;
		}

		delete *child;
	}

	if(owner != nullptr){
		std::lock_guard lock(owner->accessMutex);
		owner->removeChild(this);
	}
}
std::string Object::getName() const noexcept {
	return getStaticClass()->getName().append("_").append(std::to_string(getID()));
}

bool Object::isA(const Class* other) const noexcept {
	if(other == nullptr) {
		return false;
	}

	return other->getID() == staticClass()->getID();
}

void Object::postInitProperties() noexcept{}

void Object::__postInitProperties() noexcept {}

void Object::operator delete(void* ptr) noexcept {
	// No member function calls or any operations on member variables here, the destructor is called before the operator delete
	if(ptr == nullptr){
		return;
	}

	ObjectManager* manager = ObjectManager::get();
	if(manager == nullptr){
		return;
	}

	Object* object = static_cast<Object*>(ptr);

	if(!isValid(object)){
		return;
	}

	manager->onObjectDeleted(object);

	// Since destructor was already called, we only need to 'free' the memory here.
	std::free(ptr);
}

void Object::setOwner(Object* object) noexcept{
	Object* oldOwner = nullptr;
	Object* newOwner = (object != nullptr && object != this) ? object : nullptr;

	{
		std::lock_guard lock(accessMutex);
		oldOwner = owner.isValid() ? owner.get() : nullptr;
		owner = newOwner;
	}

	if(oldOwner != nullptr){
		std::lock_guard ownerLock(oldOwner->accessMutex);
		oldOwner->removeChild(this);
	}

	if(newOwner != nullptr){
		std::lock_guard ownerLock(newOwner->accessMutex);
		newOwner->registerChild(this);
	}

	onOwnerChanged(oldOwner);
}

void Object::onOwnerChanged(Object* oldOwner) noexcept{}

void Object::onChildAdded(Object* child) noexcept{}

void Object::onChildRemoved(Object* child) noexcept{}

void Object::forEachChild(const std::function<bool(Object*)>& function) noexcept{
	std::lock_guard lock(accessMutex);

	for(const WeakObjectPtr<Object>& child : childrenObjects){
		if(!child.isValid()){
			continue;
		}

		if(function(child.get())){
			return;
		}
	}
}

size_t Object::getChildCount() noexcept{
	std::lock_guard guard(accessMutex);
	return childrenObjects.size();
}

void Object::setInstigator(Object* object) noexcept{
	Object* oldInstigator = nullptr;

	{
		std::lock_guard lock(accessMutex);
		oldInstigator = instigator.get();
		instigator = object;
	}
	onInstigatorChanged(oldInstigator);
}

void Object::onInstigatorChanged(Object* oldInstigator) noexcept{}

void Object::scanEvents(TickType_t wait) noexcept{
	const uint64_t begin = millis();

	auto eventWaitTime = std::max(static_cast<int64_t>(0), static_cast<int64_t>(wait) - (static_cast<int64_t>(millis()) - static_cast<int64_t>(begin)));
	for(EventHandleBase* handle = nullptr; readyEventHandles.pop(handle,eventWaitTime ); ){
		if(handle == nullptr){
			continue;
		}

		handle->scan(0);
		eventWaitTime = 0;
	}

	std::lock_guard guard(accessMutex);

	// WARNING: This will not work, or will create an infinite loop if owner system is abused, this is intentional, events are dependent on their owner to scan events, outermost owner must be an async entity for this to work
	for(const WeakObjectPtr<Object>& child : childrenObjects){
		if(!child.isValid()){
			continue;
		}

		child->scanEvents(0);
	}
}

void Object::registerEventHandle(EventHandleBase* handle) const noexcept{
	if(handle == nullptr){
		return;
	}

	if(handle->getOwningObject() == nullptr){
		return;
	}

	if(handle->getOwningObject() != this && handle->getOwningObject()->getOutermostOwner() != this){
		return;
	}

	if(const Application* app = getApp()){
		if(EventScanner* scanner = app->getEventScanner()){
			scanner->registerHandle(handle);
		}
	}
}

void Object::unregisterEventHandle(EventHandleBase* handle) noexcept{
	if(handle == nullptr) {
		return;
	}

	if(handle->getOwningObject() == nullptr) {
		return;
	}

	if(handle->getOwningObject() != this && handle->getOwningObject()->getOutermostOwner() != this){
		return;
	}

	if(const Application* app = getApp()){
		if(EventScanner* scanner = app->getEventScanner()){
			scanner->unregisterHandle(handle);
		}
	}

	readyEventHandles.remove(handle);
}

void Object::readyEventHandle(EventHandleBase *handle) noexcept{
	readyEventHandles.push(handle);
}

Archive& Object::serialize(Archive& archive) noexcept{
	return archive;
}

Application* Object::getApp() noexcept{
	return ApplicationStatics::getApplication();
}

Object* Object::getOutermostOwner() const noexcept{
	Object* outermost = getOwner();
	if(outermost == nullptr){
		return nullptr;
	}

	while(outermost != nullptr && outermost->getOwner() != nullptr){
		outermost = outermost->getOwner();
	}

	return outermost;
}

Object* Object::getOutermostInstigator() const noexcept{
	Object* outermost = getInstigator();
	if(outermost == nullptr){
		return nullptr;
	}

	while(outermost != nullptr && outermost->getInstigator() != nullptr){
		outermost = outermost->getInstigator();
	}

	return outermost;
}

inline void Object::registerChild(Object* child) noexcept{
	// NOTE: accessMutex must be locked before calling this function to avoid multithreading issues. If it is not locked, bugs can occur.
	WeakObjectPtr<Object> newChild = child;
	if(!newChild.isValid()){
		return;
	}

	childrenObjects.insert(newChild);
	onChildAdded(child);
}

inline void Object::removeChild(Object* child) noexcept{
	// NOTE: accessMutex must be locked before calling this function to avoid multithreading issues. If it is not locked, bugs can occur.
	if(child == nullptr){
		return;
	}

	if(childrenObjects.erase(child) == 0){
		return;
	}

	onChildRemoved(child);
}
