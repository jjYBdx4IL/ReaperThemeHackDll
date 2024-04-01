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
* `cmake -B build ; cd build ; cmake --build . --config Release`
* Find the built dll and withdll.exe, and copy them with runwith.cmd to your Reaper installation directory.
Adjust the script if needed.
* The Reaper installation folder must be user-writable because the DLL will create a default .ini file alongside.

# Limitations

* Needs up-to-date Windows 11 installation. Windows dark theme support is very recent.

# Notes

* The project at https://github.com/jjYBdx4IL/win32-custom-menubar-aero-theme may be used to experiment
with win32 style hacks.

# External references
* https://github.com/hunterboy2023/win32-custom-menubar-aero-theme
* https://github.com/microsoft/Detours
* https://github.com/MicrosoftDocs/win32/blob/docs/desktop-src/Controls/about-custom-draw.md
* Reaper forum thread: https://forum.cockos.com/showthread.php?t=289792


--
devel/cpp/win/ReaperThemeHackDll@8088
