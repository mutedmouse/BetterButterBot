#include "Entity.h"
#include "Memory/ObjectMemory.h"

Entity::Entity() noexcept : Super() {}

Entity::~Entity() noexcept = default;

bool Entity::hasBegun() const noexcept{
	return begun;
}

void Entity::begin() noexcept{}

void Entity::__begin() noexcept {
	begun = true;
}

void Entity::tick(float deltaTime) noexcept{}

void Entity::__tick(float deltaTime) noexcept {
}