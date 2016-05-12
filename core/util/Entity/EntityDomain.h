#pragma once
#include "Entity/ComponentFactory.h"
#include "State/State.h"

namespace ff
{
	class EntitySystemBase;
	class IEntityEventHandler;
	class IServiceCollection;
	class Log;
	struct ComponentTypeEntry;
	struct EntitySystemEntry;

	// All entities, components, and systems are owned and managed by a domain.
	class EntityDomain : public State
	{
	public:
		UTIL_API EntityDomain();
		UTIL_API ~EntityDomain();

		UTIL_API static EntityDomain *FromEntity(Entity entity);

		// System methods (T must derive from EntitySystem)
		template<typename T> T *AddSystem();

		// Component methods (T must derive from Component)
		template<typename T> T *AddComponent(Entity entity);
		template<typename T> T *CloneComponent(Entity entity, Entity sourceEntity);
		template<typename T> T *LookupComponent(Entity entity);
		template<typename T> bool DeleteComponent(Entity entity);

		// Entity methods (new entities are not activated by default).
		// Only active entities will be registered with systems.
		// Only active entities will have their names registered for lookup (using GetEntity).
		UTIL_API Entity CreateEntity(StringRef name = String());
		UTIL_API Entity CloneEntity(Entity sourceEntity, StringRef name = String());
		UTIL_API Entity GetEntity(StringRef name) const;
		UTIL_API StringRef GetEntityName(Entity entity) const;
		UTIL_API void ActivateEntity(Entity entity);
		UTIL_API void DeactivateEntity(Entity entity);
		UTIL_API void DeleteEntity(Entity entity);

		// Event triggers can be entity-specific or global. They can have arguments or not.
		template<typename ArgsT> bool TriggerEvent(hash_t eventId, Entity entity, ArgsT *args);
		template<typename ArgsT> bool TriggerEvent(hash_t eventId, ArgsT *args);
		UTIL_API bool TriggerEvent(hash_t eventId, Entity entity);
		UTIL_API bool TriggerEvent(hash_t eventId);

		// Event registration can be entity-specific or global.
		UTIL_API bool AddEventHandler(hash_t eventId, IEntityEventHandler *handler);
		UTIL_API bool RemoveEventHandler(hash_t eventId, IEntityEventHandler *handler);
		UTIL_API bool AddEventHandler(hash_t eventId, Entity entity, IEntityEventHandler *handler);
		UTIL_API bool RemoveEventHandler(hash_t eventId, Entity entity, IEntityEventHandler *handler);

		// Domain services
		IServiceCollection *GetServices();

		// State
		virtual ff::State::Status GetStatus() override;
		virtual std::shared_ptr<State> Advance(AppGlobals *globals) override;
		virtual void Render(AppGlobals *globals, IRenderTarget *target) override;
		virtual void SaveState(AppGlobals *globals) override;
		virtual void LoadState(AppGlobals *globals) override;

		UTIL_API void Dump(Log &log);

	private:
		// Data structures

		struct ComponentFactoryEntry;
		struct SystemEntry;
		struct EntityEntry;
		struct EventHandlerEntry;

		struct EventHandler
		{
			bool operator==(const EventHandler &rhs) const;

			EventHandlerEntry *_event;
			IEntityEventHandler *_handler;
		};

		typedef Vector<ComponentFactoryEntry *, 8> ComponentFactoryEntries;
		typedef Vector<SystemEntry *, 8> SystemEntries;
		typedef Vector<EntityEntry *, 8> EntityEntries;
		typedef Vector<EventHandler, 8> EventHandlers;
		typedef std::shared_ptr<EventHandlers> EventHandlersPtr;

		struct ComponentFactoryEntry
		{
			std::shared_ptr<ComponentFactory> _factory;
			uint64_t _bit; // mostly unique bit (not unique when there are more than 64 factories)
			size_t _bitIndex; // unique index used to generate _bit
			SystemEntries _systems; // all systems that care about this type of component
		};

		struct SystemEntry
		{
			std::shared_ptr<EntitySystemBase> _system;
			uint64_t _componentBits;
			ComponentFactoryEntries _components;
			Map<Entity, EntitySystemEntry *> _entities;
		};

		struct EntityEntry
		{
			static EntityEntry *FromEntity(Entity entity);
			Entity ToEntity();

			EntityDomain *_domain;
			String _name;
			uint64_t _componentBits;
			ComponentFactoryEntries _components;
			SystemEntries _systems;
			EventHandlersPtr _eventHandlers;
			bool _valid; // false after this entity has been deleted
			bool _active;
		};

		struct EventHandlerEntry
		{
			hash_t _eventId;
			EventHandlersPtr _eventHandlers;
		};

		// Component methods
		UTIL_API ff::Component *AddComponent(Entity entity, ComponentFactoryEntry *factoryEntry);
		UTIL_API Component *CloneComponent(Entity entity, Entity sourceEntity, ComponentFactoryEntry *factoryEntry);
		UTIL_API Component *LookupComponent(Entity entity, ComponentFactoryEntry *factoryEntry);
		UTIL_API bool DeleteComponent(Entity entity, ComponentFactoryEntry *factoryEntry);
		UTIL_API ComponentFactoryEntry *AddComponentFactory(std::type_index componentType, CreateComponentFactoryFunc factoryFunc);
		UTIL_API ComponentFactoryEntry *GetComponentFactory(std::type_index componentType);
		template<typename T> ComponentFactoryEntry *GetComponentFactory();
		ComponentFactoryEntries FindComponentEntries(const ComponentTypeEntry *componentTypes, size_t count);

		// System methods
		UTIL_API bool AddSystem(std::shared_ptr<EntitySystemBase> system);
		void SetSystem(SystemEntry &systemEntry, std::shared_ptr<EntitySystemBase> system);
		void ClearSystem(SystemEntry &systemEntry);
		void FlushDeletedSystems();

		// Entity methods
		void FlushDeletedEntities();
		void RegisterActivatedEntity(EntityEntry *entityEntry);
		void UnregisterDeactivatedEntity(EntityEntry *entityEntry);
		bool TryRegisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry);
		void RegisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry);
		void UnregisterEntityWithSystem(EntityEntry *entityEntry, SystemEntry *systemEntry);
		void RegisterEntityWithSystems(EntityEntry *entityEntry, ComponentFactoryEntry *newFactory);
		void UnregisterEntityWithSystems(EntityEntry *entityEntry, ComponentFactoryEntry *deletingFactory);

		// Event methods
		UTIL_API bool TriggerEvent(EventHandlerEntry *eventEntry, Entity entity, void *args);
		UTIL_API bool AddEventHandler(EventHandlerEntry *eventEntry, Entity entity, IEntityEventHandler *handler);
		UTIL_API bool RemoveEventHandler(EventHandlerEntry *eventEntry, Entity entity, IEntityEventHandler *handler);
		UTIL_API EventHandlerEntry *GetEventEntry(hash_t eventId);

		// Component data
		Map<std::type_index, ComponentFactoryEntry> _componentFactories;

		// System data
		List<SystemEntry> _systems;

		// Entity data
		List<EntityEntry> _entities;
		Map<String, EntityEntry *> _entitiesByName;
		EntityEntries _deletedEntities;

		// Event data
		Map<hash_t, EventHandlerEntry, NonHasher<hash_t>> _events;

		// Services
		ComPtr<IServiceCollection> _services;
	};
}

template<typename T>
T *ff::EntityDomain::AddComponent(Entity entity)
{
	Component *component = AddComponent(entity, GetComponentFactory<T>());
	return static_cast<T *>(component);
}

template<typename T>
T *ff::EntityDomain::CloneComponent(Entity entity, Entity sourceEntity)
{
	Component *component = CloneComponent(entity, sourceEntity, GetComponentFactory<T>());
	return static_cast<T *>(component);
}

template<typename T>
T *ff::EntityDomain::LookupComponent(Entity entity)
{
	Component *component = LookupComponent(entity, GetComponentFactory<T>());
	return static_cast<T *>(component);
}

template<typename T>
bool ff::EntityDomain::DeleteComponent(Entity entity)
{
	return DeleteComponent(entity, GetComponentFactory<T>());
}

template<typename T>
ff::EntityDomain::ComponentFactoryEntry *ff::EntityDomain::GetComponentFactory()
{
	std::type_index type(typeid(T));
	ComponentFactoryEntry *factory = GetComponentFactory(type);
	if (factory == nullptr)
	{
		factory = AddComponentFactory(type, &ff::ComponentFactory::Create<T>);
	}

	return factory;
}

template<typename T>
T *ff::EntityDomain::AddSystem()
{
	std::shared_ptr<T> system = std::make_shared<T>(this);
	return AddSystem(system) ? system.get() : nullptr;
}

template<typename ArgsT>
bool ff::EntityDomain::TriggerEvent(hash_t eventId, Entity entity, ArgsT *args)
{
	return TriggerEvent(GetEventEntry(eventId), entity, args);
}

template<typename ArgsT>
bool ff::EntityDomain::TriggerEvent(hash_t eventId, ArgsT *args)
{
	return TriggerEvent(GetEventEntry(eventId), INVALID_ENTITY, args);
}
