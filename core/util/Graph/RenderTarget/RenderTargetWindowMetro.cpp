#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/MetroGlobals.h"
#include "Graph/GraphDevice.h"
#include "Graph/GraphFactory.h"
#include "Graph/RenderTarget/RenderTarget.h"
#include "Module/Module.h"
#include "Module/ModuleFactory.h"
#include "Thread/ThreadDispatch.h"
#include "Thread/ThreadUtil.h"

#if METRO_APP

#include "Windows.UI.XAML.Media.DxInterop.h"

class __declspec(uuid("52e3f024-2b70-4e42-afe2-1070ab22f42d"))
	RenderTargetWindowMetro : public ff::ComBase, public ff::IRenderTargetWindow
{
public:
	DECLARE_HEADER(RenderTargetWindowMetro);

	virtual HRESULT _Construct(IUnknown *unkOuter) override;
	bool Init(Windows::UI::Xaml::Window ^window);

	// IGraphDeviceChild
	virtual ff::IGraphDevice *GetDevice() const override;
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
	virtual Windows::UI::Xaml::Window ^GetWindow() const override;

	virtual bool Present(bool vsync) override;
	virtual bool WaitForVsync() const override;

	virtual bool CanSetFullScreen() const override;
	virtual bool IsFullScreen() override;
	virtual bool SetFullScreen(bool fullScreen) override;

	virtual bool UpdateSwapChain(
		ff::PointInt windowSize,
		ff::PointInt panelSize,
		ff::PointDouble panelCompositionScale,
		Windows::Graphics::Display::DisplayOrientations nativeOrientation,
		Windows::Graphics::Display::DisplayOrientations currentOrientation) override;

private:
	ff::ComPtr<ff::IGraphDevice> _device;
	ff::ComPtr<IDXGISwapChainX> _swapChain;
	ff::ComPtr<ID3D11Texture2D> _backBuffer;
	ff::ComPtr<ID3D11RenderTargetView> _target;

	Windows::UI::Xaml::Window ^_window;
	Windows::UI::Xaml::UIElement ^_originalContent;
	Windows::UI::Xaml::Controls::SwapChainPanel ^_panel;
	Windows::Graphics::Display::DisplayInformation ^_displayInfo;
	Windows::Graphics::Display::DisplayOrientations _nativeOrientation;
	Windows::Graphics::Display::DisplayOrientations _currentOrientation;
	Windows::UI::ViewManagement::ApplicationView ^_view;
	ff::PointInt _windowSize;
	ff::PointInt _panelSize;
	ff::PointDouble _panelScale;
	bool _cachedFullScreenMode;
	bool _fullScreenMode;
};

BEGIN_INTERFACES(RenderTargetWindowMetro)
	HAS_INTERFACE(ff::IRenderTarget)
	HAS_INTERFACE(ff::IRenderTargetWindow)
	HAS_INTERFACE(ff::IGraphDeviceChild)
END_INTERFACES()

static ff::ModuleStartup Register([](ff::Module &module)
{
	ff::StaticString name(L"RenderTargetWindow");
	module.RegisterClassT<RenderTargetWindowMetro>(name);
});

bool ff::CreateRenderTargetWindow(
	IGraphDevice *pDevice,
	Windows::UI::Xaml::Window ^hwnd,
	bool fullScreen, // ignored
	DXGI_FORMAT format, // ignored
	size_t backBuffers, // ignored
	size_t multiSamples, // ignored
	ff::IRenderTargetWindow **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ff::ComPtr<RenderTargetWindowMetro, ff::IRenderTargetWindow> myObj;
	assertHrRetVal(ff::ComAllocator<RenderTargetWindowMetro>::CreateInstance(pDevice, &myObj), false);
	assertRetVal(myObj->Init(hwnd), false);

	*obj = myObj.Detach();
	return true;
}

RenderTargetWindowMetro::RenderTargetWindowMetro()
	: _windowSize(0, 0)
	, _panelSize(0, 0)
	, _panelScale(1, 1)
	, _cachedFullScreenMode(false)
	, _fullScreenMode(false)
	, _nativeOrientation(Windows::Graphics::Display::DisplayOrientations::None)
	, _currentOrientation(Windows::Graphics::Display::DisplayOrientations::None)
{
}

RenderTargetWindowMetro::~RenderTargetWindowMetro()
{
	if (_device)
	{
		_device->RemoveChild(this);
	}
}

HRESULT RenderTargetWindowMetro::_Construct(IUnknown *unkOuter)
{
	assertRetVal(_device.QueryFrom(unkOuter), E_FAIL);
	_device->AddChild(this);

	return __super::_Construct(unkOuter);
}

bool RenderTargetWindowMetro::Init(Windows::UI::Xaml::Window ^window)
{
	assertRetVal(_device && window != nullptr, false);

	_window = window;
	_displayInfo = Windows::Graphics::Display::DisplayInformation::GetForCurrentView();
	_view = Windows::UI::ViewManagement::ApplicationView::GetForCurrentView();

	auto windowContent = dynamic_cast<Windows::UI::Xaml::Controls::UserControl ^>(_window->Content);
	assertRetVal(windowContent, false);

	_originalContent = windowContent;
	_panel = dynamic_cast<Windows::UI::Xaml::Controls::SwapChainPanel ^>(windowContent->Content);
	assertRetVal(_panel, false);

	ff::PointInt windowSize = ff::GetClientSize(_window);
	ff::PointDouble panelScale(_panel->CompositionScaleX, _panel->CompositionScaleY);
	ff::PointInt panelSize((int)(panelScale.x * _panel->ActualWidth), (int)(panelScale.y * _panel->ActualHeight));

	assertRetVal(UpdateSwapChain(
		windowSize, panelSize, panelScale,
		_displayInfo->NativeOrientation,
		_displayInfo->CurrentOrientation), false);

	return true;
}

// This method determines the rotation between the display device's native orientation and the
// current display orientation.
static DXGI_MODE_ROTATION ComputeDisplayRotation(
	Windows::Graphics::Display::DisplayOrientations nativeOrientation,
	Windows::Graphics::Display::DisplayOrientations currentOrientation)
{
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_IDENTITY;

	// Note: NativeOrientation can only be Landscape or Portrait even though
	// the DisplayOrientations enum has other values.
	switch (nativeOrientation)
	{
	case Windows::Graphics::Display::DisplayOrientations::Landscape:
		switch (currentOrientation)
		{
		case Windows::Graphics::Display::DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case Windows::Graphics::Display::DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;

		case Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		}
		break;

	case Windows::Graphics::Display::DisplayOrientations::Portrait:
		switch (currentOrientation)
		{
		case Windows::Graphics::Display::DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;

		case Windows::Graphics::Display::DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case Windows::Graphics::Display::DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case Windows::Graphics::Display::DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		}
		break;
	}

	return rotation;
}

// from RenderTargetTexture.cpp
bool CreateRenderTarget(
	ff::IGraphDevice *pDevice,
	ID3D11Texture2D *pTexture,
	size_t nArrayStart,
	size_t nArrayCount,
	size_t nMipLevel,
	ID3D11RenderTargetView **ppView);

bool RenderTargetWindowMetro::UpdateSwapChain(
	ff::PointInt windowSize,
	ff::PointInt panelSize,
	ff::PointDouble panelCompositionScale,
	Windows::Graphics::Display::DisplayOrientations nativeOrientation,
	Windows::Graphics::Display::DisplayOrientations currentOrientation)
{
	if (panelSize.x < 1 || panelSize.y < 1)
	{
		panelSize.x = std::max(_windowSize.x, 1);
		panelSize.y = std::max(_windowSize.y, 1);
	}

	DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation(nativeOrientation, currentOrientation);
	_windowSize = windowSize;
	_panelSize = panelSize;
	_panelScale = panelCompositionScale;
	_nativeOrientation = nativeOrientation;
	_currentOrientation = currentOrientation;
	_cachedFullScreenMode = false;
	_fullScreenMode = false;

	bool swapDimensions =
		displayRotation == DXGI_MODE_ROTATION_ROTATE90 ||
		displayRotation == DXGI_MODE_ROTATION_ROTATE270;

	ff::PointInt bufferSize(
		swapDimensions ? panelSize.y : panelSize.x,
		swapDimensions ? panelSize.x : panelSize.y);

	if (_swapChain)
	{
		if (bufferSize != GetBufferSize())
		{
			// Clear old state
			_target = nullptr;
			_backBuffer = nullptr;
			_device->GetContext()->ClearState();
			_device->GetContext()->Flush();

			DXGI_SWAP_CHAIN_DESC1 desc;
			_swapChain->GetDesc1(&desc);
			assertHrRetVal(_swapChain->ResizeBuffers(0, bufferSize.x, bufferSize.y, desc.Format, desc.Flags), false);
		}
	}
	else // first init
	{
		DXGI_SWAP_CHAIN_DESC1 desc;
		ff::ZeroObject(desc);

		desc.Width = bufferSize.x;
		desc.Height = bufferSize.y;
		desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.BufferCount = 2; // must be 2 for sequential flip
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Scaling = DXGI_SCALING_STRETCH;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

		ff::ComPtr<IDXGIFactoryX> cardDxgi;
		assertRetVal(ff::GetParentDXGI(_device->GetAdapter(), __uuidof(IDXGIFactoryX), (void**)&cardDxgi), false);

		ff::ComPtr<IDXGISwapChain1> swapChain;
		assertHrRetVal(cardDxgi->CreateSwapChainForComposition(_device->Get3d(), &desc, nullptr, &swapChain), false);
		assertRetVal(_swapChain.QueryFrom(swapChain), false);
		Windows::UI::Xaml::Controls::SwapChainPanel ^panel = _panel;

		ff::GetMainThreadDispatch()->Post([panel, swapChain]()
		{
			ff::ComPtr<ISwapChainPanelNative> nativePanel;
			assertRet(nativePanel.QueryFrom(panel));
			verifyHr(nativePanel->SetSwapChain(swapChain));
		}, true);

		assertHrRetVal(_device->GetDXGI()->SetMaximumFrameLatency(1), false);
	}

	// Create render target
	if (!_backBuffer)
	{
		assertHrRetVal(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&_backBuffer), false);
		assertRetVal(::CreateRenderTarget(_device, _backBuffer, 0, 1, 0, &_target), false);
	}

	// Scale the back buffer to the panel
	DXGI_MATRIX_3X2_F inverseScale = { 0 };
	inverseScale._11 = 1 / (float)panelCompositionScale.x;
	inverseScale._22 = 1 / (float)panelCompositionScale.y;

	assertHrRetVal(_swapChain->SetRotation(displayRotation), false);
	assertHrRetVal(_swapChain->SetMatrixTransform(&inverseScale), false);

	return true;
}

ff::IGraphDevice *RenderTargetWindowMetro::GetDevice() const
{
	return _device;
}

bool RenderTargetWindowMetro::Reset()
{
	_target = nullptr;
	_backBuffer = nullptr;
	_swapChain = nullptr;

	assertRetVal(UpdateSwapChain(
		_windowSize, _panelSize, _panelScale,
		_nativeOrientation, _currentOrientation), false);

	return true;
}

ff::PointInt RenderTargetWindowMetro::GetBufferSize() const
{
	assertRetVal(_swapChain, ff::PointInt(0, 0));

	DXGI_SWAP_CHAIN_DESC1 desc;
	_swapChain->GetDesc1(&desc);

	return ff::PointInt(desc.Width, desc.Height);
}

ff::PointInt RenderTargetWindowMetro::GetRotatedSize() const
{
	ff::PointInt size = GetBufferSize();

	int rotation = GetRotatedDegrees();
	if (rotation == 90 || rotation == 270)
	{
		std::swap(size.x, size.y);
	}

	return size;
}

int RenderTargetWindowMetro::GetRotatedDegrees() const
{
	DXGI_MODE_ROTATION rotation;
	if (_swapChain && SUCCEEDED(_swapChain->GetRotation(&rotation)) &&
		rotation != DXGI_MODE_ROTATION_UNSPECIFIED)
	{
		return (rotation - 1) * 90;
	}

	return 0;
}

double RenderTargetWindowMetro::GetDpiScale() const
{
	return ff::MetroGlobals::Get()->GetDpiScale();
}

void RenderTargetWindowMetro::Clear(const DirectX::XMFLOAT4 *pColor)
{
	assertRet(_target);

	static const DirectX::XMFLOAT4 defaultColor(0, 0, 0, 1);
	const DirectX::XMFLOAT4 *useColor = pColor ? pColor : &defaultColor;
	_device->GetContext()->ClearRenderTargetView(_target, &useColor->x);
}

ID3D11Texture2D *RenderTargetWindowMetro::GetTexture()
{
	return _backBuffer;
}

ID3D11RenderTargetView *RenderTargetWindowMetro::GetTarget()
{
	return _target;
}

ff::PWND RenderTargetWindowMetro::GetWindow() const
{
	return _window;
}

bool RenderTargetWindowMetro::Present(bool vsync)
{
	assertRetVal(_swapChain && _target, false);

	DXGI_PRESENT_PARAMETERS pp = { 0 };
	HRESULT hr = _swapChain->Present1(vsync ? 1 : 0, 0, &pp);

	_device->GetContext()->DiscardView1(_target, nullptr, 0);

	return hr != DXGI_ERROR_DEVICE_RESET && hr != DXGI_ERROR_DEVICE_REMOVED;
}

bool RenderTargetWindowMetro::WaitForVsync() const
{
	assertRetVal(_swapChain, false);

	ff::ComPtr<IDXGIOutput> output;
	if (SUCCEEDED(_device->GetAdapter()->EnumOutputs(0, &output)))
	{
		return SUCCEEDED(output->WaitForVBlank());
	}

	return false;
}

bool RenderTargetWindowMetro::CanSetFullScreen() const
{
	return true;
}

bool RenderTargetWindowMetro::IsFullScreen()
{
	if (!_cachedFullScreenMode)
	{
		_fullScreenMode = _view->IsFullScreenMode;
		_cachedFullScreenMode = true;
	}

	return _fullScreenMode;
}

bool RenderTargetWindowMetro::SetFullScreen(bool fullScreen)
{
	if (fullScreen)
	{
		return _view->TryEnterFullScreenMode();
	}
	else
	{
		_view->ExitFullScreenMode();
		return true;
	}
}

#endif // METRO_APP
