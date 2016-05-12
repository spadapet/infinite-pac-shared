#include "pch.h"
#include "Entity/EntityDomain.h"
#include "Entity/EntityEvents.h"

ff::EntityEventConnection::EntityEventConnection()
{
	Init();
}

ff::EntityEventConnection::~EntityEventConnection()
{
	Disconnect();
}

bool ff::EntityEventConnection::Connect(EntityDomain *domain, hash_t eventId, IEntityEventHandler *handler)
{
	return Connect(domain, eventId, INVALID_ENTITY, handler);
}

bool ff::EntityEventConnection::Connect(EntityDomain *domain, hash_t eventId, Entity entity, IEntityEventHandler *handler)
{
	assertRetVal(domain && domain->AddEventHandler(eventId, entity, handler), false);

	_domain = domain;
	_eventId = eventId;
	_entity = entity;
	_handler = handler;

	return true;
}

bool ff::EntityEventConnection::Disconnect()
{
	bool removed = _domain->RemoveEventHandler(_eventId, _entity, _handler);
	assert(removed);
	Init();

	return removed;
}

void ff::EntityEventConnection::Init()
{
	_domain = nullptr;
	_eventId = 0;
	_entity = INVALID_ENTITY;
	_handler = nullptr;
}
