#include "AsyncEntity.h"
#include "SyncEntity.h"
#include "Util/stdafx.h"
#include "Memory/ObjectMemory.h"
#include "Log/Log.h"
#include "Core/Application.h"

AsyncEntity::AsyncEntity(TickType_t interval /*= CONFIG_CMF_ASYNCENTITY_TICK_INTERVAL / portTICK_PERIOD_MS*/, size_t threadStackSize /*= CONFIG_CMF_ASYNCENTITY_STACK_SIZE*/,
	uint8_t threadPriority /*= CONFIG_CMF_ASYNCENTITY_THREAD_PRIORITY*/, int8_t cpuCore /*= CONFIG_CMF_ASYNCENTITY_CPU_CORE*/, bool internalStack /*= true*/) noexcept :
						Super(),
						threadStackSize(threadStackSize),
						threadPriority(threadPriority),
						cpuCore(cpuCore),
						internalStack(internalStack),
						lastTickTime(micros()) {
	setEventScanningTime(interval);
}

AsyncEntity::~AsyncEntity() noexcept {
	thread->stop();
}

void AsyncEntity::setOwner(Object* object) noexcept{
	// This is on purpose, async entities should not have an owner to try to prevent accidental circular ownership and issues with event scanning
	Super::setOwner(nullptr);

	if(object != nullptr && hasBegun()){
		CMF_LOG(CMF, Warning, "Attempt to set owner '%s' of async entity '%s'. Async entities aren't allowed to have an owner.", getOwner()->getName().c_str(), getName().c_str());
	}
}

TickType_t AsyncEntity::getEventScanningTime() const noexcept {
	return eventScanningTime;
}

void AsyncEntity::setEventScanningTime(TickType_t value) noexcept {
	eventScanningTime = value;
}

void AsyncEntity::__postInitProperties() noexcept {
	Super::__postInitProperties();

	thread = std::make_unique<Threaded>([this]() { this->tickHandle();}, getName().append("_Thread"), 0, threadStackSize, threadPriority, cpuCore, internalStack);
	thread->start();
}

void AsyncEntity::__begin() noexcept{
	Super::__begin();
}

void AsyncEntity::__tick(float deltaTime) noexcept{
	Super::__tick(deltaTime);
}

void AsyncEntity::tickHandle() noexcept{
	if(!isValid(this)){
		return;
	}

	if(!hasBegun()){
		begin();
		__begin();
	}

	forEachChild([](Object* child) {
		if(!isValid(child)){
			return false;
		}

		if(SyncEntity* entity = cast<SyncEntity>(child)){
			if(entity->hasBegun()) {
				return false;
			}

			entity->begin();
			entity->__begin();
		}

		return false;
	});

	const uint64_t currentTickTime = micros();
	const float deltaTime = (currentTickTime - lastTickTime) / 1000000.0f;
	const TickType_t eventScanningTime = getEventScanningTime();

	scanEvents(eventScanningTime + (eventScanningTime < portMAX_DELAY ? 1 : 0));
	tick(deltaTime);
	__tick(deltaTime);

	forEachChild([deltaTime](Object* child) {
		if(!isValid(child)){
			return false;
		}

		if(SyncEntity* entity = cast<SyncEntity>(child)){
			entity->tick(deltaTime);
			entity->__tick(deltaTime);
		}

		return false;
	});

	lastTickTime = currentTickTime;
}
