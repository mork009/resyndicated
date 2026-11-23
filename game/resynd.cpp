/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2010, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include <cstdio>
#include <cstdlib>

#include <windows.h>
#include <io.h>

#include "rs-utils/common.h"
#include "app.h"
#include "rs-utils/io/file.h"
#include "rs-utils/log/log.h"
#include "version.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

//#define CHEAP_LEAK_DETECTION

#ifdef CHEAP_LEAK_DETECTION

#define BLKSZ 65536
int blocks[BLKSZ];

void cleanupLeakDetection() {
    int unfreed = 0;
    int mfreed = 0;

    for (int i = 0; i < BLKSZ; i++) {
        if (blocks[i] > 0) {
            unfreed++;
            printf("%x ", i);
        }
        else if (blocks[i] < 0) {
            mfreed++;
            printf("m%x ", i);
        }
    }

    printf("\n%i unfreed blocks\n", unfreed);
    printf("%i multiple freed blocks\n", mfreed);
}

void initLeakDetection() {
    int ginits = 0;

    for (int i = 0; i < BLKSZ; i++) {
        if (blocks[i] > 0)
            ginits++;
        else
            assert(blocks[i] == 0);
    }

    printf("%i blocks allocated before main\n", ginits);
    atexit(&cleanupLeakDetection);
}

void *operator new(size_t n) {
    void *p = malloc(n);
    int x = ((int) p) % BLKSZ;

    if (x == 0x1ffff) {         // plug a reported hash in here and set a breakpoint
        printf("boo\n");
    }

    blocks[x]++;
    return p;
}

void operator delete(void *p) {
    if (p == 0) {
        printf("freeing NULL\n");
        return;
    }

    int x = ((int) p) % BLKSZ;

    if (x == 0x1ffff) {         // plug a reported hash in here and set a breakpoint
        printf("mboo\n");
    }

    blocks[x]--;
    free(p);
}
#endif

static bool g_attachedConsole = false;

static void sendReturnToParentConsole() {
    if (g_attachedConsole) {
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        if (hIn && hIn != INVALID_HANDLE_VALUE) {
            INPUT_RECORD ir[2] = {};
            ir[0].EventType = KEY_EVENT;
            ir[0].Event.KeyEvent.bKeyDown = TRUE;
            ir[0].Event.KeyEvent.wRepeatCount = 1;
            ir[0].Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
            ir[0].Event.KeyEvent.wVirtualScanCode = (WORD)MapVirtualKey(VK_RETURN, MAPVK_VK_TO_VSC);
            ir[0].Event.KeyEvent.uChar.UnicodeChar = L'\r';

            ir[1] = ir[0];
            ir[1].Event.KeyEvent.bKeyDown = FALSE;

            DWORD written = 0;
            WriteConsoleInputW(hIn, ir, 2, &written);
        }
        FreeConsole();
        g_attachedConsole = false;
    }
}

int main(int argc, char *argv[]) {
    bool helpRequested = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "-h" || arg == "--help" || arg == "-?") {
            helpRequested = true;
            break;
        }
    }

    if (helpRequested) {
        if (AttachConsole(ATTACH_PARENT_PROCESS)) {
            g_attachedConsole = true;
            FILE *fp;
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
        } else {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            if (hOut != NULL && hOut != INVALID_HANDLE_VALUE) {
                int fd = _open_osfhandle((intptr_t)hOut, 0);
                if (fd != -1) {
                    FILE *fp = _fdopen(fd, "w");
                    if (fp) {
                        *stdout = *fp;
                        setvbuf(stdout, NULL, _IONBF, 0);
                    }
                }
            }
        }
        std::cout.clear();
        std::cerr.clear();
        std::ios::sync_with_stdio();
    }

    printf("ReSyndicated - %i.%i\n", RS_VERSION_MAJOR, RS_VERSION_MINOR);

#ifdef CHEAP_LEAK_DETECTION
    initLeakDetection();
#endif

    srand(GetTickCount());
    
    App app;

    int res = app.run(argc, argv);

    app.destroy();

    sendReturnToParentConsole();

    return res;
}
