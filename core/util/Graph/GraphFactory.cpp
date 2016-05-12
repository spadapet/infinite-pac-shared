#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"

class __declspec(uuid("b38739e9-19e5-4cec-8065-6bb784f7ad39"))
	GraphicFactory : public ff::ComBase, public ff::IGraphicFactory
{
public:
	DECLARE_HEADER(GraphicFactory);

	bool Init();

	// IGraphicFactory
	virtual IDXGIFactoryX *GetDXGI() override;
	virtual ID2D1FactoryX *GetFactory2d() override;

	virtual bool CreateDevice(IDXGIAdapterX *pCard, ff::IGraphDevice **device) override;
	virtual bool CreateSoftwareDevice(ff::IGraphDevice **device) override;

	virtual size_t GetDeviceCount() const override;
	virtual ff::IGraphDevice *GetDevice(size_t nIndex) const override;

	virtual ff::Vector<ff::ComPtr<IDXGIAdapterX>> GetAdapters() override;
	virtual ff::Vector<ff::ComPtr<IDXGIOutputX>> GetOutputs(IDXGIAdapterX *pCard) override;

	virtual void AddChild(ff::IGraphDevice *child) override;
	virtual void RemoveChild(ff::IGraphDevice *child) override;

#if !METRO_APP
	virtual bool GetAdapterForWindow(HWND hwnd, IDXGIAdapterX **ppCard, IDXGIOutputX **ppOutput) override;
	virtual bool GetAdapterForMonitor(HMONITOR hMonitor, IDXGIAdapterX **ppCard, IDXGIOutputX **ppOutput) override;
#endif

private:
	ff::Mutex _mutex;
	ff::ComPtr<IDXGIFactoryX> _dxgi;
	ff::ComPtr<ID2D1FactoryX> _factory2d;
	ff::Vector<ff::IGraphDevice *> _devices;
};

namespace ff
{
	bool CreateHardwareGraphDevice(IGraphicFactory *factory, ff::IGraphDevice **device);
	bool CreateSoftwareGraphDevice(IGraphicFactory *factory, ff::IGraphDevice **device);
	bool CreateGraphDevice(IGraphicFactory *factory, IDXGIAdapterX *pCard, ff::IGraphDevice **device);
}

BEGIN_INTERFACES(GraphicFactory)
	HAS_INTERFACE(ff::IGraphicFactory)
END_INTERFACES()

bool ff::CreateGraphicFactory(ff::IGraphicFactory **ppObj)
{
	assertRetVal(ppObj, false);
	*ppObj = nullptr;

	ComPtr<GraphicFactory, ff::IGraphicFactory> pObj;
	assertHrRetVal(ff::ComAllocator<GraphicFactory>::CreateInstance(&pObj), false);
	assertRetVal(pObj->Init(), false);

	*ppObj = pObj.Detach();
	return *ppObj != nullptr;
}

bool ff::GetParentDXGI(IUnknown *pObject, REFGUID iid, void **ppParent)
{
	assertRetVal(pObject && ppParent, false);

	ComPtr<IDXGIObject> pObjDXGI;
	assertRetVal(pObjDXGI.QueryFrom(pObject), false);

	ComPtr<IDXGIObject> pParentDXGI;
	assertHrRetVal(pObjDXGI->GetParent(__uuidof(IDXGIObject), (void**)&pParentDXGI) && pParentDXGI, false);

	assertHrRetVal(pParentDXGI->QueryInterface(iid, ppParent), false);
	return true;
}

GraphicFactory::GraphicFactory()
{
}

GraphicFactory::~GraphicFactory()
{
}

bool GraphicFactory::Init()
{
	return true;
}

size_t GraphicFactory::GetDeviceCount() const
{
	return _devices.Size();
}

ff::IGraphDevice *GraphicFactory::GetDevice(size_t nIndex) const
{
	assertRetVal(nIndex >= 0 && nIndex < _devices.Size(), nullptr);
	return _devices[nIndex];
}

IDXGIFactoryX *GraphicFactory::GetDXGI()
{
	if (_dxgi && _dxgi->IsCurrent() != S_OK)
	{
		ff::LockMutex crit(_mutex);

		if (_dxgi && _dxgi->IsCurrent() != S_OK)
		{
			_dxgi = nullptr;
		}
	}

	if (!_dxgi)
	{
		ff::LockMutex crit(_mutex);

		if (!_dxgi)
		{
			assertHrRetVal(::CreateDXGIFactory1(__uuidof(IDXGIFactoryX), (void**)&_dxgi), nullptr);
		}
	}

	return _dxgi;
}

ID2D1FactoryX *GraphicFactory::GetFactory2d()
{
	if (!_factory2d)
	{
		ff::LockMutex crit(_mutex);

		if (!_factory2d)
		{
			D2D1_FACTORY_OPTIONS options;
			options.debugLevel = ff::GetThisModule().IsDebugBuild() ? D2D1_DEBUG_LEVEL_NONE : D2D1_DEBUG_LEVEL_WARNING;
			assertHrRetVal(::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, __uuidof(ID2D1FactoryX), &options, (void**)&_factory2d), nullptr);
		}
	}

	return _factory2d;
}

bool GraphicFactory::CreateDevice(IDXGIAdapterX *pCard, ff::IGraphDevice **device)
{
	assertRetVal(device && GetDXGI(), false);
	*device = nullptr;

	ff::ComPtr<ff::IGraphDevice> pDevice;
	if (pCard)
	{
		assertRetVal(ff::CreateGraphDevice(this, pCard, &pDevice), false);
	}
	else
	{
		assertRetVal(ff::CreateHardwareGraphDevice(this, &pDevice), false);
	}

	*device = pDevice.Detach();

	return true;
}

bool GraphicFactory::CreateSoftwareDevice(ff::IGraphDevice **device)
{
	assertRetVal(device && GetDXGI(), false);
	*device = nullptr;

	ff::ComPtr<ff::IGraphDevice> pDevice;
	assertRetVal(ff::CreateGraphDevice(this, nullptr, &pDevice), false);

	*device = pDevice.Detach();

	return true;
}

ff::Vector<ff::ComPtr<IDXGIAdapterX>> GraphicFactory::GetAdapters()
{
	ff::Vector<ff::ComPtr<IDXGIAdapterX>> cards;

	ff::ComPtr<IDXGIAdapter1> pCard;
	for (UINT i = 0; GetDXGI() && SUCCEEDED(GetDXGI()->EnumAdapters1(i++, &pCard)); pCard = nullptr)
	{
		ff::ComPtr<IDXGIAdapterX> pCardX;
		if (pCardX.QueryFrom(pCard))
		{
			cards.Push(pCardX);
		}
	}

	return cards;
}

ff::Vector<ff::ComPtr<IDXGIOutputX>> GraphicFactory::GetOutputs(IDXGIAdapterX *pCard)
{
	ff::ComPtr<IDXGIOutput> pOutput;
	ff::ComPtr<IDXGIAdapter1> pDefaultCard;
	ff::ComPtr<IDXGIAdapterX> pDefaultCardX;
	ff::Vector<ff::ComPtr<IDXGIOutputX>> outputs;

	if (!pCard && SUCCEEDED(GetDXGI()->EnumAdapters1(0, &pDefaultCard)))
	{
		if (pDefaultCardX.QueryFrom(pDefaultCard))
		{
			pCard = pDefaultCardX;
		}
	}

	for (UINT i = 0; pCard && SUCCEEDED(pCard->EnumOutputs(i++, &pOutput)); pOutput = nullptr)
	{
		ff::ComPtr<IDXGIOutputX> pOutputX;
		if (pOutputX.QueryFrom(pOutput))
		{
			outputs.Push(pOutputX);
		}
	}

	return outputs;
}

void GraphicFactory::AddChild(ff::IGraphDevice *child)
{
	ff::LockMutex crit(_mutex);

	assert(child && _devices.Find(child) == ff::INVALID_SIZE);
	_devices.Push(child);
}

void GraphicFactory::RemoveChild(ff::IGraphDevice *child)
{
	ff::LockMutex crit(_mutex);

	verify(_devices.DeleteItem(child));
}

#if !METRO_APP
bool GraphicFactory::GetAdapterForWindow(HWND hwnd, IDXGIAdapterX **ppCard, IDXGIOutput **ppOutput)
{
	assertRetVal(hwnd, false);

	HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
	return GetAdapterForMonitor(hMonitor, ppCard, ppOutput);
}
#endif

#if !METRO_APP
static bool DoesAdapterUseMonitor(IDXGIAdapterX *pCard, HMONITOR hMonitor, IDXGIOutput **ppOutput)
{
	assertRetVal(pCard, false);

	ff::ComPtr<IDXGIOutputX> pOutput;
	for (UINT i = 0; pCard && SUCCEEDED(pCard->EnumOutputs(i, &pOutput)); i++, pOutput = nullptr)
	{
		DXGI_OUTPUT_DESC desc;
		ff::ZeroObject(desc);

		if (SUCCEEDED(pOutput->GetDesc(&desc)) && desc.Monitor == hMonitor)
		{
			if (ppOutput)
			{
				*ppOutput = pOutput.Detach();
			}

			return true;
		}
	}

	return false;
}
#endif

#if !METRO_APP
bool GraphicFactory::GetAdapterForMonitor(HMONITOR hMonitor, IDXGIAdapterX **ppCard, IDXGIOutputX **ppOutput)
{
	assertRetVal(hMonitor, false);

	// Try the desktop first
	{
		ff::ComPtr<ff::IGraphDevice> pDevice;
		if (CreateHardwareGraphDevice(this, &pDevice))
		{
			if (DoesAdapterUseMonitor(pDevice->GetAdapter(), hMonitor, ppOutput))
			{
				if (ppCard)
				{
					*ppCard = ff::GetAddRef(pDevice->GetAdapter());
				}

				return true;
			}
		}
	}

	ff::Vector<ff::ComPtr<IDXGIAdapterX>> cards = GetAdapters();
	for (size_t i = 0; i < cards.Size(); i++)
	{
		if (DoesAdapterUseMonitor(cards[i], hMonitor, ppOutput))
		{
			if (ppCard)
			{
				*ppCard = ff::GetAddRef<IDXGIAdapterX>(cards[i]);
			}

			return true;
		}
	}

	assertRetVal(false, false);
}
#endif
