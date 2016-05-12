#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/KeyboardDevice.h"
#include "Module/Module.h"

#if METRO_APP

namespace ff
{
	class __declspec(uuid("12896528-8ff4-491c-98fd-527c1640e578"))
		KeyboardDevice : public ComBase, public IKeyboardDevice
	{
	public:
		DECLARE_HEADER(KeyboardDevice);

		bool Init(Windows::UI::Xaml::Window ^window);
		void Destroy();

		// IInputDevice
		virtual void Advance() override;
		virtual bool IsConnected() const override;

		// IKeyboardDevice
		virtual PWND GetWindow() const override;
		virtual bool GetKey(int vk) const override;
		virtual int GetKeyPressCount(int vk) const override;
		virtual String GetChars() const override;

	private:
		ref class KeyEvents
		{
		internal:
			KeyEvents(KeyboardDevice *pParent);

		public:
			virtual ~KeyEvents();

			void Destroy();

		private:
			void OnCharacterReceived(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::CharacterReceivedEventArgs ^args);
			void OnKeyDown(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args);
			void OnKeyUp(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args);

			KeyboardDevice *_parent;
			Windows::Foundation::EventRegistrationToken _tokens[3];
		};

		void OnCharacterReceived(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::CharacterReceivedEventArgs ^args);
		void OnKeyDown(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args);
		void OnKeyUp(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args);

		Mutex _cs;
		Windows::UI::Xaml::Window ^_window;
		KeyEvents ^_events;
		BYTE _keys[256];
		BYTE _keysPending[256];
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

ff::KeyboardDevice::KeyEvents::KeyEvents(KeyboardDevice *pParent)
	: _parent(pParent)
{
	_tokens[0] = _parent->_window->CoreWindow->CharacterReceived +=
		ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::CharacterReceivedEventArgs^>(
			this, &KeyEvents::OnCharacterReceived);

	_tokens[1] = _parent->_window->CoreWindow->KeyDown +=
		ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::KeyEventArgs^>(
			this, &KeyEvents::OnKeyDown);

	_tokens[2] = _parent->_window->CoreWindow->KeyUp +=
		ref new Windows::Foundation::TypedEventHandler<Windows::UI::Core::CoreWindow^, Windows::UI::Core::KeyEventArgs^>(
			this, &KeyEvents::OnKeyUp);
}

ff::KeyboardDevice::KeyEvents::~KeyEvents()
{
	Destroy();
}

void ff::KeyboardDevice::KeyEvents::Destroy()
{
	if (_parent)
	{
		_parent->_window->CoreWindow->CharacterReceived -= _tokens[0];
		_parent->_window->CoreWindow->KeyDown -= _tokens[1];
		_parent->_window->CoreWindow->KeyUp -= _tokens[2];
		_parent = nullptr;
	}
}

void ff::KeyboardDevice::KeyEvents::OnCharacterReceived(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::CharacterReceivedEventArgs ^args)
{
	assertRet(_parent);
	_parent->OnCharacterReceived(sender, args);
}

void ff::KeyboardDevice::KeyEvents::OnKeyDown(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args)
{
	assertRet(_parent);
	_parent->OnKeyDown(sender, args);
}

void ff::KeyboardDevice::KeyEvents::OnKeyUp(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args)
{
	assertRet(_parent);
	_parent->OnKeyUp(sender, args);
}

// static
bool ff::CreateKeyboardDevice(PWND hwnd, IKeyboardDevice **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ComPtr<KeyboardDevice> myObj;
	assertHrRetVal(ComAllocator<KeyboardDevice>::CreateInstance(&myObj), false);
	assertRetVal(myObj->Init(hwnd), false);

	*obj = myObj.Detach();
	return true;
}

ff::KeyboardDevice::KeyboardDevice()
{
	ff::ZeroObject(_keys);
	ff::ZeroObject(_keysPending);
	ff::ZeroObject(_presses);
	ff::ZeroObject(_pressesPending);
}

ff::KeyboardDevice::~KeyboardDevice()
{
	Destroy();
}

bool ff::KeyboardDevice::Init(Windows::UI::Xaml::Window ^window)
{
	assertRetVal(window, false);
	_window = window;
	_events = ref new KeyEvents(this);

	return true;
}

void ff::KeyboardDevice::Destroy()
{
	if (_events)
	{
		_events->Destroy();
		_events = nullptr;
	}
}

void ff::KeyboardDevice::Advance()
{
	ff::LockMutex crit(_cs);

	_text = _textPending;
	_textPending.clear();

	CopyMemory(_keys, _keysPending, sizeof(_keys));
	CopyMemory(_presses, _pressesPending, sizeof(_presses));
	ff::ZeroObject(_pressesPending);
}

bool ff::KeyboardDevice::IsConnected() const
{
	return true;
}

ff::PWND ff::KeyboardDevice::GetWindow() const
{
	return _window;
}

bool ff::KeyboardDevice::GetKey(int vk) const
{
	assert(vk >= 0 && vk < _countof(_keys));
	return _keys[vk] != 0;
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

void ff::KeyboardDevice::OnCharacterReceived(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::CharacterReceivedEventArgs ^args)
{
	ff::LockMutex crit(_cs);
	wchar_t ch = (wchar_t)args->KeyCode;
	_textPending.append(1, ch);
}

void ff::KeyboardDevice::OnKeyDown(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args)
{
	int vk = (int)args->VirtualKey;

	ff::LockMutex crit(_cs);

	if (vk >= 0 && vk < _countof(_pressesPending) && _pressesPending[vk] != 0xFF)
	{
		_pressesPending[vk]++;
	}

	if (vk >= 0 && vk < _countof(_keys))
	{
		_keysPending[vk] = true;
	}
}

void ff::KeyboardDevice::OnKeyUp(Windows::UI::Core::CoreWindow ^sender, Windows::UI::Core::KeyEventArgs ^args)
{
	int vk = (int)args->VirtualKey;

	ff::LockMutex crit(_cs);

	if (vk >= 0 && vk < _countof(_keys))
	{
		_keysPending[vk] = false;
	}
}

#endif
