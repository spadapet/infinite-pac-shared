#include "pch.h"
#include "Globals/ProcessGlobals.h"
#include "Globals/ProcessStartup.h"
#include "Thread/ThreadUtil.h"
#include "Windows/WinUtil.h"

ff::PointInt ff::GetClientSize(PWND window)
{
	return ff::GetClientRect(window).Size();
}

#if METRO_APP

ff::RectInt ff::GetClientRect(PWND window)
{
	Windows::Foundation::Rect rect = window->Bounds;
	float dpi = Windows::Graphics::Display::DisplayInformation::GetForCurrentView()->LogicalDpi;
	float scale = dpi / 96.0f;

	return RectInt(
		(int)(rect.X * scale),
		(int)(rect.Y * scale),
		(int)((rect.X + rect.Width) * scale),
		(int)((rect.Y + rect.Height) * scale));
}

#else // !METRO_APP

typedef ff::Vector<wchar_t, 512> StackCharVector512;

// STATIC_DATA (pod)
static bool m_bGotQuitMessage = false;
static UINT m_nNextAppMessage = WM_APP;
static long s_nWaitCursor = 0;
static long s_modalDialogCount = 0;

// STATIC_DATA (object)
static ff::Vector<HWND, 8> s_modelessDialogs;
static ff::Vector<ff::MessageFilterFunc, 8> s_messageFilters;

static ff::ProcessShutdown Cleanup([]()
{
	s_modelessDialogs.ClearAndReduce();
	s_messageFilters.ClearAndReduce();
});

void ff::AddMessageFilter(MessageFilterFunc pFilterFunc)
{
	assertRet(pFilterFunc);

	LockMutex crit(GCS_MESSAGE_FILTER);
	s_messageFilters.Push(pFilterFunc);
}

void ff::RemoveMessageFilter(MessageFilterFunc pFilterFunc)
{
	assertRet(pFilterFunc);

	LockMutex crit(GCS_MESSAGE_FILTER);

	for (size_t i = 0; i < s_messageFilters.Size(); i++)
	{
		if (s_messageFilters[i] == pFilterFunc)
		{
			// don't remove the item from the vector yet (in case an item
			// is removed while it is executing)
			s_messageFilters[i] = nullptr;
		}
	}
}

void ff::AddModelessDialog(HWND hwnd)
{
	LockMutex crit(GCS_WIN_UTIL);

	assertRet(hwnd && s_modelessDialogs.Find(hwnd) == INVALID_SIZE);

	s_modelessDialogs.Push(hwnd);
}

void ff::RemoveModelessDialog(HWND hwnd)
{
	LockMutex crit(GCS_WIN_UTIL);

	size_t i = s_modelessDialogs.Find(hwnd);
	assertRet(i != INVALID_SIZE);

	s_modelessDialogs.Delete(i);
}

bool ff::IsModelessDialog(HWND hwnd)
{
	LockMutex crit(GCS_WIN_UTIL);

	return s_modelessDialogs.Find(hwnd) != INVALID_SIZE;
}

bool ff::IsShowingModalDialog()
{
	return InterlockedAccess(s_modalDialogCount) > 0;
}

static bool FilterMessage(MSG &msg, ff::IThreadMessageFilter *filter)
{
	if (filter && filter->FilterMessage(msg))
	{
		return true;
	}

	if (s_messageFilters.Size())
	{
		ff::LockMutex crit(ff::GCS_MESSAGE_FILTER);

		bool bFoundEmptyFilter = false;

		for (size_t i = 0; i < s_messageFilters.Size(); i++)
		{
			ff::MessageFilterFunc pFilterFunc = s_messageFilters[i];

			if (pFilterFunc)
			{
				if (pFilterFunc(msg))
				{
					return true;
				}
			}
			else
			{
				bFoundEmptyFilter = true;
			}
		}

		if (bFoundEmptyFilter)
		{
			// now really remove any deleted filters

			for (size_t i = ff::PreviousSize(s_messageFilters.Size()); i != ff::INVALID_SIZE; i = ff::PreviousSize(i))
			{
				if (!s_messageFilters[i])
				{
					s_messageFilters.Delete(i);
				}
			}
		}
	}

	for (size_t i = ff::PreviousSize(s_modelessDialogs.Size()); i != ff::INVALID_SIZE; i = ff::PreviousSize(i))
	{
		if (::IsDialogMessage(s_modelessDialogs[i], &msg))
		{
			return true;
		}
	}

	return false;
}

void ff::HandleMessage(MSG &msg, IThreadMessageFilter *filter)
{
	if (!FilterMessage(msg, filter))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
}

bool ff::HandleMessages(IThreadMessageFilter *filter)
{
	// flush thread APCs
	::MsgWaitForMultipleObjectsEx(0, nullptr, 0, QS_ALLEVENTS, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);

	MSG msg;
	while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		if (msg.message != WM_QUIT)
		{
			HandleMessage(msg, filter);
		}
		else
		{
			m_bGotQuitMessage = true;
		}
	}

	return !m_bGotQuitMessage;
}

bool ff::WaitForMessage(IThreadMessageFilter *filter, DWORD timeout)
{
	if (!m_bGotQuitMessage)
	{
		// This is better than calling GetMessage because it allows APCs to be called

		DWORD nResult = ::MsgWaitForMultipleObjectsEx(
			0, // count
			nullptr, // handles
			timeout ? timeout : INFINITE,
			QS_ALLINPUT, // wake mask
			MWMO_ALERTABLE | MWMO_INPUTAVAILABLE); // flags
	}

	return HandleMessages(filter);
}

bool ff::GotQuitMessage()
{
	return m_bGotQuitMessage;
}

void ff::PostQuitMessage()
{
	::PostQuitMessage(0);
}

UINT ff::CreateAppMessage()
{
	return InterlockedIncrement(&m_nNextAppMessage);
}

void ff::PostMessageOnce(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	MSG msg;
	if (!::PeekMessage(&msg, hwnd, nMsg, nMsg, PM_NOREMOVE) ||
		msg.hwnd != hwnd ||
		msg.message != nMsg)
	{
		::PostMessage(hwnd, nMsg, wParam, lParam);
	}
}

void ff::RepostMessageOnce(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	MSG msg;
	while (::PeekMessage(&msg, hwnd, nMsg, nMsg, PM_REMOVE))
	{
		// remove all existing messages
	}

	::PostMessage(hwnd, nMsg, wParam, lParam);
}

ff::PointInt ff::GetMessageCursorPos()
{
	DWORD nPos = ::GetMessagePos();
	POINTS pos = MAKEPOINTS(nPos);

	return PointInt(pos.x, pos.y);
}

ff::PointFloat ff::GetMessageCursorPosF()
{
	DWORD nPos = ::GetMessagePos();
	POINTS pos = MAKEPOINTS(nPos);

	return PointFloat((float)pos.x, (float)pos.y);
}

ff::PointInt ff::GetMessageCursorPos(HWND hwnd)
{
	return ff::ScreenToClient(hwnd, ff::GetMessageCursorPos());
}

ff::PointFloat ff::GetMessageCursorPosF(HWND hwnd)
{
	ff::PointInt pos = ff::ScreenToClient(hwnd, ff::GetMessageCursorPos());
	return PointFloat((float)pos.x, (float)pos.y);
}

ff::PointInt ff::GetCursorPos()
{
	POINT pt;
	
	return ::GetCursorPos(&pt)
		? ToPoint(pt)
		: PointInt(0, 0);
}

HCURSOR ff::LoadCursor(HINSTANCE instance, UINT id)
{
	return (HCURSOR)::LoadImage(instance, MAKEINTRESOURCE(id), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
}

ff::PointInt ff::GetCursorPos(HWND hwnd)
{
	return ff::ScreenToClient(hwnd, ff::GetCursorPos());
}

HWND ff::WindowFromPoint(PointInt point)
{
	return ::WindowFromPoint(ToPOINT(point));
}

HWND ff::WindowFromCursor()
{
	return WindowFromPoint(GetCursorPos());
}

void ff::ReleaseCapture(HWND hwnd)
{
	if (hwnd && ::GetCapture() == hwnd)
	{
		::ReleaseCapture();
	}
}

bool ff::IsKeyDown(int vk)
{
	return GetKeyState(vk) < 0;
}

POINT ff::ToPOINT(PointInt pt)
{
	return *(POINT*)&pt.x;
}

POINT ff::ToPOINT(LPARAM lp)
{
	POINT pt = { GET_X_LPARAM(lp), GET_Y_LPARAM(lp) };
	return pt;
}

ff::PointInt ff::ToPoint(POINT pt)
{
	return PointInt(pt.x, pt.y);
}

ff::PointInt ff::ToPoint(POINTL pt)
{
	return PointInt(pt.x, pt.y);
}

ff::PointInt ff::ToPoint(POINTS pt)
{
	return PointInt(pt.x, pt.y);
}

ff::PointInt ff::ToPoint(LPARAM lp)
{
	return PointInt(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
}

ff::String ff::GetWindowText(HWND hwnd)
{
	assertRetVal(hwnd, String());

	StackCharVector512 data;

	int len = ::GetWindowTextLength(hwnd) + 1;
	data.Resize(len);

	::GetWindowText(hwnd, data.Data(), len);

	String szText(data.Data());
	return szText;
}

ff::String ff::GetDialogItemText(HWND hwnd, UINT id)
{
	return GetWindowText(::GetDlgItem(hwnd, id));
}

void ff::EnsureWindowParent(HWND child, HWND parent)
{
	if (::GetParent(child) != parent)
	{
		if (::SetParent(child, parent) != nullptr)
		{
			// MSDN says to use WM_CHANGEUISTATE and/or WM_UPDATEUISTATE
		}
	}
}

bool ff::IsAncestor(HWND child, HWND parent)
{
	assertRetVal(child && parent, false);

	for (HWND curParent = ::GetParent(child); curParent != nullptr; curParent = ::GetParent(curParent))
	{
		if (curParent == parent)
		{
			return true;
		}
	}

	return false;
}

void ff::CenterWindow(HWND hChild, HWND hParent)
{
	assertRet(hChild);

	RectInt rect;
	RectInt rectParent;
	RectInt rectWorkArea = GetWorkArea(nullptr);

	if (!hParent)
	{
		rectParent = rectWorkArea;
	}
	else
	{
		rectParent = GetWindowRect(hParent);
	}

	rect = GetWindowRect(hChild);

	rect.CenterWithin(rectParent);
	rect.MoveInside(rectWorkArea);

	MoveWindow(hChild, rect);
}

void ff::MoveWindow(HWND hwnd, RectInt rect, bool redraw)
{
	noAssertRet(hwnd);

	::MoveWindow(hwnd, rect.left, rect.top, rect.Width(), rect.Height(), redraw);
}

bool ff::SetWindowPos(HWND hwnd, HWND hwndInsertAfter, RectInt rect, UINT flags)
{
	noAssertRetVal(hwnd, false);

	return ::SetWindowPos(hwnd, hwndInsertAfter, rect.left, rect.top, rect.Width(), rect.Height(), flags) ? true : false;
}

bool ff::DeferMoveWindow(HDWP &hDefer, HWND hwnd, RectInt rect, bool redraw)
{
	noAssertRetVal(hwnd && hDefer, false);

	hDefer = ::DeferWindowPos(hDefer, hwnd, nullptr, rect.left, rect.top, rect.Width(), rect.Height(),
		SWP_NOACTIVATE | SWP_NOZORDER | (redraw ? 0 : SWP_NOREDRAW));

	return true;
}

void ff::DeferSetWindowPos(HDWP hDefer, HWND hwnd, HWND hwndInsertAfter, RectInt rect, UINT flags)
{
	noAssertRet(hwnd);

	::DeferWindowPos(hDefer, hwnd, hwndInsertAfter, rect.left, rect.top, rect.Width(), rect.Height(), flags);
}

int ff::HandleHScroll(HWND hwnd, WPARAM wParam, int lineJump, int pageJump, bool redraw)
{
	SCROLLINFO si;
	ZeroObject(si);
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	::GetScrollInfo(hwnd, SB_HORZ, &si);

	switch (LOWORD(wParam))
	{
		case SB_THUMBPOSITION: si.nPos = si.nTrackPos; break;
		case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
		case SB_LEFT: si.nPos = si.nMin; break;
		case SB_RIGHT: si.nPos = si.nMax; break;
		case SB_LINELEFT: si.nPos -= lineJump; break;
		case SB_LINERIGHT: si.nPos += lineJump; break;
		case SB_PAGELEFT: si.nPos -= pageJump; break;
		case SB_PAGERIGHT: si.nPos += pageJump; break;
	}

	si.nPos = std::min(si.nMax - (int)si.nPage + 1, si.nPos);
	si.nPos = std::max(si.nMin, si.nPos);

	si.fMask = SIF_POS;
	::SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

	if (redraw)
	{
		::InvalidateRect(hwnd, nullptr, TRUE);
	}

	return si.nPos;
}

int ff::HandleVScroll(HWND hwnd, WPARAM wParam, int lineJump, int pageJump, bool redraw)
{
	SCROLLINFO si;
	ZeroObject(si);
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	::GetScrollInfo(hwnd, SB_VERT, &si);

	switch (LOWORD(wParam))
	{
		case SB_THUMBPOSITION: si.nPos = si.nTrackPos; break;
		case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
		case SB_TOP: si.nPos = si.nMin; break;
		case SB_BOTTOM: si.nPos = si.nMax; break;
		case SB_LINEUP: si.nPos -= lineJump; break;
		case SB_LINEDOWN: si.nPos += lineJump; break;
		case SB_PAGEUP: si.nPos -= pageJump; break;
		case SB_PAGEDOWN: si.nPos += pageJump; break;
	}

	si.nPos = std::min(si.nMax - (int)si.nPage + 1, si.nPos);
	si.nPos = std::max(si.nMin, si.nPos);

	si.fMask = SIF_POS;
	::SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

	if (redraw)
	{
		::InvalidateRect(hwnd, nullptr, TRUE);
	}

	return si.nPos;
}

void ff::HandleWheelScroll(HWND hwnd, UINT msg, WPARAM wParam, PointInt &pos)
{
	switch (msg)
	{
	case WM_MOUSEWHEEL:
		pos.y += GET_WHEEL_DELTA_WPARAM(wParam);

		while (pos.y >= WHEEL_DELTA)
		{
			pos.y -= WHEEL_DELTA;
			::SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
			::SendMessage(hwnd, WM_VSCROLL, SB_ENDSCROLL, 0);
		}

		while (pos.y <= -WHEEL_DELTA)
		{
			pos.y += WHEEL_DELTA;
			::SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
			::SendMessage(hwnd, WM_VSCROLL, SB_ENDSCROLL, 0);
		}
		break;

	case WM_MOUSEHWHEEL:
		pos.x += GET_WHEEL_DELTA_WPARAM(wParam);

		while (pos.x >= WHEEL_DELTA)
		{
			pos.x -= WHEEL_DELTA;
			::SendMessage(hwnd, WM_HSCROLL, SB_LINERIGHT, 0);
			::SendMessage(hwnd, WM_HSCROLL, SB_ENDSCROLL, 0);
		}

		while (pos.x <= -WHEEL_DELTA)
		{
			pos.x += WHEEL_DELTA;
			::SendMessage(hwnd, WM_HSCROLL, SB_LINELEFT, 0);
			::SendMessage(hwnd, WM_HSCROLL, SB_ENDSCROLL, 0);
		}
		break;

	default:
		assert(false);
		break;
	}
}

void ff::SetScrollArea(HWND hwnd, RectInt area, PointInt visible, bool bHideNoScroll)
{
	SCROLLINFO si;
	ZeroObject(si);

	si.cbSize = sizeof(si);
	si.fMask = (bHideNoScroll ? 0 : SIF_DISABLENOSCROLL) | SIF_PAGE | SIF_RANGE;
	si.nMin = area.left;
	si.nMax = area.right;
	si.nPage = visible.x;
	::SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

	si.nMin = area.top;
	si.nMax = area.bottom;
	si.nPage = visible.y;
	::SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

void ff::SetScrollPos(HWND hwnd, PointInt pos)
{
	SCROLLINFO si;
	ZeroObject(si);

	bool horiz = ff::WindowHasStyle(hwnd, WS_HSCROLL);
	bool vert = ff::WindowHasStyle(hwnd, WS_VSCROLL);

	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;

	if (horiz)
	{
		si.nPos = pos.x;
		::SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
	}

	if (vert)
	{
		si.nPos = pos.y;
		::SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
	}
}

void ff::SetScrollInfo(HWND hwnd, RectInt area, PointInt visible, PointInt pos, bool bHideNoScroll)
{
	SCROLLINFO si;
	ZeroObject(si);

	si.cbSize = sizeof(si);
	si.fMask = (bHideNoScroll ? 0 : SIF_DISABLENOSCROLL) | SIF_PAGE | SIF_RANGE | SIF_POS;
	si.nMin = area.left;
	si.nMax = area.right;
	si.nPage = visible.x;
	si.nPos = pos.x;
	::SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

	si.nMin = area.top;
	si.nMax = area.bottom;
	si.nPage = visible.y;
	si.nPos = pos.y;
	::SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
}

ff::PointInt ff::GetScrollPos(HWND hwnd)
{
	SCROLLINFO si;
	ZeroObject(si);

	si.cbSize = sizeof(si);
	si.fMask = SIF_POS;

	PointInt pos(0, 0);

	if (::GetScrollInfo(hwnd, SB_HORZ, &si))
	{
		pos.x = si.nPos;
	}

	if (::GetScrollInfo(hwnd, SB_VERT, &si))
	{
		pos.y = si.nPos;
	}

	return pos;
}

ff::PointInt ff::GetThumbPos(HWND hwnd)
{
	SCROLLINFO si;
	ZeroObject(si);

	si.cbSize = sizeof(si);
	si.fMask = SIF_TRACKPOS;

	PointInt pos(0, 0);

	if (::GetScrollInfo(hwnd, SB_HORZ, &si))
	{
		pos.x = si.nPos;
	}

	if (::GetScrollInfo(hwnd, SB_VERT, &si))
	{
		pos.y = si.nPos;
	}

	return pos;
}

static ff::RectInt GetMonitorWorkArea(HMONITOR hMonitor)
{
	MONITORINFO mi;
	ff::ZeroObject(mi);
	mi.cbSize = sizeof(mi);

	if (hMonitor && ::GetMonitorInfo(hMonitor, &mi))
	{
		return ff::RectInt(mi.rcWork);
	}
	else
	{
		// gotta return something
		assertRetVal(false, ff::RectInt(0, 0, 0, 0));
	}
}

static HMONITOR GetMonitorForWindow(HWND hwnd)
{
	POINT ptPrimary = { 0, 0 };

	return hwnd
		? ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)
		: ::MonitorFromPoint(ptPrimary, MONITOR_DEFAULTTOPRIMARY);
}

ff::RectInt ff::GetWorkArea(HWND hwnd)
{
	HMONITOR hMonitor = GetMonitorForWindow(hwnd);
	return GetMonitorWorkArea(hMonitor);
}

ff::RectInt ff::GetWorkArea(PointInt point)
{
	return GetWorkArea(RectInt(point, point));
}

ff::RectInt ff::GetWorkArea(RectInt rect)
{
	POINT ptPrimary = { 0, 0 };
	RECT winRect = rect.ToRECT();

	HMONITOR hMonitor = ::MonitorFromRect(&winRect, MONITOR_DEFAULTTOPRIMARY);

	return GetMonitorWorkArea(hMonitor);
}

ff::RectInt ff::GetMonitorArea(HWND hwnd)
{
	MONITORINFO mi;
	ZeroObject(mi);
	mi.cbSize = sizeof(mi);

	HMONITOR monitor = GetMonitorForWindow(hwnd);
	if (monitor && ::GetMonitorInfo(monitor, &mi))
	{
		return RectInt(mi.rcMonitor);
	}

	// gotta return something
	assertRetVal(false, RectInt(0, 0, 0, 0));
}

ff::RectInt ff::GetClientRect(HWND hwnd)
{
	assertRetVal(hwnd, RectInt(0, 0, 0, 0));

	RECT rect;
	::GetClientRect(hwnd, &rect);

	return RectInt(rect);
}

ff::RectInt ff::GetWindowRect(HWND hwnd)
{
	assertRetVal(hwnd, RectInt(0, 0, 0, 0));

	RECT rect;
	::GetWindowRect(hwnd, &rect);

	return RectInt(rect);
}

ff::RectInt ff::GetClientOnWindow(HWND hwnd)
{
	assertRetVal(hwnd, RectInt(0, 0, 0, 0));

	RectInt rectWindow = GetWindowRect(hwnd);
	RectInt rectClient = ClientToScreen(hwnd, GetClientRect(hwnd));

	return RectInt(rectClient - rectWindow.TopLeft());
}

ff::RectInt ff::GetClientOnScreen(HWND hwnd)
{
	assertRetVal(hwnd, RectInt(0, 0, 0, 0));

	return ClientToScreen(hwnd, GetClientRect(hwnd));
}

ff::RectInt ff::GetWindowRectOnParent(HWND hwnd)
{
	assertRetVal(hwnd && GetParent(hwnd), RectInt(0, 0, 0, 0));

	RectInt rectWindow = GetWindowRect(hwnd);
	RectInt rectParent = GetWindowRect(GetParent(hwnd));

	return (rectWindow - rectParent.TopLeft());
}

ff::RectInt ff::GetWindowRectOnParentClient(HWND hwnd)
{
	assertRetVal(hwnd && GetParent(hwnd), RectInt(0, 0, 0, 0));

	RectInt rectWindow = GetWindowRect(hwnd);
	RectInt rectParent = GetClientOnScreen(GetParent(hwnd));

	return (rectWindow - rectParent.TopLeft());
}

ff::RectInt ff::ClientToScreen(HWND hwnd, RectInt rect)
{
	assertRetVal(hwnd, RectInt(0, 0, 0, 0));

	::ClientToScreen(hwnd, (LPPOINT)&rect.arr[0]);
	::ClientToScreen(hwnd, (LPPOINT)&rect.arr[2]);

	return rect;
}

ff::PointInt ff::ClientToScreen(HWND hwnd, PointInt point)
{
	assertRetVal(hwnd, PointInt(0, 0));

	::ClientToScreen(hwnd, (LPPOINT)&point.arr[0]);

	return point;
}

ff::RectInt ff::ScreenToClient(HWND hwnd, RectInt rect)
{
	assertRetVal(hwnd, RectInt(0, 0, 0, 0));

	::ScreenToClient(hwnd, (LPPOINT)&rect.arr[0]);
	::ScreenToClient(hwnd, (LPPOINT)&rect.arr[2]);

	return rect;
}

ff::PointInt ff::ScreenToClient(HWND hwnd, PointInt point)
{
	assertRetVal(hwnd, PointInt(0, 0));

	::ScreenToClient(hwnd, (LPPOINT)&point.arr[0]);

	return point;
}

ff::RectInt ff::ClientToClient(HWND hWndFrom, HWND hWndTo, RectInt rect)
{
	::MapWindowPoints(hWndFrom, hWndTo, (LPPOINT)&rect.arr[0], 2);

	return rect;
}

ff::PointInt ff::ClientToClient(HWND hWndFrom, HWND hWndTo, PointInt point)
{
	::MapWindowPoints(hWndFrom, hWndTo, (LPPOINT)&point.arr[0], 1);

	return point;
}

ff::PointInt ff::WindowSizeFromClientSize(HWND hwnd, PointInt ptClientSize)
{
	assertRetVal(hwnd, ptClientSize);

	RECT rect = { 0, 0, ptClientSize.x, ptClientSize.y };

	if (::AdjustWindowRectEx(&rect,
		GetWindowLong(hwnd, GWL_STYLE),
		GetMenu(hwnd) ? TRUE : FALSE,
		GetWindowLong(hwnd, GWL_EXSTYLE)))
	{
		return PointInt(rect.right - rect.left, rect.bottom - rect.top);
	}

	return ptClientSize;
}

bool ff::WindowHasStyle(HWND hwnd, LONG style)
{
	return hwnd && style && (::GetWindowLong(hwnd, GWL_STYLE) & style) == style;
}

bool ff::WindowHasStyleEx(HWND hwnd, LONG style)
{
	return hwnd && style && (::GetWindowLong(hwnd, GWL_EXSTYLE) & style) == style;
}

bool ff::IsWindowVisibleStyle(HWND hwnd)
{
	return ff::WindowHasStyle(hwnd, WS_VISIBLE);
}

void ff::EnsureWindowVisible(HWND hwnd, bool bVisible)
{
	if (hwnd)
	{
		bool wasVisible = ff::IsWindowVisibleStyle(hwnd);

		if (bVisible && !wasVisible)
		{
			::ShowWindow(hwnd, SW_SHOW);
		}
		else if (!bVisible && wasVisible)
		{
			::ShowWindow(hwnd, SW_HIDE);
		}
	}
}

void ff::EnsureWindowEnabled(HWND hwnd, bool bEnabled)
{
	if (hwnd)
	{
		bool wasEnabled = ::IsWindowEnabled(hwnd) ? true : false;

		if (bEnabled && !wasEnabled)
		{
			::EnableWindow(hwnd, TRUE);
		}
		else if (!bEnabled && wasEnabled)
		{
			::EnableWindow(hwnd, FALSE);
		}
	}
}

static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
	ff::Vector<HWND> *children = (ff::Vector<HWND> *)lParam;
	children->Push(hwnd);
	return TRUE;
}

ff::Vector<HWND> ff::GetChildWindows(HWND hwnd)
{
	ff::Vector<HWND> children;
	::EnumChildWindows(hwnd, EnumChildProc, (LONG_PTR)&children);

	return children;
}

void ff::DestroyChildWindows(HWND hwnd)
{
	noAssertRet(hwnd);

	ff::Vector<HWND> children = ff::GetChildWindows(hwnd);
	for (HWND child : children)
	{
		if (::IsWindow(child) && ::GetParent(child) == hwnd)
		{
			::DestroyWindow(child);
		}
	}
}

static bool WaitCursorMessageFilter(MSG &msg)
{
	assert(s_nWaitCursor > 0);

	if (msg.message == WM_SETCURSOR)
	{
		::SetCursor(::LoadCursor(nullptr, IDC_WAIT));
		return true;
	}

	return false;
}

ff::WaitCursor::WaitCursor()
{
	LockMutex crit(GCS_WIN_UTIL);

	long nWaitCursor = InterlockedIncrement(&s_nWaitCursor);
	assert(nWaitCursor > 0);

	::SetCursor(::LoadCursor(nullptr, IDC_WAIT));

	if (nWaitCursor == 1)
	{
		AddMessageFilter(WaitCursorMessageFilter);
	}
}

ff::WaitCursor::~WaitCursor()
{
	LockMutex crit(GCS_WIN_UTIL);

	long nWaitCursor = InterlockedDecrement(&s_nWaitCursor);
	assert(nWaitCursor >= 0);

	if (!nWaitCursor)
	{
		RemoveMessageFilter(WaitCursorMessageFilter);

		// Get rid of the hourglass by updating the cursor
		POINT point;
		if (::GetCursorPos(&point))
		{
			::SetCursorPos(point.x, point.y);
		}
	}
}

ff::Window::Window()
	: _hwnd(nullptr)
{
}

ff::Window::Window(Window &&rhs)
	: _hwnd(rhs._hwnd)
{
	rhs._hwnd = nullptr;
}

ff::Window::~Window()
{
	assert(!_hwnd);
	_hwnd = nullptr;
}

HWND ff::Window::Handle() const
{
	return _hwnd;
}

void ff::Window::PostNcDestroy()
{
	// do nothing
}

ff::CustomWindow::CustomWindow()
{
}

ff::CustomWindow::CustomWindow(CustomWindow &&rhs)
	: Window(std::move(rhs))
{
	if (_hwnd)
	{
		SetWindowLongPtr(_hwnd, 0, (ULONG_PTR)this);
	}
}

ff::CustomWindow::~CustomWindow()
{
	if (_hwnd)
	{
		::DestroyWindow(_hwnd);
	}
}

// static
bool ff::CustomWindow::CreateClass(
		StringRef name,
		DWORD nStyle,
		HINSTANCE hInstance,
		HCURSOR hCursor,
		HBRUSH hBrush,
		UINT menu,
		HICON hLargeIcon,
		HICON hSmallIcon)
{
	// see if the class was already registered
	{
		WNDCLASSEX clsExisting;
		ZeroObject(clsExisting);
		clsExisting.cbSize = sizeof(clsExisting);

		if (GetClassInfoEx(hInstance, name.c_str(), &clsExisting))
		{
			return true;
		}
	}

	WNDCLASSEX clsNew =
	{
		sizeof(WNDCLASSEX), // size
		nStyle, // style
		BaseWindowProc, // window procedure
		0, // extra class bytes
		sizeof(ULONG_PTR), // extra window bytes
		hInstance, // program instance
		hLargeIcon, // Icon
		hCursor, // cursor
		hBrush, // background
		MAKEINTRESOURCE(menu), // Menu
		name.c_str(), // class name
		hSmallIcon // small icon
	};

	return (RegisterClassEx(&clsNew) != 0);
}

bool ff::CustomWindow::Create(
		StringRef className,
		StringRef windowName,
		HWND hParent,
		DWORD nStyle,
		DWORD nExStyle,
		int x, int y, int cx, int cy,
		HINSTANCE hInstance,
		HMENU hMenu)
{
	assert(!_hwnd);

	HWND hNewWindow = CreateWindowEx(
		nExStyle, // extended style
		className.c_str(), // class name
		windowName.c_str(), // window name
		nStyle, // style
		x, y, cx, cy, // x, y, width, height
		hParent, // parent
		hMenu, // menu
		hInstance, // instance
		(LPVOID)this); // parameter

	if (hNewWindow)
	{
		assert(_hwnd == hNewWindow);
	}
	else
	{
		assert(!_hwnd);
	}

	return _hwnd != nullptr;
}

bool ff::CustomWindow::CreateBlank(
		StringRef windowName,
		HWND hParent,
		DWORD nStyle,
		DWORD nExStyle,
		int x, int y, int cx, int cy,
		HMENU hMenu)
{
	static StaticString className(L"BlankCustomWindow");

	assertRetVal(CreateClass(
		className.GetString(),
		CS_DBLCLKS,
		GetThisModule().GetInstance(),
		::LoadCursor(nullptr, IDC_ARROW),
		nullptr,
		0, // menu
		nullptr,
		nullptr), false);

	return Create(
		className,
		windowName,
		hParent,
		nStyle,
		nExStyle,
		x, y, cx, cy,
		GetThisModule().GetInstance(),
		hMenu);
}

bool ff::CustomWindow::CreateBlank(HWND hParent, DWORD nStyle, DWORD nExStyle, int x, int y, int cx, int cy)
{
	return CreateBlank(GetEmptyString(), hParent, nStyle, nExStyle, x, y, cx, cy, nullptr);
}

bool ff::CustomWindow::CreateMessageWindow(StringRef className, StringRef windowName)
{
	HINSTANCE hInstance = GetThisModule().GetInstance();
	String realClassName = className.size() ? className : GetMessageWindowClassName();

	return CreateClass(realClassName, 0, hInstance, nullptr, nullptr, 0, nullptr, nullptr) &&
		Create(realClassName, windowName, HWND_MESSAGE, 0, 0, 0, 0, 0, 0, hInstance, nullptr);
}

// static
ff::StringRef ff::CustomWindow::GetMessageWindowClassName()
{
	static StaticString value(L"ff::MessageOnlyWindow");
	return value;
}

LRESULT ff::CustomWindow::DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT ff::CustomWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DoDefault(hwnd, msg, wParam, lParam);
}

// static
LRESULT CALLBACK ff::CustomWindow::BaseWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_CREATE)
	{
		CustomWindow *pWnd = (CustomWindow*)(((LPCREATESTRUCT)lParam)->lpCreateParams);
		assert(pWnd && !pWnd->_hwnd);

		pWnd->_hwnd = hwnd;
		SetWindowLongPtr(hwnd, 0, (ULONG_PTR)pWnd);
	}

	CustomWindow *pWnd = (CustomWindow*)GetWindowLongPtr(hwnd, 0);

	LRESULT ret = pWnd
		? pWnd->WindowProc(hwnd, msg, wParam, lParam)
		: ::DefWindowProc(hwnd, msg, wParam, lParam);

	if (msg == WM_NCDESTROY && pWnd)
	{
		SetWindowLongPtr(hwnd, 0, 0);
		pWnd->_hwnd = nullptr;
		pWnd->PostNcDestroy();
	}

	return ret;
}

// static
bool ff::CustomWindow::IsCustomWindow(HWND hwnd)
{
	assertRetVal(hwnd, false);
	WNDPROC proc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_WNDPROC);
	return proc == BaseWindowProc;
}

HWND ff::CustomWindow::Detach()
{
	assertRetVal(_hwnd, nullptr);
	::SendMessage(_hwnd, GetDetachMessage(), 0, 0);

	// All window proc overrides must detach by now
	assertRetVal(IsCustomWindow(_hwnd), nullptr);

	HWND hwnd = _hwnd;
	_hwnd = nullptr;

	SetWindowLongPtr(hwnd, 0, 0);
	PostNcDestroy();

	return hwnd;
}

bool ff::CustomWindow::Attach(HWND hwnd)
{
	assertRetVal(!_hwnd && hwnd && IsCustomWindow(hwnd), false);

	_hwnd = hwnd;
	SetWindowLongPtr(hwnd, 0, (ULONG_PTR)this);

	return true;
}

static const UINT WM_CUSTOM_WINDOW_DETACH = ff::CreateAppMessage();
// static
UINT ff::CustomWindow::GetDetachMessage()
{
	return WM_CUSTOM_WINDOW_DETACH;
}

ff::ListenedWindow::ListenedWindow()
	: _listener(nullptr)
{
}

ff::ListenedWindow::ListenedWindow(ListenedWindow &&rhs)
	: CustomWindow(std::move(rhs))
	, _listener(rhs._listener)
{
	rhs._listener = nullptr;
}

ff::ListenedWindow::~ListenedWindow()
{
}

void ff::ListenedWindow::SetListener(IWindowProcListener *pListener)
{
	_listener = pListener;
}

LRESULT ff::ListenedWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (_listener)
	{
		LRESULT nResult = 0;

		if (_listener->ListenWindowProc(hwnd, msg, wParam, lParam, nResult))
		{
			return nResult;
		}
	}

	return DoDefault(hwnd, msg, wParam, lParam);
}

ff::ListenedDialog::ListenedDialog()
	: _listener(nullptr)
{
}

ff::ListenedDialog::ListenedDialog(ListenedDialog &&rhs)
	: Dialog(std::move(rhs))
	, _listener(rhs._listener)
{
	rhs._listener = nullptr;
}

ff::ListenedDialog::~ListenedDialog()
{
}

void ff::ListenedDialog::SetListener(IDialogProcListener *pListener)
{
	_listener = pListener;
}

INT_PTR ff::ListenedDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (_listener)
	{
		INT_PTR nResult = 0;

		if (_listener->ListenDialogProc(hwnd, msg, wParam, lParam, nResult))
		{
			return nResult;
		}
	}

	return __super::DialogProc(hwnd, msg, wParam, lParam);
}

ff::StandardWindow::StandardWindow()
	: _oldProc(nullptr)
{
}

ff::StandardWindow::StandardWindow(StandardWindow &&rhs)
	: Window(std::move(rhs))
	, _oldProc(rhs._oldProc)
{
	rhs._oldProc = nullptr;

	if (_hwnd)
	{
		SetWindowLongPtr(_hwnd, GWLP_USERDATA, (ULONG_PTR)this);
	}
}

ff::StandardWindow::~StandardWindow()
{
	if (_hwnd)
	{
		DestroyWindow(_hwnd);
	}
}

bool ff::StandardWindow::Create(
		StringRef className,
		StringRef windowName,
		HWND hParent,
		DWORD nStyle,
		DWORD nExStyle,
		int x, int y, int cx, int cy,
		HINSTANCE hInstance,
		HMENU szMenu)
{
	HWND hNewWindow = CreateWindowEx(
		nExStyle, // extended style
		className.c_str(), // class name
		windowName.c_str(), // window name
		nStyle, // style
		x, y, cx, cy, // x, y, width, height
		hParent, // parent
		szMenu, // menu
		hInstance, // instance
		(LPVOID)this); // parameter

	if (_hwnd)
	{
		if (_hwnd == hNewWindow)
		{
			return true;
		}
		else
		{
			assert(false);
		}
	}

	return hNewWindow ? Subclass(hNewWindow) : false;
}

bool ff::StandardWindow::Subclass(HWND hwnd)
{
	assert(!_hwnd && !_oldProc && hwnd);

	_hwnd = hwnd;

	SetWindowLongPtr(_hwnd, GWLP_USERDATA, (ULONG_PTR)this);
	_oldProc = SubclassWindow(_hwnd, BaseWindowProc);

	if (-1 == SendMessage(_hwnd, WM_CREATE, 0, 0))
	{
		::DestroyWindow(_hwnd);
		assert(!_hwnd && !_oldProc);
	}

	return _hwnd != nullptr;
}

LRESULT ff::StandardWindow::DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg != WM_CREATE)
	{
		return _oldProc
			? CallWindowProc(_oldProc, hwnd, msg, wParam, lParam)
			: DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

LRESULT ff::StandardWindow::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return DoDefault(hwnd, msg, wParam, lParam);
}

// static
LRESULT CALLBACK ff::StandardWindow::BaseWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	StandardWindow *pWnd = (StandardWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	LRESULT ret = pWnd
		? pWnd->WindowProc(hwnd, msg, wParam, lParam)
		: DefWindowProc(hwnd, msg, wParam, lParam);

	if (msg == WM_NCDESTROY && pWnd)
	{
		SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
		SubclassWindow(hwnd, pWnd->_oldProc);

		pWnd->_hwnd = nullptr;
		pWnd->_oldProc = nullptr;
		pWnd->PostNcDestroy();
	}

	return ret;
}

ff::Dialog::Dialog()
{
}

ff::Dialog::Dialog(Dialog &&rhs)
	: Window(std::move(rhs))
{
}

ff::Dialog::~Dialog()
{
	if (_hwnd && IsModelessDialog(_hwnd))
	{
		::DestroyWindow(_hwnd);
	}
}

INT_PTR ff::Dialog::Create(UINT nTemplate, HWND hParent, HINSTANCE hInstance, bool bModal)
{
	INT_PTR result = 0;

	if (bModal)
	{
		InterlockedIncrement(&s_modalDialogCount);

		result = DialogBoxParam(hInstance, MAKEINTRESOURCE(nTemplate), hParent, BaseDialogProc, (LPARAM)this);

		InterlockedDecrement(&s_modalDialogCount);
	}
	else
	{
		if (CreateDialogParam(hInstance, MAKEINTRESOURCE(nTemplate), hParent, BaseDialogProc, (LPARAM)this))
		{
			AddModelessDialog(_hwnd);
			return 1;
		}
	}

	return result;
}

INT_PTR ff::Dialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_DESTROY:
			if (IsModelessDialog(_hwnd))
			{
				RemoveModelessDialog(_hwnd);
			}
			break;

		case WM_COMMAND:
		{
			switch(LOWORD(lParam))
			{
				case IDOK:
					EndDialog(hwnd, IDOK);
					break;

				case IDCANCEL:
					EndDialog(hwnd, IDCANCEL);
					break;
			}
		} break;
	}

	return 0;
}

// static
INT_PTR CALLBACK ff::Dialog::BaseDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_INITDIALOG)
	{
		assert(lParam);
		((Dialog*)lParam)->_hwnd = hwnd;
		SetWindowLongPtr(hwnd, DWLP_USER, (ULONG_PTR)lParam);
		CenterWindow(hwnd, GetParent(hwnd));
	}

	Dialog *pDialog = (Dialog*)GetWindowLongPtr(hwnd, DWLP_USER);

	INT_PTR ret = pDialog
		? pDialog->DialogProc(hwnd, msg, wParam, lParam)
		: FALSE;

	if (msg == WM_NCDESTROY && pDialog)
	{
		SetWindowLongPtr(hwnd, DWLP_USER, 0);
		pDialog->_hwnd = nullptr;
		pDialog->PostNcDestroy();
	}

	return ret;
}

LRESULT ff::Dialog::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	assert(false);
	return 0;
}

LRESULT ff::Dialog::DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	assert(false);
	return 0;
}

#endif // METRO_APP
