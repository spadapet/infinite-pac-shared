#pragma once

namespace ff
{
	namespace details
	{
		// A single object in the pool
		template<typename T>
		struct PoolObj
		{
			char _obj[sizeof(T)];
			PoolObj<T> *_nextFree;

			T *ToObj()
			{
				return (T *)_obj;
			}

			static PoolObj<T> *FromObj(T *obj)
			{
				return (PoolObj<T> *)obj;
			}
		};

		// A single array of objects in the larger pool
		template<typename T>
		struct SubPool : private MemAllocator<PoolObj<T>>
		{
			PoolObj<T> *_pool;
			size_t _size;

			SubPool(size_t size, PoolObj<T> *&firstFree)
				: _pool(Malloc(size))
				, _size(size)
			{
				for (size_t i = 0; i < _size - 1; i++)
				{
					_pool[i]._nextFree = _pool + i + 1;
				}

				_pool[_size - 1]._nextFree = firstFree;
				firstFree = _pool;
			}

			SubPool(SubPool &&rhs)
				: _pool(rhs._pool)
				, _size(rhs._size)
			{
				rhs._pool = nullptr;
				rhs._size = 0;
			}

			~SubPool()
			{
				Free(_pool);
			}
		};
	}

	// A generic way to interact with a pool allocator.
	//
	// Normally this isn't used. But if you need to stash away a pointer to a pool
	// that created a certain object, using this interface might make sense.
	class IPoolAllocator
	{
	public:
		virtual void DeleteVoid(void *obj) = 0;
	};

	// Reuses memory when creating objects
	template<typename T>
	class PoolAllocator : public IPoolAllocator
	{
	public:
		PoolAllocator(bool threadSafe = true);
		PoolAllocator(PoolAllocator &&rhs);
		~PoolAllocator();

		T *New();
		T *New(const T &rhs);
		T *New(T &&rhs);
		void Delete(T *obj);

		// IPoolAllocator
		virtual void DeleteVoid(void *obj) override;

		size_t GetCurAlloc() const; // how many allocations are currently in memory?
		size_t GetTotalAlloc() const; // how many allocations have ever been made?
		size_t GetMaxAlloc() const; // the largest value ever of GetCurAlloc()
		size_t GetNumFree() const; // how much "wasted" space is there?
		size_t MemUsage() const; // how much total memory is allocated

	private:
		PoolAllocator(const PoolAllocator &rhs);
		PoolAllocator &operator=(const PoolAllocator &rhs);

		T *NewUnconstructed();

		template<typename T2>
		typename std::enable_if<std::is_default_constructible<T2>::value, T2>::type *InternalNew()
		{
			T2 *newObj = NewUnconstructed();
			::new(newObj) T2;
			return newObj;
		}

		template<typename T2>
		typename std::enable_if<std::is_copy_constructible<T2>::value, T2>::type *InternalNew(const T2 &rhs)
		{
			T2 *newObj = NewUnconstructed();
			::new(newObj) T2(rhs);
			return newObj;
		}

		template<typename T2>
		typename std::enable_if<std::is_move_constructible<T2>::value, T2>::type *InternalNew(T2 &&rhs)
		{
			T2 *newObj = NewUnconstructed();
			::new(newObj) T2(std::move(rhs));
			return newObj;
		}

		Mutex _mutex;
		Vector<details::SubPool<T>> _subPools;
		details::PoolObj<T> *_firstFree;
		size_t _curAlloc;
		size_t _maxAlloc;
		size_t _totalAlloc;
		size_t _freeAlloc;
#ifdef _DEBUG
		Vector<const T *> _allAlloc;
#endif
	};

	template<typename T>
	PoolAllocator<T>::PoolAllocator(bool threadSafe)
		: _mutex(threadSafe)
		, _firstFree(nullptr)
		, _curAlloc(0)
		, _maxAlloc(0)
		, _totalAlloc(0)
		, _freeAlloc(0)
	{
	}

	template<typename T>
	PoolAllocator<T>::PoolAllocator(PoolAllocator &&rhs)
		: _mutex(rhs._mutex.IsLockable())
		, _subPools(std::move(rhs._subPools))
		, _firstFree(rhs._firstFree)
		, _curAlloc(rhs._curAlloc)
		, _maxAlloc(rhs._maxAlloc)
		, _totalAlloc(rhs._totalAlloc)
		, _freeAlloc(rhs._freeAlloc)
#ifdef _DEBUG
		, _allAlloc(std::move(rhs._allAlloc))
#endif
	{
		assert(rhs._allAlloc.IsEmpty() && rhs._subPools.IsEmpty());

		rhs._firstFree = nullptr;
		rhs._curAlloc = 0;
		rhs._maxAlloc = 0;
		rhs._totalAlloc = 0;
		rhs._freeAlloc = 0;
	}

	template<typename T>
	PoolAllocator<T>::~PoolAllocator()
	{
		assert(_curAlloc == 0);
#ifdef _DEBUG
		for (const T *leaked: _allAlloc)
		{
			wchar_t str[32];
			_snwprintf_s(str, 32, _TRUNCATE, L"LEAK! Obj = 0x%Ix\n", (size_t)leaked);
			OutputDebugString(str);
		}
#endif
	}

	template<typename T>
	void PoolAllocator<T>::DeleteVoid(void *obj)
	{
		Delete((T *)obj);
	}

	template<typename T>
	T *PoolAllocator<T>::NewUnconstructed()
	{
		LockMutex lock(_mutex);

		if (_firstFree == nullptr)
		{
			// figure out the size of the new pool by doubling the size of the last pool
			details::SubPool<T> *lastSubPool = !_subPools.IsEmpty() ? &_subPools.GetLast() : nullptr;
			size_t size = lastSubPool ? lastSubPool->_size * 2 : 16;
			size = std::min<size_t>(size, 512);
			_freeAlloc += size;

			// just in case this pool is static
			ScopeStaticMemAlloc staticAlloc;
			_subPools.Push(details::SubPool<T>(size, _firstFree));
		}

		details::PoolObj<T> *poolObj = _firstFree;
		_firstFree = poolObj->_nextFree;
		poolObj->_nextFree = nullptr;

		_freeAlloc--;
		_curAlloc++;
		_totalAlloc++;
		_maxAlloc = std::max<size_t>(_curAlloc, _maxAlloc);
#ifdef _DEBUG
		// just in case this pool is static
		ScopeStaticMemAlloc staticAlloc;
		_allAlloc.Push(poolObj->ToObj());
#endif
		return poolObj->ToObj();
	}

	template<typename T>
	T *PoolAllocator<T>::New()
	{
		return InternalNew<T>();
	}

	template<typename T>
	T *PoolAllocator<T>::New(const T &rhs)
	{
		return InternalNew<T>(rhs);
	}

	template<typename T>
	T *PoolAllocator<T>::New(T &&rhs)
	{
		return InternalNew<T>(std::move(rhs));
	}

	template<typename T>
	void PoolAllocator<T>::Delete(T *obj)
	{
		if (obj != nullptr)
		{
			details::PoolObj<T> *poolObj = details::PoolObj<T>::FromObj(obj);
			obj->~T();

			LockMutex lock(_mutex);
			assert(!poolObj->_nextFree);
			poolObj->_nextFree = _firstFree;
			_firstFree = poolObj;

			assert(_curAlloc > 0);
			_freeAlloc++;
			_curAlloc--;
#ifdef _DEBUG
			_allAlloc[_allAlloc.Find(obj)] = _allAlloc.GetLast();
			_allAlloc.Pop();
#endif
		}
	}

	template<typename T>
	size_t PoolAllocator<T>::GetCurAlloc() const
	{
		LockMutex lock(_mutex);
		return _curAlloc;
	}

	template<typename T>
	size_t PoolAllocator<T>::GetTotalAlloc() const
	{
		LockMutex lock(_mutex);
		return _totalAlloc;
	}

	template<typename T>
	size_t PoolAllocator<T>::GetMaxAlloc() const
	{
		LockMutex lock(_mutex);
		return _maxAlloc;
	}

	template<typename T>
	size_t PoolAllocator<T>::GetNumFree() const
	{
		LockMutex lock(_mutex);
		return _freeAlloc;
	}

	template<typename T>
	size_t PoolAllocator<T>::MemUsage() const
	{
		LockMutex lock(_mutex);
		size_t bytes = _subPools.ByteSize();

		for (const details::SubPool<T> &subPool: _subPools)
		{
			bytes += subPool._size * sizeof(T);
		}

		return bytes;
	}
}
