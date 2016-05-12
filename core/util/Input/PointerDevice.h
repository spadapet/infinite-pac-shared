#pragma once

#include "Input/InputDevice.h"
#include "Windows/WinUtil.h"

namespace ff
{
	enum class TouchType
	{
		TOUCH_TYPE_NONE,
		TOUCH_TYPE_MOUSE,
		TOUCH_TYPE_FINGER,
		TOUCH_TYPE_PEN,
	};

	struct TouchInfo
	{
		TouchType type;
		PointDouble startPos;
		PointDouble pos;
		unsigned int id;
	};

	class __declspec(uuid("dc258f87-ba84-4b7a-b75a-3ce57ad23dc9")) __declspec(novtable)
		IPointerDevice : public IInputDevice
	{
	public:
		virtual PWND GetWindow() const = 0;
		virtual bool IsInWindow() const = 0;
		virtual PointDouble GetPos() const = 0; // in window coordinates
		virtual PointDouble GetRelativePos() const = 0; // since last Advance()

		virtual bool GetButton(int vkButton) const = 0;
		virtual int GetButtonClickCount(int vkButton) const = 0; // since the last Advance()
		virtual int GetButtonReleaseCount(int vkButton) const = 0; // since the last Advance()
		virtual int GetButtonDoubleClickCount(int vkButton) const = 0; // since the last Advance()
		virtual PointDouble GetWheelScroll() const = 0; // since the last Advance()

		virtual size_t GetTouchCount() const = 0;
		virtual const TouchInfo &GetTouchInfo(size_t index) const = 0;
	};

	UTIL_API bool CreatePointerDevice(PWND hwnd, IPointerDevice **obj);
}
