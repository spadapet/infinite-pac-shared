#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/BufferCache.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphDeviceChild.h"
#include "Graph/GraphFactory.h"
#include "Graph/State/GraphStateCache.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Module/Module.h"

namespace ff
{
	class __declspec(uuid("dce9ac0d-79da-4456-b441-7bde392df877"))
		GraphDevice : public ComBase, public IGraphDevice
	{
	public:
		DECLARE_HEADER(GraphDevice);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;
		bool Init(IDXGIAdapterX *pCard, bool bSoftware);

		// IGraphDevice functions
		virtual bool Reset() override;
		virtual bool ResetIfNeeded() override;
		virtual bool IsSoftware() const override;

		virtual ID3D11DeviceX *Get3d() override;
		virtual ID2D1DeviceX *Get2d() override;
		virtual IDXGIDeviceX *GetDXGI() override;
		virtual ID3D11DeviceContextX *GetContext() override;
		virtual IDXGIAdapterX *GetAdapter() override;
		virtual D3D_FEATURE_LEVEL GetFeatureLevel() const override;

		virtual BufferCache &GetVertexBuffers() override;
		virtual BufferCache &GetIndexBuffers() override;
		virtual GraphStateCache &GetStateCache() override;

		virtual void AddChild(IGraphDeviceChild *child) override;
		virtual void RemoveChild(IGraphDeviceChild *child) override;

	private:
		ff::Mutex _mutex;
		ComPtr<IGraphicFactory> _factory;
		ComPtr<ID3D11DeviceX> _device;
		ComPtr<ID2D1DeviceX> _device2d;
		ComPtr<ID3D11DeviceContextX> _deviceContext;
		ComPtr<IDXGIAdapterX> _dxgiAdapter;
		ComPtr<IDXGIFactoryX> _dxgiFactory;
		ComPtr<IDXGIDeviceX> _dxgiDevice;
		Vector<IGraphDeviceChild *> _children;
		BufferCache _vertexCache;
		BufferCache _indexCache;
		GraphStateCache _stateCache;
		D3D_FEATURE_LEVEL _featureLevel;
		bool _softwareDevice;
	};

	bool CreateHardwareGraphDevice(IGraphicFactory *factory, IGraphDevice **device)
	{
		assertRetVal(device, false);

		ComPtr<GraphDevice, IGraphDevice> pDevice;
		assertHrRetVal(ComAllocator<GraphDevice>::CreateInstance(factory, &pDevice), false);

		assertRetVal(pDevice->Init(nullptr, false), false);
		*device = pDevice.Detach();

		return true;
	}

	bool CreateSoftwareGraphDevice(IGraphicFactory *factory, IGraphDevice **device)
	{
		assertRetVal(device, false);

		ComPtr<GraphDevice, IGraphDevice> pDevice;
		assertHrRetVal(ComAllocator<GraphDevice>::CreateInstance(factory, &pDevice), false);

		assertRetVal(pDevice->Init(nullptr, true), false);
		*device = pDevice.Detach();

		return true;
	}

	bool CreateGraphDevice(IGraphicFactory *factory, IDXGIAdapterX *pCard, IGraphDevice **device)
	{
		assertRetVal(pCard && device, false);

		ComPtr<GraphDevice, IGraphDevice> pDevice;
		assertHrRetVal(ComAllocator<GraphDevice>::CreateInstance(factory, &pDevice), false);

		assertRetVal(pDevice->Init(pCard, false), false);
		*device = pDevice.Detach();

		return true;
	}
}

BEGIN_INTERFACES(ff::GraphDevice)
	HAS_INTERFACE(ff::IGraphDevice)
END_INTERFACES()

ff::GraphDevice::GraphDevice()
	: _vertexCache(D3D11_BIND_VERTEX_BUFFER)
	, _indexCache(D3D11_BIND_INDEX_BUFFER)
	, _featureLevel((D3D_FEATURE_LEVEL)0)
	, _softwareDevice(false)
{
	_vertexCache.SetDevice(this);
	_indexCache.SetDevice(this);
	_stateCache.SetDevice(this);
}

ff::GraphDevice::~GraphDevice()
{
	assert(!_children.Size());

	if (_deviceContext)
	{
		_deviceContext->ClearState();
	}

	_vertexCache.SetDevice(nullptr);
	_indexCache.SetDevice(nullptr);
	_stateCache.SetDevice(nullptr);

	if (_factory)
	{
		_factory->RemoveChild(this);
	}
}

static bool InternalCreateDevice(
	IDXGIAdapterX *pCard,
	bool bSoftware,
	ID3D11DeviceX **device,
	ID3D11DeviceContextX **ppContext,
	D3D_FEATURE_LEVEL *pFeatureLevel)
{
	assertRetVal(!bSoftware || !pCard, false);
	assertRetVal(device && ppContext, false);

	D3D_DRIVER_TYPE driverType = pCard
		? D3D_DRIVER_TYPE_UNKNOWN
		: (bSoftware ? D3D_DRIVER_TYPE_WARP : D3D_DRIVER_TYPE_HARDWARE);

	UINT nFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT |
		(ff::GetThisModule().IsDebugBuild() ? D3D11_CREATE_DEVICE_DEBUG : 0);

	const D3D_FEATURE_LEVEL featureLevels[] =
	{
#if METRO_APP
		D3D_FEATURE_LEVEL_11_1,
#endif
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};

	ff::ComPtr<ID3D11Device> pDevice;
	ff::ComPtr<ID3D11DeviceX> pDeviceX;
	ff::ComPtr<ID3D11DeviceContext> pContext;
	ff::ComPtr<ID3D11DeviceContextX> pContextX;

	assertHrRetVal(D3D11CreateDevice(
		pCard, driverType, nullptr, nFlags,
		featureLevels, _countof(featureLevels),
		D3D11_SDK_VERSION, &pDevice, pFeatureLevel, &pContext), false);

	assertRetVal(pDeviceX.QueryFrom(pDevice), false);
	assertRetVal(pContextX.QueryFrom(pContext), false);

	*device = pDeviceX.Detach();
	*ppContext = pContextX.Detach();

	return true;
}

HRESULT ff::GraphDevice::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_factory.QueryFrom(unkOuter), E_INVALIDARG);
	_factory->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool ff::GraphDevice::Init(IDXGIAdapterX *pCard, bool bSoftware)
{
	assertRetVal(!bSoftware || !pCard, false);
	assertRetVal(!_device && !_deviceContext, false);

	_featureLevel = (D3D_FEATURE_LEVEL)0;
	_softwareDevice = bSoftware;

	assertRetVal(InternalCreateDevice(pCard, bSoftware, &_device, &_deviceContext, &_featureLevel), false);
	assertRetVal(GetParentDXGI(_device, __uuidof(IDXGIAdapterX), (void**)&_dxgiAdapter), false);
	assertRetVal(GetParentDXGI(_dxgiAdapter, __uuidof(IDXGIFactoryX), (void**)&_dxgiFactory), false);
	assertRetVal(_dxgiDevice.QueryFrom(_device), false);

#if METRO_APP
	ff::ComPtr<ID2D1FactoryX> factory2d = ff::ProcessGlobals::Get()->GetGraphicFactory()->GetFactory2d();
	assertRetVal(factory2d, false);
	assertHrRetVal(factory2d->CreateDevice(_dxgiDevice, &_device2d), false);
#endif

	return true;
}

ff::BufferCache &ff::GraphDevice::GetVertexBuffers()
{
	return _vertexCache;
}

ff::BufferCache &ff::GraphDevice::GetIndexBuffers()
{
	return _indexCache;
}

ff::GraphStateCache &ff::GraphDevice::GetStateCache()
{
	return _stateCache;
}

void ff::GraphDevice::AddChild(IGraphDeviceChild *child)
{
	ff::LockMutex crit(_mutex);

	assert(child && _children.Find(child) == ff::INVALID_SIZE);
	_children.Push(child);
}

void ff::GraphDevice::RemoveChild(IGraphDeviceChild *child)
{
	ff::LockMutex crit(_mutex);

	verify(_children.DeleteItem(child));
}

bool ff::GraphDevice::Reset()
{
	ComPtr<IDXGIAdapterX> adapter;
	if (_dxgiFactory->IsCurrent())
	{
		adapter = _dxgiAdapter;
	}

	_vertexCache.Reset();
	_indexCache.Reset();
	_stateCache.Reset();

	if (_deviceContext)
	{
		_deviceContext->ClearState();
		_deviceContext->Flush();
		_deviceContext = nullptr;
	}

	_device = nullptr;
	_device2d = nullptr;
	_dxgiAdapter = nullptr;
	_dxgiFactory = nullptr;
	_dxgiDevice = nullptr;

	assertRetVal(Init(adapter, _softwareDevice), false);

	bool status = true;
	for (size_t i = 0; i < _children.Size(); i++)
	{
		if (!_children[i]->Reset())
		{
			status = false;
		}
	}

	return status;
}

bool ff::GraphDevice::ResetIfNeeded()
{
	if (_dxgiFactory->IsCurrent())
	{
		return true;
	}

	ComPtr<IDXGIAdapter1> previousDefaultAdapter;
	assertHrRetVal(_dxgiFactory->EnumAdapters1(0, &previousDefaultAdapter), false);

	DXGI_ADAPTER_DESC1 previousDesc;
	assertHrRetVal(previousDefaultAdapter->GetDesc1(&previousDesc), false);
	previousDefaultAdapter = nullptr;

	// Next, get the information for the current default adapter.

	ComPtr<IDXGIAdapter1> currentDefaultAdapter;
	ComPtr<IDXGIFactoryX> currentFactory = ff::ProcessGlobals::Get()->GetGraphicFactory()->GetDXGI();
	assertHrRetVal(currentFactory && currentFactory->EnumAdapters1(0, &currentDefaultAdapter), false);

	DXGI_ADAPTER_DESC1 currentDesc;
	assertHrRetVal(currentDefaultAdapter->GetDesc1(&currentDesc), false);
	currentDefaultAdapter = nullptr;

	// If the adapter LUIDs don't match, or if the device reports that it has been removed,
	// a new D3D device must be created.

	if (previousDesc.AdapterLuid.LowPart != currentDesc.AdapterLuid.LowPart ||
		previousDesc.AdapterLuid.HighPart != currentDesc.AdapterLuid.HighPart ||
		FAILED(_device->GetDeviceRemovedReason()))
	{
		assertRetVal(Reset(), false);
	}

	return true;
}

bool ff::GraphDevice::IsSoftware() const
{
	return _softwareDevice;
}

ID3D11DeviceX *ff::GraphDevice::Get3d()
{
	return _device;
}

ID2D1DeviceX *ff::GraphDevice::Get2d()
{
	return _device2d;
}

IDXGIDeviceX *ff::GraphDevice::GetDXGI()
{
	return _dxgiDevice;
}

ID3D11DeviceContextX *ff::GraphDevice::GetContext()
{
	return _deviceContext;
}

IDXGIAdapterX *ff::GraphDevice::GetAdapter()
{
	return _dxgiAdapter;
}

D3D_FEATURE_LEVEL ff::GraphDevice::GetFeatureLevel() const
{
	assert(_featureLevel != 0);
	return _featureLevel;
}
