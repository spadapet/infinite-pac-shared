#include "pch.h"

static HWND splash_screen_hwnd{};
static HANDLE splash_screen_thread_handle{};
static DWORD splash_screen_thread_id{};
static const BITMAPINFO* splash_screen_dib{};

static void splash_screen_paint(HWND hwnd)
{
    PAINTSTRUCT ps{};
    HDC hdc = ::BeginPaint(hwnd, &ps);
    assert_ret(hdc);
    ff::scope_exit cleanup([hwnd, &ps] { ::EndPaint(hwnd, &ps); });

    RECT rect{};
    assert_ret(::splash_screen_dib && ::GetClientRect(hwnd, &rect));

    const BITMAPINFO* dib = ::splash_screen_dib;
    const BITMAPINFOHEADER& header = dib->bmiHeader;
    const BYTE* pixels = reinterpret_cast<const BYTE*>(dib) +
        sizeof(BITMAPINFOHEADER) +
        (header.biBitCount <= 8 ? (static_cast<size_t>(1) << header.biBitCount) * sizeof(RGBQUAD) : 0);

    ::StretchDIBits(
        hdc,
        0, 0, rect.right, rect.bottom,
        0, 0, header.biWidth, std::abs(header.biHeight),
        pixels, dib,
        DIB_RGB_COLORS,
        SRCCOPY);

    ::EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK splash_screen_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_ACTIVATE:
            if (wp == WA_INACTIVE)
            {
                ::PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            break;

        case WM_NCRBUTTONUP:
            ::PostMessage(hwnd, WM_CLOSE, 0, 0);
            return 0;

        case WM_NCRBUTTONDOWN:
            return 0;

        case WM_NCHITTEST:
            return HTCAPTION;

        case WM_NCDESTROY:
            ::PostQuitMessage(0);
            break;

        case WM_PAINT:
            ::splash_screen_paint(hwnd);
            break;
    }

    return ::DefWindowProc(hwnd, msg, wp, lp);
}

static UINT choose_splash_resource(UINT dpi)
{
    if (dpi >= 96 * 2)
    {
        return 200;
    }
    else if (dpi >= 96 * 3 / 2)
    {
        return 150;
    }
    else if (dpi >= 96 * 5 / 4)
    {
        return 125;
    }

    return 100;
}

static DWORD WINAPI splash_screen_thread(void* cookie)
{
    HINSTANCE instance = static_cast<HINSTANCE>(cookie);
    RECT work_area{};
    assert_ret_val(::SystemParametersInfo(SPI_GETWORKAREA, 0, &work_area, 0), 1);
    UINT dpi = ::GetDpiForSystem();
    UINT bmp_resource_id = ::choose_splash_resource(dpi);
    double dpi_scale = bmp_resource_id / 100.0;
    HRSRC bmp_resource = ::FindResource(instance, MAKEINTRESOURCE(bmp_resource_id), RT_BITMAP);
    HGLOBAL bmp_resource_mem = bmp_resource ? ::LoadResource(instance, bmp_resource) : nullptr;
    ::splash_screen_dib = reinterpret_cast<const BITMAPINFO*>(bmp_resource_mem ? ::LockResource(bmp_resource_mem) : nullptr);

    constexpr std::wstring_view class_name = L"ff::splash_screen";
    const WNDCLASS window_class
    {
        CS_DROPSHADOW,
        ::splash_screen_proc,
        0, 0, // extra bytes
        instance,
        nullptr, // icon
        ::LoadCursor(nullptr, IDC_ARROW),
        nullptr, // background
        nullptr, // menu
        class_name.data()
    };

    const int window_width = ::splash_screen_dib->bmiHeader.biWidth;
    const int window_height = std::abs(::splash_screen_dib->bmiHeader.biHeight);
    const int window_x = work_area.left + (work_area.right - work_area.left - window_width) / 2;
    const int window_y = work_area.top + (work_area.bottom - work_area.top - window_height) / 2;

    if (::RegisterClass(&window_class))
    {
        ::splash_screen_hwnd = ::CreateWindowEx(
            WS_EX_TOOLWINDOW,
            class_name.data(),
            nullptr,
            WS_POPUP | WS_VISIBLE,
            window_x, window_y, window_width, window_height,
            nullptr, // parent
            nullptr, // menu
            instance,
            nullptr); // param

        int progress_margin = static_cast<int>(4 * dpi_scale);
        int progress_height = static_cast<int>(6 * dpi_scale);

        HWND progress_hwnd = ::CreateWindowEx(
            0, PROGRESS_CLASS, nullptr, // ex style, class, name
            WS_CHILD | WS_VISIBLE | PBS_MARQUEE,
            progress_margin,
            window_height - progress_height - progress_margin,
            window_width - progress_margin * 2,
            progress_height,
            ::splash_screen_hwnd,
            nullptr, instance, nullptr); // menu, instance, param

        ::PostMessage(progress_hwnd, PBM_SETMARQUEE, 1, 10);
    }

    MSG msg;
    while (::GetMessage(&msg, nullptr, 0, 0))
    {
        if (!msg.hwnd && msg.message == WM_USER && ::splash_screen_hwnd)
        {
            ::PostMessage(::splash_screen_hwnd, WM_CLOSE, 0, 0);
        }
        else
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }

    return 0;
}

bool show_splash_screen(HINSTANCE instance)
{
    ::splash_screen_thread_handle = ::CreateThread(nullptr, 0, ::splash_screen_thread, static_cast<void*>(instance), 0, &::splash_screen_thread_id);
    return ::splash_screen_thread_handle != nullptr;
}

void close_splash_screen()
{
    check_ret(::splash_screen_thread_handle);

    ::PostThreadMessage(::splash_screen_thread_id, WM_USER, 0, 0);
    ::CloseHandle(::splash_screen_thread_handle);

    ::splash_screen_thread_handle = nullptr;
    ::splash_screen_thread_id = 0;
}
