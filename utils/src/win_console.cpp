/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2026  mork009 <mork009resynd@gmail.com>
 *
 *   This program is free software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *  See the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "rs-utils/misc/win_console.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <io.h>
#include <cstdio>
#include <iostream>
#endif

namespace rs_utl {

WinParentConsole::~WinParentConsole() {
    detach();
}

void WinParentConsole::attach() {
#ifdef _WIN32
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        attached_ = true;
        FILE *fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
    } else {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != nullptr && hOut != INVALID_HANDLE_VALUE) {
            int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hOut), 0);
            if (fd != -1) {
                FILE *fp = _fdopen(fd, "w");
                if (fp) {
                    *stdout = *fp;
                    setvbuf(stdout, nullptr, _IONBF, 0);
                }
            }
        }
    }
    std::cout.clear();
    std::cerr.clear();
    std::ios::sync_with_stdio();
#endif
}

void WinParentConsole::detach() {
#ifdef _WIN32
    if (!attached_) {
        return;
    }
    std::fflush(stdout);
    std::fflush(stderr);

    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn && hIn != INVALID_HANDLE_VALUE) {
        INPUT_RECORD ir[2] = {};
        ir[0].EventType = KEY_EVENT;
        ir[0].Event.KeyEvent.bKeyDown = TRUE;
        ir[0].Event.KeyEvent.wRepeatCount = 1;
        ir[0].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
        ir[0].Event.KeyEvent.wVirtualScanCode = static_cast<WORD>(MapVirtualKeyW(VK_RETURN, MAPVK_VK_TO_VSC));
        ir[0].Event.KeyEvent.uChar.UnicodeChar = L'\r';

        ir[1] = ir[0];
        ir[1].Event.KeyEvent.bKeyDown = FALSE;

        DWORD written = 0;
        WriteConsoleInputW(hIn, ir, 2, &written);
    }
    FreeConsole();
    attached_ = false;
#endif
}

} // namespace rs_utl
