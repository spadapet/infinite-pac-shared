#pragma once

#include "Thread/Mutex.h"

namespace ff
{
	// This class is only good for allocating memory internal to the String class.
	//
	// There is really no need to use this for any other situation. There is always
	// a global StringManager attached to the ProcessGlobals, which is created as
	// soon as the app boots up. But still, don't use it. Just use StringAllocator::New
	// if you really want to create a raw string buffer in the global pool.
	class StringManager
	{
	public:
		StringManager();
		~StringManager();

		wchar_t *New(size_t count);
		void Delete(wchar_t *str);

		typedef SharedStringVectorAllocator::SharedStringVector SharedStringVector;
		SharedStringVector *NewVector();
		SharedStringVector *NewVector(const SharedStringVector &rhs);
		SharedStringVector *NewVector(SharedStringVector &&rhs);
		void DeleteVector(SharedStringVector *str);

	private:
		void DebugDump();

		template<size_t N>
		struct Chars
		{
			IPoolAllocator *_pool;
			std::array<wchar_t, N> _str;
		};

		Mutex _mutex;
		long _numLargeStringAlloc;
		long _curLargeStringAlloc;
		long _maxLargeStringAlloc;

		PoolAllocator<Chars<32>> _pool_32;
		PoolAllocator<Chars<64>> _pool_64;
		PoolAllocator<Chars<128>> _pool_128;
		PoolAllocator<Chars<256>> _pool_256;
		PoolAllocator<SharedStringVector> _vectorPool;
	};
}
