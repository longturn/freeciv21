@echo off
if not "%1" == "auto" set LANG=%1
start "%~n0" /D . "%~dp0\freeciv21-server.exe" %2 %3 %4 %5 %6 %7 %8 %9
