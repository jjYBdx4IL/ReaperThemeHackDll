#include "reaper_plugin_functions.h"
#include "UAHMenuBar.h"

HHOOK hook_ = nullptr;
HWND main_hwnd{ nullptr };
LRESULT CALLBACK CallWndSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// dark mode for menu popups
// only works after toggling fullscreen button twice
// https://gist.github.com/rounk-ctrl/b04e5622e30e0d62956870d5c22b7017
void allowDarkMode(HWND hWnd) {
    static HMODULE hUxtheme = NULL;

    enum class PreferredAppMode
    {
        Default,
        AllowDark,
        ForceDark,
        ForceLight,
        Max
    };
    using fnShouldAppsUseDarkMode = bool (WINAPI*)(); // ordinal 132
    using fnAllowDarkModeForWindow = bool (WINAPI*)(HWND hWnd, bool allow); // ordinal 133
    using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode); // ordinal 135, in 1903
    fnSetPreferredAppMode SetPreferredAppMode;
    fnAllowDarkModeForWindow AllowDarkModeForWindow;

    if (!hUxtheme) {
        HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        ASSERT(hUxtheme);
        SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
        ASSERT(SetPreferredAppMode);
        AllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));
        ASSERT(AllowDarkModeForWindow);
        //SetPreferredAppMode(PreferredAppMode::AllowDark);
        SetPreferredAppMode(PreferredAppMode::ForceDark);
        //FreeLibrary(hUxtheme);
    }

    if (hUxtheme) {
        UX::SetWindowTheme(hWnd, L"Explorer", NULL);
        AllowDarkModeForWindow(hWnd, true);
        SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);
    }
}

// https://stackoverflow.com/questions/39261826/change-the-color-of-the-title-bar-caption-of-a-win32-application
void enableImmersiveDarkMode(HWND hWnd) {
    BOOL USE_DARK_MODE = true;
    BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(UX::DwmSetWindowAttribute(
        hWnd, UX::DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
        &USE_DARK_MODE, sizeof(USE_DARK_MODE)));
    //ASSERT(SET_IMMERSIVE_DARK_MODE_SUCCESS);
}

#ifdef USE_HOOK_SOLUTION
LRESULT CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode < 0)
        return CallNextHookEx(hook_, nCode, wParam, lParam);

    CWPSTRUCT* cwp = (CWPSTRUCT*)(lParam);
    HWND hwnd = cwp->hwnd;
    UINT message = cwp->message;
    
    LRESULT lr = 0;
    if (UAHWndProc(hwnd, message, cwp->wParam, cwp->lParam, &lr)) {
        OutputDebugString(L"UAHWndProc TRUE\n");
        //return lr;
        //return CallNextHookEx(hook_, nCode, wParam, lParam);
    }
    else {
        OutputDebugString(L"UAHWndProc FALSE\n");
    }
    return 0;
    

    //if (hwnd == main_hwnd)
    //{
        //switch (message)
        //{
        //case WM_SETFOCUS:  OutputDebugStringA("set focus\n");  break;
        //case WM_KILLFOCUS: OutputDebugStringA("kill focus\n"); break;
        //default: break;
        //}
    //}

    return CallNextHookEx(hook_, nCode, wParam, lParam);
}
#endif

LRESULT CALLBACK CBTProc(int nCode,
    WPARAM wParam, LPARAM lParam)
{
    //OutputDebugString(L"CBTProc\n");

    TCHAR buff[512];
#define DBG_HWND(x) \
        { \
            GetWindowText(x, buff, 512); \
            OutputDebugString(buff); \
            OutputDebugString(L" - "); \
            GetClassName(x, buff, 512); \
            OutputDebugString(buff); \
            OutputDebugString(L"\n"); \
        }


    if (nCode == HCBT_CREATEWND) {
        HWND hWnd = (HWND)wParam;
        DBG_HWND(hWnd);

        //allowDarkMode(hWnd);
        //enableImmersiveDarkMode(hWnd);

        //if (StrCmp(L"#32770", buff)) {
            BOOL lr = SetWindowSubclass(hWnd, CallWndSubClassProc, 1, 0);
            ASSERT(lr);
        //}
    }
    if (nCode == HCBT_DESTROYWND) {
        HWND hWnd = (HWND)wParam;
        RemoveWindowSubclass(hWnd, CallWndSubClassProc, 1);
    }

    return 0;

    if (nCode == HCBT_ACTIVATE)
    {
        HWND hWnd = (HWND)wParam;
        
        DBG_HWND(hWnd);
        
        allowDarkMode(hWnd);
        enableImmersiveDarkMode(hWnd);

        if (StrCmp(L"#32770", buff)) {
            BOOL lr = SetWindowSubclass(hWnd, CallWndSubClassProc, 1, 0);
            //ASSERT(lr);
        }

        HWND h2 = NULL;
        int i = 0;
        h2 = GetDlgItem(hWnd, i);
        while (h2) {
            DBG_HWND(h2);
            if (StrCmp(L"#32770", buff)) {
                allowDarkMode(h2);

                HWND h3 = GetDlgItem(h2, 0);
                if (h3) {
                    DBG_HWND(h3);
                    allowDarkMode(h3);
                    HWND h4 = GetDlgItem(h3, 0);
                    if (h4) {
                        DBG_HWND(h4);
                        allowDarkMode(h4);
                    }
                }
            }
            i++;
            h2 = GetDlgItem(hWnd, i);
        }

        if (_tcscmp(buff, _T("Effects")) == 0)
        {
            //ShowWindow(hWnd, SW_HIDE);
            //g_hWndEffects = hWnd;
            //UnhookWindowsHookEx(g_hook);
        }
        if (_tcscmp(buff, _T("Display Properties")) == 0)
        {
            //ShowWindow(hWnd, SW_HIDE);
        }
    }

    return 0;
}

bool APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpRes)
{
    OutputDebugString(L"DllMain");
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        OutputDebugString(L"DLL_PROCESS_ATTACH\n");
        break;
    case DLL_THREAD_ATTACH:
        OutputDebugString(L"DLL_THREAD_ATTACH\n");
        break;
    case DLL_THREAD_DETACH:
        OutputDebugString(L"DLL_THREAD_DETACH\n");
        break;
    case DLL_PROCESS_DETACH:
        OutputDebugString(L"DLL_PROCESS_DETACH\n");
        break;
    }
    return true;
}

#ifdef USE_SUBCLASS_SOLUTION
LRESULT CALLBACK CallWndSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    //OutputDebugString(L"CallWndSubClassProc\n");

    //switch (uMsg) {
    //case WM_ACTIVATE:
    //{
    //    WCHAR buf[200];
    //    GetClassName(hWnd, buf, 200);
    //    std::wstringstream ss;
    //    ss << L"WM_ACTIVATE classname: " << buf << std::endl;
    //    OutputDebugString(ss.str().c_str());
    //    enableImmersiveDarkMode(hWnd);
    //}
    //    break;
    //default:
    //    break;
    //}

    if (uIdSubclass == 0) {
        switch (uMsg) {
        //case WM_CHILDACTIVATE:
        //    return true;
        default:
            break;
        }
    }

    if (uIdSubclass == 0) {
        LRESULT lr = 0;
        if (UAHWndProc(hWnd, uMsg, wParam, lParam, &lr)) {
            //OutputDebugString(L"UAHWndProc TRUE\n");
            return TRUE;
        }
    }

    if (uIdSubclass == 1) {
        switch (uMsg) {
        case WM_SHOWWINDOW:
            allowDarkMode(hWnd);
            enableImmersiveDarkMode(hWnd);
            break;
        //case WM_PAINT:
        //    return true;
        //case WM_ERASEBKGND:
        //    return true;
        default:
            break;
        }
    }

    //OutputDebugString(L"UAHWndProc FALSE\n");
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}
#endif


REAPER_PLUGIN_HINSTANCE g_hInst{ nullptr };
extern int WM_NCACTIVATE_cnt;
extern int WM_THEMECHANGED_cnt;
extern int WM_UAHDRAWMENU_cnt;
extern int WM_UAHDRAWMENUITEM_cnt;
extern int WM_UAHMEASUREMENUITEM_cnt;

extern "C" {

    REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec)
    {
        OutputDebugString(L"REAPER_PLUGIN_ENTRYPOINT\n");
        ASSERT(hInstance);
        g_hInst = hInstance;
        if (rec)
        {
            if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc) {
                OutputDebugString(L"REAPER_PLUGIN_ENTRYPOINT: ERROR\n");
                return 0;
            }

            //*((void**)&resolve_fn) = rec->GetFunc("resolve_fn");
            //*((void**)&relative_fn) = rec->GetFunc("relative_fn");
            //*((void**)&PCM_Source_CreateFromFile) = rec->GetFunc("PCM_Source_CreateFromFile");

            //if (!resolve_fn ||
            //    !PCM_Source_CreateFromFile ||
            //    !rec->Register ||
            //    !rec->Register("projectimport", &myRegStruct))
            //    return 0;

            // our plugin registered, return success

            *((void**)&GetMainHwnd) = rec->GetFunc("GetMainHwnd");
            ASSERT(GetMainHwnd);

            main_hwnd = GetMainHwnd();
            ASSERT(main_hwnd);

            enableImmersiveDarkMode(main_hwnd);
            allowDarkMode(main_hwnd);

#ifndef NDEBUG
            WCHAR path[MAX_PATH];
            GetModuleFileNameW(hInstance, path, MAX_PATH);
            OutputDebugString(path);
            OutputDebugString(L"\n");
#endif


#ifndef NDEBUG
            {
                WCHAR str[100];
                int n = GetClassName(main_hwnd, str, 100);
                ASSERT(n);
                OutputDebugString(str);
                OutputDebugString(L"\n");
                ASSERT(!wcscmp(str, L"REAPERwnd"));
            }
#endif

#ifdef USE_SUBCLASS_SOLUTION
            BOOL lr = SetWindowSubclass(main_hwnd, CallWndSubClassProc, 0, 0);
            ASSERT(lr);

            ASSERT(!hook_);
            hook_ = SetWindowsHookEx(WH_CBT, CBTProc, NULL, GetCurrentThreadId());
            ASSERT(hook_);
            OutputDebugString(L"CBT Hook installed.\n");
#endif

#ifdef USE_HOOK_SOLUTION
            ASSERT(!hook_);
            //if (!(hook_ = SetWindowsHookEx(WH_CALLWNDPROC, CallWndProc, hModule, 0)))
            hook_ = SetWindowsHookEx(WH_CALLWNDPROCRET, CallWndProc, NULL, GetCurrentThreadId());
            ASSERT(hook_);
            OutputDebugString(L"Hook installed.\n");
#endif

            OutputDebugString(L"REAPER_PLUGIN_ENTRYPOINT: SUCCESS\n");
            return 1;
        }
        else
        {
#ifdef USE_SUBCLASS_SOLUTION
            BOOL lr = RemoveWindowSubclass(main_hwnd, CallWndSubClassProc, 0);
            if (!lr) {
                int err = GetLastError();
                std::wstringstream ss;
                ss << "ERROR: " << err << std::endl;
                OutputDebugString(ss.str().c_str());
                //ASSERT(err == 1400); // invalid window handle
            }

            if (hook_) {
                UnhookWindowsHookEx(hook_);
                hook_ = NULL;
                OutputDebugString(L"CBT Hook uninstalled.\n");
            }
#endif

#ifdef USE_HOOK_SOLUTION
            ASSERT(hook_);
            UnhookWindowsHookEx(hook_);
            hook_ = NULL;
            OutputDebugString(L"Hook uninstalled.\n");
#endif

            OutputDebugString(L"REAPER_PLUGIN_ENTRYPOINT: ERROR\n");

            std::wstringstream str;
            str << L"WM_NCACTIVATE_cnt = " << WM_NCACTIVATE_cnt << std::endl;
            str << L"WM_THEMECHANGED_cnt = " << WM_THEMECHANGED_cnt << std::endl;
            str << L"WM_UAHDRAWMENU_cnt = " << WM_UAHDRAWMENU_cnt << std::endl;
            str << L"WM_UAHDRAWMENUITEM_cnt = " << WM_UAHDRAWMENUITEM_cnt << std::endl;
            str << L"WM_UAHMEASUREMENUITEM_cnt = " << WM_UAHMEASUREMENUITEM_cnt << std::endl;
            OutputDebugString(str.str().c_str());

            return 0;
        }
    }

}
