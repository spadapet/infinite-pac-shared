#pragma once
// Include this in your module's main CPP file only! (through MainUtilInclude.h)

static HINSTANCE s_delayLoadInstance = nullptr;

namespace ff
{
	static void SetDelayLoadInstance(HINSTANCE instance)
	{
		s_delayLoadInstance = instance;
	}

	static HINSTANCE GetDelayLoadInstance()
	{
		return s_delayLoadInstance;
	}
}

#if !METRO_APP

#include <delayimp.h>

namespace ff
{
	static void FixModulePath(PDelayLoadInfo loadInfo)
	{
		if (strchr(loadInfo->szDll, '\\'))
		{
			return;
		}

		static char staticPath[MAX_PATH * 2];

		if (!::GetModuleFileNameA(s_delayLoadInstance, staticPath, _countof(staticPath)) ||
			GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			return;
		}

		char *slash = strrchr(staticPath, '\\');
		if (slash == nullptr)
		{
			return;
		}

		slash[1] = 0;

		if (strcat_s(staticPath, loadInfo->szDll) != 0)
		{
			// path too long
			return;
		}

		WIN32_FIND_DATAA data;
		HANDLE findHandle = ::FindFirstFileA(staticPath, &data);

		if (findHandle == INVALID_HANDLE_VALUE)
		{
			return;
		}

		FindClose(findHandle);

		loadInfo->szDll = staticPath;
	}
}

FARPROC WINAPI DelayLoadHook(unsigned int notify, PDelayLoadInfo loadInfo)
{
	switch (notify)
	{
	case dliNotePreLoadLibrary:
		ff::FixModulePath(loadInfo);
		break;

	case dliFailGetProc:
	case dliFailLoadLib:
	case dliNoteEndProcessing:
	case dliNotePreGetProcAddress:
	case dliStartProcessing:
	default:
		break;
	}

	return nullptr;
}

const PfnDliHook __pfnDliNotifyHook2 = DelayLoadHook;

#endif // !METRO_APP
