#include "pch.h"
#include "COM/ComAlloc.h"
#include "Input/InputMapping.h"
#include "Input/Joystick/JoystickDevice.h"
#include "Module/Module.h"

#if METRO_APP

class __declspec(uuid("a46186b2-1e05-4a3b-bce0-547b42c2bcf5"))
	XboxJoystick : public ff::ComBase, public ff::IXboxJoystick
{
public:
	DECLARE_HEADER(XboxJoystick);

	bool Init(Windows::Gaming::Input::Gamepad ^gamepad);

	// IInputDevice
	virtual void Advance() override;
	virtual bool IsConnected() const override;

	// IJoystickDevice
	virtual size_t GetStickCount() const override;
	virtual ff::PointFloat GetStickPos(size_t nStick, bool bDigital) const override;
	virtual ff::RectInt GetStickPressCount(size_t nStick) const override;
	virtual ff::String GetStickName(size_t nStick) const override;

	virtual size_t GetDPadCount() const override;
	virtual ff::PointInt GetDPadPos(size_t nDPad) const override;
	virtual ff::RectInt GetDPadPressCount(size_t nDPad) const override;
	virtual ff::String GetDPadName(size_t nDPad) const override;

	virtual size_t GetButtonCount() const override;
	virtual bool GetButton(size_t nButton) const override;
	virtual int GetButtonPressCount(size_t nButton) const override;
	virtual ff::String GetButtonName(size_t nButton) const override;

	virtual size_t GetTriggerCount() const override;
	virtual float GetTrigger(size_t nTrigger, bool bDigital) const override;
	virtual int GetTriggerPressCount(size_t nTrigger) const override;
	virtual ff::String GetTriggerName(size_t nTrigger) const override;

	virtual bool HasSpecialButton(ff::ESpecialJoystickButton button) const override;
	virtual bool GetSpecialButton(ff::ESpecialJoystickButton button) const override;
	virtual int GetSpecialButtonPressCount(ff::ESpecialJoystickButton button) const override;
	virtual ff::String GetSpecialButtonName(ff::ESpecialJoystickButton button) const override;

	// IXboxJoystick
	virtual Windows::Gaming::Input::Gamepad ^GetGamepad() const override;
	virtual void SetGamepad(Windows::Gaming::Input::Gamepad ^gamepad) override;

private:
	void CheckPresses(const Windows::Gaming::Input::GamepadReading &prevState);
	BYTE &GetPressed(int vk);
	BYTE GetPressed(int vk) const;
		
	Windows::Gaming::Input::Gamepad ^_gamepad;
	Windows::Gaming::Input::GamepadReading _state;
	float _triggerPressing[2];
	ff::PointFloat _stickPressing[2];
	BYTE _pressed[ff::VK_GAMEPAD_COUNT];
};

BEGIN_INTERFACES(XboxJoystick)
	HAS_INTERFACE(ff::IXboxJoystick)
	HAS_INTERFACE(ff::IJoystickDevice)
	HAS_INTERFACE(ff::IInputDevice)
END_INTERFACES()

bool ff::CreateXboxJoystick(Windows::Gaming::Input::Gamepad ^gamepad, IXboxJoystick **device)
{
	assertRetVal(device, false);
	*device = nullptr;

	ComPtr<XboxJoystick> pDevice;
	assertHrRetVal(ComAllocator<XboxJoystick>::CreateInstance(&pDevice), false);
	assertRetVal(pDevice->Init(gamepad), false);

	*device = pDevice.Detach();
	return true;
}

XboxJoystick::XboxJoystick()
{
	ff::ZeroObject(_triggerPressing);
	ff::ZeroObject(_stickPressing);
	ff::ZeroObject(_pressed);
}

XboxJoystick::~XboxJoystick()
{
}

bool XboxJoystick::Init(Windows::Gaming::Input::Gamepad ^gamepad)
{
	_gamepad = gamepad;
	return true;
}

void XboxJoystick::Advance()
{
	Windows::Gaming::Input::GamepadReading prevState = _state;

	if (_gamepad)
	{
		_state = _gamepad->GetCurrentReading();
	}
	else
	{
		ff::ZeroObject(_state);
		ff::ZeroObject(_triggerPressing);
		ff::ZeroObject(_stickPressing);
	}

	CheckPresses(prevState);
}

static bool WasPressed(
	Windows::Gaming::Input::GamepadButtons button,
	Windows::Gaming::Input::GamepadButtons pressed,
	Windows::Gaming::Input::GamepadButtons prevPressed = Windows::Gaming::Input::GamepadButtons::None)
{
	return (button & pressed) != Windows::Gaming::Input::GamepadButtons::None &&
		(button & prevPressed) == Windows::Gaming::Input::GamepadButtons::None;
}

void XboxJoystick::CheckPresses(const Windows::Gaming::Input::GamepadReading &prevState)
{
	ff::ZeroObject(_pressed);

	if (_state.Buttons != Windows::Gaming::Input::GamepadButtons::None)
	{
		GetPressed(VK_GAMEPAD_A) = WasPressed(Windows::Gaming::Input::GamepadButtons::A, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_B) = WasPressed(Windows::Gaming::Input::GamepadButtons::B, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_X) = WasPressed(Windows::Gaming::Input::GamepadButtons::X, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_Y) = WasPressed(Windows::Gaming::Input::GamepadButtons::Y, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_RIGHT_SHOULDER) = WasPressed(Windows::Gaming::Input::GamepadButtons::RightShoulder, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_LEFT_SHOULDER) = WasPressed(Windows::Gaming::Input::GamepadButtons::LeftShoulder, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_DPAD_UP) = WasPressed(Windows::Gaming::Input::GamepadButtons::DPadUp, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_DPAD_DOWN) = WasPressed(Windows::Gaming::Input::GamepadButtons::DPadDown, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_DPAD_LEFT) = WasPressed(Windows::Gaming::Input::GamepadButtons::DPadLeft, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_DPAD_RIGHT) = WasPressed(Windows::Gaming::Input::GamepadButtons::DPadRight, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_MENU) = WasPressed(Windows::Gaming::Input::GamepadButtons::Menu, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_VIEW) = WasPressed(Windows::Gaming::Input::GamepadButtons::View, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON) = WasPressed(Windows::Gaming::Input::GamepadButtons::LeftThumbstick, _state.Buttons, prevState.Buttons);
		GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON) = WasPressed(Windows::Gaming::Input::GamepadButtons::RightThumbstick, _state.Buttons, prevState.Buttons);
	}

	float t0 = (float)_state.LeftTrigger;
	float t1 = (float)_state.RightTrigger;

	float x0 = (float)_state.LeftThumbstickX;
	float y0 = (float)-_state.LeftThumbstickY;
	float x1 = (float)_state.RightThumbstickX;
	float y1 = (float)-_state.RightThumbstickY;

	const float rmax = 0.55f;
	const float rmin = 0.50f;

	// Left trigger press
	if (t0 >= rmax) { GetPressed(VK_GAMEPAD_LEFT_TRIGGER) = (_triggerPressing[0] == 0); _triggerPressing[0] = 1; }
	else if (t0 <= rmin) { _triggerPressing[0] = 0; }

	// Right trigger press
	if (t1 >= rmax) { GetPressed(VK_GAMEPAD_RIGHT_TRIGGER) = (_triggerPressing[1] == 0); _triggerPressing[1] = 1; }
	else if (t1 <= rmin) { _triggerPressing[1] = 0; }

	// Left stick X press
	if (x0 >= rmax) { GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT) = (_stickPressing[0].x != 1); _stickPressing[0].x = 1; }
	else if (x0 <= -rmax) { GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_LEFT) = (_stickPressing[0].x != -1); _stickPressing[0].x = -1; }
	else if (fabsf(x0) <= rmin) { _stickPressing[0].x = 0; }

	// Left stick Y press
	if (y0 >= rmax) { GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_DOWN) = (_stickPressing[0].y != 1); _stickPressing[0].y = 1; }
	else if (y0 <= -rmax) { GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_UP) = (_stickPressing[0].y != -1); _stickPressing[0].y = -1; }
	else if (fabsf(y0) <= rmin) { _stickPressing[0].y = 0; }

	// Right stick X press
	if (x1 >= rmax) { GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT) = (_stickPressing[1].x != 1); _stickPressing[1].x = 1; }
	else if (x1 <= -rmax) { GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT) = (_stickPressing[1].x != -1); _stickPressing[1].x = -1; }
	else if (fabsf(x1) <= rmin) { _stickPressing[1].x = 0; }

	// Right stick Y press
	if (y1 >= rmax) { GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN) = (_stickPressing[1].y != 1); _stickPressing[1].y = 1; }
	else if (y1 <= -rmax) { GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_UP) = (_stickPressing[1].y != -1); _stickPressing[1].y = -1; }
	else if (fabsf(y1) <= rmin) { _stickPressing[1].y = 0; }
}

bool XboxJoystick::IsConnected() const
{
	return _gamepad != nullptr;
}

size_t XboxJoystick::GetStickCount() const
{
	return 2;
}

ff::PointFloat XboxJoystick::GetStickPos(size_t nStick, bool bDigital) const
{
	ff::PointFloat pos(0, 0);

	if (bDigital)
	{
		switch (nStick)
		{
		case 0: return _stickPressing[0]; break;
		case 1: return _stickPressing[1]; break;
		default: assert(false);
		}
	}
	else
	{
		switch (nStick)
		{
		case 0:
			pos.x = (float)_state.LeftThumbstickX;
			pos.y = (float)-_state.LeftThumbstickY;
			break;

		case 1:
			pos.x = (float)_state.RightThumbstickX;
			pos.y = (float)_state.RightThumbstickY;
			break;

		default:
			assert(false);
		}
	}

	return pos;
}

ff::RectInt XboxJoystick::GetStickPressCount(size_t nStick) const
{
	ff::RectInt press(0, 0, 0, 0);

	switch (nStick)
	{
	case 0:
		press.left = GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_LEFT);
		press.right = GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT);
		press.top = GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_UP);
		press.bottom = GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_DOWN);
		break;

	case 1:
		press.left = GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT);
		press.right = GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT);
		press.top = GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_UP);
		press.bottom = GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN);
		break;

	default:
		assert(false);
	}

	return press;
}

ff::String XboxJoystick::GetStickName(size_t nStick) const
{
	switch (nStick)
	{
	case 0: return ff::GetThisModule().GetString(ff::String(L"XBOX_PAD_LEFT"));
	case 1: return ff::GetThisModule().GetString(ff::String(L"XBOX_PAD_RIGHT"));
	default: assertRetVal(false, ff::GetThisModule().GetString(ff::String(L"INPUT_STICK_UNKNOWN")));
	}
}

size_t XboxJoystick::GetDPadCount() const
{
	return 1;
}

ff::PointInt XboxJoystick::GetDPadPos(size_t nDPad) const
{
	ff::PointInt pos(0, 0);

	if (!nDPad)
	{
		pos.x = WasPressed(Windows::Gaming::Input::GamepadButtons::DPadLeft, _state.Buttons) ? -1 :
			(WasPressed(Windows::Gaming::Input::GamepadButtons::DPadRight, _state.Buttons) ? 1 : 0);

		pos.y = WasPressed(Windows::Gaming::Input::GamepadButtons::DPadUp, _state.Buttons) ? -1 :
			(WasPressed(Windows::Gaming::Input::GamepadButtons::DPadDown, _state.Buttons) ? 1 : 0);
	}

	return pos;
}

ff::RectInt XboxJoystick::GetDPadPressCount(size_t nDPad) const
{
	ff::RectInt press(0, 0, 0, 0);

	if (!nDPad)
	{
		press.left = GetPressed(VK_GAMEPAD_DPAD_LEFT);
		press.right = GetPressed(VK_GAMEPAD_DPAD_RIGHT);
		press.top = GetPressed(VK_GAMEPAD_DPAD_UP);
		press.bottom = GetPressed(VK_GAMEPAD_DPAD_DOWN);
	}

	return press;
}

ff::String XboxJoystick::GetDPadName(size_t nDPad) const
{
	switch (nDPad)
	{
	case 0: return ff::GetThisModule().GetString(ff::String(L"XBOX_PAD_DIGITAL"));
	default: assertRetVal(false, ff::GetThisModule().GetString(ff::String(L"INPUT_STICK_UNKNOWN")));
	}
}

size_t XboxJoystick::GetButtonCount() const
{
	return 10;
}

bool XboxJoystick::GetButton(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return WasPressed(Windows::Gaming::Input::GamepadButtons::A, _state.Buttons);
	case 1: return WasPressed(Windows::Gaming::Input::GamepadButtons::B, _state.Buttons);
	case 2: return WasPressed(Windows::Gaming::Input::GamepadButtons::X, _state.Buttons);
	case 3: return WasPressed(Windows::Gaming::Input::GamepadButtons::Y, _state.Buttons);
	case 4: return WasPressed(Windows::Gaming::Input::GamepadButtons::View, _state.Buttons);
	case 5: return WasPressed(Windows::Gaming::Input::GamepadButtons::Menu, _state.Buttons);
	case 6: return WasPressed(Windows::Gaming::Input::GamepadButtons::LeftShoulder, _state.Buttons);
	case 7: return WasPressed(Windows::Gaming::Input::GamepadButtons::RightShoulder, _state.Buttons);
	case 8: return WasPressed(Windows::Gaming::Input::GamepadButtons::LeftThumbstick, _state.Buttons);
	case 9: return WasPressed(Windows::Gaming::Input::GamepadButtons::RightThumbstick, _state.Buttons);
	default: assertRetVal(false, false);
	}
}

int XboxJoystick::GetButtonPressCount(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return GetPressed(VK_GAMEPAD_A);
	case 1: return GetPressed(VK_GAMEPAD_B);
	case 2: return GetPressed(VK_GAMEPAD_X);
	case 3: return GetPressed(VK_GAMEPAD_Y);
	case 4: return GetPressed(VK_GAMEPAD_VIEW);
	case 5: return GetPressed(VK_GAMEPAD_MENU);
	case 6: return GetPressed(VK_GAMEPAD_LEFT_SHOULDER);
	case 7: return GetPressed(VK_GAMEPAD_RIGHT_SHOULDER);
	case 8: return GetPressed(VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON);
	case 9: return GetPressed(VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON);
	default: assertRetVal(false, 0);
	}
}

ff::String XboxJoystick::GetButtonName(size_t nButton) const
{
	switch (nButton)
	{
	case 0: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_A"));
	case 1: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_B"));
	case 2: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_X"));
	case 3: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_Y"));
	case 4: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_BACK"));
	case 5: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_START"));
	case 6: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_LSHOULDER"));
	case 7: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_RSHOULDER"));
	case 8: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_LTHUMB"));
	case 9: return ff::GetThisModule().GetString(ff::String(L"XBOX_BUTTON_RTHUMB"));
	default: assertRetVal(false, ff::String());
	}
}

size_t XboxJoystick::GetTriggerCount() const
{
	return 2;
}

float XboxJoystick::GetTrigger(size_t nTrigger, bool bDigital) const
{
	if (bDigital)
	{
		switch (nTrigger)
		{
		case 0: return _triggerPressing[0];
		case 1: return _triggerPressing[1];
		default: assertRetVal(false, 0);
		}
	}
	else
	{
		switch (nTrigger)
		{
		case 0: return (float)_state.LeftTrigger;
		case 1: return (float)_state.RightThumbstickY;
		default: assertRetVal(false, 0);
		}
	}
}

int XboxJoystick::GetTriggerPressCount(size_t nTrigger) const
{
	switch (nTrigger)
	{
	case 0: return GetPressed(VK_GAMEPAD_LEFT_TRIGGER);
	case 1: return GetPressed(VK_GAMEPAD_RIGHT_TRIGGER);
	default: assertRetVal(false, 0);
	}
}

ff::String XboxJoystick::GetTriggerName(size_t nTrigger) const
{
	switch (nTrigger)
	{
	case 0: return ff::GetThisModule().GetString(ff::String(L"XBOX_LTRIGGER"));
	case 1: return ff::GetThisModule().GetString(ff::String(L"XBOX_RTRIGGER"));
	default: assertRetVal(false, ff::String());
	}
}

BYTE &XboxJoystick::GetPressed(int vk)
{
	assert(vk >= ff::VK_GAMEPAD_FIRST && vk < ff::VK_GAMEPAD_FIRST + ff::VK_GAMEPAD_COUNT);
	return _pressed[vk - ff::VK_GAMEPAD_FIRST];
}

BYTE XboxJoystick::GetPressed(int vk) const
{
	assert(vk >= ff::VK_GAMEPAD_FIRST && vk < ff::VK_GAMEPAD_FIRST + ff::VK_GAMEPAD_COUNT);
	return _pressed[vk - ff::VK_GAMEPAD_FIRST];
}

bool XboxJoystick::HasSpecialButton(ff::ESpecialJoystickButton button) const
{
	switch (button)
	{
	case ff::JOYSTICK_BUTTON_BACK:
	case ff::JOYSTICK_BUTTON_START:
	case ff::JOYSTICK_BUTTON_LEFT_BUMPER:
	case ff::JOYSTICK_BUTTON_RIGHT_BUMPER:
	case ff::JOYSTICK_BUTTON_LEFT_STICK:
	case ff::JOYSTICK_BUTTON_RIGHT_STICK:
	case ff::JOYSTICK_BUTTON_A:
	case ff::JOYSTICK_BUTTON_B:
	case ff::JOYSTICK_BUTTON_X:
	case ff::JOYSTICK_BUTTON_Y:
		return true;
	}

	return false;
}

bool XboxJoystick::GetSpecialButton(ff::ESpecialJoystickButton button) const
{
	switch (button)
	{
	case ff::JOYSTICK_BUTTON_BACK:
		return GetButton(4);

	case ff::JOYSTICK_BUTTON_START:
		return GetButton(5);

	case ff::JOYSTICK_BUTTON_LEFT_BUMPER:
		return GetButton(6);

	case ff::JOYSTICK_BUTTON_RIGHT_BUMPER:
		return GetButton(7);

	case ff::JOYSTICK_BUTTON_LEFT_STICK:
		return GetButton(9);

	case ff::JOYSTICK_BUTTON_RIGHT_STICK:
		return GetButton(10);

	case ff::JOYSTICK_BUTTON_A:
		return GetButton(0);

	case ff::JOYSTICK_BUTTON_B:
		return GetButton(1);

	case ff::JOYSTICK_BUTTON_X:
		return GetButton(2);

	case ff::JOYSTICK_BUTTON_Y:
		return GetButton(3);
	}

	assertRetVal(false, false);
}

int XboxJoystick::GetSpecialButtonPressCount(ff::ESpecialJoystickButton button) const
{
	switch (button)
	{
	case ff::JOYSTICK_BUTTON_BACK:
		return GetButtonPressCount(4);

	case ff::JOYSTICK_BUTTON_START:
		return GetButtonPressCount(5);

	case ff::JOYSTICK_BUTTON_LEFT_BUMPER:
		return GetButtonPressCount(6);

	case ff::JOYSTICK_BUTTON_RIGHT_BUMPER:
		return GetButtonPressCount(7);

	case ff::JOYSTICK_BUTTON_LEFT_STICK:
		return GetButtonPressCount(9);

	case ff::JOYSTICK_BUTTON_RIGHT_STICK:
		return GetButtonPressCount(10);

	case ff::JOYSTICK_BUTTON_A:
		return GetButtonPressCount(0);

	case ff::JOYSTICK_BUTTON_B:
		return GetButtonPressCount(1);

	case ff::JOYSTICK_BUTTON_X:
		return GetButtonPressCount(2);

	case ff::JOYSTICK_BUTTON_Y:
		return GetButtonPressCount(3);
	}

	assertRetVal(false, 0);
}

ff::String XboxJoystick::GetSpecialButtonName(ff::ESpecialJoystickButton button) const
{
	switch (button)
	{
	case ff::JOYSTICK_BUTTON_BACK:
		return GetButtonName(4);

	case ff::JOYSTICK_BUTTON_START:
		return GetButtonName(5);

	case ff::JOYSTICK_BUTTON_LEFT_BUMPER:
		return GetButtonName(6);

	case ff::JOYSTICK_BUTTON_RIGHT_BUMPER:
		return GetButtonName(7);

	case ff::JOYSTICK_BUTTON_LEFT_STICK:
		return GetButtonName(9);

	case ff::JOYSTICK_BUTTON_RIGHT_STICK:
		return GetButtonName(10);

	case ff::JOYSTICK_BUTTON_A:
		return GetButtonName(0);

	case ff::JOYSTICK_BUTTON_B:
		return GetButtonName(1);

	case ff::JOYSTICK_BUTTON_X:
		return GetButtonName(2);

	case ff::JOYSTICK_BUTTON_Y:
		return GetButtonName(3);
	}

	assertRetVal(false, ff::String());
}

Windows::Gaming::Input::Gamepad ^XboxJoystick::GetGamepad() const
{
	return _gamepad;
}

void XboxJoystick::SetGamepad(Windows::Gaming::Input::Gamepad ^gamepad)
{
	_gamepad = gamepad;
}

#endif
