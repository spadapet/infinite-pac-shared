#include "pch.h"
#include "Globals/Log.h"
#include "String/StringManager.h"

// #define TRACK_STRINGS
#if defined(_DEBUG) && defined(TRACK_STRINGS)
static Vector<wchar_t *> s_allStrings;
#endif

ff::StringManager::StringManager()
	: _numLargeStringAlloc(0)
	, _curLargeStringAlloc(0)
	, _maxLargeStringAlloc(0)
{
}

ff::StringManager::~StringManager()
{
#if defined(_DEBUG) && defined(TRACK_STRINGS)
	for (const wchar_t *str: s_allStrings)
	{
		OutputDebugString(L"Leaked string: ");
		OutputDebugString(str);
	}
#endif

	assert(_curLargeStringAlloc == 0);

	DebugDump();
}

wchar_t *ff::StringManager::New(size_t count)
{
	wchar_t * str = nullptr;

	if(count > 256)
	{
		// Need room before the actual string to store a null IPoolAllocator
		BYTE *mem = new BYTE[sizeof(wchar_t) * count + sizeof(IPoolAllocator *)];
		*(IPoolAllocator **)mem = nullptr;
		str = (wchar_t *)(mem + sizeof(IPoolAllocator *));

		LockMutex crit(_mutex);
		_numLargeStringAlloc++;
		_curLargeStringAlloc++;
		_maxLargeStringAlloc = std::max(_curLargeStringAlloc, _maxLargeStringAlloc);
	}
	else if(count > 128)
	{
		auto mem = _pool_256.New();
		mem->_pool = &_pool_256;
		str = mem->_str.data();
	}
	else if(count > 64)
	{
		auto mem = _pool_128.New();
		mem->_pool = &_pool_128;
		str = mem->_str.data();
	}
	else if(count > 32)
	{
		auto mem = _pool_64.New();
		mem->_pool = &_pool_64;
		str = mem->_str.data();
	}
	else
	{
		auto mem = _pool_32.New();
		mem->_pool = &_pool_32;
		str = mem->_str.data();
	}

#if defined(_DEBUG) && defined(TRACK_STRINGS)
	s_allStrings.Push(str);
#endif
	return str;
}

void ff::StringManager::Delete(wchar_t * str)
{
	assertRet(str != nullptr);
#if defined(_DEBUG) && defined(TRACK_STRINGS)
	s_allStrings.Delete(s_allStrings.Find(str));
#endif

	IPoolAllocator **afterPool = (IPoolAllocator **)str;
	IPoolAllocator *pool = *(afterPool - 1);
	BYTE *structStart = (BYTE *)(afterPool - 1);

	if (pool == nullptr)
	{
		assert(_curLargeStringAlloc > 0);
		delete[] structStart;

		LockMutex lock(_mutex);
		_curLargeStringAlloc--;
	}
	else
	{
		pool->DeleteVoid(structStart);
	}
}

ff::StringManager::SharedStringVector *ff::StringManager::NewVector()
{
	return _vectorPool.New();
}

ff::StringManager::SharedStringVector *ff::StringManager::NewVector(const SharedStringVector &rhs)
{
	return _vectorPool.New(rhs);
}

ff::StringManager::SharedStringVector *ff::StringManager::NewVector(SharedStringVector &&rhs)
{
	return _vectorPool.New(std::move(rhs));
}

void ff::StringManager::DeleteVector(SharedStringVector *str)
{
	return _vectorPool.Delete(str);
}

void ff::StringManager::DebugDump()
{
#ifdef _DEBUG
	Log::DebugTraceF(L"- DUMPING STRING ALLOCATION STATS ---------------------\n");
	Log::DebugTraceF(L"    Number of allocated  32  char strings: %d\n", _pool_32.GetTotalAlloc());
	Log::DebugTraceF(L"    Maximum simultaneous 32  char strings: %d\n", _pool_32.GetMaxAlloc());
	Log::DebugTraceF(L"    Number of allocated  64  char strings: %d\n", _pool_64.GetTotalAlloc());
	Log::DebugTraceF(L"    Maximum simultaneous 64  char strings: %d\n", _pool_64.GetMaxAlloc());
	Log::DebugTraceF(L"    Number of allocated  128 char strings: %d\n", _pool_128.GetTotalAlloc());
	Log::DebugTraceF(L"    Maximum simultaneous 128 char strings: %d\n", _pool_128.GetMaxAlloc());
	Log::DebugTraceF(L"    Number of allocated  256 char strings: %d\n", _pool_256.GetTotalAlloc());
	Log::DebugTraceF(L"    Maximum simultaneous 256 char strings: %d\n", _pool_256.GetMaxAlloc());
	Log::DebugTraceF(L"    Number of allocated  large strings:    %d\n", _numLargeStringAlloc);
	Log::DebugTraceF(L"    Maximum simultaneous large strings:    %d\n", _maxLargeStringAlloc);
	Log::DebugTraceF(L"    Number of allocated vectors:           %d\n", _vectorPool.GetTotalAlloc());
	Log::DebugTraceF(L"    Maximum simultaneous vectors:          %d\n", _vectorPool.GetMaxAlloc());
	Log::DebugTraceF(L"- completed string dumping ----------------------------\n\n");
#endif
}
