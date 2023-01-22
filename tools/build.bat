@echo off

setlocal
setlocal EnableDelayedExpansion

set Configuration=Release
if not X%1==X set Configuration=%1

cd %~dp0..\build\VStudio

if exist build\ for /R build\ %%d in (%Configuration%) do (
    if exist "%%d" rmdir /s/q "%%d"
)

set tag=""
for /f "usebackq tokens=*" %%i in (`git describe`) do (
    set Version=%%i
    REM remove leading "v" from tag
    set Version=!Version:v=!
)

echo Version=%Version% Configuration=%Configuration%
echo:

call "%~dp0vcvarsall.bat" x64
devenv bang.sln /build "%Configuration%|x64"
if errorlevel 1 goto fail
devenv bang.sln /build "%Configuration%|x86"
if errorlevel 1 goto fail

set ZipArchive=bang-%Version%.zip
set ZipFiles=bang64.exe,bang64.dll,bang32.exe,bang32.dll

pushd build\%Configuration%
powershell -NoProfile -ExecutionPolicy Bypass -Command "Compress-Archive -DestinationPath %ZipArchive% -Path %ZipFiles%"
if errorlevel 1 goto fail
popd

exit /b 0

:fail
exit /b 1
