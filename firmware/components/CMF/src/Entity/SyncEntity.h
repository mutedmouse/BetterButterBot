#ifndef CMF_SYNCENTITY_H
#define CMF_SYNCENTITY_H

#include "Entity.h"
#include "Object/Class.h"
#include "Memory/SmartPtr/WeakObjectPtr.h"

class AsyncEntity;

/**
 * @brief Sync entity is an Entity implementation with an owner that controls its lifetime while alive, that ticks synchronously with the owner.
 */
class SyncEntity : public Entity {
	GENERATED_BODY(SyncEntity, Entity, void)

public:
	/**
	 * @brief Creates a sync entity.
	 */
	explicit SyncEntity() noexcept;

	/**
	 * @brief Default empty destructor.
	 */
	virtual ~SyncEntity() noexcept override;

	/**
	 * @brief Controls the lifetime functionality of all synch entity children of this entity.
	 * @param deltaTime Time since the last tick call.
	 */
	virtual void __tick(float deltaTime) noexcept override final;

	virtual void tick(float deltaTime) noexcept override;

	/**
	 * @brief Also calls the begin of all children entities.
	 */
	virtual void __begin() noexcept override final;

	virtual void begin() noexcept override;

protected:
	/**
	 * @brief Ensures that the sync entity has an owner. Even if invalid owner is set, the owner of the entity will be set to the application instance.
	 * @param oldOwner 
	 */
	virtual void onOwnerChanged(Object* oldOwner) noexcept override;

private:
	virtual void __postInitProperties() noexcept override final;

private:
	// TODO remove or change to Object
	WeakObjectPtr<AsyncEntity> ownerEntity;
};

#endif //CMF_SYNCENTITY_H