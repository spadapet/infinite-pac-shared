#pragma once
// Include this in your module's main CPP file only! (through MainUtilInclude.h)

#include "Globals/ProcessGlobals.h"
#include "Module/Module.h"

static const ff::Module &InternalDllGetModule()
{
	return ff::GetThisModule();
}

static HRESULT InternalDllCanUnloadNow()
{
	return ff::DidProgramStart() && ff::GetThisModule().IsLocked() ? S_FALSE : S_OK;
}

static HRESULT InternalDllUnregisterServer()
{
	// COM objects are only internal
	return S_OK;
}

static HRESULT InternalDllRegisterServer()
{
	// COM objects are only internal
	return S_OK;
}

static HRESULT InternalDllGetClassObject(REFGUID clsid, REFIID iid, void **obj)
{
	ff::ComPtr<IClassFactory> factory;
	if (ff::DidProgramStart() && ff::GetThisModule().GetClassFactory(clsid, &factory))
	{
		return factory->QueryInterface(iid, obj);
	}

	return CLASS_E_CLASSNOTAVAILABLE;
}

#if METRO_APP

// Don't want conflicts with default exports provided by the Windows SDK

STDAPI_(const ff::Module &) DllGetModule()
{
	return InternalDllGetModule();
}

STDAPI MetroDllCanUnloadNow()
{
	return InternalDllCanUnloadNow();
}

STDAPI MetroDllUnregisterServer()
{
	return InternalDllUnregisterServer();
}

STDAPI MetroDllRegisterServer()
{
	return InternalDllRegisterServer();
}

STDAPI MetroDllGetClassObject(REFGUID clsid, REFIID iid, void **obj)
{
	return InternalDllGetClassObject(clsid, iid, obj);
}

#else

STDAPI_(const ff::Module &) DllGetModule()
{
	return InternalDllGetModule();
}

STDAPI DllCanUnloadNow()
{
	return InternalDllCanUnloadNow();
}

STDAPI DllUnregisterServer()
{
	return InternalDllUnregisterServer();
}

STDAPI DllRegisterServer()
{
	return InternalDllRegisterServer();
}

STDAPI DllGetClassObject(REFGUID clsid, REFIID iid, void **obj)
{
	return InternalDllGetClassObject(clsid, iid, obj);
}

#endif
