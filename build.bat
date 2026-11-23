@echo off
::
::  ReSyndicated - a modernized game engine and full map/mission editor for the
::  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
::  project, FreeSynd.
::
::   Copyright (C) 2026  mork009 <mork009resynd@gmail.com>
::
::   This program is free software: you can redistribute it and/or
::  modify it under the terms of the GNU General Public License as
::  published by the Free Software Foundation, either version 3 of the
::  License, or (at your option) any later version.
::
::   This program is distributed in the hope that it will be useful,
::  but WITHOUT ANY WARRANTY; without even the implied warranty of
::  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
::  See the GNU General Public License for more details.
::
::   You should have received a copy of the GNU General Public License
::  along with this program. If not, see <https://www.gnu.org/licenses/>.
::

set "BUILD_TYPE=debug"
set "DO_CONFIGURE=0"
set "DO_PACKAGE=0"

if /i "%~1"=="package" (
    set "BUILD_TYPE=release"
    set "DO_PACKAGE=1"
)
if /i "%~2"=="package" set "DO_PACKAGE=1"
if /i "%~1"=="release" set "BUILD_TYPE=release"
if /i "%~1"=="debug" set "BUILD_TYPE=debug"
if /i "%~1"=="configure" set "DO_CONFIGURE=1"
if /i "%~2"=="configure" set "DO_CONFIGURE=1"
if /i "%~2"=="--configure" set "DO_CONFIGURE=1"
if /i "%~1"=="configure" if /i "%~2"=="release" set "BUILD_TYPE=release"
if /i "%~1"=="configure" if /i "%~2"=="debug" set "BUILD_TYPE=debug"
if /i "%~1"=="configure" if /i "%~2"=="package" (
    set "BUILD_TYPE=release"
    set "DO_PACKAGE=1"
)

if /i "%BUILD_TYPE%"=="release" (
    set "PRESET=ninja-msvc-x64-release"
    set "BUILD_DIR=build\ninja-msvc-x64-release"
) else (
    set "PRESET=ninja-msvc-x64-debug"
    set "BUILD_DIR=build\ninja-msvc-x64-debug"
)

echo ========================================================
echo Building ReSyndicated [%BUILD_TYPE%] with preset [%PRESET%]
echo ========================================================

:: Discover Conan in user environment if not already on PATH
where conan >nul 2>&1
if errorlevel 1 (
    for /d %%a in ("%LOCALAPPDATA%\Packages\PythonSoftwareFoundation.Python*") do (
        for /d %%b in ("%%a\LocalCache\local-packages\Python*") do (
            if exist "%%b\Scripts\conan.exe" set "PATH=%%b\Scripts;%PATH%"
        )
    )
)
where conan >nul 2>&1
if errorlevel 1 (
    for /d %%a in ("%LOCALAPPDATA%\Programs\Python\Python*") do (
        if exist "%%a\Scripts\conan.exe" set "PATH=%%a\Scripts;%PATH%"
    )
)
where conan >nul 2>&1
if errorlevel 1 (
    for /d %%a in ("%APPDATA%\Python\Python*") do (
        if exist "%%a\Scripts\conan.exe" set "PATH=%%a\Scripts;%PATH%"
    )
)

:: Locate and initialize Visual Studio MSVC environment if cl.exe is not already in PATH
where cl >nul 2>&1
if errorlevel 1 call :init_msvc

:: Configure CMake preset if requested or missing
if "%DO_CONFIGURE%"=="1" (
    cmake --preset %PRESET%
    if errorlevel 1 exit /b %errorlevel%
    if /i "%~1"=="configure" exit /b 0
) else if not exist "%BUILD_DIR%\build.ninja" (
    cmake --preset %PRESET%
    if errorlevel 1 exit /b %errorlevel%
)

cmake --build %BUILD_DIR%
if errorlevel 1 exit /b %errorlevel%

echo Copying SDL dependencies...
copy /y "extern\sdl3\lib\x64\SDL3.dll" "%BUILD_DIR%\game\" >nul
copy /y "extern\SDL3_image\lib\x64\SDL3_image.dll" "%BUILD_DIR%\game\" >nul

if not exist "%BUILD_DIR%\engine\test" mkdir "%BUILD_DIR%\engine\test"
copy /y "extern\sdl3\lib\x64\SDL3.dll" "%BUILD_DIR%\engine\test\" >nul
copy /y "extern\SDL3_image\lib\x64\SDL3_image.dll" "%BUILD_DIR%\engine\test\" >nul

if not exist "%BUILD_DIR%\kernel\test" mkdir "%BUILD_DIR%\kernel\test"
copy /y "extern\sdl3\lib\x64\SDL3.dll" "%BUILD_DIR%\kernel\test\" >nul
copy /y "extern\SDL3_image\lib\x64\SDL3_image.dll" "%BUILD_DIR%\kernel\test\" >nul

if not exist "%BUILD_DIR%\editor" mkdir "%BUILD_DIR%\editor"
copy /y "extern\sdl3\lib\x64\SDL3.dll" "%BUILD_DIR%\editor\" >nul
copy /y "extern\SDL3_image\lib\x64\SDL3_image.dll" "%BUILD_DIR%\editor\" >nul

if "%DO_PACKAGE%"=="1" (
    echo ========================================================
    echo Packaging ReSyndicated Release Distribution
    echo ========================================================
    cmake --build "%BUILD_DIR%" --target package
    if errorlevel 1 exit /b %errorlevel%
)

exit /b 0

:: Subroutine: Locate and run vcvars64.bat using vswhere
:init_msvc
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" set "VSWHERE=%ProgramFiles%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" (
    echo [ERROR] vswhere.exe was not found. Please ensure Visual Studio is installed.
    exit /b 1
)

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -property installationPath`) do (
    if exist "%%i\VC\Auxiliary\Build\vcvars64.bat" (
        call "%%i\VC\Auxiliary\Build\vcvars64.bat"
        exit /b 0
    )
)
echo [ERROR] vcvars64.bat was not found in Visual Studio installation.
exit /b 1