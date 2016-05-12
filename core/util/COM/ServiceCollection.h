#pragma once

namespace ff
{
	class __declspec(uuid("57dd646a-0d90-4be9-be6a-936678c21ce9")) __declspec(novtable)
		IServiceProvider : public IUnknown
	{
	public:
		COM_FUNC QueryService(REFGUID guidService, REFIID riid, void **ppvObject) = 0;
	};

	class __declspec(uuid("58f51830-33c4-42fb-98c6-4f1fc1f97ad8")) __declspec(novtable)
		IServiceCollection : public IServiceProvider
	{
	public:
		virtual bool AddService(REFGUID guidService, IUnknown *pObj) = 0;
		virtual bool RemoveService(REFGUID guidService) = 0;
		virtual void RemoveAllServices() = 0;

		virtual bool AddProvider(IServiceProvider *pSP) = 0;
		virtual bool RemoveProvider(IServiceProvider *pSP) = 0;
		virtual void RemoveAllProviders() = 0;
	};

	UTIL_API bool CreateServiceCollection(IServiceCollection **services);
	UTIL_API bool CreateServiceCollection(IServiceProvider *parent, IServiceCollection **services);

	template<typename T>
	bool GetService(IServiceProvider *pSP, REFGUID guidService, T **ppService)
	{
		assertRetVal(ppService, false);
		*ppService = nullptr;

		assertRetVal(pSP, false);
		return SUCCEEDED(pSP->QueryService(guidService, __uuidof(T), (void**)ppService));
	}

	template<typename T>
	bool GetService(IServiceProvider *pSP, T **ppService)
	{
		return GetService<T>(pSP, __uuidof(T), ppService);
	}

	template<typename T>
	bool GetGlobalService(REFGUID guidService, T **ppService)
	{
		return GetService<T>(ProcessGlobals::Get()->GetServices(), guidService, ppService);
	}

	template<typename T>
	bool GetGlobalService(T **ppService)
	{
		return GetService<T>(ProcessGlobals::Get()->GetServices(), __uuidof(T), ppService);
	}
}
