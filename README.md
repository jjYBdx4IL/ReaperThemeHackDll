# Description

The Reaper DAW does not support skinning the menu bar under Windows.
This is a PRELOAD dll that hacks itself into the menu bar
drawing process by subclassing the main window REAPERwnd.
It also tries to do a lot of other stuff (see source code).

The general setup is currently to use CBT callbacks to create subclasses
for windows and widgets.

The dll can run as both, a Reaper user plugin, or a preload dll injected
before Reaper startup using Detours withdll.exe.

See CMakeLists.txt for more information.

# Status

* Headers and scroll bars in Sys(List|Tree)View32 widgets cannot be changed.
* Some minor UI defects. Mileage may vary depending on your workflow.
* INI file is very minimal.

# Installation

* Required: Visual Studio C/C++ + latest Win11 SDK
* `cmake -B build && cd build && cmake --build . --config Release`
* Find the built dll and withdll.exe, and copy them with runwithdll.cmd to your Reaper installation directory.
Adjust the script if needed.
* The Reaper installation folder must be user-writable because the DLL will create a default .ini file alongside.
INI color defaults are optimized for the rLive Reaper theme.

# Limitations

* Needs up-to-date Windows 11 installation. Windows dark theme support is very recent.

# Notes

* The project at https://github.com/jjYBdx4IL/win32-custom-menubar-aero-theme may be used to experiment
with win32 style hacks.
* How to add cmd script to Win 11 task bar: rename to exe, right click, pin, rename back, shift-right-click on task bar entry, adjust script name and optionally the icon (you can 'steal' it from reaper.exe).
* How to make the console window go away: shift-right-click the task bar entry and prepend the script location with: "powershell Start -WindowStyle hidden " (excluding the quotation marks)

# External references
* https://github.com/hunterboy2023/win32-custom-menubar-aero-theme
* https://github.com/microsoft/Detours
* https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/Controls/about-custom-draw.md
* Reaper forum thread: https://forum.cockos.com/showthread.php?t=289792
* custom draw of Sys(Tree|List)View32??
  * https://deweymao.github.io/c/c++/2017/06/04/custom_draw_implementation_of_tree_view_control.html
  * https://learn.microsoft.com/en-us/windows/win32/controls/nm-customdraw
```
16:38:59:271	 dbgMsg(0000000000911132,0) 'SysTreeView32'() WM_NCCREATE 0x0081(129), 0, 1357328 (parent=0000000000360D8E) (style=0x40010027)
16:38:59:271	 dbgMsg(0000000000911132,0) 'SysTreeView32'(Tree1) WM_NCCALCSIZE 0x0083(131), 0, 1357440 (parent=0000000000360D8E) (style=0x40010027)
16:38:59:271	 dbgMsg(0000000000911132,0) 'SysTreeView32'(Tree1) WM_CREATE 0x0001(1), 0, 1357296 (parent=0000000000360D8E) (style=0x40010027)
16:38:59:271	 dbgMsg(0000000000360D8E,0) '#32770'(Add new FX) WM_NOTIFYFORMAT 0x0055(85), 9507122, 3 (parent=0000000001D61116) (style=0x84cd0044)
16:38:59:271	 dbgMsg(0000000000360D8E,0) '#32770'(Add new FX) WM_QUERYUISTATE 0x0129(297), 0, 0 (parent=0000000001D61116) (style=0x84cd0044)
16:38:59:271	 dbgMsg(0000000000360D8E,0) '#32770'(Add new FX) WM_NOTIFY 0x004e(78), 1176, 1353552 (parent=0000000001D61116) (style=0x84cd0044)
16:38:59:271	    WM_NOTIFY: idFrom=1176, hwndFrom=0000000000911132, code=4294967284 NM_CUSTOMDRAW, style=1073807399
```

# Screenshots

![](https://i.imgur.com/XNwMWHD.png)


--
devel/cpp/win/ReaperThemeHackDll@8094
