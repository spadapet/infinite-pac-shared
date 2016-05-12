#include "pch.h"
#include "State/State.h"

ff::State::State()
{
}

ff::State::~State()
{
}

std::shared_ptr<ff::State> ff::State::Advance(AppGlobals *globals)
{
	return nullptr;
}

void ff::State::Render(AppGlobals *globals, IRenderTarget *target)
{
}

void ff::State::SaveState(AppGlobals *globals)
{
}

void ff::State::LoadState(AppGlobals *globals)
{
}

bool ff::State::Notify(hash_t eventId, int data1, void *data2)
{
	return false;
}

ff::State::Status ff::State::GetStatus()
{
	return Status::Alive;
}
