#pragma once

#include "Module/Module.h"

namespace ff
{
	// Use static instances of this to hook into the startup of the owner module.
	// That is your only chance to register stuff (like COM classes) with the module.
	struct ModuleStartup
	{
		typedef std::function<void(Module &)> FuncType;

		ModuleStartup(FuncType func);

		FuncType const _func;
		ModuleStartup *const _next;
	};

	// Modules need to be created on demand, so make a static instance of this struct
	// to register a function to create a module of a certain name.
	class ModuleFactory
	{
	public:
		typedef std::function<std::unique_ptr<Module>()> FactoryFuncType;
		typedef std::function<const ModuleStartup *()> StartupFuncType;
		typedef std::function<HINSTANCE()> GetInstanceFuncType;

		UTIL_API ModuleFactory(
			StringRef name,
			REFGUID id,
			GetInstanceFuncType instance,
			StartupFuncType startup,
			FactoryFuncType func = nullptr);

		UTIL_API ~ModuleFactory();

		static std::unique_ptr<Module> Create(StringRef name);
		static std::unique_ptr<Module> Create(REFGUID id);
		static std::unique_ptr<Module> Create(HINSTANCE instance);
		static Vector<String> GetAllNames();
		static Vector<GUID> GetAllIds();
		static Vector<HINSTANCE> GetAllInstances();
		static size_t GetChangeStamp(); // changes whenever an instance is added/removed

	private:
		static void InitModule(Module &module, const ModuleFactory *factory);
		HINSTANCE GetInstance() const;

		String _name;
		REFGUID _id;
		mutable HINSTANCE _cachedInstance;
		GetInstanceFuncType const _instance;
		StartupFuncType const _startup;
		FactoryFuncType const _func;
		ModuleFactory *_next;
		ModuleFactory *_prev;
	};
}
