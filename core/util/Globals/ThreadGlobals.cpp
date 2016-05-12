#include "pch.h"
#include "Globals/ThreadGlobals.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadUtil.h"

static __declspec(thread) ff::ThreadGlobals *s_threadGlobals = nullptr;

ff::ThreadGlobals::ThreadGlobals(bool allowDispatch)
	: _state(State::UNSTARTED)
	, _id(::GetCurrentThreadId())
	, _allowDispatch(allowDispatch)
{
	assert(s_threadGlobals == nullptr);
	s_threadGlobals = this;
}

ff::ThreadGlobals::~ThreadGlobals()
{
	assert(s_threadGlobals == this && IsShuttingDown());
	s_threadGlobals = nullptr;
}

ff::ThreadGlobals *ff::ThreadGlobals::Get()
{
	assert(s_threadGlobals);
	return s_threadGlobals;
}

bool ff::ThreadGlobals::Exists()
{
	return s_threadGlobals != nullptr;
}

bool ff::ThreadGlobals::Startup()
{
	_state = State::STARTED;
	
	if (_allowDispatch)
	{
		assertRetVal(ff::CreateCurrentThreadDispatch(&_dispatch), false);
	}

	return IsValid();
}

void ff::ThreadGlobals::Shutdown()
{
	CallShutdownFunctions();

	if (_dispatch)
	{
		_dispatch->Destroy();
		_dispatch = nullptr;
	}
}

ff::IThreadDispatch *ff::ThreadGlobals::GetDispatch() const
{
	return _dispatch;
}

unsigned int ff::ThreadGlobals::ThreadId() const
{
	return _id;
}

bool ff::ThreadGlobals::IsValid() const
{
	switch (_state)
	{
	case State::UNSTARTED:
	case State::FAILED:
		return false;

	default:
		return true;
	}
}

bool ff::ThreadGlobals::IsShuttingDown() const
{
	return _state == State::SHUT_DOWN;
}

void ff::ThreadGlobals::AtShutdown(std::function<void()> func)
{
	assert(!IsShuttingDown());

	LockMutex lock(_cs);
	_shutdownFunctions.emplace_back(std::move(func));
}

void ff::ThreadGlobals::CallShutdownFunctions()
{
	_state = State::SHUT_DOWN;

	Vector<std::function<void()>> shutdownFunctions;
	{
		LockMutex lock(_cs);
		shutdownFunctions.Reserve(_shutdownFunctions.size());

		while (!_shutdownFunctions.empty())
		{
			shutdownFunctions.Push(std::move(_shutdownFunctions.back()));
			_shutdownFunctions.erase(--_shutdownFunctions.end());
		}
	}

	for (const auto &func: shutdownFunctions)
	{
		func();
	}
}

void ff::AtThreadShutdown(std::function<void()> func)
{
	assertRet(s_threadGlobals != nullptr);

	if (s_threadGlobals->IsShuttingDown())
	{
		assertSz(false, L"Why register a thread shutdown function during shutdown?");
		func();
	}
	else
	{
		s_threadGlobals->AtShutdown(func);
	}
}
