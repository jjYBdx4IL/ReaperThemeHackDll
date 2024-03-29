# Description

The Reaper DAW does not support skinning the menu bar.
This is a plugin dll that hacks itself into the menu bar
drawing process by subclassing the main window REAPERwnd.

# Installation

* Copy the plugin dll into UserPlugins/ folder below the reaper installation
* That folder must be user-writable because the DLL will create a .ini file alongside (otherwise
copy a valid ini file there - the name is the dll's name plus '.ini').
* You can configure that .ini after the first start with a text editor.
* For reaper to load the dll it seems to have to follow a naming convention. Rename it to
'reaper_themehack-x64.dll'.

# Todos?

* Menu dropdowns?
* Other widgets?
* ...

# Notes

* The project at https://github.com/jjYBdx4IL/win32-custom-menubar-aero-theme may be used to experiment
with win32 style hacks.


--
devel/cpp/win/ReaperThemeHackDll@8080
