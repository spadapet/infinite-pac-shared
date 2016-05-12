#pragma once

namespace ff
{
	// Stupidity to avoid infinite template recursion
	template<typename T>
	struct SharedObjectAllocator;

	// This adds shared reference counting to any object.
	//
	// AddRef and Release are thread safe.
	template<typename T, typename Allocator = SharedObjectAllocator<T>>
	class SharedObject : public T
	{
		typedef SharedObject<T, Allocator> MyType;

	public:
		SharedObject();
		SharedObject(const MyType &rhs);
		SharedObject(MyType &&rhs);
		SharedObject(const T &rhs);
		SharedObject(T &&rhs);
		~SharedObject();

		void AddRef();
		void Release();
		void DisableRefs();
		bool IsShared() const; // not thread safe
		bool IsOwnerThread() const;

		// This function is useful for copy-on-write behavior. It makes a copy if there
		// are multiple references to an object. Otherwise, the pointer doesn't change.
		static void GetUnshared(MyType **obj);

	private:
		long _refs;
		unsigned int _threadId;
	};

	template<typename T>
	struct SharedObjectAllocator : public MemAllocator<SharedObject<T>>
	{
	};

	template<typename T, typename Allocator>
	SharedObject<T, Allocator>::SharedObject()
		: _refs(0)
		, _threadId(GetCurrentThreadId())
	{
	}

	template<typename T, typename Allocator>
	SharedObject<T, Allocator>::SharedObject(const MyType &rhs)
		: T(rhs)
		, _refs(0)
		, _threadId(GetCurrentThreadId())
	{
	}

	template<typename T, typename Allocator>
	SharedObject<T, Allocator>::SharedObject(MyType &&rhs)
		: T(std::move(rhs))
		, _refs(rhs._refs)
		, _threadId(rhs._threadId)
	{
		rhs._refs = 0;
	}

	template<typename T, typename Allocator>
	SharedObject<T, Allocator>::SharedObject(const T &rhs)
		: T(rhs)
		, _refs(0)
		, _threadId(GetCurrentThreadId())
	{
	}

	template<typename T, typename Allocator>
	SharedObject<T, Allocator>::SharedObject(T &&rhs)
		: T(std::move(rhs))
		, _refs(0)
		, _threadId(GetCurrentThreadId())
	{
	}

	template<typename T, typename Allocator>
	SharedObject<T, Allocator>::~SharedObject()
	{
		assert(_refs == 0 || _refs == -1);
	}

	template<typename T, typename Allocator>
	void SharedObject<T, Allocator>::AddRef()
	{
		if (_refs != -1)
		{
			InterlockedIncrement(&_refs);
		}
	}

	template<typename T, typename Allocator>
	void SharedObject<T, Allocator>::Release()
	{
		if (_refs != -1 && InterlockedDecrement(&_refs) == 0)
		{
			Allocator().DeleteOne(this);
		}
	}

	template<typename T, typename Allocator>
	void SharedObject<T, Allocator>::DisableRefs()
	{
		_refs = -1;
	}

	template<typename T, typename Allocator>
	bool SharedObject<T, Allocator>::IsShared() const
	{
		// Shared objects aren't designed to be modified and used on different threads.
		// Multi-thread use must be strictly read-only (so an interlocked check here isn't necessary).
		return _refs != 1;
	}

	template<typename T, typename Allocator>
	bool SharedObject<T, Allocator>::IsOwnerThread() const
	{
		return GetCurrentThreadId() == _threadId;
	}

	// static
	template<typename T, typename Allocator>
	void SharedObject<T, Allocator>::GetUnshared(MyType **obj)
	{
		assertRet(obj != nullptr);

		if (*obj == nullptr)
		{
			*obj = Allocator().NewOne();
			(*obj)->_refs = 1;
		}
		else if ((*obj)->IsShared())
		{
			MyType *newObj = Allocator().NewOne(**obj);
			newObj->_refs = 1;
			(*obj)->Release();
			*obj = newObj;
		}
	}
}
