#include "pch.h"
#include "resource.h"

static BOOL CALLBACK replace_string_enum(HWND hwnd, LPARAM lp)
{
    wchar_t class_name[128];
    if (::GetClassName(hwnd, class_name, _countof(class_name)) && !std::wcscmp(class_name, L"Static"))
    {
        wchar_t buffer[256];
        if (::GetWindowText(hwnd, buffer, _countof(buffer)) > 0)
        {
            std::wstring original_text(buffer);
            std::wstring text = original_text;

            size_t pos = text.find(L"<name>");
            if (pos != std::wstring::npos)
            {
                text.replace(pos, 6, ff::string::to_wstring(ff::app_version().product_name));
            }

            pos = text.find(L"<version>");
            if (pos != std::wstring::npos)
            {
                text.replace(pos, 9, ff::string::to_wstring(ff::app_version().product_version));
            }

            pos = text.find(L"<copyright>");
            if (pos != std::wstring::npos)
            {
                text.replace(pos, 11, ff::string::to_wstring(ff::app_version().copyright));
            }

            pos = text.find(L"<company>");
            if (pos != std::wstring::npos)
            {
                text.replace(pos, 9, ff::string::to_wstring(ff::app_version().company_name));
            }

            if (text != original_text)
            {
                ::SetWindowText(hwnd, text.c_str());
            }
        }
    }

    return TRUE;
}

static INT_PTR CALLBACK about_dialog_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg)
    {
        case WM_INITDIALOG:
            ff::win32::center_window(hwnd);
            ::EnumChildWindows(hwnd, ::replace_string_enum, 0);
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wp) == IDOK || LOWORD(wp) == IDCANCEL)
            {
                ::EndDialog(hwnd, LOWORD(wp));
                return TRUE;
            }
            break;

        case WM_CLOSE:
            ::EndDialog(hwnd, IDCANCEL);
            return TRUE;

        case WM_NOTIFY:
            {
                const NMHDR* header = reinterpret_cast<const NMHDR*>(lp);
                std::wstring url;

                if (header->code == NM_CLICK || header->code == NM_RETURN)
                {
                    switch (header->idFrom)
                    {
                        case IDC_LINK_SITE:
                        case IDC_LINK_SUPPORT:
                        case IDC_LINK_SOURCE:
                            {
                                const NMLINK* link = reinterpret_cast<const NMLINK*>(lp);
                                url = link->item.szUrl;
                            }
                            break;

                        case IDC_LINK_LIBS:
                            {
                                const NMLINK* link = reinterpret_cast<const NMLINK*>(lp);
                                if (!std::wcscmp(link->item.szUrl, L"zlib"))
                                {
                                    url = L"https://zlib.net/";
                                }
                                else if (!std::wcscmp(link->item.szUrl, L"libpng"))
                                {
                                    url = L"http://www.libpng.org/pub/png/libpng.html";
                                }
                                else if (!std::wcscmp(link->item.szUrl, L"dxtex"))
                                {
                                    url = L"https://github.com/microsoft/DirectXTex";
                                }
                                else if (!std::wcscmp(link->item.szUrl, L"ff"))
                                {
                                    url = L"https://github.com/spadapet/ff_game_library";
                                }
                            }
                            break;
                    }
                }

                if (url.size())
                {
                    ::ShellExecute(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                    return TRUE;
                }
            }
            break;
    }

    return 0;
}

void show_about_dialog()
{
    ::DialogBox(ff::get_hinstance(), MAKEINTRESOURCE(IDD_ABOUT_DIALOG), ff::app_window(), ::about_dialog_proc);
}
