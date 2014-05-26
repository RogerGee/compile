:: make script for project 'compile'
@echo off

if '%1'=='clean' goto clean

if not exist obj\ mkdir obj\

cl /c /Foobj\compile.obj compile.c /DBUILD_COMPILE_WINDOWS
cl /c /Foobj\compiler.obj compiler.c /DBUILD_COMPILE_WINDOWS
cl /c /Foobj\settings.obj settings.c /DBUILD_COMPILE_WINDOWS
cl /c /Foobj\stringbuf.obj stringbuf.c /DBUILD_COMPILE_WINDOWS

cl /Fecompile.exe obj\*.obj Shell32.lib
goto end

:clean
if exist obj\ rd /s obj
if exist Debug\ rd /s Debug
if exist Release\ rd /s Release
if exist x64\ rd /s x64
if exist compile.exe del compile.exe

goto end

:end