#pragma once

#include "Input/InputDevice.h"
#include "Windows/WinUtil.h"

namespace ff
{
	class __declspec(uuid("85404e39-cfbe-4551-9afa-bcc726ae7b02")) __declspec(novtable)
		IKeyboardDevice : public IInputDevice
	{
	public:
		virtual PWND GetWindow() const = 0;
		virtual bool GetKey(int vk) const = 0;
		virtual int GetKeyPressCount(int vk) const = 0; // since last Advance()
		virtual String GetChars() const = 0;
	};

	UTIL_API bool CreateKeyboardDevice(PWND hwnd, IKeyboardDevice **obj);
}
