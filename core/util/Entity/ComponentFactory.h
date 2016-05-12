#pragma once
#include "Entity/Entity.h"

namespace ff
{
	class ComponentFactory;
	struct Component;

	typedef std::shared_ptr<ComponentFactory> (*CreateComponentFactoryFunc)();

	// Creates and deletes components of a certain type for any entity
	class ComponentFactory
	{
	public:
		template<typename T>
		static std::shared_ptr<ComponentFactory> Create();

		UTIL_API ~ComponentFactory();

		ff::Component &New(Entity entity, bool *usedExisting = nullptr);
		ff::Component *Clone(Entity entity, Entity sourceEntity);
		ff::Component *Lookup(Entity entity) const;
		bool Delete(Entity entity);

		void *CastToVoid(Component *component) const;

		template<typename T> T &New(Entity entity, bool *usedExisting = nullptr);
		template<typename T> T *Clone(Entity entity, Entity sourceEntity);
		template<typename T> T *Lookup(Entity entity) const;

	private:
		UTIL_API ComponentFactory(
			size_t dataSize,
			std::function<void(Component &)> &&constructor,
			std::function<void(Component &, const Component &)> &&copyConstructor,
			std::function<void(Component &)> &&destructor,
			std::function<Component *(void *)> &&castToBase,
			std::function<void *(Component *)> &&castFromBase);

		UTIL_API ComponentFactory(
			size_t dataSize,
			std::function<Component *(void *)> &&castToBase,
			std::function<void *(Component *)> &&castFromBase);

		size_t LookupIndex(Entity entity) const;
		Component &LookupByIndex(size_t index) const;
		size_t AllocateIndex();

		size_t _dataSize;
		std::function<void(Component &)> _constructor;
		std::function<void(Component &, const Component &)> _copyConstructor;
		std::function<void(Component &)> _destructor;
		std::function<Component *(void *)> _castToBase;
		std::function<void *(Component *)> _castFromBase;

		static const size_t COMPONENT_BUCKET_SIZE = 256;
		typedef Vector<BYTE, 0, AlignedByteAllocator<16>> ComponentBucket;

		Vector<std::unique_ptr<ComponentBucket>, 8> _components;
		Vector<size_t> _freeComponents;
		Map<Entity, size_t> _usedComponents;
	};
}

template<typename T>
std::shared_ptr<ff::ComponentFactory> ff::ComponentFactory::Create()
{
	assert(16 % __alignof(T) == 0);

	if (std::is_pod<T>::value)
	{
		return std::shared_ptr<ComponentFactory>(
			new ComponentFactory(
				sizeof(T),
				// castToBase
				[](void *component)
				{
					return static_cast<Component *>((T *)component);
				},
				// castFromBase
				[](Component *component)
				{
					return static_cast<T *>(component);
				}));
	}

	return std::shared_ptr<ComponentFactory>(
		new ComponentFactory(
			sizeof(T),
			// T::T constructor
			[](Component &component)
			{
				T *myComponent = static_cast<T *>(&component);
				::new(myComponent) T();
			},
			// T::T(T) constructor
			[](Component &component, const Component &sourceComponent)
			{
				T *myComponent = static_cast<T *>(&component);
				const T *mySourceComponent = static_cast<const T *>(&sourceComponent);
				::new(myComponent) T(*mySourceComponent);
			},
			// T::~T destructor
			[](Component &component)
			{
				T *myComponent = static_cast<T *>(&component);
				myComponent->~T();
			},
			// castToBase
			[](void *component)
			{
				return static_cast<Component *>((T *)component);
			},
			// castFromBase
			[](Component *component)
			{
				return static_cast<T *>(component);
			}));
}

template<typename T>
T &ff::ComponentFactory::New(Entity entity, bool *usedExisting)
{
	return static_cast<T &>(New(entity, usedExisting));
}

template<typename T>
T *ff::ComponentFactory::Clone(Entity entity, Entity sourceEntity)
{
	return static_cast<T *>(Clone(entity, sourceEntity));
}

template<typename T>
T *ff::ComponentFactory::Lookup(Entity entity) const
{
	return static_cast<T *>(Lookup(entity));
}
