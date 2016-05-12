#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "Module/Module.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadUtil.h"
#include "Windows/Handles.h"
#include "Windows/WinUtil.h"

HANDLE ff::CreateEvent(bool initialSet, bool manualReset)
{
	return ::CreateEventEx(nullptr, nullptr,
		(initialSet ? CREATE_EVENT_INITIAL_SET : 0) |
		(manualReset ? CREATE_EVENT_MANUAL_RESET : 0),
		EVENT_ALL_ACCESS);
}

bool ff::IsEventSet(HANDLE handle)
{
	assertRetVal(handle, true);

	return ::WaitForSingleObjectEx(handle, 0, FALSE) == WAIT_OBJECT_0;
}

bool ff::WaitForEventAndReset(HANDLE handle)
{
	noAssertRetVal(ff::WaitForHandle(handle), false);
	return ::ResetEvent(handle) != FALSE;
}

bool ff::WaitForHandle(HANDLE handle)
{
	return ff::WaitForAnyHandle(&handle, 1) == 0;
}

size_t ff::WaitForAnyHandle(HANDLE *handles, size_t count)
{
	if (ff::ThreadGlobals::Exists())
	{
		return ff::ThreadGlobals::Get()->GetDispatch()->WaitForAnyHandle(handles, count);
	}

	while (true)
	{
		DWORD result = ::WaitForMultipleObjectsEx((DWORD)count, handles, FALSE, INFINITE, TRUE);
		switch (result)
		{
		default:
			if (result < count)
			{
				return result;
			}
			else if (result >= WAIT_ABANDONED_0 && result < WAIT_ABANDONED_0 + count)
			{
				return ff::INVALID_SIZE;
			}
			break;

		case WAIT_TIMEOUT:
		case WAIT_FAILED:
			return ff::INVALID_SIZE;

		case WAIT_IO_COMPLETION:
			break;
		}
	}
}

void ff::SetDebuggerThreadName(StringRef name, DWORD nThreadID)
{
#ifdef _DEBUG
	if (IsDebuggerPresent())
	{
		CHAR szNameACP[512] = "";
		WideCharToMultiByte(CP_ACP, 0, name.c_str(), -1, szNameACP, _countof(szNameACP), nullptr, nullptr);

		typedef struct tagTHREADNAME_INFO
		{
			ULONG_PTR dwType; // must be 0x1000
			const char *szName; // pointer to name (in user addr space)
			ULONG_PTR dwThreadID; // thread ID (-1=caller thread)
			ULONG_PTR dwFlags; // reserved for future use, must be zero
		} THREADNAME_INFO;

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.szName = szNameACP;
		info.dwThreadID = nThreadID ? nThreadID : GetCurrentThreadId();
		info.dwFlags = 0;

		__try
		{
			RaiseException(0x406D1388, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_CONTINUE_EXECUTION)
		{
		}
	}

#endif // _DEBUG
}
