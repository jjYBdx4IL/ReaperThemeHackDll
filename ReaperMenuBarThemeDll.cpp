#include "reaper_plugin_functions.h"
#include "UAHMenuBar.h"

HHOOK hook_ = nullptr;
HWND main_hwnd{ nullptr };

// dark mode for menu popups
// only works after toggling fullscreen button twice
void allowDarkMode(HWND hWnd) {
    //HRESULT hr = CoInitialize(0);
    //ASSERT(SUCCEEDED(hr));

    // https://gist.github.com/rounk-ctrl/b04e5622e30e0d62956870d5c22b7017
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
    HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    ASSERT(hUxtheme);
    if (hUxtheme) {
        fnSetPreferredAppMode SetPreferredAppMode;
        SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
        ASSERT(SetPreferredAppMode);
        fnAllowDarkModeForWindow AllowDarkModeForWindow;
        AllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133));
        ASSERT(AllowDarkModeForWindow);
        //SetPreferredAppMode(PreferredAppMode::AllowDark);
        SetPreferredAppMode(PreferredAppMode::ForceDark);

        UX::SetWindowTheme(hWnd, L"Explorer", NULL);
        AllowDarkModeForWindow(hWnd, true);
        SendMessageW(hWnd, WM_THEMECHANGED, 0, 0);

        FreeLibrary(hUxtheme);
    }


    //CoUninitialize();
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

    LRESULT lr = 0;
    if (UAHWndProc(hWnd, uMsg, wParam, lParam, &lr)) {
        //OutputDebugString(L"UAHWndProc TRUE\n");
        return TRUE;
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
                ASSERT(err == 1400); // invalid window handle
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
