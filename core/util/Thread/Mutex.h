#pragma once

namespace ff
{
	enum GlobalMutex;

	class Mutex
	{
	public:
		UTIL_API Mutex(bool lockable = true);
		UTIL_API ~Mutex();

		UTIL_API void Enter() const;
		UTIL_API bool TryEnter() const;
		UTIL_API void Leave() const;
		UTIL_API bool IsLockable() const;
		UTIL_API void SetLockable(bool lockable);
		UTIL_API bool WaitForCondition(CONDITION_VARIABLE &condition) const;

	private:
		CRITICAL_SECTION *_lockable;
		CRITICAL_SECTION _mutex;

	private:
		Mutex(const Mutex &rhs);
		const Mutex &operator=(const Mutex &rhs);
	};

	class Condition
	{
	public:
		UTIL_API Condition();
		UTIL_API operator CONDITION_VARIABLE &();
		UTIL_API void WakeOne();
		UTIL_API void WakeAll();

	private:
		CONDITION_VARIABLE _condition;
	};

	class LockMutex
	{
	public:
		UTIL_API LockMutex();
		UTIL_API LockMutex(const Mutex &mutex);
		UTIL_API LockMutex(LockMutex &&rhs);
		UTIL_API LockMutex(GlobalMutex type);
		UTIL_API ~LockMutex();

		UTIL_API void Unlock();

	private:
		const Mutex *_mutex;
	};

	// Rather than have static mutexes all over the place, just put them in here
	enum GlobalMutex
	{
		GCS_COM_BASE,
		GCS_FILE_UTIL,
		GCS_INPUT_MAPPING,
		GCS_LOG,
		GCS_MEM_ALLOC,
		GCS_MEM_ALLOC_HOOK,
		GCS_MESSAGE_FILTER,
		GCS_MODULE_FACTORY,
		GCS_RESOURCE_VALUE,
		GCS_VALUE,
		GCS_WIN_UTIL,

		GCS_COUNT
	};
}
