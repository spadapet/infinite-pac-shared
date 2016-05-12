#include "pch.h"
#include "COM/ServiceCollection.h"
#include "Entity/Component.h"
#include "Entity/EntityDomain.h"
#include "Entity/EntityEvents.h"
#include "Entity/EntitySystem.h"
#include "Globals/AppGlobals.h"

bool ff::EntityDomain::EventHandler::operator==(const EventHandler &rhs) const
{
	return _event == rhs._event && _handler == rhs._handler;
}

// static
ff::EntityDomain::EntityEntry *ff::EntityDomain::EntityEntry::FromEntity(Entity entity)
{
	return static_cast<EntityEntry *>(entity);
}

ff::Entity ff::EntityDomain::EntityEntry::ToEntity()
{
	return static_cast<Entity>(this);
}

ff::EntityDomain::EntityDomain()
{
	verify(ff::CreateServiceCollection(&_services));
}

ff::EntityDomain::~EntityDomain()
{
	for (EntityEntry &entityEntry: _entities)
	{
		if (entityEntry._valid)
		{
			DeleteEntity(entityEntry.ToEntity());
		}
	}

	for (SystemEntry &systemEntry: _systems)
	{
		ClearSystem(systemEntry);
	}

	FlushDeletedEntities();
	FlushDeletedSystems();
}

// static
ff::EntityDomain *ff::EntityDomain::FromEntity(Entity entity)
{
	assertRetVal(entity != INVALID_ENTITY, nullptr);
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	return entityEntry->_domain;
}

ff::State::Status ff::EntityDomain::GetStatus()
{
	return _systems.IsEmpty()
		? ff::State::Status::Dead
		: ff::State::Status::Alive;
}

std::shared_ptr<ff::State> ff::EntityDomain::Advance(ff::AppGlobals *globals)
{
	for (SystemEntry &systemEntry: _systems)
	{
		if (systemEntry._system != nullptr)
		{
			systemEntry._system->Advance(globals);
		}
	}

	FlushDeletedEntities();
	FlushDeletedSystems();

	return nullptr;
}

void ff::EntityDomain::Render(AppGlobals *globals, IRenderTarget *target)
{
	for (SystemEntry &systemEntry: _systems)
	{
		if (systemEntry._system != nullptr)
		{
			systemEntry._system->Render(globals, target);
		}
	}
}

void ff::EntityDomain::SaveState(AppGlobals *globals)
{
	for (SystemEntry &systemEntry: _systems)
	{
		if (systemEntry._system != nullptr)
		{
			systemEntry._system->SaveState(globals);
		}
	}
}

void ff::EntityDomain::LoadState(AppGlobals *globals)
{
	for (SystemEntry &systemEntry: _systems)
	{
		if (systemEntry._system != nullptr)
		{
			systemEntry._system->LoadState(globals);
		}
	}
}

ff::Entity ff::EntityDomain::CreateEntity(StringRef name)
{
	EntityEntry &entityEntry = _entities.Insert();
	entityEntry._domain = this;
	entityEntry._name = name;
	entityEntry._componentBits = 0;
	entityEntry._valid = true;
	entityEntry._active = false;

	return entityEntry.ToEntity();
}

ff::Entity ff::EntityDomain::CloneEntity(Entity sourceEntity, StringRef name)
{
	EntityEntry *sourceEntityEntry = EntityEntry::FromEntity(sourceEntity);
	assertRetVal(sourceEntityEntry->_valid, INVALID_ENTITY);

	Entity newEntity = CreateEntity(name);
	EntityEntry *newEntityEntry = EntityEntry::FromEntity(newEntity);

	for (auto factoryEntry: sourceEntityEntry->_components)
	{
		factoryEntry->_factory->Clone(newEntity, sourceEntity);
	}

	newEntityEntry->_componentBits = sourceEntityEntry->_componentBits;
	newEntityEntry->_components = sourceEntityEntry->_components;
	newEntityEntry->_systems = sourceEntityEntry->_systems;

	return newEntity;
}

ff::Entity ff::EntityDomain::GetEntity(StringRef name) const
{
	auto i = _entitiesByName.Get(name);
	return i != INVALID_ITER ? _entitiesByName.ValueAt(i) : INVALID_ENTITY;
}

ff::StringRef ff::EntityDomain::GetEntityName(Entity entity) const
{
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	return entityEntry->_name;
}

// Adds a new or deactivated entity to the systems that need to know about it
void ff::EntityDomain::ActivateEntity(Entity entity)
{
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	assertRet(entityEntry->_valid);

	if (!entityEntry->_active)
	{
		entityEntry->_active = true;
		RegisterActivatedEntity(entityEntry);
	}
}

// Keeps the entity in memory, but it will be removed from any systems that know about it
void ff::EntityDomain::DeactivateEntity(Entity entity)
{
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	assertRet(entityEntry->_valid);

	if (entityEntry->_active)
	{
		entityEntry->_active = false;
		UnregisterDeactivatedEntity(entityEntry);
	}
}

// Deletes an entity from memory and all systems that know about it
void ff::EntityDomain::DeleteEntity(Entity entity)
{
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	assertRet(entityEntry->_valid);

	DeactivateEntity(entity);

	entityEntry->_valid = false;
	_deletedEntities.Push(entityEntry);

	// Wait until frame cleanup to reclaim the entity memory
}

ff::EntityDomain::ComponentFactoryEntry *ff::EntityDomain::AddComponentFactory(
	std::type_index componentType, CreateComponentFactoryFunc factoryFunc)
{
	BucketIter iter = _componentFactories.Get(componentType);
	if (iter == INVALID_ITER)
	{
		std::shared_ptr<ComponentFactory> factory = factoryFunc();
		assertRetVal(factory != nullptr, nullptr);

		ComponentFactoryEntry factoryEntry;
		factoryEntry._factory = factory;
		factoryEntry._bitIndex = _componentFactories.Size();
		factoryEntry._bit = (uint64_t)1 << (factoryEntry._bitIndex % 64);

		iter = _componentFactories.SetKey(std::move(componentType), std::move(factoryEntry));
	}

	return &_componentFactories.ValueAt(iter);
}

ff::EntityDomain::ComponentFactoryEntry *ff::EntityDomain::GetComponentFactory(std::type_index componentType)
{
	BucketIter i = _componentFactories.Get(componentType);
	return (i != INVALID_ITER) ? &_componentFactories.ValueAt(i) : nullptr;
}

ff::Component *ff::EntityDomain::AddComponent(Entity entity, ComponentFactoryEntry *factoryEntry)
{
	assertRetVal(factoryEntry, nullptr);

	bool usedExisting = false;
	Component *component = &factoryEntry->_factory->New(entity, &usedExisting);

	if (!usedExisting)
	{
		EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
		assert(entityEntry->_components.Find(factoryEntry) == INVALID_SIZE);

		entityEntry->_componentBits |= factoryEntry->_bit;
		entityEntry->_components.Push(factoryEntry);

		RegisterEntityWithSystems(entityEntry, factoryEntry);
	}

	return component;
}

ff::Component *ff::EntityDomain::CloneComponent(Entity entity, Entity sourceEntity, ComponentFactoryEntry *factoryEntry)
{
	assertRetVal(factoryEntry, nullptr);
	Component *component = LookupComponent(entity, factoryEntry);
	assertRetVal(component == nullptr, component);

	component = factoryEntry->_factory->Clone(entity, sourceEntity);
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
	assert(entityEntry->_components.Find(factoryEntry) == INVALID_SIZE);

	entityEntry->_componentBits |= factoryEntry->_bit;
	entityEntry->_components.Push(factoryEntry);

	RegisterEntityWithSystems(entityEntry, factoryEntry);

	return component;
}

ff::Component *ff::EntityDomain::LookupComponent(Entity entity, ComponentFactoryEntry *factoryEntry)
{
	assertRetVal(factoryEntry, nullptr);
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);

	return ((entityEntry->_componentBits & factoryEntry->_bit) != 0)
		? factoryEntry->_factory->Lookup(entity)
		: nullptr;
}

bool ff::EntityDomain::DeleteComponent(Entity entity, ComponentFactoryEntry *factoryEntry)
{
	assertRetVal(factoryEntry, false);
	EntityEntry *entityEntry = EntityEntry::FromEntity(entity);

	size_t i = (entityEntry->_componentBits & factoryEntry->_bit) != 0
		? entityEntry->_components.Find(factoryEntry)
		: INVALID_SIZE;

	if (i != INVALID_SIZE)
	{
		UnregisterEntityWithSystems(entityEntry, factoryEntry);
		verify(factoryEntry->_factory->Delete(entity));

		entityEntry->_components.Delete(i);
		entityEntry->_componentBits = 0;

		for (ComponentFactoryEntry *factoryEntry: entityEntry->_components)
		{
			entityEntry->_componentBits |= factoryEntry->_bit;
		}

		return true;
	}

	return false;
}

ff::EntityDomain::ComponentFactoryEntries ff::EntityDomain::FindComponentEntries(
	const ComponentTypeEntry *componentTypes, size_t count)
{
	ComponentFactoryEntries entries;
	entries.Reserve(count);

	for (const ComponentTypeEntry *componentType = componentTypes;
		componentType != componentTypes + count;
		componentType++)
	{
		ComponentFactoryEntry *factoryEntry = AddComponentFactory(componentType->_type, componentType->_factory);
		assertRetVal(factoryEntry != nullptr, ComponentFactoryEntries());
		entries.Push(factoryEntry);
	}

	return entries;
}

bool ff::EntityDomain::AddSystem(std::shared_ptr<EntitySystemBase> system)
{
	assertRetVal(system != nullptr && system->GetDomain() == this, false);

	SystemEntry &systemEntry = _systems.Insert();
	SetSystem(systemEntry, system);

	return true;
}

void ff::EntityDomain::SetSystem(SystemEntry &systemEntry, std::shared_ptr<EntitySystemBase> system)
{
	size_t componentCount = 0;
	const ComponentTypeEntry *componentTypes = system->GetComponentTypes(componentCount);

	systemEntry._system = system;
	systemEntry._components = FindComponentEntries(componentTypes, componentCount);
	systemEntry._componentBits = 0;

	for (ComponentFactoryEntry *factoryEntry: systemEntry._components)
	{
		systemEntry._componentBits |= factoryEntry->_bit;
		factoryEntry->_systems.Push(&systemEntry);
	}

	for (EntityEntry &entityEntry: _entities)
	{
		TryRegisterEntityWithSystem(&entityEntry, &systemEntry);
	}
}

void ff::EntityDomain::ClearSystem(SystemEntry &systemEntry)
{
	for (auto &i: systemEntry._entities)
	{
		systemEntry._system->DeleteEntry(i.GetValue());
	}

	for (ComponentFactoryEntry *componentEntry: systemEntry._components)
	{
		verify(componentEntry->_systems.DeleteItem(&systemEntry));
	}

	for (EntityEntry &entityEntry: _entities)
	{
		entityEntry._systems.DeleteItem(&systemEntry);
	}

	systemEntry._system.reset();
	systemEntry._entities.Clear();
	systemEntry._components.Clear();
	systemEntry._componentBits = 0;
}

void ff::EntityDomain::FlushDeletedSystems()
{
	for (SystemEntry *systemEntry = _systems.GetFirst(); systemEntry != nullptr; )
	{
		if (systemEntry->_system == nullptr || systemEntry->_system->GetStatus() == ff::State::Status::Dead)
		{
			systemEntry = _systems.Delete(*systemEntry);
		}
		else
		{
			systemEntry = _systems.GetNext(*systemEntry);
		}
	}
}

void ff::EntityDomain::FlushDeletedEntities()
{
	for (EntityEntry *entityEntry: _deletedEntities)
	{
		assert(!entityEntry->_valid);

		for (auto factoryEntry: entityEntry->_components)
		{
			factoryEntry->_factory->Delete(entityEntry->ToEntity());
		}

		_entities.Delete(*entityEntry);
	}

	_deletedEntities.Clear();
}

void ff::EntityDomain::RegisterActivatedEntity(EntityEntry *entityEntry)
{
	assert(entityEntry->_valid && entityEntry->_active);

	if (!entityEntry->_name.empty())
	{
		_entitiesByName.Insert(entityEntry->_name, entityEntry);
	}

	for (SystemEntry *systemEntry: entityEntry->_systems)
	{
		RegisterEntityWithSystem(entityEntry, systemEntry);
	}
}

void ff::EntityDomain::UnregisterDeactivatedEntity(EntityEntry *entityEntry)
{
	assert(entityEntry->_valid && !entityEntry->_active);

	if (!entityEntry->_name.empty())
	{
		bool deleted = false;

		for (BucketIter i = _entitiesByName.Get(entityEntry->_name); i != INVALID_ITER; i = _entitiesByName.GetNext(i))
		{
			if (_entitiesByName.ValueAt(i) == entityEntry)
			{
				_entitiesByName.DeletePos(i);
				deleted = true;
				break;
			}
		}

		assert(deleted);
	}

	for (SystemEntry *systemEntry: entityEntry->_systems)
	{
		UnregisterEntityWithSystem(entityEntry, systemEntry);
	}
}

bool ff::EntityDomain::TryRegisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry)
{
	bool allFound = true;

	if ((systemEntry->_componentBits & entityEntry->_componentBits) == systemEntry->_componentBits &&
		!systemEntry->_components.IsEmpty())
	{
		for (ComponentFactoryEntry *factoryEntry: systemEntry->_components)
		{
			if ((entityEntry->_componentBits & factoryEntry->_bit) == 0 ||
				entityEntry->_components.Find(factoryEntry) == INVALID_SIZE)
			{
				allFound = false;
				break;
			}
		}

		if (allFound)
		{
			entityEntry->_systems.Push(systemEntry);

			if (entityEntry->_active)
			{
				RegisterEntityWithSystem(entityEntry, systemEntry);
			}
		}
	}

	return allFound;
}

void ff::EntityDomain::RegisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry)
{
	assert(entityEntry->_valid && entityEntry->_active);

	struct FakeEntry : public EntitySystemEntry
	{
		void *_components[1];
	};

	EntitySystemEntry *systemEntityEntry = systemEntry->_system->NewEntry();
	systemEntityEntry->_entity = entityEntry->ToEntity();

	void **derivedComponents = static_cast<FakeEntry *>(systemEntityEntry)->_components;
	for (ComponentFactoryEntry *factoryEntry: systemEntry->_components)
	{
		Component *component = LookupComponent(systemEntityEntry->_entity, factoryEntry);
		assert(component != nullptr);

		*derivedComponents = factoryEntry->_factory->CastToVoid(component);
		derivedComponents++;
	}

	systemEntry->_entities.SetKey(systemEntityEntry->_entity, systemEntityEntry);
}

void ff::EntityDomain::UnregisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry)
{
	BucketIter iter = systemEntry->_entities.Get(entityEntry->ToEntity());
	assertRet(iter != INVALID_ITER && entityEntry->_valid && !entityEntry->_active);

	systemEntry->_system->DeleteEntry(systemEntry->_entities.ValueAt(iter));
	systemEntry->_entities.DeletePos(iter);
}

void ff::EntityDomain::RegisterEntityWithSystems(EntityEntry *entityEntry, ComponentFactoryEntry *factoryEntry)
{
	for (SystemEntry *systemEntry: factoryEntry->_systems)
	{
		TryRegisterEntityWithSystem(entityEntry, systemEntry);
	}
}

void ff::EntityDomain::UnregisterEntityWithSystems(EntityEntry *entityEntry, ComponentFactoryEntry *factoryEntry)
{
	for (SystemEntry *systemEntry: factoryEntry->_systems)
	{
		if ((systemEntry->_componentBits & entityEntry->_componentBits) == systemEntry->_componentBits)
		{
			size_t i = entityEntry->_systems.Find(systemEntry);
			assert(i != INVALID_SIZE);

			if (i != INVALID_SIZE)
			{
				UnregisterEntityWithSystem(entityEntry, systemEntry);
				entityEntry->_systems.Delete(i);
			}
		}
	}
}

bool ff::EntityDomain::AddEventHandler(hash_t eventId, IEntityEventHandler *handler)
{
	return AddEventHandler(GetEventEntry(eventId), INVALID_ENTITY, handler);
}

bool ff::EntityDomain::RemoveEventHandler(hash_t eventId, IEntityEventHandler *handler)
{
	return RemoveEventHandler(GetEventEntry(eventId), INVALID_ENTITY, handler);
}

bool ff::EntityDomain::AddEventHandler(hash_t eventId, Entity entity, IEntityEventHandler *handler)
{
	return AddEventHandler(GetEventEntry(eventId), entity, handler);
}

bool ff::EntityDomain::RemoveEventHandler(hash_t eventId, Entity entity, IEntityEventHandler *handler)
{
	return RemoveEventHandler(GetEventEntry(eventId), entity, handler);
}

bool ff::EntityDomain::AddEventHandler(EventHandlerEntry *eventEntry, Entity entity, IEntityEventHandler *handler)
{
	assertRetVal(eventEntry && handler, false);

	EventHandlersPtr &eventHandlers = (entity != INVALID_ENTITY)
		? EntityEntry::FromEntity(entity)->_eventHandlers
		: eventEntry->_eventHandlers;
	ff::MakeUnshared(eventHandlers);

	EventHandler eventHandler;
	eventHandler._event = eventEntry;
	eventHandler._handler = handler;

	eventHandlers->Push(eventHandler);

	return true;
}

bool ff::EntityDomain::RemoveEventHandler(EventHandlerEntry *eventEntry, Entity entity, IEntityEventHandler *handler)
{
	assertRetVal(eventEntry && handler, false);

	EventHandlersPtr &eventHandlers = (entity != INVALID_ENTITY)
		? EntityEntry::FromEntity(entity)->_eventHandlers
		: eventEntry->_eventHandlers;

	if (eventHandlers != nullptr && !eventHandlers->IsEmpty())
	{
		ff::MakeUnshared(eventHandlers);

		EventHandler eventHandler;
		eventHandler._event = eventEntry;
		eventHandler._handler = handler;

		assertRetVal(eventHandlers->DeleteItem(eventHandler), false);

		if (eventHandlers->IsEmpty())
		{
			eventHandlers = nullptr;
		}

		return true;
	}

	return false;
}

ff::EntityDomain::EventHandlerEntry *ff::EntityDomain::GetEventEntry(hash_t eventId)
{
	assertRetVal(eventId, nullptr);

	BucketIter iter = _events.Get(eventId);
	if (iter == INVALID_ITER)
	{
		EventHandlerEntry eventEntry;
		eventEntry._eventId = eventId;

		iter = _events.SetKey(eventId, eventEntry);
	}

	return (iter != INVALID_ITER) ? &_events.ValueAt(iter) : nullptr;
}

bool ff::EntityDomain::TriggerEvent(hash_t eventId, Entity entity)
{
	return TriggerEvent(GetEventEntry(eventId), entity, nullptr);
}

bool ff::EntityDomain::TriggerEvent(hash_t eventId)
{
	return TriggerEvent(GetEventEntry(eventId), INVALID_ENTITY, nullptr);
}

bool ff::EntityDomain::TriggerEvent(EventHandlerEntry *eventEntry, Entity entity, void *args)
{
	assertRetVal(eventEntry, false);

	// Call listeners for the specific entity first
	if (entity != INVALID_ENTITY)
	{
		EntityEntry *entityEntry = EntityEntry::FromEntity(entity);
		EventHandlersPtr eventHandlers = entityEntry->_eventHandlers;
		if (eventHandlers != nullptr && !eventHandlers->IsEmpty())
		{
			for (const EventHandler &eventHandler: *eventHandlers)
			{
				if (eventHandler._event == eventEntry)
				{
					eventHandler._handler->OnEntityEvent(this, entity, eventEntry->_eventId, args);
				}
			}
		}
	}

	// Call global listeners too
	{
		EventHandlersPtr eventHandlers = eventEntry->_eventHandlers;
		if (eventHandlers != nullptr && !eventHandlers->IsEmpty())
		{
			for (const EventHandler &eventHandler: *eventHandlers)
			{
				assert(eventHandler._event == eventEntry);
				eventHandler._handler->OnEntityEvent(this, entity, eventEntry->_eventId, args);
			}
		}
	}

	return true;
}

ff::IServiceCollection *ff::EntityDomain::GetServices()
{
	return _services;
}

void ff::EntityDomain::Dump(Log &log)
{
}
