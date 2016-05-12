#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "Thread/ReaderWriterLock.h"

ff::ReaderWriterLock::ReaderWriterLock(bool lockable)
{
	::InitializeSRWLock(&_lock);
	_lockable = lockable ? &_lock : nullptr;
}

ff::ReaderWriterLock::~ReaderWriterLock()
{
}

void ff::ReaderWriterLock::EnterRead() const
{
	assert(DidProgramStart());

	if (_lockable)
	{
		::AcquireSRWLockShared(_lockable);
	}
}

bool ff::ReaderWriterLock::TryEnterRead() const
{
	assert(DidProgramStart());

	return _lockable && ::TryAcquireSRWLockShared(_lockable);
}

void ff::ReaderWriterLock::EnterWrite() const
{
	assert(DidProgramStart());

	if (_lockable)
	{
		::AcquireSRWLockExclusive(_lockable);
	}
}

bool ff::ReaderWriterLock::TryEnterWrite() const
{
	assert(DidProgramStart());

	return _lockable && ::TryAcquireSRWLockExclusive(_lockable);
}

void ff::ReaderWriterLock::LeaveRead() const
{
	if (_lockable)
	{
		::ReleaseSRWLockShared(_lockable);
	}
}

void ff::ReaderWriterLock::LeaveWrite() const
{
	if (_lockable)
	{
		::ReleaseSRWLockExclusive(_lockable);
	}
}

bool ff::ReaderWriterLock::IsLockable() const
{
	return _lockable != nullptr;
}

void ff::ReaderWriterLock::SetLockable(bool lockable)
{
	_lockable = lockable ? &_lock : nullptr;
}

ff::LockReader::LockReader(const ReaderWriterLock &lock)
	: _lock(IsProgramRunning() ? &lock : nullptr)
{
	if (_lock)
	{
		_lock->EnterRead();
	}
}

ff::LockReader::LockReader(LockReader &&rhs)
	: _lock(rhs._lock)
{
	rhs._lock = nullptr;
}

ff::LockReader::~LockReader()
{
	Unlock();
}

void ff::LockReader::Unlock()
{
	if (_lock)
	{
		_lock->LeaveRead();
	}
}

ff::LockWriter::LockWriter(const ReaderWriterLock &lock)
	: _lock(IsProgramRunning() ? &lock : nullptr)
{
	if (_lock)
	{
		_lock->EnterWrite();
	}
}

ff::LockWriter::LockWriter(LockWriter &&rhs)
	: _lock(rhs._lock)
{
	rhs._lock = nullptr;
}

ff::LockWriter::~LockWriter()
{
	Unlock();
}

void ff::LockWriter::Unlock()
{
	if (_lock)
	{
		_lock->LeaveWrite();
	}
}
