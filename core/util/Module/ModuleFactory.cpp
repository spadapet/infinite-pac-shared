#include "pch.h"
#include "Module/ModuleFactory.h"
#include "Thread/ThreadUtil.h"

static ff::ModuleFactory *s_headModuleFactory = nullptr;
static size_t s_moduleStamp = 0;

ff::ModuleFactory::ModuleFactory(
	StringRef name,
	REFGUID id,
	GetInstanceFuncType instance,
	StartupFuncType startup,
	FactoryFuncType func)
	: _name(name)
	, _id(id)
	, _cachedInstance(nullptr)
	, _instance(instance)
	, _startup(startup)
	, _func(func)
{
	LockMutex crit(GCS_MODULE_FACTORY);

	if (s_headModuleFactory != nullptr)
	{
		s_headModuleFactory->_prev = this;
	}

	_next = s_headModuleFactory;
	s_headModuleFactory = this;
	s_moduleStamp++;
}

ff::ModuleFactory::~ModuleFactory()
{
	LockMutex crit(GCS_MODULE_FACTORY);

	if (_prev != nullptr)
	{
		_prev->_next = _next;
	}

	if (_next != nullptr)
	{
		_next->_prev = _prev;
	}

	if (s_headModuleFactory == this)
	{
		s_headModuleFactory = _next;
	}

	s_moduleStamp++;
}

// static
std::unique_ptr<ff::Module> ff::ModuleFactory::Create(StringRef name)
{
	LockMutex crit(GCS_MODULE_FACTORY);

	for (const ModuleFactory *factory = s_headModuleFactory; factory != nullptr; factory = factory->_next)
	{
		if (name == factory->_name)
		{
			std::unique_ptr<Module> module = (factory->_func != nullptr)
				? factory->_func()
				: std::make_unique<ff::Module>(factory->_name, factory->_id, factory->GetInstance());
			assertRetVal(module != nullptr, module);

			InitModule(*module, factory);
			return module;
		}
	}

	assert(false);
	return std::unique_ptr<Module>();
}

// static
std::unique_ptr<ff::Module> ff::ModuleFactory::Create(REFGUID id)
{
	LockMutex crit(GCS_MODULE_FACTORY);

	for (const ModuleFactory *factory = s_headModuleFactory; factory != nullptr; factory = factory->_next)
	{
		if (id == factory->_id)
		{
			std::unique_ptr<Module> module = (factory->_func != nullptr)
				? factory->_func()
				: std::make_unique<ff::Module>(factory->_name, factory->_id, factory->GetInstance());
			assertRetVal(module != nullptr, module);

			InitModule(*module, factory);
			return module;
		}
	}

	assert(false);
	return std::unique_ptr<Module>();
}

// static
std::unique_ptr<ff::Module> ff::ModuleFactory::Create(HINSTANCE instance)
{
	assertRetVal(instance, std::unique_ptr<Module>());

	LockMutex crit(GCS_MODULE_FACTORY);

	for (const ModuleFactory *factory = s_headModuleFactory; factory != nullptr; factory = factory->_next)
	{
		if (instance == factory->GetInstance())
		{
			std::unique_ptr<Module> module = (factory->_func != nullptr)
				? factory->_func()
				: std::make_unique<ff::Module>(factory->_name, factory->_id, factory->GetInstance());
			assertRetVal(module != nullptr, module);

			InitModule(*module, factory);
			return module;
		}
	}

	assert(false);
	return std::unique_ptr<Module>();
}

// static
ff::Vector<ff::String> ff::ModuleFactory::GetAllNames()
{
	LockMutex crit(GCS_MODULE_FACTORY);
	Vector<String> names;

	for (const ModuleFactory *factory = s_headModuleFactory; factory != nullptr; factory = factory->_next)
	{
		names.Push(factory->_name);
	}

	return names;
}

// static
ff::Vector<GUID> ff::ModuleFactory::GetAllIds()
{
	LockMutex crit(GCS_MODULE_FACTORY);
	Vector<GUID> ids;

	for (const ModuleFactory *factory = s_headModuleFactory; factory != nullptr; factory = factory->_next)
	{
		ids.Push(factory->_id);
	}

	return ids;
}

// static
ff::Vector<HINSTANCE> ff::ModuleFactory::GetAllInstances()
{
	LockMutex crit(GCS_MODULE_FACTORY);
	Vector<HINSTANCE> instances;

	for (const ModuleFactory *factory = s_headModuleFactory; factory != nullptr; factory = factory->_next)
	{
		instances.Push(factory->GetInstance());
	}

	return instances;
}

// static
size_t ff::ModuleFactory::GetChangeStamp()
{
	LockMutex crit(GCS_MODULE_FACTORY);

	return s_moduleStamp;
}

// static
void ff::ModuleFactory::InitModule(Module &module, const ModuleFactory *factory)
{
	if (factory != nullptr && factory->_startup != nullptr)
	{
		for (const ModuleStartup *startup = factory->_startup(); startup != nullptr; startup = startup->_next)
		{
			assert(startup->_func != nullptr);
			if (startup->_func != nullptr)
			{
				startup->_func(module);
			}
		}
	}
}

HINSTANCE ff::ModuleFactory::GetInstance() const
{
	if (_cachedInstance == nullptr && _instance != nullptr)
	{
		LockMutex crit(GCS_MODULE_FACTORY);

		if (_cachedInstance == nullptr)
		{
			_cachedInstance = _instance();
			assert(_cachedInstance != nullptr);
		}
	}

	return _cachedInstance;
}
