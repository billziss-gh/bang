@echo off

setlocal

call "%~dp0vcvarsall.bat" %1

cd %~dp0..\ext\detours\src
nmake
if errorlevel 1 goto fail

cd ..\..
xcopy /siy detours\include include
xcopy /siy detours\lib.%1 lib\%1

exit /b 0

:fail
exit /b 1
