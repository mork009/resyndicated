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

set "BUILD_TYPE=%~1"
if "%BUILD_TYPE%"=="" set "BUILD_TYPE=debug"

set "FILTER=%~2"

if /i "%BUILD_TYPE%"=="release" (
    set "PRESET=ninja-msvc-x64-release"
    set "BUILD_DIR=build\ninja-msvc-x64-release"
) else (
    set "PRESET=ninja-msvc-x64-debug"
    set "BUILD_DIR=build\ninja-msvc-x64-debug"
)

echo ========================================================
echo Running ReSyndicated Kernel Tests [%BUILD_TYPE%]
echo ========================================================

call build.bat %BUILD_TYPE%
if errorlevel 1 (
    echo Build failed. Aborting tests.
    exit /b %errorlevel%
)

if not "%FILTER%"=="" (
    echo Running specific test filter: %FILTER%
    ".\%BUILD_DIR%\kernel\test\test-kernel.exe" "%FILTER%"
) else (
    ctest --test-dir %BUILD_DIR% --output-on-failure --timeout 120
)