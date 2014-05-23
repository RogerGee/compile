:: make script for project 'compile'
@echo off

if '%1'=='clean' del obj\*.obj && del compile.exe && goto end

if not exist obj\ mkdir obj\

cl /c /Foobj\compile.obj compile.c /DBUILD_COMPILE_WINDOWS
cl /c /Foobj\compiler.obj compiler.c /DBUILD_COMPILE_WINDOWS
cl /c /Foobj\settings.obj settings.c /DBUILD_COMPILE_WINDOWS
cl /c /Foobj\stringbuf.obj stringbuf.c /DBUILD_COMPILE_WINDOWS

cl /Fecompile.exe obj\*.obj Shell32.lib

:end