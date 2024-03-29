# Description

The Reaper DAW does not support skinning the menu bar.
This is a plugin dll that hacks itself into the menu bar
drawing process by subclassing the main window REAPERwnd.

# Limitations

* Not yet workung in fullscreen mode.

# Todos?

* Menu dropdowns?
* Other widgets?
* ...

# NOTES
* Reaper main window style normal: 14CF0844, fullscreen: 140B0844
  * https://learn.microsoft.com/en-us/windows/win32/winmsg/window-class-styles
  * https://learn.microsoft.com/en-us/windows/win32/winmsg/extended-window-styles
  * 0x00040000 WS_EX_APPWINDOW
  * 0x00400000L WS_EX_LAYOUTRTL
  * 0x00800000 ???
  * posted q: https://stackoverflow.com/questions/78244786/theme-hack-fails-to-work-in-fullscreen-mode

--
devel/cpp/win/ReaperThemeHackDll@8079
