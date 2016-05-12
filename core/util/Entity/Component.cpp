#include "pch.h"
#include "Entity/Component.h"

static size_t s_nextFactoryIndex = 0;

size_t ff::Component::GetNextFactoryIndex()
{
	return ::InterlockedIncrement(&s_nextFactoryIndex) - 1;
}
