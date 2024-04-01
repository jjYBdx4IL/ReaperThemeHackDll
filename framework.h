#pragma once

// Including SDKDDKVer.h defines the highest available Windows platform.
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
// Windows header files
#include <cstdio>
#include <windows.h>
#include <tlhelp32.h>
#include <atlstr.h>

// COM header files
#include <ole2.h>
#include <ocidl.h>

// generic C++ stuff
#include <iostream>
#include <filesystem>
#include <format>
#include <fstream>
#include <utility>
#include <vector>

// C RunTime header files
#include <assert.h>

// for subclassing
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#include <Uxtheme.h>
#pragma comment(lib, "uxtheme.lib")
#include <vsstyle.h>
#include <Vssym32.h>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#define ASSERT assert
