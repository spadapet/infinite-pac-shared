#include "pch.h"
#include "Entity/ComponentFactory.h"

ff::ComponentFactory::ComponentFactory(
	size_t dataSize,
	std::function<void(Component &)> &&constructor,
	std::function<void(Component &, const Component &)> &&copyConstructor,
	std::function<void(Component &)> &&destructor,
	std::function<Component *(void *)> &&castToBase,
	std::function<void *(Component *)> &&castFromBase)
	: _dataSize(dataSize)
	, _constructor(std::move(constructor))
	, _copyConstructor(std::move(copyConstructor))
	, _destructor(std::move(destructor))
	, _castToBase(std::move(castToBase))
	, _castFromBase(std::move(castFromBase))
{
}

ff::ComponentFactory::ComponentFactory(
	size_t dataSize,
	std::function<Component *(void *)> &&castToBase,
	std::function<void *(Component *)> &&castFromBase)
	: _dataSize(dataSize)
	, _constructor(nullptr)
	, _copyConstructor(nullptr)
	, _destructor(nullptr)
	, _castToBase(std::move(castToBase))
	, _castFromBase(std::move(castFromBase))
{
}

ff::ComponentFactory::~ComponentFactory()
{
	assert(_usedComponents.IsEmpty());

	if (_destructor != nullptr)
	{
		// just in case the component list isn't empty, clean them up
		for (auto &entry: _usedComponents)
		{
			Component &component = LookupByIndex(entry.GetValue());
			_destructor(component);
		}
	}
}

ff::Component &ff::ComponentFactory::New(Entity entity, bool *usedExisting)
{
	Component *component = Lookup(entity);

	if (usedExisting != nullptr)
	{
		*usedExisting = (component != nullptr);
	}

	if (component == nullptr)
	{
		size_t index = AllocateIndex();
		component = &LookupByIndex(index);
		_usedComponents.SetKey(entity, index);

		if (_constructor != nullptr)
		{
			_constructor(*component);
		}
	}

	assert(component != nullptr);
	return *component;
}

ff::Component *ff::ComponentFactory::Clone(Entity entity, Entity sourceEntity)
{
	Component *component = Lookup(entity);
	Component *sourceComponent = Lookup(sourceEntity);
	assert(component == nullptr);

	if (component == nullptr && sourceComponent != nullptr)
	{
		size_t index = AllocateIndex();
		component = &LookupByIndex(index);
		_usedComponents.SetKey(entity, index);

		if (_copyConstructor != nullptr)
		{
			_copyConstructor(*component, *sourceComponent);
		}
		else
		{
			std::memcpy(component, sourceComponent, _dataSize);
		}
	}

	return component;
}

ff::Component *ff::ComponentFactory::Lookup(Entity entity) const
{
	size_t index = LookupIndex(entity);
	return index != INVALID_SIZE ? &LookupByIndex(index) : nullptr;
}

bool ff::ComponentFactory::Delete(Entity entity)
{
	BucketIter iter = _usedComponents.Get(entity);
	if (iter != INVALID_ITER)
	{
		size_t index = _usedComponents.ValueAt(iter);
		if (_destructor != nullptr)
		{
			Component &component = LookupByIndex(index);
			_destructor(component);
		}

		_freeComponents.Push(index);
		_usedComponents.DeletePos(iter);

		return true;
	}

	return false;
}

void *ff::ComponentFactory::CastToVoid(Component *component) const
{
	return _castFromBase(component);
}

size_t ff::ComponentFactory::LookupIndex(Entity entity) const
{
	auto iter = _usedComponents.Get(entity);
	return iter != INVALID_ITER ? _usedComponents.ValueAt(iter) : INVALID_SIZE;
}

ff::Component &ff::ComponentFactory::LookupByIndex(size_t index) const
{
	size_t bucket = index / COMPONENT_BUCKET_SIZE;
	size_t offset = index % COMPONENT_BUCKET_SIZE;
	BYTE *bytes = &_components[bucket]->GetAt(offset * _dataSize);

	return *(Component *)bytes;
}

size_t ff::ComponentFactory::AllocateIndex()
{
	size_t index;
	if (_freeComponents.IsEmpty())
	{
		ComponentBucket *lastBucket = !_components.IsEmpty() ? _components.GetLast().get() : nullptr;
		if (lastBucket == nullptr || lastBucket->Size() == lastBucket->Allocated())
		{
			_components.Push(std::make_unique<ComponentBucket>());
			lastBucket = _components.GetLast().get();
			lastBucket->Reserve(COMPONENT_BUCKET_SIZE * _dataSize);
		}

		index = (_components.Size() - 1) * COMPONENT_BUCKET_SIZE + lastBucket->Size() / _dataSize;
		lastBucket->InsertDefault(lastBucket->Size(), _dataSize);
	}
	else
	{
		index = _freeComponents.Pop();
	}

	return index;
}
