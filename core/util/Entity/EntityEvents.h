#pragma once
#include "Entity/Entity.h"

namespace ff
{
	class EntityDomain;

	class IEntityEventHandler
	{
	public:
		virtual ~IEntityEventHandler() { }

		virtual void OnEntityEvent(EntityDomain *domain, Entity entity, hash_t eventId, void *eventArgs) = 0;
	};

	class EntityEventConnection
	{
	public:
		UTIL_API EntityEventConnection();
		UTIL_API ~EntityEventConnection();

		UTIL_API bool Connect(EntityDomain *domain, hash_t eventId, IEntityEventHandler *handler);
		UTIL_API bool Connect(EntityDomain *domain, hash_t eventId, Entity entity, IEntityEventHandler *handler);
		UTIL_API bool Disconnect();

	private:
		void Init();

		EntityDomain *_domain;
		hash_t _eventId;
		Entity _entity;
		IEntityEventHandler *_handler;
	};
}
