#include "pch.h"
#include "COM/ComAlloc.h"
#include "Globals/MetroGlobals.h"
#include "Input/KeyboardDevice.h"
#include "Input/PointerDevice.h"
#include "Module/Module.h"
#include "Thread/ThreadUtil.h"
#include "Windows/Handles.h"

#if METRO_APP

class PointerDevice;

ref class PointerEvents
{
internal:
	PointerEvents();
	bool Init(PointerDevice *parent);
	void Destroy();

public:
	virtual ~PointerEvents();

private:
	void OnPointerEntered(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerExited(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerMoved(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerPressed(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerReleased(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerCaptureLost(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);

	PointerDevice *_parent;
	ff::WinHandle _event;
	Windows::UI::Core::CoreIndependentInputSource ^_inputSource;
	Windows::Foundation::IAsyncAction ^_inputAction;
};

class __declspec(uuid("08dfcb6a-7d69-4e5a-aed3-628dd89a3e8d"))
	PointerDevice
		: public ff::ComBase
		, public ff::IPointerDevice
{
public:
	DECLARE_HEADER(PointerDevice);

	bool Init(Windows::UI::Xaml::Window ^window);
	void Destroy();

	// IInputDevice
	virtual void Advance() override;
	virtual bool IsConnected() const override;

	// IPointerDevice
	virtual Windows::UI::Xaml::Window ^GetWindow() const override;
	virtual bool IsInWindow() const override;
	virtual ff::PointDouble GetPos() const override;
	virtual ff::PointDouble GetRelativePos() const override;

	virtual bool GetButton(int vkButton) const override;
	virtual int GetButtonClickCount(int vkButton) const override;
	virtual int GetButtonReleaseCount(int vkButton) const override;
	virtual int GetButtonDoubleClickCount(int vkButton) const override;
	virtual ff::PointDouble GetWheelScroll() const override;

	virtual size_t GetTouchCount() const override;
	virtual const ff::TouchInfo &GetTouchInfo(size_t index) const override;

	// Callbacks
	void OnPointerEntered(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerExited(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerMoved(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerPressed(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerReleased(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);
	void OnPointerCaptureLost(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args);

private:
	void OnMouseMoved(Windows::UI::Core::PointerEventArgs ^args);
	void OnMousePressed(Windows::UI::Core::PointerEventArgs ^args);
	void OnTouchMoved(Windows::UI::Core::PointerEventArgs ^args);
	void OnTouchPressed(Windows::UI::Core::PointerEventArgs ^args);
	void OnTouchReleased(Windows::UI::Core::PointerEventArgs ^args);

	struct MouseInfo
	{
		ff::PointDouble _pos;
		ff::PointDouble _posRelative;
		ff::PointDouble _wheel;
		bool _buttons[6];
		BYTE _clicks[6];
		BYTE _releases[6];
		BYTE _doubleClicks[6];
	};

	struct InternalTouchInfo
	{
		Windows::UI::Input::PointerPoint ^point;
		ff::TouchInfo info;
	};

	InternalTouchInfo *FindTouchInfo(Windows::UI::Input::PointerPoint ^point);
	void UpdateTouchInfo(InternalTouchInfo &info);

	ff::Mutex _cs;
	ff::MetroGlobals *_globals;
	PointerEvents ^_events;
	Windows::UI::Xaml::Window ^_window;
	Windows::UI::Xaml::Controls::SwapChainPanel ^_panel;
	MouseInfo _mouse;
	MouseInfo _pendingMouse;
	ff::Vector<InternalTouchInfo> _touches;
	ff::Vector<InternalTouchInfo> _pendingTouches;
	bool _insideWindow;
	bool _pendingInsideWindow;
};

BEGIN_INTERFACES(PointerDevice)
	HAS_INTERFACE(ff::IPointerDevice)
	HAS_INTERFACE(ff::IInputDevice)
END_INTERFACES()

bool ff::CreatePointerDevice(Windows::UI::Xaml::Window ^window, IPointerDevice **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ComPtr<PointerDevice> myObj;
	assertHrRetVal(ComAllocator<PointerDevice>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(window), false);

	*obj = myObj.Detach();
	return true;
}

PointerEvents::PointerEvents()
	: _parent(nullptr)
{
}

PointerEvents::~PointerEvents()
{
	Destroy();
}

bool PointerEvents::Init(PointerDevice *parent)
{
	ff::MetroGlobals *globals = ff::MetroGlobals::Get();
	assertRetVal(!_parent && parent && globals, false);

	_parent = parent;
	_event = ff::CreateEvent();

	auto workItem = [this, globals](Windows::Foundation::IAsyncAction ^action)
	{
		_inputSource = globals->GetSwapChainPanel()->CreateCoreIndependentInputSource(
			Windows::UI::Core::CoreInputDeviceTypes::Mouse |
			Windows::UI::Core::CoreInputDeviceTypes::Touch |
			Windows::UI::Core::CoreInputDeviceTypes::Pen);

		Windows::Foundation::EventRegistrationToken tokens[6];
		tokens[0] = _inputSource->PointerEntered +=
			ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerEntered);
		tokens[1] = _inputSource->PointerExited +=
			ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerExited);
		tokens[2] = _inputSource->PointerMoved +=
			ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerMoved);
		tokens[3] = _inputSource->PointerPressed +=
			ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerPressed);
		tokens[4] = _inputSource->PointerReleased +=
			ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerReleased);
		tokens[5] = _inputSource->PointerCaptureLost +=
			ref new Windows::Foundation::TypedEventHandler<Platform::Object^, Windows::UI::Core::PointerEventArgs^>(this, &PointerEvents::OnPointerCaptureLost);

		::SetEvent(_event);

		_inputSource->Dispatcher->ProcessEvents(Windows::UI::Core::CoreProcessEventsOption::ProcessUntilQuit);

		_inputSource->PointerEntered -= tokens[0];
		_inputSource->PointerExited -= tokens[1];
		_inputSource->PointerMoved -= tokens[2];
		_inputSource->PointerPressed -= tokens[3];
		_inputSource->PointerReleased -= tokens[4];
		_inputSource->PointerCaptureLost -= tokens[5];

		::SetEvent(_event);
	};

	auto workItemHandler = ref new Windows::System::Threading::WorkItemHandler(workItem);

	_inputAction = Windows::System::Threading::ThreadPool::RunAsync(
		workItemHandler,
		Windows::System::Threading::WorkItemPriority::High,
		Windows::System::Threading::WorkItemOptions::TimeSliced);

	ff::WaitForEventAndReset(_event);

	return true;
}

void PointerEvents::Destroy()
{
	if (_inputSource)
	{
		_inputSource->Dispatcher->StopProcessEvents();
		ff::WaitForHandle(_event);
	}

	_inputSource = nullptr;
	_inputAction = nullptr;
	_parent = nullptr;
}

void PointerEvents::OnPointerEntered(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	noAssertRet(_parent);
	_parent->OnPointerEntered(sender, args);
}

void PointerEvents::OnPointerExited(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	noAssertRet(_parent);
	_parent->OnPointerExited(sender, args);
}

void PointerEvents::OnPointerMoved(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	noAssertRet(_parent);
	_parent->OnPointerMoved(sender, args);
}

void PointerEvents::OnPointerPressed(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	noAssertRet(_parent);
	_parent->OnPointerPressed(sender, args);
}

void PointerEvents::OnPointerReleased(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	noAssertRet(_parent);
	_parent->OnPointerReleased(sender, args);
}

void PointerEvents::OnPointerCaptureLost(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	noAssertRet(_parent);
	_parent->OnPointerCaptureLost(sender, args);
}

PointerDevice::PointerDevice()
	: _insideWindow(false)
	, _pendingInsideWindow(false)
{
	ff::ZeroObject(_mouse);
	ff::ZeroObject(_pendingMouse);
}

PointerDevice::~PointerDevice()
{
	Destroy();
}

bool PointerDevice::Init(Windows::UI::Xaml::Window ^window)
{
	assertRetVal(window && !_globals, false);
	_globals = ff::MetroGlobals::Get();
	assertRetVal(_globals && _globals->GetWindow() == window, false);

	_window = window;
	_panel = _globals->GetSwapChainPanel();
	_events = ref new PointerEvents();
	assertRetVal(_events->Init(this), false);

	return true;
}

void PointerDevice::Destroy()
{
	if (_events != nullptr)
	{
		_events->Destroy();
		_events = nullptr;
	}

	_window = nullptr;
	_panel = nullptr;
	_touches.Clear();
	_pendingTouches.Clear();
}

void PointerDevice::Advance()
{
	ff::LockMutex crit(_cs);
	_mouse = _pendingMouse;
	_touches = _pendingTouches;
	_insideWindow = _pendingInsideWindow;

	ff::ZeroObject(_pendingMouse._posRelative);
	ff::ZeroObject(_pendingMouse._wheel);
	ff::ZeroObject(_pendingMouse._clicks);
	ff::ZeroObject(_pendingMouse._releases);
	ff::ZeroObject(_pendingMouse._doubleClicks);
}

bool PointerDevice::IsConnected() const
{
	return true;
}

Windows::UI::Xaml::Window ^PointerDevice::GetWindow() const
{
	return _window;
}

bool PointerDevice::IsInWindow() const
{
	return _insideWindow;
}

ff::PointDouble PointerDevice::GetPos() const
{
	return _mouse._pos * _globals->GetDpiScale();
}

ff::PointDouble PointerDevice::GetRelativePos() const
{
	return _mouse._posRelative * _globals->GetDpiScale();
}

static bool IsValidButton(int vkButton)
{
	switch (vkButton)
	{
	case VK_LBUTTON:
	case VK_RBUTTON:
	case VK_MBUTTON:
	case VK_XBUTTON1:
	case VK_XBUTTON2:
		return true;

	default:
		assertRetVal(false, false);
	}
}

bool PointerDevice::GetButton(int vkButton) const
{
	return ::IsValidButton(vkButton) ? _mouse._buttons[vkButton] : false;
}

int PointerDevice::GetButtonClickCount(int vkButton) const
{
	return ::IsValidButton(vkButton) ? _mouse._clicks[vkButton] : 0;
}

int PointerDevice::GetButtonReleaseCount(int vkButton) const
{
	return ::IsValidButton(vkButton) ? _mouse._releases[vkButton] : 0;
}

int PointerDevice::GetButtonDoubleClickCount(int vkButton) const
{
	return ::IsValidButton(vkButton) ? _mouse._doubleClicks[vkButton] : 0;
}

ff::PointDouble PointerDevice::GetWheelScroll() const
{
	return _mouse._wheel;
}

size_t PointerDevice::GetTouchCount() const
{
	return _touches.Size();
}

const ff::TouchInfo &PointerDevice::GetTouchInfo(size_t index) const
{
	return _touches[index].info;
}

void PointerDevice::OnPointerEntered(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	ff::LockMutex crit(_cs);
	_pendingInsideWindow = true;
	OnMouseMoved(args);
	OnTouchMoved(args);
}

void PointerDevice::OnPointerExited(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	ff::LockMutex crit(_cs);
	_pendingInsideWindow = false;
	OnTouchReleased(args);
}

void PointerDevice::OnPointerMoved(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	ff::LockMutex crit(_cs);
	OnMouseMoved(args);
	OnTouchMoved(args);
}

void PointerDevice::OnPointerPressed(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	ff::LockMutex crit(_cs);
	OnMousePressed(args);
	OnMouseMoved(args);
	OnTouchPressed(args);
}

void PointerDevice::OnPointerReleased(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	ff::LockMutex crit(_cs);
	OnMousePressed(args);
	OnTouchReleased(args);
}

void PointerDevice::OnPointerCaptureLost(Platform::Object ^sender, Windows::UI::Core::PointerEventArgs ^args)
{
	ff::LockMutex crit(_cs);
	OnTouchReleased(args);
}

void PointerDevice::OnMouseMoved(Windows::UI::Core::PointerEventArgs ^args)
{
	Windows::UI::Input::PointerPoint ^point = args->CurrentPoint;
	_pendingMouse._pos.x = point->Position.X;
	_pendingMouse._pos.y = point->Position.Y;
	_pendingMouse._posRelative = _pendingMouse._pos - _mouse._pos;
}

void PointerDevice::OnMousePressed(Windows::UI::Core::PointerEventArgs ^args)
{
	Windows::UI::Input::PointerPoint ^point = args->CurrentPoint;
	int nPresses = 0;
	int vkButton = 0;

	switch (point->Properties->PointerUpdateKind)
	{
	case Windows::UI::Input::PointerUpdateKind::LeftButtonPressed:
		vkButton = VK_LBUTTON;
		nPresses = 1;
		break;

	case Windows::UI::Input::PointerUpdateKind::LeftButtonReleased:
		vkButton = VK_LBUTTON;
		break;

	case Windows::UI::Input::PointerUpdateKind::RightButtonPressed:
		vkButton = VK_RBUTTON;
		nPresses = 1;
		break;

	case Windows::UI::Input::PointerUpdateKind::RightButtonReleased:
		vkButton = VK_RBUTTON;
		break;

	case Windows::UI::Input::PointerUpdateKind::XButton1Pressed:
		vkButton = VK_XBUTTON1;
		nPresses = 1;
		break;

	case Windows::UI::Input::PointerUpdateKind::XButton1Released:
		vkButton = VK_XBUTTON1;
		break;

	case Windows::UI::Input::PointerUpdateKind::XButton2Pressed:
		vkButton = VK_XBUTTON2;
		nPresses = 1;
		break;

	case Windows::UI::Input::PointerUpdateKind::XButton2Released:
		vkButton = VK_XBUTTON2;
		break;
	}

	if (vkButton)
	{
		switch (nPresses)
		{
		case 2:
			if (_pendingMouse._doubleClicks[vkButton] != 0xFF)
			{
				_pendingMouse._doubleClicks[vkButton]++;
			}
			__fallthrough;

		case 1:
			_pendingMouse._buttons[vkButton] = true;

			if (_pendingMouse._clicks[vkButton] != 0xFF)
			{
				_pendingMouse._clicks[vkButton]++;
			}
			break;

		case 0:
			_pendingMouse._buttons[vkButton] = false;

			if (_pendingMouse._releases[vkButton] != 0xFF)
			{
				_pendingMouse._releases[vkButton]++;
			}
			break;
		}
	}
}

void PointerDevice::OnTouchMoved(Windows::UI::Core::PointerEventArgs ^args)
{
	Windows::UI::Input::PointerPoint ^point = args->CurrentPoint;
	InternalTouchInfo *info = FindTouchInfo(point);

	if (info != nullptr)
	{
		info->point = point;
		UpdateTouchInfo(*info);

		// Log::DebugTraceF(L"*** Moved touch: %u\n", point->PointerId);
	}
}

void PointerDevice::OnTouchPressed(Windows::UI::Core::PointerEventArgs ^args)
{
	Windows::UI::Input::PointerPoint ^point = args->CurrentPoint;
	InternalTouchInfo *info = FindTouchInfo(point);

	if (info != nullptr)
	{
		assert(false);
		info->point = point;
		UpdateTouchInfo(*info);
	}
	else
	{
		InternalTouchInfo newInfo;
		newInfo.point = point;
		UpdateTouchInfo(newInfo);
		newInfo.info.startPos = newInfo.info.pos;
		_pendingTouches.Push(newInfo);

		// Log::DebugTraceF(L"*** New touch: %u\n", point->PointerId);
	}
}

void PointerDevice::OnTouchReleased(Windows::UI::Core::PointerEventArgs ^args)
{
	Windows::UI::Input::PointerPoint ^point = args->CurrentPoint;
	InternalTouchInfo *info = FindTouchInfo(point);

	if (info != nullptr)
	{
		_pendingTouches.Delete(info - _pendingTouches.Data());

		// Log::DebugTraceF(L"*** Deleted touch: %u\n", point->PointerId);
	}
}

PointerDevice::InternalTouchInfo *PointerDevice::FindTouchInfo(Windows::UI::Input::PointerPoint ^point)
{
	for (size_t i = 0; i < _pendingTouches.Size(); i++)
	{
		if (point->PointerId == _pendingTouches[i].point->PointerId)
		{
			return &_pendingTouches[i];
		}
	}

	return nullptr;
}

void PointerDevice::UpdateTouchInfo(InternalTouchInfo &info)
{
	ff::TouchType type = ff::TouchType::TOUCH_TYPE_NONE;
	Windows::Foundation::Point pos = info.point->Position;
	DWORD id = info.point->PointerId;

	switch (info.point->PointerDevice->PointerDeviceType)
	{
	case Windows::Devices::Input::PointerDeviceType::Mouse:
		type = ff::TouchType::TOUCH_TYPE_MOUSE;
		break;

	case Windows::Devices::Input::PointerDeviceType::Pen:
		type = ff::TouchType::TOUCH_TYPE_PEN;
		break;

	case Windows::Devices::Input::PointerDeviceType::Touch:
		type = ff::TouchType::TOUCH_TYPE_FINGER;
		break;
	}

	info.info.type = type;
	info.info.pos.SetPoint(pos.X * _globals->GetDpiScale(), pos.Y * _globals->GetDpiScale());
	info.info.id = id;
}

#endif // METRO_APP
