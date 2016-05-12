#pragma once
// Include this in your module's main CPP file only! (through MainUtilInclude.h)

#include "Core/DelayLoadInclude.h"
#include "Globals/ProcessGlobals.h"
#include "Module/Module.h"
#include "Module/Modules.h"
#include "Module/ModuleFactory.h"

namespace ff
{
	namespace ModuleUtil
	{
		static ff::ModuleStartup *s_moduleStartup = nullptr;
		static String s_moduleName;
		static GUID s_moduleId = GUID_NULL;
		static HINSTANCE s_moduleInstance = nullptr;
	}

	static void SetThisModule(StringRef name, REFGUID id, HINSTANCE instance)
	{
		assert(name.size() && id != GUID_NULL);
		ModuleUtil::s_moduleName = name;
		ModuleUtil::s_moduleId = id;
		ModuleUtil::s_moduleInstance = instance;

		ff::SetDelayLoadInstance(instance);
	}

	static void SetMainModule(StringRef name, REFGUID id, HINSTANCE instance)
	{
		SetThisModule(name, id, instance);
		SetMainModuleInstance(instance);
	}

	static ModuleStartup *GetModuleStartup()
	{
		return ModuleUtil::s_moduleStartup;
	}
}

const ff::Module &ff::GetThisModule()
{
	const Module *module = ProcessGlobals::Get()->GetModules().Get(ModuleUtil::s_moduleId);
	assert(module);
	return *module;
}

const ff::Module *ff::GetMainModule()
{
	return ff::ProcessGlobals::Get()->GetModules().GetMain();
}

ff::ModuleStartup::ModuleStartup(FuncType func)
	: _func(func)
	, _next(ModuleUtil::s_moduleStartup)
{
	ModuleUtil::s_moduleStartup = this;
}
