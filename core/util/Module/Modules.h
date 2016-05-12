#pragma once

#include "Module/Module.h"

namespace ff
{
	// Automatically creates and caches modules.
	class Modules
	{
	public:
		Modules();

		UTIL_API const Module *Get(StringRef name);
		UTIL_API const Module *Get(REFGUID id);
		UTIL_API const Module *Get(HINSTANCE instance);
		UTIL_API const Module *GetMain();

		UTIL_API Vector<HINSTANCE> GetAllInstances() const;
		UTIL_API bool AreResourcesLoading() const;

		UTIL_API const ModuleClassInfo *FindClassInfo(ff::StringRef name);
		UTIL_API const ModuleClassInfo *FindClassInfo(REFGUID classId);
		UTIL_API const ModuleClassInfo *FindClassInfoForInterface(REFGUID interfaceId);

		UTIL_API bool FindClassFactory(REFGUID classId, IClassFactory **factory);
		UTIL_API ComPtr<IUnknown> CreateClass(ff::StringRef name, AppGlobals *globals);

		UTIL_API void Clear();

	private:
		void CreateAllModules();

		Mutex _mutex;
		Vector<std::unique_ptr<Module>> _modules;
	};
}
