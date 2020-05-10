if not defined DevEnvDir (
	call "%VS120COMNTOOLS%vsvars32.bat"
)

@if not defined INCLUDE goto :FAIL

@setlocal

@if "%1"=="debug-5.1" goto :DEBUG_5_1

rem These should not have quotes
@set LUA_INCLUDE=%CORONA_ENTERPRISE%/Corona/shared/include/lua
@set LUA_LIB=%CORONA_ENTERPRISE%/Corona/win/lib/lua.lib
@set LUA_EXE=%CORONA_ENTERPRISE%/Corona/win/bin/lua.exe
rem This the name of the dll that can be handed to LoadLibrary. This should not have a path.

@goto :DEBUG

:DEBUG_5_1
@set LUA_INCLUDE=Z:\c\lua-5.1.4\src
@set LUA_LIB=Z:\c\lua-5.1.4\lua5.1.lib
@set LUA_EXE=Z:\c\lua-5.1.4\lua.exe
@set LUA_DLL=lua5.1.dll

:DEBUG
@set DO_CL=cl.exe /nologo /c /MDd /FC /Zi /Od /W3 /WX /D_CRT_SECURE_NO_DEPRECATE /DLUA_FFI_BUILD_AS_DLL
rem /I"msvc"
@set DO_LINK=link /nologo /debug
@set DO_MT=mt /nologo

@if "%1"=="debug" goto :COMPILE
@if "%1"=="debug-5.1" goto :COMPILE
@if "%1"=="test" goto :COMPILE
@if "%1"=="clean" goto :CLEAN
@if "%1"=="release" goto :RELEASE
@if "%1"=="test-release" goto :RELEASE

:RELEASE
@set DO_CL=cl.exe /nologo /c /MD /Ox /W3 /Zi /WX /D_CRT_SECURE_NO_DEPRECATE /DLUA_FFI_BUILD_AS_DLL
rem /I"msvc"
@set DO_LINK=link.exe /nologo
@set DO_MT=mt.exe /nologo
@goto :COMPILE

:COMPILE
cd ../shared

"%LUA_EXE%" dynasm\dynasm.lua -LNE -D X32WIN -o call_x86.h call_x86.dasc
"%LUA_EXE%" dynasm\dynasm.lua -LNE -D X64 -o call_x64.h call_x86.dasc
"%LUA_EXE%" dynasm\dynasm.lua -LNE -D X64 -D X64WIN -o call_x64win.h call_x86.dasc
"%LUA_EXE%" dynasm\dynasm.lua -LNE -o call_arm.h call_arm.dasc
%DO_CL% /I"." /I"%LUA_INCLUDE%" call.c ctype.c ffi.c parser.c
%DO_LINK% /DLL /OUT:ffi.dll "%LUA_LIB%" *.obj
if exist ffi.dll.manifest^
    %DO_MT% -manifest ffi.dll.manifest -outputresource:"ffi.dll;2"

robocopy . ../win32 ffi.dll /MOV
	
@goto :CLEAN_OBJ

:CLEAN
del *.dll
:CLEAN_OBJ
del *.obj *.manifest ffi.exp ffi.lib vc1*.*
@goto :END

:FAIL
@echo You must open a "Visual Studio .NET Command Prompt" to run this script
:END

