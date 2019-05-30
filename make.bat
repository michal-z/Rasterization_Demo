@echo off
setlocal
setlocal enableextensions
setlocal enabledelayedexpansion

set NAME=Demo
set HLSLC=dxc.exe /Ges /O3 /WX /nologo
set SHADER_BEGIN=0
set SHADER_END=3
set SRC=Demo.cpp Library.cpp

for /L %%G in (%SHADER_BEGIN%,1,%SHADER_END%) do (
    if exist Data\Shaders\%%G.vs.cso del Data\Shaders\%%G.vs.cso
    if exist Data\Shaders\%%G.ps.cso del Data\Shaders\%%G.ps.cso
    %HLSLC% /D VS_%%G /E VS%%G_Main /Fo Data\Shaders\%%G.vs.cso /T vs_6_0 %NAME%.hlsl & if !ERRORLEVEL! neq 0 (goto :end)
    %HLSLC% /D PS_%%G /E PS%%G_Main /Fo Data\Shaders\%%G.ps.cso /T ps_6_0 %NAME%.hlsl & if !ERRORLEVEL! neq 0 (goto :end)
)

set RELEASE=/Zi /O2 /DNDEBUG /MT
set DEBUG=/Zi /Od /D_DEBUG /MTd
if not defined CONFIG set CONFIG=%DEBUG%
::if not defined CONFIG set CONFIG=%RELEASE%
set CFLAGS=%CONFIG% /W4 /EHa- /GR- /Gw /Gy /nologo /IExternal /wd4238 /wd4324 /wd4530

if exist %NAME%.exe del %NAME%.exe

if not exist External.pch (cl %CFLAGS% /c /YcExternal.h External.cpp /FoExternal.obj.pch)
cl %CFLAGS% /YuExternal.h %SRC% /link /incremental:no /opt:ref External.obj.pch kernel32.lib user32.lib gdi32.lib /out:%NAME%.exe

if exist *.obj del *.obj
if "%1" == "run" if exist %NAME%.exe %NAME%.exe
:end
