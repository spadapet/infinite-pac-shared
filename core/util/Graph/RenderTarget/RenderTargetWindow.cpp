#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/ProcessGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/GraphTexture.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Module/ModuleFactory.h"

#if !METRO_APP

// from RenderTargetTexture.cpp
bool CreateRenderTarget(
	ff::IGraphDevice *pDevice,
	ID3D11Texture2D *pTexture,
	size_t nArrayStart,
	size_t nArrayCount,
	size_t nMipLevel,
	ID3D11RenderTargetView **ppView);

namespace ff
{
	class __declspec(uuid("e3a230cb-287b-406a-8dfd-a05073c08904"))
		RenderTargetWindow : public ComBase, public IRenderTargetWindow
	{
	public:
		DECLARE_HEADER(RenderTargetWindow);

		virtual HRESULT _Construct(IUnknown *unkOuter) override;

		bool Init(
			HWND hwnd,
			bool bFullScreen,
			DXGI_FORMAT format,
			size_t nBackBuffer,
			size_t nMultiSamples);

		// IGraphDeviceChild
		virtual IGraphDevice *GetDevice() const override;
		virtual bool Reset() override;

		// IRenderTarget
		virtual ff::PointInt GetBufferSize() const override;
		virtual ff::PointInt GetRotatedSize() const override;
		virtual int GetRotatedDegrees() const override;
		virtual double GetDpiScale() const override;
		virtual void Clear(const DirectX::XMFLOAT4 *pColor = nullptr) override;

		virtual ID3D11Texture2D *GetTexture() override;
		virtual ID3D11RenderTargetView *GetTarget() override;

		// IRenderTargetWindow
		virtual HWND GetWindow() const override;
		virtual HWND GetTopWindow() const override;
		virtual bool SetSize(PointInt size) override;
		virtual void SetAutoSize() override;

		virtual bool Present(bool vsync) override;
		virtual bool WaitForVsync() const override;

		virtual bool CanSetFullScreen() const override;
		virtual bool IsFullScreen() override;
		virtual bool SetFullScreen(bool fullScreen) override;

	private:
		void Destroy();
		bool InitSwapChain(size_t backBuffers, bool fullScreen);
		bool ResizeSwapChain(PointInt size);
		void OnWindowSize(WPARAM command = SIZE_RESTORED);
		void OnPrintScreen();
		void UpdateWindowStyle(bool fullScreen);

		ComPtr<IGraphDevice> _device;
		ComPtr<IDXGISwapChainX> _swapChain;
		ComPtr<ID3D11Texture2D> _backBuffer;
		ComPtr<ID3D11RenderTargetView> _target;
		PointInt _size;
		DXGI_FORMAT _format;
		size_t _multiSamples;

		// Window stuff
		static LRESULT CALLBACK StaticDeviceWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK StaticDeviceTopWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK DeviceWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK DeviceTopWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HWND _hwnd;
		HWND _topHwnd;
		WNDPROC _oldWndProc;
		WNDPROC _oldTopWndProc;
		bool _mainWindow;
		bool _autoSize;
		bool _modalSize;
		bool _modalMenu;
		bool _sizedWhileModal;
		bool _occluded;
	};
}

BEGIN_INTERFACES(ff::RenderTargetWindow)
	HAS_INTERFACE(ff::IRenderTarget)
	HAS_INTERFACE(ff::IRenderTargetWindow)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	static ff::StaticString name(L"RenderTargetWindow");
	module.RegisterClassT<ff::RenderTargetWindow>(name);
});

// STATIC_DATA (object)
static ff::Vector<ff::RenderTargetWindow *> s_allRenderWindows;

bool ff::CreateRenderTargetWindow(
	IGraphDevice *pDevice,
	HWND hwnd,
	bool bFullScreen,
	DXGI_FORMAT format,
	size_t nBackBuffers,
	size_t nMultiSamples,
	IRenderTargetWindow **ppRender)
{
	assertRetVal(ppRender, false);
	*ppRender = nullptr;

	ComPtr<RenderTargetWindow> pRender;
	assertHrRetVal(ComAllocator<RenderTargetWindow>::CreateInstance(pDevice, &pRender), false);
	assertRetVal(pRender->Init(hwnd, bFullScreen, format, nBackBuffers, nMultiSamples), false);

	*ppRender = pRender.Detach();
	return true;
}

// helper function to get the screen mode
static ff::PointInt GetMonitorResolution(HWND hwnd, IDXGIAdapterX *pCard, IDXGIOutputX **ppOutput)
{
	assertRetVal(hwnd, ff::PointInt(0, 0));

	ff::Vector<ff::ComPtr<IDXGIOutputX>> outputs = ff::ProcessGlobals::Get()->GetGraphicFactory()->GetOutputs(pCard);
	if (outputs.IsEmpty())
	{
		// just use the desktop monitors
		outputs = ff::ProcessGlobals::Get()->GetGraphicFactory()->GetOutputs(nullptr);
		assertRetVal(outputs.Size(), ff::PointInt::Zeros());
	}

	HMONITOR hFoundMonitor = nullptr;
	HMONITOR hTargetMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
	ff::ComPtr<IDXGIOutputX> pOutput;
	ff::PointInt size(0, 0);

	for (size_t i = 0; i < outputs.Size(); i++)
	{
		DXGI_OUTPUT_DESC desc;
		ff::ZeroObject(desc);

		if (SUCCEEDED(outputs[i]->GetDesc(&desc)) && (hFoundMonitor == nullptr || desc.Monitor == hTargetMonitor))
		{
			MONITORINFO mi;
			ff::ZeroObject(mi);
			mi.cbSize = sizeof(mi);

			if (GetMonitorInfo(desc.Monitor, &mi))
			{
				pOutput = outputs[i];
				size.SetPoint(mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top);
			}

			hFoundMonitor = desc.Monitor;
		}
	}

	assertRetVal(pOutput, ff::PointInt(0, 0));

	if (ppOutput)
	{
		*ppOutput = pOutput.Detach();
	}

	return size;
}

ff::RenderTargetWindow::RenderTargetWindow()
	: _size(0, 0)
	, _format(DXGI_FORMAT_UNKNOWN)
	, _multiSamples(0)
	, _hwnd(nullptr)
	, _topHwnd(nullptr)
	, _oldWndProc(nullptr)
	, _oldTopWndProc(nullptr)
	, _mainWindow(false)
	, _autoSize(true)
	, _modalSize(false)
	, _modalMenu(false)
	, _sizedWhileModal(false)
	, _occluded(false)
{
	s_allRenderWindows.Push(this);
}

ff::RenderTargetWindow::~RenderTargetWindow()
{
	Destroy();

	s_allRenderWindows.Delete(s_allRenderWindows.Find(this));
	if (s_allRenderWindows.IsEmpty())
	{
		s_allRenderWindows.Reduce();
	}

	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT ff::RenderTargetWindow::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool ff::RenderTargetWindow::Init(
	HWND hwnd,
	bool bFullScreen,
	DXGI_FORMAT format,
	size_t nBackBuffers,
	size_t nMultiSamples)
{
	assertRetVal(_device && hwnd, false);

	_hwnd = hwnd;
	_topHwnd = ff::WindowHasStyle(hwnd, WS_CHILD) ? GetAncestor(hwnd, GA_ROOT) : hwnd;
	_mainWindow = (_hwnd == _topHwnd);
	_format = format;
	_multiSamples = nMultiSamples;
	_oldWndProc = SubclassWindow(_hwnd, StaticDeviceWindowProc);
	_oldTopWndProc = SubclassWindow(_topHwnd, StaticDeviceTopWindowProc);

	assertRetVal(InitSwapChain(nBackBuffers, bFullScreen), false);

	return true;
}

void ff::RenderTargetWindow::Destroy()
{
	if (_topHwnd && _oldTopWndProc)
	{
		SubclassWindow(_topHwnd, _oldTopWndProc);
		_oldTopWndProc = nullptr;
		_topHwnd = nullptr;
	}

	if (_hwnd && _oldWndProc)
	{
		SubclassWindow(_hwnd, _oldWndProc);
		_oldWndProc = nullptr;
		_hwnd = nullptr;
	}

	if (_swapChain)
	{
		_swapChain->SetFullscreenState(FALSE, nullptr);
	}
}

void ff::RenderTargetWindow::UpdateWindowStyle(bool fullScreen)
{
	if (_mainWindow)
	{
		LONG_PTR style = GetWindowStyle(_hwnd);

		if (fullScreen)
		{
			style &= ~WS_OVERLAPPEDWINDOW;
		}
		else
		{
			style |= WS_OVERLAPPEDWINDOW;
		}

		SetWindowLongPtr(_hwnd, GWL_STYLE, style);
	}
}

bool ff::RenderTargetWindow::InitSwapChain(size_t nBackBuffers, bool bFullScreen)
{
	// Get info about the active monitor
	ComPtr<IDXGIOutputX> pOutput;
	ComPtr<IDXGIAdapterX> pCard = _device->GetAdapter();
	PointInt monitorSize = GetMonitorResolution(_hwnd, _device->IsSoftware() ? pCard : nullptr, &pOutput);

	// Figure out the size of the back buffer

	UpdateWindowStyle(bFullScreen);

	if (bFullScreen)
	{
		_size = monitorSize;
	}
	else if (!_size.x || !_size.y)
	{
		_size = GetClientSize(_hwnd);
	}

	_size.x = std::max(8, _size.x);
	_size.y = std::max(8, _size.y);

	// Figure out the properties of the swap chain

	DXGI_SWAP_CHAIN_DESC desc;
	ZeroObject(desc);

	if (_format == DXGI_FORMAT_UNKNOWN)
	{
		_format = DXGI_FORMAT_R8G8B8A8_UNORM;

		if (pOutput)
		{
			DXGI_MODE_DESC closeMode;
			ZeroObject(closeMode);

			if (SUCCEEDED(pOutput->FindClosestMatchingMode(&desc.BufferDesc, &closeMode, _device->Get3d())))
			{
				_format = closeMode.Format;
			}
		}
	}

	desc.BufferCount = (UINT)std::max<size_t>(1, nBackBuffers);
	desc.BufferDesc.Width = _size.x;
	desc.BufferDesc.Height = _size.y;
	desc.BufferDesc.Format = _format;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.Flags = 0; // DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
	desc.OutputWindow = _hwnd;
	desc.SampleDesc.Count = (UINT)_multiSamples;
	desc.SampleDesc.Quality = 0;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.Windowed = !bFullScreen;

	// Create the swap chain
	ComPtr<IDXGIFactoryX> pFactory;

#ifdef _DEBUG
	// These D3D hooks will crash if the actual DXGI parent of pCard is used
	if (GetModuleHandle(L"VsGraphicsHelper.dll") || GetModuleHandle(L"PIXHelper.dll"))
	{
		pFactory = ProcessGlobals::Get()->GetGraphicFactory()->GetDXGI();
	}
	else
#endif
	{
		assertRetVal(GetParentDXGI(pCard, __uuidof(IDXGIFactoryX), (void**)&pFactory), false);
	}

	assertRetVal(pFactory, false);
	assertHrRetVal(pFactory->CreateSwapChain(_device->Get3d(), &desc, &_swapChain), false);
	assertHrRetVal(pFactory->MakeWindowAssociation(_hwnd, DXGI_MWA_NO_WINDOW_CHANGES), false);

	return true;
}

bool ff::RenderTargetWindow::ResizeSwapChain(PointInt size)
{
	if (_swapChain && _size != size)
	{
		_size = size;
		_target = nullptr;
		_backBuffer = nullptr;

		_device->GetContext()->ClearState();

		size.x = std::max(size.x, 8);
		size.y = std::max(size.y, 8);

		assertHrRetVal(_swapChain->ResizeBuffers(0, size.x, size.y, DXGI_FORMAT_UNKNOWN, 0), false);
	}

	return true;
}

ff::IGraphDevice *ff::RenderTargetWindow::GetDevice() const
{
	return _device;
}

bool ff::RenderTargetWindow::Reset()
{
	assertRetVal(_swapChain, false);

	DXGI_SWAP_CHAIN_DESC desc;
	ZeroObject(desc);
	_swapChain->GetDesc(&desc);

	// Can't be in full screen mode when releasing the swap chain
	// or else DXGI might crash (at least it does on my dev machine)
	bool bFullScreen = IsFullScreen();
	SetFullScreen(false);

	_target = nullptr;
	_swapChain = nullptr;
	_backBuffer = nullptr;

	_occluded = false;
	_modalSize = false;
	_modalMenu = false;

	assertRetVal(InitSwapChain(desc.BufferCount, bFullScreen), false);
	return true;
}

ff::PointInt ff::RenderTargetWindow::GetBufferSize() const
{
	return _size;
}

ff::PointInt ff::RenderTargetWindow::GetRotatedSize() const
{
	return _size;
}

int ff::RenderTargetWindow::GetRotatedDegrees() const
{
	return 0;
}

double ff::RenderTargetWindow::GetDpiScale() const
{
	return 1.0;
}

void ff::RenderTargetWindow::Clear(const DirectX::XMFLOAT4 *pColor)
{
	if (GetTarget())
	{
		static const DirectX::XMFLOAT4 defaultColor(0, 0, 0, 1);
		const DirectX::XMFLOAT4 *pUseColor = pColor ? pColor : &defaultColor;

		_device->GetContext()->ClearRenderTargetView(GetTarget(), &pUseColor->x);
	}
}

ID3D11Texture2D *ff::RenderTargetWindow::GetTexture()
{
	assertRetVal(_swapChain, nullptr);

	_backBuffer = nullptr;
	assertHrRetVal(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&_backBuffer), nullptr);

	return _backBuffer;
}

ID3D11RenderTargetView *ff::RenderTargetWindow::GetTarget()
{
	if (_occluded || !_hwnd)
	{
		return nullptr;
	}

	if (!_target && GetTexture())
	{
		assertRetVal(CreateRenderTarget(_device, GetTexture(), 0, 1, 0, &_target), false);
	}

	return _target;
}

HWND ff::RenderTargetWindow::GetWindow() const
{
	return _hwnd;
}

HWND ff::RenderTargetWindow::GetTopWindow() const
{
	return _topHwnd;
}

bool ff::RenderTargetWindow::SetSize(PointInt size)
{
	_autoSize = false;

	assertRetVal(ResizeSwapChain(size), false);

	return true;
}

void ff::RenderTargetWindow::SetAutoSize()
{
	_autoSize = true;

	OnWindowSize();
}

bool ff::RenderTargetWindow::Present(bool vsync)
{
	assertRetVal(_swapChain, true);

	UINT nSync = (vsync && !_occluded) ? 1 : 0;
	UINT nFlags = _occluded ? DXGI_PRESENT_TEST : 0;

	switch (_swapChain->Present(nSync, nFlags))
	{
	case S_OK:
		_occluded = false;
		break;

	case DXGI_STATUS_OCCLUDED:
		_occluded = true;
		break;

	case DXGI_ERROR_DEVICE_RESET:
	case DXGI_ERROR_DEVICE_REMOVED:
		return _device->Reset();

	default:
		assert(false);
		break;
	}

	return true;
}

bool ff::RenderTargetWindow::WaitForVsync() const
{
	assertRetVal(_swapChain, false);

	ComPtr<IDXGIOutputX> pOutput;
	if (SUCCEEDED(_swapChain->GetContainingOutput(&pOutput)))
	{
		return SUCCEEDED(pOutput->WaitForVBlank());
	}

	return false;
}

bool ff::RenderTargetWindow::CanSetFullScreen() const
{
	Vector<ComPtr<IDXGIOutputX>> outputs;

	if (_swapChain && _mainWindow)
	{
		outputs = ProcessGlobals::Get()->GetGraphicFactory()->GetOutputs(_device->GetAdapter());
	}

	return !outputs.IsEmpty();
}

bool ff::RenderTargetWindow::IsFullScreen()
{
	if (_swapChain && _mainWindow)
	{
		ComPtr<IDXGIOutputX> pOutput;
		BOOL bFullScreen = FALSE;
	
		return SUCCEEDED(_swapChain->GetFullscreenState(&bFullScreen, &pOutput)) && bFullScreen;
	}

	return false;
}

bool ff::RenderTargetWindow::SetFullScreen(bool bFullScreen)
{
	assertRetVal(_swapChain, false);

	if (CanSetFullScreen())
	{
		UpdateWindowStyle(bFullScreen);

		if (SUCCEEDED(_swapChain->SetFullscreenState(bFullScreen, nullptr)))
		{
			return true;
		}
		else
		{
			UpdateWindowStyle(IsFullScreen());
			assertRetVal(false, false);
		}
	}

	return false;
}

// static
LRESULT CALLBACK ff::RenderTargetWindow::StaticDeviceWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	for (size_t h = 0; h < s_allRenderWindows.Size(); h++)
	{
		if (s_allRenderWindows[h]->GetWindow() == hwnd)
		{
			return s_allRenderWindows[h]->DeviceWindowProc(hwnd, msg, wParam, lParam);
		}
	}

	assert(false);
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

// static
LRESULT CALLBACK ff::RenderTargetWindow::StaticDeviceTopWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	for (size_t h = 0; h < s_allRenderWindows.Size(); h++)
	{
		if (s_allRenderWindows[h]->GetTopWindow() == hwnd)
		{
			return s_allRenderWindows[h]->DeviceTopWindowProc(hwnd, msg, wParam, lParam);
		}
	}

	assert(false);
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ff::RenderTargetWindow::DeviceWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// In case a message handler destroys this device
	WNDPROC pOldWndProc = _oldWndProc;

	switch (msg)
	{
	case WM_SIZE:
		OnWindowSize(wParam);
		break;

	case WM_DESTROY:
		Destroy();
		break;

	case WM_KEYDOWN:
		if ((wParam == VK_LWIN || wParam == VK_RWIN) && IsFullScreen())
		{
			SetFullScreen(false);
		}
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		if (wParam == VK_SNAPSHOT && IsFullScreen())
		{
			OnPrintScreen();
		}
		break;

	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN) // ALT-ENTER to toggle full screen mode
		{
			SetFullScreen(!IsFullScreen());
			return 0;
		}
		break;

	case WM_SYSCHAR:
		if (wParam == VK_RETURN)
		{
			// prevent a 'ding' sound when switching between modes
			return 0;
		}
		break;
	}

	if (msg == ff::CustomWindow::GetDetachMessage())
	{
		Destroy();
	}

	return pOldWndProc
		? CallWindowProc(pOldWndProc, hwnd, msg, wParam, lParam)
		: DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK ff::RenderTargetWindow::DeviceTopWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// In case a message handler destroys this device
	WNDPROC pOldWndProc = _oldTopWndProc;

	switch (msg)
	{
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE && _mainWindow && IsFullScreen())
		{
			SetFullScreen(false);
		}
		break;

	case WM_ENTERMENULOOP:
		_modalMenu = true;
		break;

	case WM_EXITMENULOOP:
		_modalMenu = false;
		break;

	case WM_ENTERSIZEMOVE:
		_modalSize = true;
		break;

	case WM_EXITSIZEMOVE:
		_modalSize = false;

		if (_sizedWhileModal)
		{
			OnWindowSize();
		}
		break;
	}

	return pOldWndProc
		? CallWindowProc(pOldWndProc, hwnd, msg, wParam, lParam)
		: DefWindowProc(hwnd, msg, wParam, lParam);
}

void ff::RenderTargetWindow::OnWindowSize(WPARAM command)
{
	switch (command)
	{
	case SIZE_MAXIMIZED:
	case SIZE_RESTORED:
		{
			bool bResize = false;

			if (_modalSize)
			{
				_sizedWhileModal = true;
			}
			else if (_autoSize)
			{
				bResize = true;
			}

			if (bResize)
			{
				_sizedWhileModal = false;

				PointInt size = GetClientSize(_hwnd);
				ResizeSwapChain(size);
			}
		}
		break;
	}
}

void ff::RenderTargetWindow::OnPrintScreen()
{
	assertRet(_swapChain && IsFullScreen());

	// Copy the screen to a new texture
	ComPtr<IDXGIOutputX> pOutput;
	assertRet(SUCCEEDED(_swapChain->GetContainingOutput(&pOutput)));

	DXGI_OUTPUT_DESC desc;
	assertRet(SUCCEEDED(pOutput->GetDesc(&desc)));

	RectInt desktopRect(desc.DesktopCoordinates);
	PointInt size(desktopRect.Size());
	int nDestPitch = size.x * 4;

	ComPtr<IGraphTexture> pTexture;
	assertRet(CreateStagingTexture(_device, size, DXGI_FORMAT_B8G8R8A8_UNORM, true, true, &pTexture));

	ComPtr<IDXGISurfaceX> pSurface;
	assertRet(pSurface.QueryFrom(pTexture->GetTexture()));
	assertRet(SUCCEEDED(pOutput->GetDisplaySurfaceData(pSurface)));

	// Create a new DIB in memory
	Vector<BYTE> data;
	data.Resize(size.y * nDestPitch + sizeof(BITMAPINFOHEADER));
	ZeroMemory(data.Data(), data.ByteSize());

	BITMAPINFO *pInfo = (BITMAPINFO*)data.Data();
	pInfo->bmiHeader.biSize = sizeof(pInfo->bmiHeader);
	pInfo->bmiHeader.biWidth = size.x;
	pInfo->bmiHeader.biHeight = size.y;
	pInfo->bmiHeader.biPlanes = 1;
	pInfo->bmiHeader.biBitCount = 32;
	pInfo->bmiHeader.biCompression = BI_RGB;

	// Map the texture to memory and copy it to the DIB (flip vertically)
	{
		D3D11_MAPPED_SUBRESOURCE map;
		assertRet(SUCCEEDED(_device->GetContext()->Map(pTexture->GetTexture(), 0, D3D11_MAP_READ, 0, &map)));
		LPBYTE pSourceData = (LPBYTE)map.pData;
		LPBYTE pDestData = (LPBYTE)pInfo->bmiColors;

		for (int y = 0; y < size.y; y++)
		{
			int fromY = size.y - y - 1;
			CopyMemory(&pDestData[y * nDestPitch], &pSourceData[fromY * map.RowPitch], nDestPitch);
		}

		_device->GetContext()->Unmap(pTexture->GetTexture(), 0);
		pTexture = nullptr;
	}

	// Copy the DIB memory to globally allocated memory
	{
		HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, data.ByteSize());
		assertRet(hGlobal);

		CopyMemory(GlobalLock(hGlobal), data.Data(), data.ByteSize());
		GlobalUnlock(hGlobal);

		// and move it to the clipboard
		if (OpenClipboard(_hwnd))
		{
			if (EmptyClipboard())
			{
				SetClipboardData(CF_DIB, hGlobal);
				hGlobal = nullptr;
			}

			CloseClipboard();
		}

		if (hGlobal)
		{
			GlobalFree(hGlobal);
			hGlobal = nullptr;
		}
	}
}

#endif // !METRO_APP
