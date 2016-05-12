#pragma once
#include "Entity/ComponentFactory.h"
#include "Entity/Entity.h"
#include "State/State.h"

namespace ff
{
	class EntityDomain;

	struct EntitySystemEntry
	{
		// Derived classes must add a list of component pointers of the correct type,
		// not Component, but the actual type of the component.
		// After the components, it's OK to add custom state variables.

		Entity _entity;
	};

	struct ComponentTypeEntry
	{
		std::type_index _type;
		CreateComponentFactoryFunc _factory;
	};

	// Processes all entities that contain the specified types of components
	class EntitySystemBase : public State
	{
	public:
		UTIL_API EntitySystemBase(EntityDomain *domain, const ComponentTypeEntry *componentTypes, size_t componentCount);
		UTIL_API virtual ~EntitySystemBase();

		UTIL_API EntityDomain *GetDomain() const;
		const ComponentTypeEntry *GetComponentTypes(size_t &count) const;
		template<typename T> bool GetService(T **obj) const;

		virtual EntitySystemEntry *NewEntry() = 0;
		virtual void DeleteEntry(EntitySystemEntry *entry) = 0;

	private:
		EntityDomain *_domain;
		const ComponentTypeEntry *_components;
		size_t _componentCount;
	};

	// All of your entity systems must derive from this class.
	// EntryT must derive from EntitySystemEntry
	template<typename EntryT>
	class EntitySystem : public EntitySystemBase
	{
	public:
		EntitySystem(EntityDomain *domain);
		virtual ~EntitySystem();

		virtual EntitySystemEntry *NewEntry() final;
		virtual void DeleteEntry(EntitySystemEntry *entry) final;

	protected:
		const List<EntryT> &GetEntries() const;
		size_t GetEntryChangeStamp() const;

	private:
		List<EntryT> _entries;
		size_t _changeStamp;
	};
}

template<typename T>
bool ff::EntitySystemBase::GetService(T **obj) const
{
	return SUCCEEDED(GetDomain()->GetServices()->QueryService(__uuidof(T), __uuidof(T), obj));
}

template<typename EntryT>
ff::EntitySystem<EntryT>::EntitySystem(EntityDomain *domain)
	: ff::EntitySystemBase(domain, EntryT::GetComponentTypes(), EntryT::GetComponentCount())
{
#ifdef _DEBUG
	// Make sure there's enough memory in EntryT to store all the components
	size_t entrySize = sizeof(EntitySystemEntry) + EntryT::GetComponentCount() * sizeof(void *);
	assert(entrySize <= sizeof(EntryT));
#endif
}

template<typename EntryT>
ff::EntitySystem<EntryT>::~EntitySystem()
{
	assert(_entries.Size() == 0);
}

template<typename EntryT>
ff::EntitySystemEntry *ff::EntitySystem<EntryT>::NewEntry()
{
	_changeStamp++;
	return &_entries.Insert();
}

template<typename EntryT>
void ff::EntitySystem<EntryT>::DeleteEntry(EntitySystemEntry *entry)
{
	_changeStamp++;
	_entries.Delete(*static_cast<EntryT *>(entry));
}

template<typename EntryT>
const ff::List<EntryT> &ff::EntitySystem<EntryT>::GetEntries() const
{
	return _entries;
}

template<typename EntryT>
size_t ff::EntitySystem<EntryT>::GetEntryChangeStamp() const
{
	return _changeStamp;
}

// Helper macros

#define DECLARE_SYSTEM_COMPONENTS() \
	static const ff::ComponentTypeEntry *GetComponentTypes(); \
	static size_t GetComponentCount();

#define DECLARE_SYSTEM_COMPONENTS_EXPORT(apiType) \
	apiType static const ff::ComponentTypeEntry *GetComponentTypes(); \
	apiType static size_t GetComponentCount();

#define BEGIN_SYSTEM_COMPONENTS(className) \
	static const ff::ComponentTypeEntry s_components_##className[] \
	{

#define HAS_COMPONENT(componentClass) \
		{ std::type_index(typeid(componentClass)), &ff::ComponentFactory::Create<componentClass> },

#define END_SYSTEM_COMPONENTS(className) \
	}; \
	const ff::ComponentTypeEntry *className::GetComponentTypes() \
	{ \
		return s_components_##className; \
	} \
	size_t className::GetComponentCount() \
	{ \
		return _countof(s_components_##className); \
	}
