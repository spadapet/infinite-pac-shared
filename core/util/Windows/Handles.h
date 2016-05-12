#pragma once

namespace ff
{
	template<typename T, typename TBase, void (*DestroyHandleFunc)(TBase)>
	class WinHandleBase
	{
		typedef WinHandleBase<T, TBase, DestroyHandleFunc> MyType;

	public:
		WinHandleBase()
			: _handle(nullptr)
		{
		}

		WinHandleBase(MyType &&rhs)
			: _handle(rhs._handle)
		{
			rhs._handle = nullptr;
		}

		WinHandleBase(T handle)
			: _handle(handle)
		{
		}

		virtual ~WinHandleBase()
		{
			Close();
		}

		WinHandleBase<T, TBase, DestroyHandleFunc> &operator=(T handle)
		{
			if (_handle != handle)
			{
				Close();
				_handle = handle;
			}

			return *this;
		}

		void Close()
		{
			if (_handle)
			{
				DestroyHandleFunc(_handle);
				_handle = nullptr;
			}
		}

		void Attach(T handle)
		{
			Close();
			_handle = handle;
		}

		T Detach()
		{
			T handle = _handle;
			_handle = nullptr;
			return handle;
		}

		operator T() const
		{
			return _handle;
		}

		T *operator&()
		{
			assert(!_handle);
			return &_handle;
		}

	private:
		WinHandleBase(const MyType &rhs);
		MyType &operator=(const MyType &rhs);

		T _handle;
	};

	UTIL_API HANDLE DuplicateHandle(HANDLE handle);

	UTIL_API void MyDestroyHandle(HANDLE handle);
	UTIL_API void MyDestroyGdiHandle(HGDIOBJ handle);
	UTIL_API void MyDestroyDcHandle(HDC handle);
#if !METRO_APP
	UTIL_API void MyDestroyCursorHandle(HCURSOR handle);
	UTIL_API void MyDestroyFontHandle(HANDLE handle);
	UTIL_API void MyDestroyAccelHandle(HACCEL handle);
	UTIL_API void MyDestroyMenuHandle(HMENU handle);
	UTIL_API void MyDestroyFileChangeHandle(HANDLE handle);
#endif

	typedef WinHandleBase<HANDLE, HANDLE, MyDestroyHandle> WinHandle;
#if !METRO_APP
	typedef WinHandleBase<HGDIOBJ, HGDIOBJ, MyDestroyGdiHandle> GdiHandle;
	typedef WinHandleBase<HPEN, HGDIOBJ, MyDestroyGdiHandle> PenHandle;
	typedef WinHandleBase<HBRUSH, HGDIOBJ, MyDestroyGdiHandle> BrushHandle;
	typedef WinHandleBase<HFONT, HGDIOBJ, MyDestroyGdiHandle> FontHandle;
	typedef WinHandleBase<HBITMAP, HGDIOBJ, MyDestroyGdiHandle> BitmapHandle;
	typedef WinHandleBase<HRGN, HGDIOBJ, MyDestroyGdiHandle> RegionHandle;
	typedef WinHandleBase<HPALETTE, HGDIOBJ, MyDestroyGdiHandle> PaletteHandle;
	typedef WinHandleBase<HDC, HDC, MyDestroyDcHandle> CreateDcHandle;
	typedef WinHandleBase<HCURSOR, HCURSOR, MyDestroyCursorHandle> CursorHandle;
	typedef WinHandleBase<HANDLE, HANDLE, MyDestroyFontHandle> FontResourceHandle;
	typedef WinHandleBase<HACCEL, HACCEL, MyDestroyAccelHandle> AccelHandle;
	typedef WinHandleBase<HMENU, HMENU, MyDestroyMenuHandle> MenuHandle;
	typedef WinHandleBase<HANDLE, HANDLE, MyDestroyFileChangeHandle> FileChangeHandle;
#endif

	template<typename T>
	class SelectGdiObject
	{
	public:
		SelectGdiObject(HDC hdc, T handle)
			: _hdc(hdc)
		{
			_oldHandle = ::SelectObject(_hdc, static_cast<HGDIOBJ>(handle));
		}

		~SelectGdiObject()
		{
			::SelectObject(_hdc, _oldHandle);
		}

	private:
		HDC _hdc;
		HGDIOBJ _oldHandle;
	};
}
