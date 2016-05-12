#include "pch.h"
#include "Globals/ProcessGlobals.h"

// STATIC_DATA (object) - CRITICAL SECTIONS
static ff::Mutex s_staticMutex[ff::GCS_COUNT];

ff::Mutex::Mutex(bool lockable)
{
	::InitializeCriticalSectionEx(&_mutex, 3000, 0);
	_lockable = lockable ? &_mutex : nullptr;
}

ff::Mutex::~Mutex()
{
	if (_lockable == &_mutex)
	{
		::DeleteCriticalSection(&_mutex);
	}
}

void ff::Mutex::Enter() const
{
	assert(DidProgramStart());

	if (_lockable != nullptr)
	{
		::EnterCriticalSection(_lockable);
	}
}

bool ff::Mutex::TryEnter() const
{
	assert(DidProgramStart());

	return _lockable != nullptr && ::TryEnterCriticalSection(_lockable) != FALSE;
}

void ff::Mutex::Leave() const
{
	if (_lockable != nullptr)
	{
		::LeaveCriticalSection(_lockable);
	}
}

bool ff::Mutex::IsLockable() const
{
	return _lockable != nullptr;
}

void ff::Mutex::SetLockable(bool lockable)
{
	_lockable = lockable ? &_mutex : nullptr;
}

bool ff::Mutex::WaitForCondition(CONDITION_VARIABLE &condition) const
{
	assert(DidProgramStart());
	assertRetVal(_lockable, false);

	return ::SleepConditionVariableCS(&condition, _lockable, INFINITE) != FALSE;
}

ff::Condition::Condition()
{
	::InitializeConditionVariable(&_condition);
}

ff::Condition::operator CONDITION_VARIABLE &()
{
	return _condition;
}

void ff::Condition::WakeOne()
{
	::WakeConditionVariable(&_condition);
}

void ff::Condition::WakeAll()
{
	::WakeAllConditionVariable(&_condition);
}

ff::LockMutex::LockMutex()
	: _mutex(nullptr)
{
}

ff::LockMutex::LockMutex(const Mutex &mutex)
	: _mutex(IsProgramRunning() ? &mutex : nullptr)
{
	if (_mutex != nullptr)
	{
		_mutex->Enter();
	}
}

ff::LockMutex::LockMutex(LockMutex &&rhs)
	: _mutex(rhs._mutex)
{
	rhs._mutex = nullptr;
}

ff::LockMutex::LockMutex(GlobalMutex type)
	: _mutex(IsProgramRunning() ? &s_staticMutex[type] : nullptr)
{
	if (_mutex != nullptr)
	{
		_mutex->Enter();
	}
}

ff::LockMutex::~LockMutex()
{
	Unlock();
}

void ff::LockMutex::Unlock()
{
	if (_mutex != nullptr)
	{
		_mutex->Leave();
		_mutex = nullptr;
	}
}
