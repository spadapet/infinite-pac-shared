#pragma once

namespace ff
{
	// Put this on the stack when allocating static memory so that leaks aren't tracked
	class ScopeStaticMemAlloc
	{
	public:
		UTIL_API ScopeStaticMemAlloc();
		UTIL_API ~ScopeStaticMemAlloc();
	};

	class AtScope
	{
	public:
		UTIL_API AtScope(std::function<void()> closeFunc);
		UTIL_API AtScope(std::function<void()> openFunc, std::function<void()> closeFunc);
		UTIL_API ~AtScope();

		UTIL_API void Close();

	private:
		std::function<void()> _closeFunc;
	};

	// An object for using the global C++ allocator
	template<typename T>
	struct MemAllocator
	{
		T *NewOne()
		{
			return new T();
		}

		T *NewOne(const T &rhs)
		{
			return new T(rhs);
		}

		T *NewOne(T &&rhs)
		{
			return new T(std::move(rhs));
		}

		void DeleteOne(T *pOne)
		{
			delete pOne;
		}

		T *New(size_t nCount)
		{
			return new T[nCount];
		}

		void Delete(T *pArray)
		{
			delete[] pArray;
		}

		T *Malloc(size_t nCount)
		{
			return (T *)_aligned_malloc(nCount * sizeof(T), __alignof(T));
		}

		T *Realloc(T *pArray, size_t nCount)
		{
			return (T *)_aligned_realloc(pArray, nCount * sizeof(T), __alignof(T));
		}

		void Free(T *pArray)
		{
			_aligned_free(pArray);
		}
	};

	template<size_t Alignment>
	struct AlignedByteAllocator
	{
		BYTE *Malloc(size_t bytes)
		{
			return (BYTE *)_aligned_malloc(bytes, Alignment);
		}

		void Free(BYTE *mem)
		{
			_aligned_free(mem);
		}
	};
}
