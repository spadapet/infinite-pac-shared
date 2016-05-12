#include "pch.h"
#include "Entity/EntitySystem.h"

ff::EntitySystemBase::EntitySystemBase(EntityDomain *domain, const ComponentTypeEntry *componentTypes, size_t componentCount)
	: _domain(domain)
	, _components(componentTypes)
	, _componentCount(componentCount)
{
	assert(_domain != nullptr);
}

ff::EntitySystemBase::~EntitySystemBase()
{
}

ff::EntityDomain *ff::EntitySystemBase::GetDomain() const
{
	return _domain;
}

const ff::ComponentTypeEntry *ff::EntitySystemBase::GetComponentTypes(size_t &count) const
{
	count = _componentCount;
	return _components;
}
