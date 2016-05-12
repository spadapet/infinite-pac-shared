#pragma once

namespace ff
{
	class ReaderWriterLock
	{
	public:
		UTIL_API ReaderWriterLock(bool lockable = true);
		UTIL_API ~ReaderWriterLock();

		UTIL_API void EnterRead() const;
		UTIL_API bool TryEnterRead() const;
		UTIL_API void EnterWrite() const;
		UTIL_API bool TryEnterWrite() const;
		UTIL_API void LeaveRead() const;
		UTIL_API void LeaveWrite() const;
		UTIL_API bool IsLockable() const;
		UTIL_API void SetLockable(bool lockable);

	private:
		SRWLOCK *_lockable;
		SRWLOCK _lock;

	private:
		ReaderWriterLock(const ReaderWriterLock &rhs);
		const ReaderWriterLock &operator=(const ReaderWriterLock &rhs);
	};

	class LockReader
	{
	public:
		UTIL_API LockReader(const ReaderWriterLock &lock);
		UTIL_API LockReader(LockReader &&rhs);
		UTIL_API ~LockReader();

		void Unlock();

	private:
		const ReaderWriterLock *_lock;
	};

	class LockWriter
	{
	public:
		UTIL_API LockWriter(const ReaderWriterLock &lock);
		UTIL_API LockWriter(LockWriter &&rhs);
		UTIL_API ~LockWriter();

		void Unlock();

	private:
		const ReaderWriterLock *_lock;
	};

	template<typename T>
	class LockObjectRead
	{
	public:
		LockObjectRead(T &obj, ReaderWriterLock &lock);
		LockObjectRead(LockObjectRead<T> &&rhs);

		T &Get() const;
		T *operator->() const;

	private:
		T &_obj;
		LockReader _lock;
	};

	template<typename T>
	class LockObjectWrite
	{
	public:
		LockObjectWrite(T &obj, ReaderWriterLock &lock);
		LockObjectWrite(LockObjectWrite<T> &&rhs);

		T &Get() const;
		T *operator->() const;

	private:
		T &_obj;
		LockWriter _lock;
	};
}

template<typename T>
ff::LockObjectRead<T>::LockObjectRead(T &obj, ReaderWriterLock &lock)
	: _obj(obj)
	, _lock(lock)
{
}

template<typename T>
ff::LockObjectRead<T>::LockObjectRead(LockObjectRead<T> &&rhs)
	: _obj(rhs._obj)
	, _lock(std::move(rhs._lock))
{
}

template<typename T>
T &ff::LockObjectRead<T>::Get() const
{
	return _obj;
}

template<typename T>
T *ff::LockObjectRead<T>::operator->() const
{
	return &_obj;
}

template<typename T>
ff::LockObjectWrite<T>::LockObjectWrite(T &obj, ReaderWriterLock &lock)
	: _obj(obj)
	, _lock(lock)
{
}

template<typename T>
ff::LockObjectWrite<T>::LockObjectWrite(LockObjectWrite<T> &&rhs)
	: _obj(rhs._obj)
	, _lock(std::move(rhs._lock))
{
}

template<typename T>
T &ff::LockObjectWrite<T>::Get() const
{
	return _obj;
}

template<typename T>
T *ff::LockObjectWrite<T>::operator->() const
{
	return &_obj;
}
