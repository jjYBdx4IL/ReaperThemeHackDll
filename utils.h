#pragma once
LPCWSTR get_message_name(DWORD msg);
void GetAllWindowsFromProcessID(DWORD dwProcessID, std::vector<HWND>& vhWnds);
void GetAllChildWindowsFromParent(HWND parent, std::vector<HWND>& vhWnds);
bool isClass(HWND hwnd, const TCHAR* classname);
void dbgMsg(HWND hWnd, UINT_PTR subclass, UINT message, WPARAM wParam, LPARAM lParam);
void SendInputF11();

typedef struct {
    COLORREF menubar_textcolor;
    COLORREF menubar_textcolor_disabled;
    COLORREF menubar_bgcolor;
    COLORREF menubaritem_bgcolor;
    COLORREF menubaritem_bgcolor_hot;
    COLORREF menubaritem_bgcolor_selected;

    HBRUSH menubar_bgbrush;
    HBRUSH menubaritem_bgbrush;
    HBRUSH menubaritem_bgbrush_hot;
    HBRUSH menubaritem_bgbrush_selected;
} _cfg_type;
const _cfg_type* load_config();

#define ASSERT_LAST_ERR(x) \
{ \
BOOL rc = x; \
if (!rc) { \
    std::wstringstream ss; \
    ss << L"GetLastError() = " << GetLastError() << std::endl; \
    OutputDebugString(ss.str().c_str()); \
} \
assert(rc); \
}
