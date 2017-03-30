@echo off
set MSYSPATH=c:\msys64

rem set MSYSTEM=MINGW32
rem %MSYSPATH%\usr\bin\bash.exe --login '%~dp0\build.sh' %*

set MSYSTEM=MINGW64
%MSYSPATH%\usr\bin\bash.exe --login '%~dp0\build.sh' %*
