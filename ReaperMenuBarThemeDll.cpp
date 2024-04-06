#include "UAHMenuBar.h"
#include "utils.h"
#define REAPERAPI_MINIMAL
#define REAPERAPI_WANT_GetMainHwnd
// #include "reaper_plugin_functions.h"

int WM_NCACTIVATE_cnt = 0;
int WM_THEMECHANGED_cnt = 0;
int WM_UAHDRAWMENU_cnt = 0;
int WM_UAHDRAWMENUITEM_cnt = 0;
int WM_UAHMEASUREMENUITEM_cnt = 0;

static HTHEME g_menuTheme = nullptr;

void UAHDrawMenuNCBottomLine(HWND hWnd)
{
    MENUBARINFO mbi = { sizeof(mbi) };
    if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
    {
        char tmp[200];
        sprintf(tmp, "GetMenuBarInfo error %d\n", GetLastError());
        OutputDebugStringA(tmp);
        return;
    }
    RECT rcClient = { 0 };
    GetClientRect(hWnd, &rcClient);
    MapWindowPoints(hWnd, nullptr, (POINT*)&rcClient, 2);

    RECT rcWindow = { 0 };
    GetWindowRect(hWnd, &rcWindow);
    OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);
    // the rcBar is offset by the window rect
    RECT rcAnnoyingLine = rcClient;
    rcAnnoyingLine.bottom = rcAnnoyingLine.top;
    rcAnnoyingLine.top--;

    HDC hdc = GetWindowDC(hWnd);
    FillRect(hdc, &rcAnnoyingLine, load_config()->menubar_bgbrush);
    ReleaseDC(hWnd, hdc);
}

static TCHAR __debug_buff[512];
#undef DBG_HWND
#define DBG_HWND(x,y) \
        { \
            swprintf_s(__debug_buff, 512, L"%p", x); \
            std::wstringstream ss; \
            ss << std::format(L"{:%T}", std::chrono::system_clock::now()) << L" DBG_HWND(0x" << __debug_buff << L") " << L#y << L" "; \
            GetWindowText(x, __debug_buff, 512); \
            ss << __debug_buff << L" / "; \
            GetClassName(x, __debug_buff, 512); \
            ss << __debug_buff << std::endl; \
            OutputDebugString(ss.str().c_str()); \
        }

HHOOK hook_ = nullptr;
HWND main_hwnd{ nullptr };
LRESULT CALLBACK CallWndSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

// https://gist.github.com/rounk-ctrl/b04e5622e30e0d62956870d5c22b7017
// https://github.com/microsoft/WindowsAppSDK/issues/41
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
        //DWORD dwFlags = (STAP_ALLOW_NONCLIENT |
        //    STAP_ALLOW_CONTROLS | STAP_ALLOW_WEBCONTENT);
        //SetThemeAppProperties(dwFlags);

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

    if (hUxtheme && hWnd) {
        //SetWindowTheme(hWnd, L"wstr", L"wstr");
        SetWindowTheme(hWnd, L"DarkMode_Explorer", NULL);
        //SetWindowTheme(hWnd, L"Explorer", NULL);
        //SetWindowTheme(hWnd, L"CFD", NULL);
        AllowDarkModeForWindow(hWnd, true);
    }
}

// https://stackoverflow.com/questions/39261826/change-the-color-of-the-title-bar-caption-of-a-win32-application
void enableImmersiveDarkMode(HWND hWnd) {
    BOOL USE_DARK_MODE = true;
    BOOL SET_IMMERSIVE_DARK_MODE_SUCCESS = SUCCEEDED(DwmSetWindowAttribute(
        hWnd, DWMWINDOWATTRIBUTE::DWMWA_USE_IMMERSIVE_DARK_MODE,
        &USE_DARK_MODE, sizeof(USE_DARK_MODE)));
    //ASSERT(SET_IMMERSIVE_DARK_MODE_SUCCESS);
}

LRESULT CALLBACK CBTProc(int nCode,
    WPARAM wParam, LPARAM lParam)
{
    switch (nCode) {
    case HCBT_ACTIVATE:
    {
        HWND hWnd = (HWND)wParam;
        DBG_HWND(hWnd, HCBT_ACTIVATE);
        break;
    }
    case HCBT_CREATEWND:
    {
        HWND hWnd = (HWND)wParam;
        DBG_HWND(hWnd, HCBT_CREATEWND);

        DWORD style = GetWindowLong(hWnd, GWL_STYLE);

        // need to be careful with subclassing (avoiding conflict with SWS plugin)
        if (isClass(hWnd, L"Ableton Live Window Class") || isClass(hWnd, L"REAPERMediaExplorerMainwnd")
            || isClass(hWnd, L"#32770") || isClass(hWnd, L"Button") || isClass(hWnd, L"tooltips_class32")
            || isClass(hWnd, L"ComboBox")
            // pick some widgets that aren't theme-able and leave the others alone
            || (isClass(hWnd, L"SysListView32") && style & WS_CHILDWINDOW)
            || (isClass(hWnd, L"SysTreeView32") && style & WS_CHILDWINDOW)
            ) {
            enableImmersiveDarkMode(hWnd);
            allowDarkMode(hWnd);
            BOOL lr = SetWindowSubclass(hWnd, CallWndSubClassProc, 0, 0);
            ASSERT(lr);
        }
        break;
    }
    case HCBT_DESTROYWND:
    {
        HWND hWnd = (HWND)wParam;
        DBG_HWND(hWnd, HCBT_DESTROYWND);

        DWORD style = GetWindowLong(hWnd, GWL_STYLE);

        if (isClass(hWnd, L"Ableton Live Window Class") || isClass(hWnd, L"REAPERMediaExplorerMainwnd") 
            || isClass(hWnd, L"#32770") || isClass(hWnd, L"Button") || isClass(hWnd, L"tooltips_class32")
            || isClass(hWnd, L"ComboBox")
            // pick some widgets that aren't theme-able and leave the others alone
            || (isClass(hWnd, L"SysListView32") && style & WS_CHILDWINDOW)
            || (isClass(hWnd, L"SysTreeView32") && style & WS_CHILDWINDOW)
            ) {
            RemoveWindowSubclass(hWnd, CallWndSubClassProc, 0);
        }
        break;
    }
    case HCBT_CLICKSKIPPED:
    case HCBT_KEYSKIPPED:
    case HCBT_MINMAX:
    case HCBT_MOVESIZE:
    case HCBT_QS:
    case HCBT_SETFOCUS:
    case HCBT_SYSCOMMAND:
    {
        break;
    }
    default:
        break;
    }

    return 0;
}

// https://stackoverflow.com/questions/16313333/drawing-rounded-and-colored-owner-draw-buttons
void paint_ODT_BUTTON(const DRAWITEMSTRUCT& dis) {
    HWND hwnd = dis.hwndItem;

    RECT rc;
    GetClientRect(hwnd, &rc);

    PAINTSTRUCT ps;
    //auto hdc = BeginPaint(hwnd, &ps);
    auto hdc = dis.hDC;
    auto bkcolor = load_config()->menubar_bgcolor;
    auto brush = CreateSolidBrush(bkcolor);
    auto pen = CreatePen(PS_SOLID, 1, load_config()->menubar_textcolor);
    auto oldbrush = SelectObject(hdc, brush);
    auto oldpen = SelectObject(hdc, pen);
    SelectObject(hdc, (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0));
    SetBkColor(hdc, bkcolor);
    SetTextColor(hdc, load_config()->menubar_textcolor);

    HBRUSH tmp = CreateSolidBrush(RGB(127, 192, 127)); // border
    FillRect(hdc, &rc, tmp);

    Rectangle(hdc, 0, 0, rc.right, rc.bottom);

    if (GetFocus() == hwnd)
    {
        RECT temp = rc;
        InflateRect(&temp, -2, -2);
        DrawFocusRect(hdc, &temp);
    }

    TCHAR buf[128];
    GetWindowText(hwnd, buf, 128);
    //std::wstringstream ss;
    //ss << "paint_ODT_BUTTON button text: " << buf << std::endl;
    //OutputDebugString(ss.str().c_str());
    DrawText(hdc, buf, -1, &rc, DT_EDITCONTROL | DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldpen);
    SelectObject(hdc, oldbrush);
    DeleteObject(brush);
    DeleteObject(pen);
}

LRESULT CALLBACK CallWndSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam,
    LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    dbgMsg(hWnd, uIdSubclass, uMsg, wParam, lParam);

    ASSERT(uIdSubclass == 0);

    switch (uMsg) {
    case WM_ACTIVATEAPP:
    {
        break;
    }
    case WM_CREATE:
    {
        break;
    }
    case WM_CTLCOLORBTN:
    {
        break;
    }
    case WM_CTLCOLORDLG: 
    {
        HDC hdc = (HDC)wParam;
        HWND dialog = (HWND)lParam;
        if (!isClass(dialog, L"REAPERRoutePanel"))
            return (INT_PTR)load_config()->menubar_bgcolor;
        break;
    }
    case WM_CTLCOLOREDIT:
    {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, load_config()->menubar_textcolor);
        SetBkColor(hdcStatic, load_config()->menubar_bgcolor);
        return (INT_PTR)load_config()->menubar_bgbrush;
    }
    case WM_CTLCOLORLISTBOX:
    {
        if (isClass(hWnd, L"ComboBox")) {
            COMBOBOXINFO info;
            info.cbSize = sizeof(info);
            SendMessage(hWnd, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info);

            if ((HWND)lParam == info.hwndList)
            {
                HDC dc = (HDC)wParam;
                SetBkMode(dc, OPAQUE);
                SetTextColor(dc, load_config()->menubar_textcolor);
                SetBkColor(dc, load_config()->menubar_bgcolor);
                return (LRESULT)load_config()->menubar_bgbrush;
            }
        }
        break;
    }
    case WM_CTLCOLORSCROLLBAR:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, load_config()->menubar_textcolor);
        SetBkColor(hdc, load_config()->menubar_bgcolor);
        return reinterpret_cast<LRESULT>(load_config()->menubar_bgbrush);
        break;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, load_config()->menubar_textcolor);
        SetBkColor(hdc, load_config()->menubar_bgcolor);
        return reinterpret_cast<LRESULT>(load_config()->menubar_bgbrush);
        break;
    }
    case WM_DRAWITEM:
    {
        const DRAWITEMSTRUCT& dis = *(DRAWITEMSTRUCT*)lParam;

        //TCHAR buf[128];
        //GetWindowText(dis.hwndItem, buf, 128);
        //DWORD style = GetWindowLongPtr(dis.hwndItem, GWL_STYLE);
        //std::wstringstream ss;
        //ss << "WM_DRAWITEM button text: " << buf << ", style=" << style << std::endl;
        //OutputDebugString(ss.str().c_str());

        if (dis.CtlType == ODT_BUTTON) {
            if (dis.itemAction) { // & ODA_DRAWENTIRE) {
                paint_ODT_BUTTON(dis);
                if (dis.itemState & ODS_FOCUS) {
                    DrawFocusRect(dis.hDC, &dis.rcItem);
                }
            }
            return TRUE;
        }
        break;
    }
    case WM_ERASEBKGND:
    {
        DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
        switch (LOWORD(style)) {
        case BS_GROUPBOX: // 0x7
            break;
        default:
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect(reinterpret_cast<HDC>(wParam), &rc, load_config()->menubar_bgbrush);
            return TRUE;
            break;
        }
        }
    }
    case WM_IME_SETCONTEXT:
    {
        break;
    }
    case WM_MOVE:
    {
        break;
    }
    case WM_NCACTIVATE:
    {
        if (isClass(hWnd, L"Ableton Live Window Class") || isClass(hWnd, L"REAPERMediaExplorerMainwnd") || isClass(hWnd, L"#32770")) {
            WM_NCACTIVATE_cnt++;
            LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
            UAHDrawMenuNCBottomLine(hWnd);
            return lr;
        }
        break;
    }
    case WM_NCCREATE:
    {
        allowDarkMode(hWnd);
        enableImmersiveDarkMode(hWnd);
        if (isClass(hWnd, L"tooltips_class32")) {
            SetWindowTheme(hWnd, L"wstr", L"wstr");
        }
        else if (isClass(hWnd, L"ComboBox")) {
            SetWindowTheme(hWnd, L"wstr", L"wstr");
        }
        else if (isClass(hWnd, L"Button")) {
            DWORD style = GetWindowLongPtr(hWnd, GWL_STYLE);
            switch(LOWORD(style)) {
            case BS_AUTOCHECKBOX: // 0x3
            case BS_AUTORADIOBUTTON: // 0x9
            case BS_CHECKBOX: // 0x2
            case BS_GROUPBOX: // 0x7
                SetWindowTheme(hWnd, L"wstr", L"wstr");
                break;
            case BS_LEFT: // 0x100
            case BS_PUSHBUTTON: // 0x0
            case BS_CENTER:
            case BS_RIGHT:
            case BS_DEFPUSHBUTTON:
                style = style | BS_OWNERDRAW;
                //style = WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON;
                SetWindowLongPtr(hWnd, GWL_STYLE, style);
                break;
            }
        }
        break;
    }
    case WM_NCPAINT:
    {
        if (isClass(hWnd, L"Ableton Live Window Class") || isClass(hWnd, L"REAPERMediaExplorerMainwnd") || isClass(hWnd, L"#32770")) {
            LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
            UAHDrawMenuNCBottomLine(hWnd);
            return lr;
        }
        break;
    }
    case WM_NOTIFY:
    {
        break;
    }
    case WM_PAINT:
    {
        if (isClass(hWnd, L"tooltips_class32")) {
            SendMessage(hWnd, TTM_SETTIPBKCOLOR, load_config()->menubar_bgcolor, 0);
            SendMessage(hWnd, TTM_SETTIPTEXTCOLOR, load_config()->menubar_textcolor, 0);
        }
        else if (isClass(hWnd, L"SysTreeView32")) { // left pane of config dialog
            TreeView_SetBkColor(hWnd, load_config()->menubar_bgcolor);
            TreeView_SetTextColor(hWnd, load_config()->menubar_textcolor);
        }
        else if (isClass(hWnd, L"SysListView32")) { // left pane of FX dialog
            ListView_SetBkColor(hWnd, load_config()->menubar_bgcolor);
            ListView_SetTextBkColor(hWnd, load_config()->menubar_bgcolor);
            ListView_SetTextColor(hWnd, load_config()->menubar_textcolor);
        }
        break;
    }
    case WM_SETFONT:
    {
        break;
    }
    case WM_SHOWWINDOW:
    {
        break;
    }
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
    {
        if (isClass(hWnd, L"Ableton Live Window Class")) {
            // prevent propagation to prevent menu bar from going back to the standard one...?!? FIXME
            return true;
        }
        break;
    }
    case WM_THEMECHANGED:
    {
        if (isClass(hWnd, L"Ableton Live Window Class") || isClass(hWnd, L"REAPERMediaExplorerMainwnd") || isClass(hWnd, L"#32770")) {
            WM_THEMECHANGED_cnt++;
            if (g_menuTheme) {
                CloseThemeData(g_menuTheme);
                g_menuTheme = nullptr;
            }
        }
        break;
    }
    // https://stackoverflow.com/questions/77985210/how-to-set-menu-bar-color-in-win32
    case WM_UAHDRAWMENU:
    {
        if (isClass(hWnd, L"Ableton Live Window Class") || isClass(hWnd, L"REAPERMediaExplorerMainwnd") || isClass(hWnd, L"#32770")) {
            WM_UAHDRAWMENU_cnt++;
            UAHMENU* pUDM = (UAHMENU*)lParam;
            RECT rc = { 0 };
            {
                MENUBARINFO mbi = { sizeof(mbi) };
                GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

                RECT rcWindow;
                GetWindowRect(hWnd, &rcWindow);
                // the rcBar is offset by the window rect
                rc = mbi.rcBar;
                OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
            }
            FillRect(pUDM->hdc, &rc, load_config()->menubar_bgbrush);
            return true;
        }
        break;
    }
    case WM_UAHDRAWMENUITEM:
    {
        if (isClass(hWnd, L"Ableton Live Window Class") || isClass(hWnd, L"REAPERMediaExplorerMainwnd") || isClass(hWnd, L"#32770")) {
            WM_UAHDRAWMENUITEM_cnt++;
            UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;

            const HBRUSH* pbrBackground = &load_config()->menubaritem_bgbrush;
            // get the menu item string
            wchar_t menuString[256] = { 0 };
            MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
            {
                mii.dwTypeData = menuString;
                mii.cch = (sizeof(menuString) / 2) - 1;

                GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
            }
            // get the item state for drawing
            DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
            int iTextStateID = 0;
            int iBackgroundStateID = 0;
            {
                if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT)) {
                    // normal display
                    iTextStateID = MPI_NORMAL;
                    iBackgroundStateID = MPI_NORMAL;
                }
                if (pUDMI->dis.itemState & ODS_HOTLIGHT) {
                    // hot tracking
                    iTextStateID = MPI_HOT;
                    iBackgroundStateID = MPI_HOT;
                    pbrBackground = &load_config()->menubaritem_bgbrush_hot;
                }
                if (pUDMI->dis.itemState & ODS_SELECTED) {
                    // clicked -- MENU_POPUPITEM has no state for this, though MENU_BARITEM does
                    iTextStateID = MPI_HOT;
                    iBackgroundStateID = MPI_HOT;
                    pbrBackground = &load_config()->menubaritem_bgbrush_selected;
                }
                if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED)) {
                    // disabled / grey text
                    iTextStateID = MPI_DISABLED;
                    iBackgroundStateID = MPI_DISABLED;
                }
                if (pUDMI->dis.itemState & ODS_NOACCEL) {
                    dwFlags |= DT_HIDEPREFIX;
                }
            }
            if (!g_menuTheme) {
                g_menuTheme = OpenThemeData(hWnd, L"Menu");
            }

            DTTOPTS opts = { sizeof(opts), DTT_TEXTCOLOR, iTextStateID != MPI_DISABLED ? load_config()->menubar_textcolor : load_config()->menubar_textcolor_disabled };
            FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, *pbrBackground);
            DrawThemeTextEx(g_menuTheme, pUDMI->um.hdc, MENU_BARITEM, MBI_NORMAL, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &opts);
            return true;
        }
        break;
    }
    case WM_UAHMEASUREMENUITEM:
    {
        if (!isClass(hWnd, L"Ableton Live Window Class") && !isClass(hWnd, L"REAPERMediaExplorerMainwnd") && !isClass(hWnd, L"#32770")) {
            WM_UAHMEASUREMENUITEM_cnt++;
            UAHMEASUREMENUITEM* pMmi = (UAHMEASUREMENUITEM*)lParam;

            // allow the default window procedure to handle the message
            // since we don't really care about changing the width
            //*lr = DefWindowProc(hWnd, message, wParam, lParam);
            LRESULT lr = DefSubclassProc(hWnd, uMsg, wParam, lParam);
            // but we can modify it here to make it 1/3rd wider for example
            //pMmi->mis.itemWidth = (pMmi->mis.itemWidth * 4) / 3;
            return lr;
        }
        break;
    }
    case WM_ENTERIDLE:
    {
        break;
    }
    default:
        break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}


bool APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpRes)
{
    OutputDebugString(L"DllMain");
    static int cnt = 0;
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
    {
        //while (!IsDebuggerPresent()) {
        //    Sleep(100);
        //}
        OutputDebugString(L"DLL_PROCESS_ATTACH\n");
        cnt++;
        ASSERT(cnt == 1);

        allowDarkMode(NULL);

        // catch windows that have been created before we set up the CBTProc hook
        std::vector<HWND> vec;
        std::wstringstream ss;
        GetAllWindowsFromProcessID(GetCurrentProcessId(), vec);
        ss << L"GetAllWindowsFromProcessID " << vec.size() << std::endl;
        OutputDebugString(ss.str().c_str());
        for (const HWND& h : vec) {
            dbgMsg(h, 0, 0, 0, 0);
            
            BOOL lr = SetWindowSubclass(h, CallWndSubClassProc, 0, 0);
            ASSERT(lr);

            enableImmersiveDarkMode(h);
            allowDarkMode(h);
        }

        ASSERT(!hook_);
        hook_ = SetWindowsHookEx(WH_CBT, CBTProc, NULL, GetCurrentThreadId());
        ASSERT(hook_);
        OutputDebugString(L"CBT Hook installed.\n");

        break;
    }
    case DLL_THREAD_ATTACH:
        OutputDebugString(L"DLL_THREAD_ATTACH\n");
        break;
    case DLL_THREAD_DETACH:
        OutputDebugString(L"DLL_THREAD_DETACH\n");
        break;
    case DLL_PROCESS_DETACH:
        OutputDebugString(L"DLL_PROCESS_DETACH\n");

        if (hook_) {
            UnhookWindowsHookEx(hook_);
            hook_ = NULL;
            OutputDebugString(L"CBT Hook uninstalled.\n");
        }

        std::wstringstream str;
        str << L"WM_NCACTIVATE_cnt = " << WM_NCACTIVATE_cnt << std::endl;
        str << L"WM_THEMECHANGED_cnt = " << WM_THEMECHANGED_cnt << std::endl;
        str << L"WM_UAHDRAWMENU_cnt = " << WM_UAHDRAWMENU_cnt << std::endl;
        str << L"WM_UAHDRAWMENUITEM_cnt = " << WM_UAHDRAWMENUITEM_cnt << std::endl;
        str << L"WM_UAHMEASUREMENUITEM_cnt = " << WM_UAHMEASUREMENUITEM_cnt << std::endl;
        OutputDebugString(str.str().c_str());

        break;
    }
    return true;
}


// REAPER_PLUGIN_HINSTANCE g_hInst{ nullptr };

// extern "C" {

//     REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t* rec)
//     {
//         OutputDebugString(L"REAPER_PLUGIN_ENTRYPOINT\n");
//         ASSERT(hInstance);
//         g_hInst = hInstance;
//         if (rec)
//         {
//             if (rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc) {
//                 OutputDebugString(L"REAPER_PLUGIN_ENTRYPOINT: ERROR\n");
//                 return 0;
//             }

//             *((void**)&GetMainHwnd) = rec->GetFunc("GetMainHwnd");
//             ASSERT(GetMainHwnd);

//             main_hwnd = GetMainHwnd();
//             ASSERT(main_hwnd);

//             OutputDebugString(L"REAPER_PLUGIN_ENTRYPOINT: SUCCESS\n");
//             return 1;
//         }
//         else
//         {
//             OutputDebugString(L"REAPER_PLUGIN_ENTRYPOINT: ERROR\n");
//             return 0;
//         }
//     }

// }
