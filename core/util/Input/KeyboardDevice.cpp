#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/KeyboardDevice.h"
#include "Module/Module.h"

#if !METRO_APP

namespace ff
{
	class __declspec(uuid("c62f6c11-e3ef-4e92-8a5e-3c7d92dac943"))
		KeyboardDevice : public ComBase, public IKeyboardDevice
	{
	public:
		DECLARE_HEADER(KeyboardDevice);

		bool Init(HWND hwnd);
		void Destroy();

		// IInputDevice
		virtual void Advance() override;
		virtual bool IsConnected() const override;

		// IKeyboardDevice
		virtual HWND GetWindow() const override;
		virtual bool GetKey(int vk) const override;
		virtual int GetKeyPressCount(int vk) const override;
		virtual String GetChars() const override;

	private:
		static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void OnKeyMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		HWND _hwnd;
		WNDPROC _oldWindowProc;
		BYTE _keys[256];
		BYTE _presses[256];
		BYTE _pressesPending[256];
		String _text;
		String _textPending;
	};
}

BEGIN_INTERFACES(ff::KeyboardDevice)
	HAS_INTERFACE(ff::IKeyboardDevice)
	HAS_INTERFACE(ff::IInputDevice)
END_INTERFACES()

static ff::Vector<ff::KeyboardDevice*, 8> s_allKeyInputs;

bool ff::CreateKeyboardDevice(HWND hwnd, IKeyboardDevice **ppInput)
{
	assertRetVal(ppInput, false);
	*ppInput = nullptr;

	for (size_t i = 0; i < s_allKeyInputs.Size(); i++)
	{
		if (s_allKeyInputs[i]->GetWindow() == hwnd)
		{
			*ppInput = GetAddRef(s_allKeyInputs[i]);
			return true;
		}
	}

	ComPtr<KeyboardDevice> pInput;
	assertHrRetVal(ComAllocator<KeyboardDevice>::CreateInstance(&pInput), false);
	assertRetVal(pInput->Init(hwnd), false);

	*ppInput = pInput.Detach();
	return true;
}

ff::KeyboardDevice::KeyboardDevice()
	: _hwnd(nullptr)
	, _oldWindowProc(nullptr)
{
	ZeroObject(_keys);
	ZeroObject(_presses);
	ZeroObject(_pressesPending);

	s_allKeyInputs.Push(this);
}

ff::KeyboardDevice::~KeyboardDevice()
{
	Destroy();

	s_allKeyInputs.Delete(s_allKeyInputs.Find(this));

	if (!s_allKeyInputs.Size())
	{
		s_allKeyInputs.Reduce();
	}
}

bool ff::KeyboardDevice::Init(HWND hwnd)
{
	assertRetVal(hwnd, false);

	_hwnd = hwnd;
	_oldWindowProc = SubclassWindow(_hwnd, StaticWindowProc);

	return true;
}

void ff::KeyboardDevice::Destroy()
{
	if (_hwnd && _oldWindowProc)
	{
		SubclassWindow(_hwnd, _oldWindowProc);

		_oldWindowProc = nullptr;
		_hwnd = nullptr;
	}
}

void ff::KeyboardDevice::Advance()
{
	_text = _textPending;
	_textPending.clear();

	CopyMemory(_presses, _pressesPending, sizeof(_presses));
	ZeroObject(_pressesPending);

	GetKeyboardState(_keys);
}

bool ff::KeyboardDevice::IsConnected() const
{
	return true;
}

HWND ff::KeyboardDevice::GetWindow() const
{
	return _hwnd;
}

bool ff::KeyboardDevice::GetKey(int vk) const
{
	assert(vk >= 0 && vk < _countof(_keys));
	return (_keys[vk] & 0x80) != 0;
}

int ff::KeyboardDevice::GetKeyPressCount(int vk) const
{
	assert(vk >= 0 && vk < _countof(_presses));
	return (int)_presses[vk];
}

ff::String ff::KeyboardDevice::GetChars() const
{
	return _text;
}

// static
LRESULT ff::KeyboardDevice::StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	for (size_t i = 0; i < s_allKeyInputs.Size(); i++)
	{
		if (s_allKeyInputs[i]->GetWindow() == hwnd)
		{
			return s_allKeyInputs[i]->WindowProc(hwnd, msg, wParam, lParam);
		}
	}

	assert(false);
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT ff::KeyboardDevice::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC pOldWndProc = _oldWindowProc;

	switch (msg)
	{
	case WM_DESTROY:
		Destroy();
		break;

	case WM_KEYDOWN:
		if (!(lParam & 0x40000000) && // was not pressed
			wParam < _countof(_pressesPending) &&
			_pressesPending[wParam] != 0xFF)
		{
			_pressesPending[wParam]++;
		}
		break;

	case WM_CHAR:
		_textPending.append(1, (wchar_t)wParam);
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

#endif // !METRO_APP
