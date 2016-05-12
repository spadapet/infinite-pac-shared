#pragma once

#if METRO_APP

#include "Windows/WinUtil.h"

namespace ff
{
	class IJoystickDevice;

	class __declspec(uuid("1ba6b747-4375-41cb-a0a2-340287461bb1")) __declspec(novtable)
		IJoystickInput : public IUnknown
	{
	public:
		virtual void Advance() = 0;
		virtual void Reset() = 0;
		virtual PWND GetWindow() const = 0;

		virtual size_t GetCount() const = 0;
		virtual IJoystickDevice *GetJoystick(size_t nJoy) const = 0;
	};

	UTIL_API bool CreateJoystickInput(PWND hwnd, IJoystickInput **obj);
}

#endif
