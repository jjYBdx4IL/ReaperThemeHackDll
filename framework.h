#pragma once

//#ifndef WINVER                  // Allow use of features specific to Windows XP or later.
//#define WINVER 0x001           // Change this to the appropriate value to target other versions of Windows.
//#endif
//
//#ifndef _WIN32_WINNT            // Allow use of features specific to Windows XP or later.
//#define _WIN32_WINNT 0x0601     // Change this to the appropriate value to target other versions of Windows.
//#endif
//
//#ifndef _WIN32_WINDOWS          // Allow use of features specific to Windows 98 or later.
//#define _WIN32_WINDOWS 0x0410   // Change this to the appropriate value to target Windows Me or later.
//#endif
//
//#ifndef _WIN32_IE               // Allow use of features specific to IE 6.0 or later.
//#define _WIN32_IE 0x0600        // Change this to the appropriate value to target other versions of IE.
//#endif

//#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
// Windows header files
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include <atlstr.h>

// COM header files
#include <ole2.h>
#include <ocidl.h>

// see for explanation:
// https://learn.microsoft.com/en-us/windows/win32/controls/subclassing-overview?redirectedfrom=MSDN
#define USE_SUBCLASS_SOLUTION
#ifndef USE_SUBCLASS_SOLUTION
    // IF NOT SET: -> hook callback solution
    // problem of the hook solution: we get called before the system's default WM_PAINT and cannot block it
    // (ie that solution is not invasive enough)
    #define USE_HOOK_SOLUTION
#endif

#ifdef USE_SUBCLASS_SOLUTION
    #include <commctrl.h>
    #pragma comment(lib, "comctl32.lib")
#endif

// generic C++ stuff
#include <iostream>
#include <filesystem>
#include <fstream>
#include <utility>

// C RunTime header files
#include <assert.h>
#define ASSERT assert
