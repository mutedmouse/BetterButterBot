#include "SyncEntity.h"
#include "Statics/ApplicationStatics.h"
#include "Core/Application.h"
#include "Memory/ObjectMemory.h"

SyncEntity::SyncEntity() noexcept : Super() {}

SyncEntity::~SyncEntity() noexcept = default;

void SyncEntity::__tick(float deltaTime) noexcept {
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
}

void SyncEntity::tick(float deltaTime) noexcept {}

void SyncEntity::__begin() noexcept {
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
}

void SyncEntity::begin() noexcept {}

void SyncEntity::onOwnerChanged(Object* oldOwner) noexcept{
	Super::onOwnerChanged(oldOwner);

	// This is intentional, sync entities have to have an owner, otherwise their behavior will not work
	if(getOwner() == nullptr && isValid(ApplicationStatics::getApplication())){
		abort();
		setOwner(ApplicationStatics::getApplication());
	}
}

void SyncEntity::__postInitProperties() noexcept{
	Super::__postInitProperties();
}
