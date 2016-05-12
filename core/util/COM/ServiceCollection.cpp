#include "pch.h"
#include "COM/ComAlloc.h"
#include "COM/ServiceCollection.h"
#include "Globals/Log.h"
#include "Globals/ProcessGlobals.h"

namespace ff
{
	class __declspec(uuid("47744916-f538-47b6-801a-06c590ce01e1"))
		CServiceCollection : public ComBase, public IServiceCollection
	{
	public:
		DECLARE_HEADER(CServiceCollection);

		bool Init(bool threadSafe);

		// IServiceCollection functions

		virtual bool AddService(REFGUID guidService, IUnknown *pObj) override;
		virtual bool RemoveService(REFGUID guidService) override;
		virtual void RemoveAllServices() override;

		virtual bool AddProvider(IServiceProvider *pCol) override;
		virtual bool RemoveProvider(IServiceProvider *pCol) override;
		virtual void RemoveAllProviders() override;

		// IServiceProvider

		COM_FUNC QueryService(REFGUID guidService, REFIID riid, void **ppvObject) override;

	private:
		struct SService
		{
			ComPtr<IUnknown> _service;
			ff::hash_t _guidHash;

			bool operator<(const SService &rhs) const
			{
				return _guidHash < rhs._guidHash;
			}
		};

		size_t FindService(REFGUID guidService) const;

		Mutex _cs;
		Vector<SService> _services;
		Vector<ComPtr<IServiceProvider>> _children;
	};
}

BEGIN_INTERFACES(ff::CServiceCollection)
	HAS_INTERFACE(ff::IServiceProvider)
	HAS_INTERFACE(ff::IServiceCollection)
END_INTERFACES()

bool ff::CreateServiceCollection(IServiceCollection **services)
{
	return CreateServiceCollection(nullptr, services);
}

bool ff::CreateServiceCollection(IServiceProvider *parent, IServiceCollection **services)
{
	assertRetVal(services, false);

	ComPtr<CServiceCollection, IServiceCollection> myServices;
	assertHrRetVal(ComAllocator<CServiceCollection>::CreateInstance(&myServices), false);
	assertRetVal(myServices->Init(true), false);

	if (parent != nullptr)
	{
		myServices->AddProvider(parent);
	}

	*services = myServices.Detach();
	return true;
}

ff::CServiceCollection::CServiceCollection()
{
}

ff::CServiceCollection::~CServiceCollection()
{
}

bool ff::CServiceCollection::Init(bool threadSafe)
{
	_cs.SetLockable(threadSafe);
	return true;
}

bool ff::CServiceCollection::AddService(REFGUID guidService, IUnknown *pObj)
{
	LockMutex crit(_cs);

	assertRetVal(pObj && guidService != GUID_NULL && FindService(guidService) == INVALID_SIZE, false);

	SService ss;
	ss._guidHash = ShortenGuid(guidService);
	ss._service = pObj;

	_services.SortInsert(ss);

	return true;
}

bool ff::CServiceCollection::RemoveService(REFGUID guidService)
{
	LockMutex crit(_cs);

	size_t i = FindService(guidService);
	assertRetVal(i != INVALID_SIZE, false);

	_services.Delete(i);

	return true;
}

void ff::CServiceCollection::RemoveAllServices()
{
	LockMutex crit(_cs);

	_services.Clear();
}

bool ff::CServiceCollection::AddProvider(IServiceProvider *pSP)
{
	LockMutex crit(_cs);

	assertRetVal(pSP && pSP != this, false);
	ComPtr<IServiceProvider> pAddSP = pSP;

	size_t i = _children.Find(pAddSP);
	assertRetVal(i == INVALID_SIZE, false);

	_children.Push(pAddSP);

	return true;
}

bool ff::CServiceCollection::RemoveProvider(IServiceProvider *pSP)
{
	LockMutex crit(_cs);

	ComPtr<IServiceProvider> pRemoveSP = pSP;
	size_t i = _children.Find(pRemoveSP);
	assertRetVal(i != INVALID_SIZE, false);

	_children.Delete(i);

	return true;
}

void ff::CServiceCollection::RemoveAllProviders()
{
	LockMutex crit(_cs);
	_children.Clear();
}

HRESULT ff::CServiceCollection::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
	LockMutex crit(_cs);

	size_t i = FindService(guidService);

	if (i != INVALID_SIZE)
	{
		IUnknown *pUnk = _services[i]._service;
		return pUnk->QueryInterface(riid, ppvObject);
	}
	else
	{
		// Check child service providers

		for (size_t i = 0; i < _children.Size(); i++)
		{
			HRESULT hr = _children[i]->QueryService(guidService, riid, ppvObject);

			if (SUCCEEDED(hr))
			{
				return hr;
			}
		}

		return E_NOINTERFACE;
	}
}

size_t ff::CServiceCollection::FindService(REFGUID guidService) const
{
	size_t nCount = _services.Size();

	if (nCount)
	{
		ff::hash_t nGuidHash = ShortenGuid(guidService);

		for (size_t i = 0; i < nCount; i++)
		{
			const SService &ss = _services[i];

			if (ss._guidHash == nGuidHash)
			{
				return i;
			}
		}
	}

	return INVALID_SIZE;
}
