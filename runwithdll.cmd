@echo off

cd /d %~dp0

start /B .\withdll.exe /d:ReaperThemeHackDll.dll .\reaper.exe
