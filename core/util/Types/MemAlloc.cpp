#include "pch.h"
#include "Globals/Log.h"

#include <crtdbg.h>

// STATIC_DATA (pod)
static long s_staticMemAlloc = 0;

ff::ScopeStaticMemAlloc::ScopeStaticMemAlloc()
{
	LockMutex crit(GCS_MEM_ALLOC);

	if (++s_staticMemAlloc == 1)
	{
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) & ~_CRTDBG_ALLOC_MEM_DF);
	}

	assert(s_staticMemAlloc >= 1);
}

ff::ScopeStaticMemAlloc::~ScopeStaticMemAlloc()
{
	LockMutex crit(GCS_MEM_ALLOC);

	if (!--s_staticMemAlloc)
	{
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_ALLOC_MEM_DF);
	}

	assert(s_staticMemAlloc >= 0);
}

ff::AtScope::AtScope(std::function<void()> closeFunc)
	: _closeFunc(closeFunc)
{
}

ff::AtScope::AtScope(std::function<void()> openFunc, std::function<void()> closeFunc)
	: _closeFunc(closeFunc)
{
}

ff::AtScope::~AtScope()
{
	_closeFunc();
}

void ff::AtScope::Close()
{
	_closeFunc();
	_closeFunc = []() {};
}

#ifdef _DEBUG

// Keep CRT memory usage statistics
static size_t s_totalAlloc = 0;
static size_t s_curAlloc = 0;
static size_t s_maxAlloc = 0;
static size_t s_allocCount = 0;

static int CrtAllocHook(
	int allocType,
	void *userData,
	size_t size,
	int blockType,
	long requestNumber,
	const unsigned char *filename,
	int lineNumber)
{
	// don't call any CRT functions when blockType == _CRT_BLOCK

	switch(allocType)
	{
	case _HOOK_ALLOC:
	case _HOOK_REALLOC:
		{
			ff::LockMutex crit(ff::GCS_MEM_ALLOC_HOOK);
			s_allocCount++;
			s_totalAlloc += size;
			s_curAlloc += size;
			s_maxAlloc = std::max(s_curAlloc, s_maxAlloc);
		}
		break;

	case _HOOK_FREE:
		{
			ff::LockMutex crit(ff::GCS_MEM_ALLOC_HOOK);
			size = _msize(userData);
			s_curAlloc = (size > s_curAlloc) ? 0 : s_curAlloc - size;
		}
		break;
	}

	return TRUE;
}

#endif // _DEBUG

namespace ff
{
	void HookCrtMemAlloc();
	void UnhookCrtMemAlloc();
}

void ff::HookCrtMemAlloc()
{
	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
	_CrtSetAllocHook(CrtAllocHook);
}

void ff::UnhookCrtMemAlloc()
{
	_CrtSetAllocHook(nullptr);

#ifdef _DEBUG
	Log::DebugTraceF(L"- DUMPING MEMORY ALLOCATION STATS ---------------------\n");
	Log::DebugTraceF(L"    Number of allocations:  %lu\n", s_allocCount);
	Log::DebugTraceF(L"    Total allocated bytes:  %lu\n", s_totalAlloc);
	Log::DebugTraceF(L"    Max simultaneous bytes: %lu\n", s_maxAlloc);
	Log::DebugTraceF(L"- completed memory dumping ----------------------------\n\n");
#endif
}
