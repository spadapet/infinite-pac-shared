#pragma once

#if METRO_APP

namespace ff
{
	typedef Windows::UI::Xaml::Window ^PWND;
	UTIL_API RectInt GetClientRect(PWND window);
	UTIL_API PointInt GetClientSize(PWND window);
}

#else

namespace ff
{
	typedef HWND PWND;
	typedef bool (*MessageFilterFunc)(MSG &msg); // return true from a filter to prevent further processing

	class IThreadMessageFilter
	{
	public:
		virtual bool FilterMessage(MSG &msg) = 0;
	};

	UTIL_API void AddMessageFilter(MessageFilterFunc pFilterFunc);
	UTIL_API void RemoveMessageFilter(MessageFilterFunc pFilterFunc);
	UTIL_API void AddModelessDialog(HWND hwnd);
	UTIL_API void RemoveModelessDialog(HWND hwnd);
	UTIL_API bool IsModelessDialog(HWND hwnd);
	UTIL_API bool IsShowingModalDialog();

	UTIL_API void HandleMessage(MSG &msg, IThreadMessageFilter *filter = nullptr); // filters and dispatches messages
	UTIL_API bool HandleMessages(IThreadMessageFilter *filter = nullptr); // handles all queued messages, returns false on WM_QUIT
	UTIL_API bool WaitForMessage(IThreadMessageFilter *filter = nullptr, DWORD timeout = INFINITE); // waits for a single message and returns (false on WM_QUIT)
	UTIL_API bool GotQuitMessage(); // was WM_QUIT ever received?
	UTIL_API void PostQuitMessage(); // Post WM_QUIT
	UTIL_API UINT CreateAppMessage(); // Registers a message within the WM_APP range (0x8000 - 0xBFFF)
	UTIL_API void PostMessageOnce(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	UTIL_API void RepostMessageOnce(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	UTIL_API PointInt GetMessageCursorPos();
	UTIL_API PointFloat GetMessageCursorPosF();
	UTIL_API PointInt GetMessageCursorPos(HWND hwnd);
	UTIL_API PointFloat GetMessageCursorPosF(HWND hwnd);
	UTIL_API PointInt GetCursorPos();
	UTIL_API PointInt GetCursorPos(HWND hwnd);
	UTIL_API HCURSOR LoadCursor(HINSTANCE instance, UINT id);
	UTIL_API HWND WindowFromPoint(PointInt point);
	UTIL_API HWND WindowFromCursor();
	UTIL_API void ReleaseCapture(HWND hwnd);
	UTIL_API bool IsKeyDown(int vk);

	UTIL_API POINT ToPOINT(PointInt pt);
	UTIL_API POINT ToPOINT(LPARAM lp);
	UTIL_API PointInt ToPoint(POINT pt);
	UTIL_API PointInt ToPoint(POINTL pt);
	UTIL_API PointInt ToPoint(POINTS pt);
	UTIL_API PointInt ToPoint(LPARAM lp);

	UTIL_API String GetWindowText(HWND hwnd);
	UTIL_API String GetDialogItemText(HWND hwnd, UINT id);

	UTIL_API void EnsureWindowParent(HWND child, HWND parent);
	UTIL_API bool IsAncestor(HWND child, HWND parent);
	UTIL_API void CenterWindow(HWND hChild, HWND hParent = nullptr); // centers on the screen if hParent is nullptr
	UTIL_API void MoveWindow(HWND hwnd, RectInt rect, bool redraw = true);
	UTIL_API bool SetWindowPos(HWND hwnd, HWND hwndInsertAfter, RectInt rect, UINT flags);
	UTIL_API bool DeferMoveWindow(HDWP &hDefer, HWND hwnd, RectInt rect, bool redraw = true);
	UTIL_API void DeferSetWindowPos(HDWP hDefer, HWND hwnd, HWND hwndInsertAfter, RectInt rect, UINT flags);
	UTIL_API int HandleHScroll(HWND hwnd, WPARAM wParam, int lineJump, int pageJump, bool redraw = true);
	UTIL_API int HandleVScroll(HWND hwnd, WPARAM wParam, int lineJump, int pageJump, bool redraw = true);
	UTIL_API void HandleWheelScroll(HWND hwnd, UINT msg, WPARAM wParam, PointInt &pos);
	UTIL_API void SetScrollArea(HWND hwnd, RectInt area, PointInt visible, bool bHideNoScroll = true);
	UTIL_API void SetScrollPos(HWND hwnd, PointInt pos);
	UTIL_API void SetScrollInfo(HWND hwnd, RectInt area, PointInt visible, PointInt pos, bool bHideNoScroll = true);
	UTIL_API PointInt GetScrollPos(HWND hwnd);
	UTIL_API PointInt GetThumbPos(HWND hwnd);

	UTIL_API RectInt GetWorkArea(HWND hwnd); // hwnd can be nullptr for the main monitor
	UTIL_API RectInt GetWorkArea(PointInt point);
	UTIL_API RectInt GetWorkArea(RectInt rect);
	UTIL_API RectInt GetMonitorArea(HWND hwnd);
	UTIL_API RectInt GetClientRect(HWND hwnd);
	UTIL_API PointInt GetClientSize(PWND window);
	UTIL_API RectInt GetWindowRect(HWND hwnd);
	UTIL_API RectInt GetClientOnWindow(HWND hwnd); // maps client area to upper left of window
	UTIL_API RectInt GetClientOnScreen(HWND hwnd); // screen coords of client area
	UTIL_API RectInt GetWindowRectOnParent(HWND hwnd); // window rect based on upper left of parent window
	UTIL_API RectInt GetWindowRectOnParentClient(HWND hwnd); // window rect based on upper left of parent client area
	UTIL_API RectInt ClientToScreen(HWND hwnd, RectInt rect);
	UTIL_API PointInt ClientToScreen(HWND hwnd, PointInt point);
	UTIL_API RectInt ScreenToClient(HWND hwnd, RectInt rect);
	UTIL_API PointInt ScreenToClient(HWND hwnd, PointInt point);
	UTIL_API RectInt ClientToClient(HWND hWndFrom, HWND hWndTo, RectInt rect);
	UTIL_API PointInt ClientToClient(HWND hWndFrom, HWND hWndTo, PointInt point);
	UTIL_API PointInt WindowSizeFromClientSize(HWND hwnd, PointInt ptClientSize);

	UTIL_API bool WindowHasStyle(HWND hwnd, LONG style);
	UTIL_API bool WindowHasStyleEx(HWND hwnd, LONG style);
	UTIL_API bool IsWindowVisibleStyle(HWND hwnd);
	UTIL_API void EnsureWindowVisible(HWND hwnd, bool bVisible);
	UTIL_API void EnsureWindowEnabled(HWND hwnd, bool bEnabled);
	UTIL_API ff::Vector<HWND> GetChildWindows(HWND hwnd);
	UTIL_API void DestroyChildWindows(HWND hwnd);

	// Shows the hourglass for as long as one CWaitCursor is on the stack

	class UTIL_API WaitCursor
	{
	public:
		WaitCursor();
		~WaitCursor();
	};

	// CWindow is the parent of the other windows classes,
	// you should only derive from the other classes

	class UTIL_API Window
	{
	public:
		Window();
		Window(Window &&rhs);
		virtual ~Window();

		HWND Handle() const;

	protected:
		// this is where you handle all windows messages
		virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;

		// ALWAYS call this instead of DefWindowProc
		virtual LRESULT DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) = 0;

		// override this if you want to "delete this"
		virtual void PostNcDestroy();

		// data
		HWND _hwnd;

	private:
		Window(const Window &rhs);
		Window &operator=(const Window &rhs);
	};

	// CustomWindow is for creating windows that you also
	// registered the class for (using CreateClass)

	class UTIL_API CustomWindow : public Window
	{
	public:
		CustomWindow();
		CustomWindow(CustomWindow &&rhs);
		virtual ~CustomWindow();

		HWND Detach();
		bool Attach(HWND hwnd);
		static UINT GetDetachMessage();

		// registers a new window class
		static bool CreateClass(StringRef name, DWORD nStyle,
			HINSTANCE hInstance, HCURSOR hCursor, HBRUSH hBrush,
			UINT menu, HICON hLargeIcon, HICON hSmallIcon);

		// creates a new window
		bool Create(StringRef className, StringRef windowName, HWND hParent,
			DWORD nStyle, DWORD nExStyle,
			int x, int y, int cx, int cy,
			HINSTANCE hInstance, HMENU hMenu);

		bool CreateBlank(
			StringRef windowName,
			HWND hParent,
			DWORD nStyle,
			DWORD nExStyle = 0,
			int x = 0,
			int y = 0,
			int cx = 0,
			int cy = 0,
			HMENU hMenu = nullptr);

		bool CreateBlank(
			HWND hParent,
			DWORD nStyle,
			DWORD nExStyle = 0,
			int x = 0,
			int y = 0,
			int cx = 0,
			int cy = 0);

		bool CreateMessageWindow(StringRef className = GetEmptyString(), StringRef windowName = GetEmptyString());
		static StringRef GetMessageWindowClassName();

	protected:
		virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

		// call this instead of DefWindowProc
		virtual LRESULT DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	private:
		CustomWindow(const CustomWindow &rhs);
		CustomWindow &operator=(const CustomWindow &rhs);

		static LRESULT CALLBACK BaseWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static bool IsCustomWindow(HWND hwnd);
	};

	class IWindowProcListener
	{
	public:
		// return true to skip calling the default WindowProc
		virtual bool ListenWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT &nResult) = 0;
	};

	class UTIL_API ListenedWindow : public CustomWindow
	{
	public:
		ListenedWindow();
		ListenedWindow(ListenedWindow &&rhs);
		virtual ~ListenedWindow();

		void SetListener(IWindowProcListener *pListener);

	protected:
		virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	private:
		ListenedWindow(const ListenedWindow &rhs);
		ListenedWindow &operator=(const ListenedWindow &rhs);

		IWindowProcListener *_listener;
	};

	// CStdWindow is for subclassing of windows
	// that you didn't register the class for.

	class UTIL_API StandardWindow : public Window
	{
	public:
		StandardWindow();
		StandardWindow(StandardWindow &&rhs);
		virtual ~StandardWindow();

		// creates a new window
		bool Create(StringRef className, StringRef windowName, HWND hParent,
			DWORD nStyle, DWORD nExStyle,
			int x, int y, int cx, int cy,
			HINSTANCE hInstance, HMENU szMenu);

		// subclasses an existing window (do NOT call Create if you do this)
		bool Subclass(HWND hwnd);

	protected:
		// call this instead of DefWindowProc
		virtual LRESULT DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		// you can still override this
		virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		StandardWindow(const StandardWindow &rhs);
		StandardWindow &operator=(const StandardWindow &rhs);

		static LRESULT CALLBACK BaseWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		WNDPROC _oldProc;
	};

	// CDialog is NOT for property pages, only "real" dialogs

	class UTIL_API Dialog : public Window
	{
	public:
		Dialog();
		Dialog(Dialog &&rhs);
		virtual ~Dialog();

		// creates a modal or modeless dialog, return value =
		// Modal dialog: return value of finished modal dialog
		// Modeless dialog: 0 for failure to create, 1 for success
		INT_PTR Create(UINT nTemplate, HWND hParent, HINSTANCE hInstance, bool bModal = true);

	protected:
		// override this to handle dialog messages
		virtual INT_PTR DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	private:
		static INT_PTR CALLBACK BaseDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		// make these inaccessible
		Dialog(const Dialog &rhs);
		Dialog &operator=(const Dialog &rhs);
		virtual LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT DoDefault(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};

	class IDialogProcListener
	{
	public:
		// return true to skip calling the default DialogProp
		virtual bool ListenDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, INT_PTR &nResult) = 0;
	};

	class UTIL_API ListenedDialog : public Dialog
	{
	public:
		ListenedDialog();
		ListenedDialog(ListenedDialog &&rhs);
		virtual ~ListenedDialog();

		void SetListener(IDialogProcListener *pListener);

	protected:
		virtual INT_PTR DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;

	private:
		ListenedDialog(const ListenedDialog &rhs);
		ListenedDialog &operator=(const ListenedDialog &rhs);

		IDialogProcListener *_listener;
	};
}

#endif // METRO_APP
