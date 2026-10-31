@echo off
setlocal
cd /d "%~dp0"
if not exist build\dsr mkdir build\dsr

set SOURCES=src\dsr\dllmain.cpp src\dsr\proxy.cpp src\common\log.cpp src\common\diag.cpp src\dsr\settings.cpp src\dsr\bonfire.cpp src\dsr\patches.cpp src\dsr\trace.cpp src\dsr\watch.cpp src\dsr\inputdump.cpp src\dsr\offline.cpp src\dsr\skipintro.cpp
set CLFLAGS=/nologo /std:c++17 /O2 /MT /EHsc /W3 /LD /Isrc\common /Isrc\dsr
set LINKFLAGS=/link /OPT:REF ws2_32.lib

set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsall.bat"
if not exist %VCVARS% set VCVARS="C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

rem Already in a Visual Studio developer prompt (or a CI job that set one up): build directly.
where cl >nul 2>nul
if errorlevel 1 (
    cmd /c "call %VCVARS% x64 && cl %CLFLAGS% %SOURCES% /Fobuild\dsr\ /Febuild\dsr\dinput8.dll %LINKFLAGS%"
) else (
    cl %CLFLAGS% %SOURCES% /Fobuild\dsr\ /Febuild\dsr\dinput8.dll %LINKFLAGS%
)
if errorlevel 1 exit /b 1

echo Built build\dsr\dinput8.dll
