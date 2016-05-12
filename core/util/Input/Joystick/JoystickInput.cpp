#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Input/Joystick/JoystickInput.h"

#if METRO_APP

class __declspec(uuid("3292c799-eb7a-4337-b2c2-87861ec1a03f"))
	JoystickInput : public ff::ComBase, public ff::IJoystickInput
{
public:
	DECLARE_HEADER(JoystickInput);

	bool Init(ff::PWND hwnd);

	// IJoystickInput
	virtual void Advance() override;
	virtual void Reset() override;
	virtual ff::PWND GetWindow() const override;

	virtual size_t GetCount() const override;
	virtual ff::IJoystickDevice *GetJoystick(size_t index) const override;

private:
	ref class JoyEvents
	{
	internal:
		JoyEvents(JoystickInput *parent);

	public:
		virtual ~JoyEvents();

		void Destroy();

	private:
		void OnGamepadAdded(Platform::Object ^sender, Windows::Gaming::Input::Gamepad ^gamepad);
		void OnGamepadRemoved(Platform::Object ^sender, Windows::Gaming::Input::Gamepad ^gamepad);

		JoystickInput *_parent;
		Windows::Foundation::EventRegistrationToken _tokens[2];
	};

	void OnGamepadAdded(Platform::Object ^sender, Windows::Gaming::Input::Gamepad ^gamepad);
	void OnGamepadRemoved(Platform::Object ^sender, Windows::Gaming::Input::Gamepad ^gamepad);

	ff::PWND _hwnd;
	JoyEvents ^_events;
	ff::Vector<ff::ComPtr<ff::IXboxJoystick>> _joysticks;
};

BEGIN_INTERFACES(JoystickInput)
	HAS_INTERFACE(ff::IJoystickInput)
END_INTERFACES()

bool ff::CreateJoystickInput(PWND hwnd, IJoystickInput **obj)
{
	assertRetVal(obj, false);
	*obj = nullptr;

	ComPtr<JoystickInput> pInput;
	assertHrRetVal(ComAllocator<JoystickInput>::CreateInstance(&pInput), false);
	assertRetVal(pInput->Init(hwnd), false);

	*obj = pInput.Detach();
	return true;
}

JoystickInput::JoyEvents::JoyEvents(JoystickInput *parent)
	: _parent(parent)
{
	_tokens[0] = Windows::Gaming::Input::Gamepad::GamepadAdded +=
		ref new Windows::Foundation::EventHandler<Windows::Gaming::Input::Gamepad ^>(
			this, &JoyEvents::OnGamepadAdded);

	_tokens[1] = Windows::Gaming::Input::Gamepad::GamepadRemoved +=
		ref new Windows::Foundation::EventHandler<Windows::Gaming::Input::Gamepad ^>(
			this, &JoyEvents::OnGamepadRemoved);
}

JoystickInput::JoyEvents::~JoyEvents()
{
	Destroy();
}

void JoystickInput::JoyEvents::Destroy()
{
	if (_parent)
	{
		Windows::Gaming::Input::Gamepad::GamepadAdded -= _tokens[0];
		Windows::Gaming::Input::Gamepad::GamepadRemoved -= _tokens[1];
		_parent = nullptr;
	}
}

void JoystickInput::JoyEvents::OnGamepadAdded(Platform::Object ^sender, Windows::Gaming::Input::Gamepad ^gamepad)
{
	if (_parent)
	{
		_parent->OnGamepadAdded(sender, gamepad);
	}
}

void JoystickInput::JoyEvents::OnGamepadRemoved(Platform::Object ^sender, Windows::Gaming::Input::Gamepad ^gamepad)
{
	if (_parent)
	{
		_parent->OnGamepadRemoved(sender, gamepad);
	}
}

JoystickInput::JoystickInput()
	: _hwnd(nullptr)
{
}

JoystickInput::~JoystickInput()
{
	if (_events)
	{
		_events->Destroy();
		_events = nullptr;
	}
}

bool JoystickInput::Init(ff::PWND hwnd)
{
	_hwnd = hwnd;
	_events = ref new JoyEvents(this);

	for (size_t i = 0; i < 4; i++)
	{
		ff::ComPtr<ff::IXboxJoystick> device;
		assertRetVal(ff::CreateXboxJoystick(nullptr, &device), false);
		_joysticks.Push(device);
	}

	for (Windows::Gaming::Input::Gamepad ^gamepad : Windows::Gaming::Input::Gamepad::Gamepads)
	{
		OnGamepadAdded(nullptr, gamepad);
	}

	return true;
}

void JoystickInput::Advance()
{
	for (ff::IXboxJoystick *device : _joysticks)
	{
		device->Advance();
	}
}

void JoystickInput::Reset()
{
	_joysticks.Clear();
	verify(Init(_hwnd));
}

ff::PWND JoystickInput::GetWindow() const
{
	return _hwnd;
}

size_t JoystickInput::GetCount() const
{
	return _joysticks.Size();
}

ff::IJoystickDevice *JoystickInput::GetJoystick(size_t index) const
{
	assertRetVal(index >= 0 && index < _joysticks.Size(), nullptr);
	return _joysticks[index];
}

void JoystickInput::OnGamepadAdded(Platform::Object ^sender, Windows::Gaming::Input::Gamepad ^gamepad)
{
	for (size_t i = 0; i < _joysticks.Size(); i++)
	{
		if (!_joysticks[i]->GetGamepad())
		{
			_joysticks[i]->SetGamepad(gamepad);
			gamepad = nullptr;
			break;
		}
	}

	if (gamepad)
	{
		ff::ComPtr<ff::IXboxJoystick> device;
		assertRet(ff::CreateXboxJoystick(gamepad, &device));
		_joysticks.Push(device);
	}
}

void JoystickInput::OnGamepadRemoved(Platform::Object ^sender, Windows::Gaming::Input::Gamepad ^gamepad)
{
	for (size_t i = 0; i < _joysticks.Size(); i++)
	{
		if (_joysticks[i]->GetGamepad() == gamepad)
		{
			_joysticks[i]->SetGamepad(nullptr);
			break;
		}
	}
}

#endif
