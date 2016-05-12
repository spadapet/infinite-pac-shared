#include "pch.h"
#include "MainUtilInclude.h"
#include "Module/Module.h"

// {0C55D6ED-16E8-404C-9E11-211A81AA74F8}
static const GUID s_moduleId = {0x0c55d6ed,0x16e8,0x404c,{0x9e,0x11,0x21,0x1a,0x81,0xaa,0x74,0xf8}};
static ff::StaticString s_moduleName(L"util");
static ff::ModuleFactory RegisterThisModule(s_moduleName, s_moduleId, ff::GetDelayLoadInstance, ff::GetModuleStartup);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		ff::SetThisModule(s_moduleName, s_moduleId, instance);
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}
