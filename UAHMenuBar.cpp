// https://stackoverflow.com/questions/77985210/how-to-set-menu-bar-color-in-win32

#include "UAHMenuBar.h"
#include <vsstyle.h>
#include <Vssym32.h>

#include "inipp.h"
#include "utils.h"

static UX::HTHEME g_menuTheme = nullptr;

static BOOL config_loaded = FALSE;

// ugly colors for illustration purposes
static HBRUSH menubar_bgcolor = NULL;
static HBRUSH menubaritem_bgcolor = NULL;
static HBRUSH menubaritem_bgcolor_hot = NULL;
static HBRUSH menubaritem_bgcolor_selected = NULL;
static COLORREF menubar_textcolor;
static COLORREF menubar_textcolor_disabled;

inline void load_config() {
    if (config_loaded) return;
    config_loaded = TRUE;

    HMODULE hm = NULL;
    BOOL lr = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)load_config, &hm);
    ASSERT(lr);
    ASSERT(hm);
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(hm, path, MAX_PATH);
    CStringW inifn(path);
    inifn.Append(L".ini");
    OutputDebugString(L"INI filename: " + inifn + L"\n");

    if (!std::filesystem::exists(inifn.GetString())) {
        std::ofstream myfile;
        myfile.open(inifn.GetString());
        myfile << "menubar_textcolor = 200,200,200" << std::endl;
        myfile << "menubar_textcolor_disabled = 160,160,160" << std::endl;
        myfile << "menubar_bgcolor = 20,20,20" << std::endl;
        myfile << "menubaritem_bgcolor = 20,20,20" << std::endl;
        myfile << "menubaritem_bgcolor_hot = 20,20,20" << std::endl;
        myfile << "menubaritem_bgcolor_selected = 20,20,20" << std::endl;
        myfile.close();
    }

    inipp::Ini<char> ini;
    std::ifstream is(inifn.GetString());
    ini.parse(is);
    //is.close();
    std::ostringstream oss;
    ini.generate(oss);
    OutputDebugStringA(oss.str().c_str());
    OutputDebugString(L"\n");
    OutputDebugStringA(ini.sections[""]["bkgd"].c_str());
    OutputDebugString(L"\n");
    
#define _PARSE_COLOR(x) \
    { \
        int r, g, b; \
        char comma; \
        std::stringstream ss(ini.sections[""][#x]); \
        ss >> r >> comma >> g >> comma >> b; \
        x = RGB(r, g, b); \
    }

    _PARSE_COLOR(menubar_textcolor);
    _PARSE_COLOR(menubar_textcolor_disabled);

#define _PARSE_COLOR_BRUSH(x) \
    { \
        int r, g, b; \
        char comma; \
        std::stringstream ss(ini.sections[""][#x]); \
        ss >> r >> comma >> g >> comma >> b; \
        x = CreateSolidBrush(RGB(r, g, b)); \
    }

    _PARSE_COLOR_BRUSH(menubar_bgcolor);
    _PARSE_COLOR_BRUSH(menubaritem_bgcolor);
    _PARSE_COLOR_BRUSH(menubaritem_bgcolor_hot);
    _PARSE_COLOR_BRUSH(menubaritem_bgcolor_selected);
}

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
    FillRect(hdc, &rcAnnoyingLine, menubar_bgcolor);
    ReleaseDC(hWnd, hdc);
}

int WM_NCACTIVATE_cnt = 0;
int WM_THEMECHANGED_cnt = 0;
int WM_UAHDRAWMENU_cnt = 0;
int WM_UAHDRAWMENUITEM_cnt = 0;
int WM_UAHMEASUREMENUITEM_cnt = 0;

bool UAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr)
{
#ifndef NDEBUG
    static int dupe_counter = 0;
    static int last_message = -1;
    if (message == last_message) {
        dupe_counter++;
    }
    else {
        if (dupe_counter) {
            std::wstringstream str;
            str << L"Suppressed " << dupe_counter << " sequential messages of the same type" << std::endl;
            OutputDebugString(str.str().c_str());
            dupe_counter = 0;
        }
        last_message = message;
    }
    if (dupe_counter == 0) {
        std::wstringstream str;
        WCHAR tmp[5];
        swprintf_s(tmp, 5, L"%04x", message);
        std::wstring s1 = std::format(L"{:%T}", std::chrono::system_clock::now());
        str << s1 << L" UAHWndProc(" << hWnd << ", " << get_message_name(message) << " 0x" << tmp << " " << message << ", " << wParam << ", " << lParam << ")" << std::endl;
        switch (message) { // https://wiki.winehq.org/List_Of_Windows_Messages
        case WM_SETCURSOR: // 0x0020 32
        case WM_NCMOUSEMOVE: // 0x00a0		160
        case WM_NCHITTEST: // 0x0084		132
        case WM_MOUSEMOVE: // 0x0200 512
        case WM_NCMOUSELEAVE: // 0x02a2		674
            break;
        default:
            OutputDebugString(str.str().c_str());
        }
    }
#endif

    load_config();
    switch (message)
    {
    case WM_UAHDRAWMENU:
    {
        OutputDebugString(L"WM_UAHDRAWMENU\n");
        WM_UAHDRAWMENU_cnt++;
        UAHMENU* pUDM = (UAHMENU*)lParam;
        RECT rc = { 0 };
        // get the menubar rect
        {
            MENUBARINFO mbi = { sizeof(mbi) };
            GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

            RECT rcWindow;
            GetWindowRect(hWnd, &rcWindow);
            // the rcBar is offset by the window rect
            rc = mbi.rcBar;
            OffsetRect(&rc, -rcWindow.left, -rcWindow.top);
        }
        FillRect(pUDM->hdc, &rc, menubar_bgcolor);
        return true;
    }
    case WM_UAHDRAWMENUITEM:
    {
        OutputDebugString(L"WM_UAHDRAWMENUITEM\n");
        WM_UAHDRAWMENUITEM_cnt++;
        UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;

        HBRUSH* pbrBackground = &menubaritem_bgcolor;
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
                pbrBackground = &menubaritem_bgcolor_hot;
            }
            if (pUDMI->dis.itemState & ODS_SELECTED) {
                // clicked -- MENU_POPUPITEM has no state for this, though MENU_BARITEM does
                iTextStateID = MPI_HOT;
                iBackgroundStateID = MPI_HOT;
                pbrBackground = &menubaritem_bgcolor_selected;
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
            g_menuTheme = UX::OpenThemeData(hWnd, L"Menu");
        }

        UX::DTTOPTS opts = { sizeof(opts), DTT_TEXTCOLOR, iTextStateID != MPI_DISABLED ? menubar_textcolor : menubar_textcolor_disabled };
        FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, *pbrBackground);
        DrawThemeTextEx(g_menuTheme, pUDMI->um.hdc, MENU_BARITEM, MBI_NORMAL, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &opts);
        return true;
    }
    case WM_UAHMEASUREMENUITEM:
    {
        OutputDebugString(L"WM_UAHMEASUREMENUITEM\n");
        WM_UAHMEASUREMENUITEM_cnt++;
        UAHMEASUREMENUITEM* pMmi = (UAHMEASUREMENUITEM*)lParam;

        // allow the default window procedure to handle the message
        // since we don't really care about changing the width
        *lr = DefWindowProc(hWnd, message, wParam, lParam);
        // but we can modify it here to make it 1/3rd wider for example
        //pMmi->mis.itemWidth = (pMmi->mis.itemWidth * 4) / 3;
        return true;
    }
    case WM_THEMECHANGED:
    {
        OutputDebugString(L"WM_THEMECHANGED\n");
        WM_THEMECHANGED_cnt++;
        if (g_menuTheme) {
            UX::CloseThemeData(g_menuTheme);
            g_menuTheme = nullptr;
        }
        // continue processing in main wndproc
        return false;
    }
    case WM_NCPAINT:
        OutputDebugString(L"WM_NCPAINT\n");
        *lr = DefWindowProc(hWnd, message, wParam, lParam);
        UAHDrawMenuNCBottomLine(hWnd);
        return true;
        break;
    case WM_NCACTIVATE:
        OutputDebugString(L"WM_NCACTIVATE\n");
        WM_NCACTIVATE_cnt++;
        *lr = DefWindowProc(hWnd, message, wParam, lParam);
        UAHDrawMenuNCBottomLine(hWnd);
        return true;
        break;
    // the following two messages come through when fullscreen is toggled.
    // apparently it helps to block them. ??
    case WM_STYLECHANGING:
    case WM_STYLECHANGED:
        return true;
    default:
        //OutputDebugString(L"default\n");
        return false;
    }
}
