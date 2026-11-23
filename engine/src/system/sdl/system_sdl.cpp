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

#include "system_sdl.h"

#ifdef HAVE_SDL_MIXER

#include "mixer/sdlmixeraudio.h"

#endif // HAVE_SDL_MIXER

#include <SDL3_image/SDL_image.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include "utf8.h"

#include "rs-engine/config.h"
#include "rs-engine/sound/audio.h"
#include "rs-utils/io/file.h"
#include "rs-utils/log/log.h"
#include "rstexture_sdl.h"

#if defined(_WIN32)
#include <windows.h>
#include <dwmapi.h>
#include <commdlg.h>
#pragma comment(lib, "dwmapi.lib")

static HWND getHwndFromWindow(SDL_Window *pWindow) {
    if (!pWindow) return nullptr;
    return static_cast<HWND>(SDL_GetPointerProperty(SDL_GetWindowProperties(pWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL));
}
#endif
#include "rs-utils/version.h"
#include "rs-engine/ui/settings_dialog.h"
#include "rs-engine/appcontext.h"
#ifdef __APPLE__
#include "../../ui/game_menu_macos.h"
#endif

namespace rs_eng {

#ifdef _WIN32
namespace {

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

constexpr COLORREF kDarkDlgBg           = RGB(28, 30, 34);    // #1C1E22
constexpr COLORREF kDarkAccentCyan      = RGB(0, 220, 245);   // Syndicate Cyan
constexpr COLORREF kDarkText            = RGB(225, 230, 236); // #E1E6EC
constexpr COLORREF kDarkTextMuted       = RGB(160, 168, 178); // #A0A8B2

static void enableDarkModeForWindow(HWND hWnd) {
    if (!hWnd) return;
    BOOL darkMode = TRUE;
    if (FAILED(DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode)))) {
        DwmSetWindowAttribute(hWnd, 19, &darkMode, sizeof(darkMode));
    }
}

static WNDPROC s_oldBtnProc = nullptr;

static LRESULT CALLBACK GameAboutButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEMOVE:
            if (!GetPropW(hWnd, L"Hovered")) {
                SetPropW(hWnd, L"Hovered", (HANDLE)1);
                TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;

        case WM_MOUSELEAVE:
            if (GetPropW(hWnd, L"Hovered")) {
                RemovePropW(hWnd, L"Hovered");
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;

        case WM_NCDESTROY:
            RemovePropW(hWnd, L"Hovered");
            break;

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_ENABLE:
            InvalidateRect(hWnd, nullptr, FALSE);
            break;

        case WM_SETTEXT: {
            DefWindowProcW(hWnd, WM_SETTEXT, wParam, lParam);
            InvalidateRect(hWnd, nullptr, FALSE);
            UpdateWindow(hWnd);
            return TRUE;
        }

        case BM_SETSTATE: {
            LRESULT res = CallWindowProcW(s_oldBtnProc, hWnd, uMsg, wParam, lParam);
            InvalidateRect(hWnd, nullptr, FALSE);
            UpdateWindow(hWnd);
            return res;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP hbm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HBITMAP oldBm = (HBITMAP)SelectObject(memDC, hbm);

            bool isEnabled = IsWindowEnabled(hWnd) != FALSE;
            LRESULT state = SendMessageW(hWnd, BM_GETSTATE, 0, 0);
            bool isPressed = (state & BST_PUSHED) != 0;
            bool isFocused = (GetFocus() == hWnd);
            bool isHovered = (GetPropW(hWnd, L"Hovered") != nullptr);

            COLORREF bgCol, borderCol, textCol;
            if (!isEnabled) {
                bgCol = RGB(30, 32, 36);
                borderCol = RGB(46, 50, 56);
                textCol = RGB(110, 118, 128);
            } else if (isPressed) {
                bgCol = RGB(22, 25, 29);
                borderCol = kDarkAccentCyan;
                textCol = RGB(255, 255, 255);
            } else if (isHovered) {
                bgCol = RGB(46, 54, 64);
                borderCol = kDarkAccentCyan;
                textCol = RGB(255, 255, 255);
            } else {
                bgCol = RGB(36, 40, 46);
                borderCol = RGB(0, 160, 200);
                textCol = kDarkText;
            }

            HBRUSH hbrBg = CreateSolidBrush(bgCol);
            FillRect(memDC, &rc, hbrBg);
            DeleteObject(hbrBg);

            HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
            HPEN oldPen = (HPEN)SelectObject(memDC, hPen);
            HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
            Rectangle(memDC, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(memDC, oldPen);
            SelectObject(memDC, oldBrush);
            DeleteObject(hPen);

            if (isFocused && isEnabled) {
                RECT rcFocus = rc;
                InflateRect(&rcFocus, -3, -3);
                HPEN hPenFocus = CreatePen(PS_DOT, 1, RGB(70, 130, 160));
                HPEN oldPenF = (HPEN)SelectObject(memDC, hPenFocus);
                HBRUSH oldBrF = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
                Rectangle(memDC, rcFocus.left, rcFocus.top, rcFocus.right, rcFocus.bottom);
                SelectObject(memDC, oldPenF);
                SelectObject(memDC, oldBrF);
                DeleteObject(hPenFocus);
            }

            HFONT hFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
            HFONT oldFont = hFont ? (HFONT)SelectObject(memDC, hFont) : nullptr;

            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, textCol);

            wchar_t text[128] = {};
            GetWindowTextW(hWnd, text, 128);

            RECT rcText = rc;
            if (isPressed) {
                OffsetRect(&rcText, 1, 1);
            }
            DrawTextW(memDC, text, -1, &rcText, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            if (oldFont) SelectObject(memDC, oldFont);

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBm);
            DeleteObject(hbm);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
            return 0;
        }
    }

    return CallWindowProcW(s_oldBtnProc, hWnd, uMsg, wParam, lParam);
}

static HFONT s_hFontHeader = nullptr;
static HFONT s_hFontBold = nullptr;
static HFONT s_hFont = nullptr;

static LRESULT CALLBACK GameAboutDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            if (!s_hFont) {
                s_hFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            }
            if (!s_hFontBold) {
                s_hFontBold = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            }
            if (!s_hFontHeader) {
                s_hFontHeader = CreateFontW(-20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            }

            RECT rc;
            GetClientRect(hWnd, &rc);
            HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                rc.right - 106, rc.bottom - 42, 90, 28,
                hWnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
            SendMessageW(hBtnOk, WM_SETFONT, reinterpret_cast<WPARAM>(s_hFont), TRUE);

            s_oldBtnProc = (WNDPROC)SetWindowLongPtrW(hBtnOk, GWLP_WNDPROC, (LONG_PTR)GameAboutButtonProc);

            enableDarkModeForWindow(hWnd);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            HBRUSH hbrBg = CreateSolidBrush(kDarkDlgBg);
            FillRect(hdc, &rc, hbrBg);
            DeleteObject(hbrBg);

            HFONT oldFont = (HFONT)SelectObject(hdc, s_hFontHeader);
            SetBkMode(hdc, TRANSPARENT);

            // App Icon
            HICON hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
            if (!hIcon) {
                std::string iconPath = rs_utl::File::getResyndDataFullPath("resources/logo.ico");
                int wLen = MultiByteToWideChar(CP_UTF8, 0, iconPath.c_str(), -1, nullptr, 0);
                if (wLen > 0) {
                    std::wstring wPath(wLen, 0);
                    MultiByteToWideChar(CP_UTF8, 0, iconPath.c_str(), -1, &wPath[0], wLen);
                    hIcon = (HICON)LoadImageW(nullptr, wPath.c_str(), IMAGE_ICON, 48, 48, LR_LOADFROMFILE);
                }
            }
            if (hIcon) {
                DrawIconEx(hdc, 24, 20, hIcon, 48, 48, 0, nullptr, DI_NORMAL);
                DestroyIcon(hIcon);
            }

            // Title
            SetTextColor(hdc, kDarkAccentCyan);
            RECT rcTitle = { 86, 16, rc.right - 24, 46 };
            DrawTextW(hdc, L"ReSyndicated", -1, &rcTitle, DT_LEFT | DT_SINGLELINE | DT_NOCLIP);

            // App Version
            SelectObject(hdc, s_hFontBold);
            SetTextColor(hdc, kDarkText);
            RECT rcVer = { 86, 48, rc.right - 24, 70 };
            DrawTextW(hdc, L"Version " RS_VERSION_WIDE, -1, &rcVer, DT_LEFT | DT_SINGLELINE | DT_NOCLIP);

            // Description
            SelectObject(hdc, s_hFont);
            SetTextColor(hdc, kDarkText);
            RECT rcDesc = { 24, 78, rc.right - 24, rc.bottom };
            int hDesc = DrawTextW(hdc, L"ReSyndicated is a modernized game engine for the classic Bullfrog strategy game, Syndicate. "
                                       L"It features real-time isometric tactical squad combat, tactical AI behavior, "
                                       L"vehicle transport dynamics, and multi-layered cybernetic agent upgrades.",
                                  -1, &rcDesc, DT_LEFT | DT_WORDBREAK);

            // Authors section
            int curY = 78 + hDesc + 14;
            SelectObject(hdc, s_hFontBold);
            SetTextColor(hdc, kDarkAccentCyan);
            RECT rcAuthTitle = { 24, curY, rc.right - 24, curY + 22 };
            DrawTextW(hdc, L"Author && Acknowledgments", -1, &rcAuthTitle, DT_LEFT | DT_SINGLELINE | DT_NOCLIP);
            curY += 24;

            SelectObject(hdc, s_hFont);
            SetTextColor(hdc, kDarkText);
            RECT rcAuth1 = { 24, curY, rc.right - 24, rc.bottom };
            int hAuth1 = DrawTextW(hdc, L"ReSyndicated Author: mork009 <mork009resynd@gmail.com>",
                                   -1, &rcAuth1, DT_LEFT | DT_WORDBREAK);
            curY += hAuth1 + 6;

            SetTextColor(hdc, kDarkTextMuted);
            RECT rcAuth2 = { 24, curY, rc.right - 24, rc.bottom };
            int hAuth2 = DrawTextW(hdc, L"ReSyndicated is forked off the open source project, FreeSynd. "
                                        L"Special thanks to The FreeSynd Team for building the original project.",
                                   -1, &rcAuth2, DT_LEFT | DT_WORDBREAK);
            curY += hAuth2 + 14;

            // License section
            SelectObject(hdc, s_hFontBold);
            SetTextColor(hdc, kDarkAccentCyan);
            RECT rcLicTitle = { 24, curY, rc.right - 24, curY + 22 };
            DrawTextW(hdc, L"License", -1, &rcLicTitle, DT_LEFT | DT_SINGLELINE | DT_NOCLIP);
            curY += 24;

            SelectObject(hdc, s_hFont);
            SetTextColor(hdc, kDarkTextMuted);
            RECT rcLic = { 24, curY, rc.right - 24, rc.bottom - 46 };
            DrawTextW(hdc, L"Released under the GNU General Public License Version 3 (GPLv3). "
                           L"This program is free software: you can redistribute it and/or modify it "
                           L"under the terms of the GNU General Public License as published by the Free Software Foundation.",
                      -1, &rcLic, DT_LEFT | DT_WORDBREAK);

            SelectObject(hdc, oldFont);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE || wParam == VK_RETURN) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_DESTROY:
            if (s_hFont) { DeleteObject(s_hFont); s_hFont = nullptr; }
            if (s_hFontBold) { DeleteObject(s_hFontBold); s_hFontBold = nullptr; }
            if (s_hFontHeader) { DeleteObject(s_hFontHeader); s_hFontHeader = nullptr; }
            return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void showGameAboutDialog(HWND hParent) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedGameAboutDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = GameAboutDlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wc.hbrBackground = CreateSolidBrush(kDarkDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    int dlgW = 680;
    int dlgH = 455;

    RECT rcParent = {};
    if (hParent && IsWindow(hParent)) {
        GetWindowRect(hParent, &rcParent);
    } else {
        rcParent.right = GetSystemMetrics(SM_CXSCREEN);
        rcParent.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (dlgX < 0) dlgX = 0;
    if (dlgY < 0) dlgY = 0;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        kClassName,
        L"About",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dlgX, dlgY, dlgW, dlgH,
        hParent, nullptr, hInstance, nullptr
    );

    if (!hDlg) return;

    if (hParent && IsWindow(hParent)) {
        EnableWindow(hParent, FALSE);
    }

    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg = {};
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    MSG quitMsg;
    while (PeekMessageW(&quitMsg, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE)) {}

    if (hParent && IsWindow(hParent)) {
        EnableWindow(hParent, TRUE);
        SetForegroundWindow(hParent);
        SetFocus(hParent);
    }
}

struct CustomSizeDlgState {
    int curW = 640;
    int curH = 400;
    int minW = 640;
    int minH = 400;
    int maxW = 3840;
    int maxH = 2160;
    bool accepted = false;
    HWND hEditW = nullptr;
    HWND hEditH = nullptr;
};

static LRESULT CALLBACK CustomSizeDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    CustomSizeDlgState *pState = reinterpret_cast<CustomSizeDlgState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<CustomSizeDlgState*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_CREATE: {
            if (!s_hFont) {
                s_hFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            }
            HINSTANCE hInst = GetModuleHandleW(nullptr);

            HWND hLblW = CreateWindowExW(0, L"STATIC", L"Window Width (pixels):",
                WS_CHILD | WS_VISIBLE, 24, 24, 150, 20, hWnd, nullptr, hInst, nullptr);
            SendMessageW(hLblW, WM_SETFONT, (WPARAM)s_hFont, TRUE);

            pState->hEditW = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", std::to_wstring(pState->curW).c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | WS_BORDER,
                180, 22, 140, 24, hWnd, nullptr, hInst, nullptr);
            SendMessageW(pState->hEditW, WM_SETFONT, (WPARAM)s_hFont, TRUE);

            HWND hLblH = CreateWindowExW(0, L"STATIC", L"Window Height (pixels):",
                WS_CHILD | WS_VISIBLE, 24, 60, 150, 20, hWnd, nullptr, hInst, nullptr);
            SendMessageW(hLblH, WM_SETFONT, (WPARAM)s_hFont, TRUE);

            pState->hEditH = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", std::to_wstring(pState->curH).c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_NUMBER | WS_BORDER,
                180, 58, 140, 24, hWnd, nullptr, hInst, nullptr);
            SendMessageW(pState->hEditH, WM_SETFONT, (WPARAM)s_hFont, TRUE);

            std::wstring helper = std::format(L"Limits: {}x{} minimum up to {}x{} (display limit)",
                pState->minW, pState->minH, pState->maxW, pState->maxH);
            HWND hHelper = CreateWindowExW(0, L"STATIC", helper.c_str(),
                WS_CHILD | WS_VISIBLE, 24, 96, 310, 36, hWnd, nullptr, hInst, nullptr);
            SendMessageW(hHelper, WM_SETFONT, (WPARAM)s_hFont, TRUE);

            HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                130, 144, 90, 28, hWnd, reinterpret_cast<HMENU>(IDOK), hInst, nullptr);
            SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)s_hFont, TRUE);
            if (!s_oldBtnProc) {
                s_oldBtnProc = (WNDPROC)GetWindowLongPtrW(hBtnOk, GWLP_WNDPROC);
            }
            SetWindowLongPtrW(hBtnOk, GWLP_WNDPROC, (LONG_PTR)GameAboutButtonProc);

            HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                230, 144, 90, 28, hWnd, reinterpret_cast<HMENU>(IDCANCEL), hInst, nullptr);
            SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)s_hFont, TRUE);
            SetWindowLongPtrW(hBtnCancel, GWLP_WNDPROC, (LONG_PTR)GameAboutButtonProc);

            enableDarkModeForWindow(hWnd);
            SetFocus(pState->hEditW);
            SendMessageW(pState->hEditW, EM_SETSEL, 0, -1);
            return 0;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(240, 240, 240));
            SetBkColor(hdcEdit, RGB(36, 40, 46));
            static HBRUSH s_hbrEdit = CreateSolidBrush(RGB(36, 40, 46));
            return (LRESULT)s_hbrEdit;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, kDarkText);
            SetBkColor(hdcStatic, kDarkDlgBg);
            static HBRUSH s_hbrStatic = CreateSolidBrush(kDarkDlgBg);
            return (LRESULT)s_hbrStatic;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            HBRUSH hbrBg = CreateSolidBrush(kDarkDlgBg);
            FillRect(hdc, &rc, hbrBg);
            DeleteObject(hbrBg);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK) {
                if (pState && pState->hEditW && pState->hEditH) {
                    wchar_t bufW[32] = {};
                    GetWindowTextW(pState->hEditW, bufW, 32);
                    wchar_t bufH[32] = {};
                    GetWindowTextW(pState->hEditH, bufH, 32);
                    int w = _wtoi(bufW);
                    int h = _wtoi(bufH);
                    if (w < pState->minW) w = pState->minW;
                    if (w > pState->maxW) w = pState->maxW;
                    if (h < pState->minH) h = pState->minH;
                    if (h > pState->maxH) h = pState->maxH;
                    pState->curW = w;
                    pState->curH = h;
                    pState->accepted = true;
                }
                DestroyWindow(hWnd);
                return 0;
            } else if (id == IDCANCEL) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool showCustomSizeDialog(HWND hParent, int &ioW, int &ioH, int minW, int minH, int maxW, int maxH) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedCustomSizeDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = CustomSizeDlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wc.hbrBackground = CreateSolidBrush(kDarkDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    CustomSizeDlgState state;
    state.curW = ioW;
    state.curH = ioH;
    state.minW = minW;
    state.minH = minH;
    state.maxW = maxW;
    state.maxH = maxH;

    int dlgW = 360;
    int dlgH = 225;

    RECT rcParent = {};
    if (hParent && IsWindow(hParent)) {
        GetWindowRect(hParent, &rcParent);
    } else {
        rcParent.right = GetSystemMetrics(SM_CXSCREEN);
        rcParent.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (dlgX < 0) dlgX = 0;
    if (dlgY < 0) dlgY = 0;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        kClassName,
        L"Custom Window Size",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dlgX, dlgY, dlgW, dlgH,
        hParent, nullptr, hInstance, &state
    );

    if (!hDlg) return false;

    if (hParent && IsWindow(hParent)) {
        EnableWindow(hParent, FALSE);
    }

    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg = {};
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    MSG quitMsg;
    while (PeekMessageW(&quitMsg, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE)) {}

    if (hParent && IsWindow(hParent)) {
        EnableWindow(hParent, TRUE);
        SetForegroundWindow(hParent);
        SetFocus(hParent);
    }

    if (state.accepted) {
        ioW = state.curW;
        ioH = state.curH;
        return true;
    }
    return false;
}

struct CustomZoomDlgState {
    double curZoom = 1.0;
    double minZoom = 0.5;
    double maxZoom = 4.0;
    bool accepted = false;
    HWND hEditZoom = nullptr;
};

static LRESULT CALLBACK CustomZoomDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    CustomZoomDlgState *pState = reinterpret_cast<CustomZoomDlgState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<CustomZoomDlgState*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_CREATE: {
            if (!s_hFont) {
                s_hFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            }
            HINSTANCE hInst = GetModuleHandleW(nullptr);

            HWND hLbl = CreateWindowExW(0, L"STATIC", L"Map Zoom Factor:",
                WS_CHILD | WS_VISIBLE, 24, 24, 150, 20, hWnd, nullptr, hInst, nullptr);
            SendMessageW(hLbl, WM_SETFONT, (WPARAM)s_hFont, TRUE);

            wchar_t zBuf[32] = {};
            swprintf_s(zBuf, L"%.2f", pState->curZoom);
            pState->hEditZoom = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", zBuf,
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER,
                180, 22, 140, 24, hWnd, nullptr, hInst, nullptr);
            SendMessageW(pState->hEditZoom, WM_SETFONT, (WPARAM)s_hFont, TRUE);

            const wchar_t *helper = L"Limits: 0.50x to 4.00x";
            HWND hHelper = CreateWindowExW(0, L"STATIC", helper,
                WS_CHILD | WS_VISIBLE, 24, 58, 310, 42, hWnd, nullptr, hInst, nullptr);
            SendMessageW(hHelper, WM_SETFONT, (WPARAM)s_hFont, TRUE);

            HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                130, 116, 90, 28, hWnd, reinterpret_cast<HMENU>(IDOK), hInst, nullptr);
            SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)s_hFont, TRUE);
            if (!s_oldBtnProc) {
                s_oldBtnProc = (WNDPROC)GetWindowLongPtrW(hBtnOk, GWLP_WNDPROC);
            }
            SetWindowLongPtrW(hBtnOk, GWLP_WNDPROC, (LONG_PTR)GameAboutButtonProc);

            HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                230, 116, 90, 28, hWnd, reinterpret_cast<HMENU>(IDCANCEL), hInst, nullptr);
            SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)s_hFont, TRUE);
            SetWindowLongPtrW(hBtnCancel, GWLP_WNDPROC, (LONG_PTR)GameAboutButtonProc);

            enableDarkModeForWindow(hWnd);
            SetFocus(pState->hEditZoom);
            SendMessageW(pState->hEditZoom, EM_SETSEL, 0, -1);
            return 0;
        }

        case WM_CTLCOLOREDIT: {
            HDC hdcEdit = (HDC)wParam;
            SetTextColor(hdcEdit, RGB(240, 240, 240));
            SetBkColor(hdcEdit, RGB(36, 40, 46));
            static HBRUSH s_hbrEdit = CreateSolidBrush(RGB(36, 40, 46));
            return (LRESULT)s_hbrEdit;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdcStatic = (HDC)wParam;
            SetTextColor(hdcStatic, kDarkText);
            SetBkColor(hdcStatic, kDarkDlgBg);
            static HBRUSH s_hbrStatic = CreateSolidBrush(kDarkDlgBg);
            return (LRESULT)s_hbrStatic;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            HBRUSH hbrBg = CreateSolidBrush(kDarkDlgBg);
            FillRect(hdc, &rc, hbrBg);
            DeleteObject(hbrBg);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK) {
                if (pState && pState->hEditZoom) {
                    wchar_t bufZ[32] = {};
                    GetWindowTextW(pState->hEditZoom, bufZ, 32);
                    double z = wcstod(bufZ, nullptr);
                    if (z <= 0.0) z = 1.0;
                    if (z < pState->minZoom) z = pState->minZoom;
                    if (z > pState->maxZoom) z = pState->maxZoom;
                    pState->curZoom = z;
                    pState->accepted = true;
                }
                DestroyWindow(hWnd);
                return 0;
            } else if (id == IDCANCEL) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool showCustomZoomDialog(HWND hParent, double &ioZoom) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedCustomZoomDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = CustomZoomDlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wc.hbrBackground = CreateSolidBrush(kDarkDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    CustomZoomDlgState state;
    state.curZoom = ioZoom;

    int dlgW = 360;
    int dlgH = 195;

    RECT rcParent = {};
    if (hParent && IsWindow(hParent)) {
        GetWindowRect(hParent, &rcParent);
    } else {
        rcParent.right = GetSystemMetrics(SM_CXSCREEN);
        rcParent.bottom = GetSystemMetrics(SM_CYSCREEN);
    }

    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (dlgX < 0) dlgX = 0;
    if (dlgY < 0) dlgY = 0;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        kClassName,
        L"Custom Map Zoom",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dlgX, dlgY, dlgW, dlgH,
        hParent, nullptr, hInstance, &state
    );

    if (!hDlg) return false;

    if (hParent && IsWindow(hParent)) {
        EnableWindow(hParent, FALSE);
    }

    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg = {};
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    MSG quitMsg;
    while (PeekMessageW(&quitMsg, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE)) {}

    if (hParent && IsWindow(hParent)) {
        EnableWindow(hParent, TRUE);
        SetForegroundWindow(hParent);
        SetFocus(hParent);
    }

    if (state.accepted) {
        ioZoom = state.curZoom;
        return true;
    }
    return false;
}

} // namespace
#endif

double g_mapZoom = 1.0;

SDL_Joystick *joy = NULL;

const int SystemSDL::kCursorWidth = 24;

SystemSDL::SystemSDL() {
    keyModState_ = 0;
    showCoordinates_ = false;
    showAgentPaths_ = false;
    showStats_ = false;
    mouse_in_window_ = true;

    pCursorTexture_ = nullptr;
    pWindow_ = nullptr;
    pRenderer_ = nullptr;
#ifdef _WIN32
    hWindowMenu_ = nullptr;
    hMapZoomMenu_ = nullptr;
    hViewMenu_ = nullptr;
    hDebugMenu_ = nullptr;
#endif
}

SystemSDL::~SystemSDL() {
#ifdef __APPLE__
    removeMacGameMenus(macMenuController_);
#endif
    if (pCursorTexture_) {
        SDL_DestroyTexture(pCursorTexture_);
        pCursorTexture_ = nullptr;
    }

    if (audio_) {
        audio_->quit();
    }

    // SDL3_image does not require explicit IMG_Quit()

    if (pRenderer_) {
        SDL_DestroyRenderer(pRenderer_);
        pRenderer_ = nullptr;
    }

    if (pWindow_) {
        SDL_DestroyWindow( pWindow_ );
        pWindow_ = nullptr;
    }

    SDL_Quit();
}

void SystemSDL::initialize(bool fullscreen) {
    LOG(Log::k_FLG_INFO, "SystemSDL", "initialize", ("initializing System SDL"))

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw InitializationFailedException(std::format("Critical error, SDL could not be initialized! SDL Error :  {}", SDL_GetError()));
    }

    pWindow_ = createWindow(fullscreen);

    if (pWindow_ == nullptr) {
        throw InitializationFailedException(std::format("Critical error, SDL could not be initialized! SDL Error :  {}", SDL_GetError()));
    }

#ifdef _WIN32
    HWND hwnd = getHwndFromWindow(pWindow_);
    if (hwnd) {
        
        HMENU hMenuBar = CreateMenu();
        
        HMENU hFileMenu = CreateMenu();
        AppendMenuW(hFileMenu, MF_STRING, 1002, L"&Settings...");
        AppendMenuW(hFileMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hFileMenu, MF_STRING, 1001, L"Quit");
        AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hFileMenu, L"File");
        
        hWindowMenu_ = CreateMenu();
        const wchar_t* scaleLabels[] = {
            nullptr,
            L"1x (640x480)",
            L"2x (1280x960)",
            L"3x (1920x1440)",
            L"4x (2560x1920)"
        };
        for (int s = 1; s <= 4; ++s) {
            UINT flags = MF_STRING;
            if (!isScaleAllowed(s)) {
                flags |= (MF_GRAYED | MF_DISABLED);
            }
            AppendMenuW((HMENU)hWindowMenu_, flags, 2000 + s, scaleLabels[s]);
        }
        AppendMenuW((HMENU)hWindowMenu_, MF_SEPARATOR, 0, NULL);
        AppendMenuW((HMENU)hWindowMenu_, MF_STRING, 2006, L"Custom Size...");
        CheckMenuRadioItem((HMENU)hWindowMenu_, 2001, 2006, 2001, MF_BYCOMMAND);

        hMapZoomMenu_ = CreateMenu();
        AppendMenuW((HMENU)hMapZoomMenu_, MF_STRING, 4005, L"0.5x (Close)");
        AppendMenuW((HMENU)hMapZoomMenu_, MF_STRING, 4001, L"1.0x (Default)");
        AppendMenuW((HMENU)hMapZoomMenu_, MF_STRING, 4002, L"1.5x (Medium)");
        AppendMenuW((HMENU)hMapZoomMenu_, MF_STRING, 4003, L"2.0x (Double)");
        AppendMenuW((HMENU)hMapZoomMenu_, MF_SEPARATOR, 0, NULL);
        AppendMenuW((HMENU)hMapZoomMenu_, MF_STRING, 4004, L"Custom Zoom...");
        CheckMenuRadioItem((HMENU)hMapZoomMenu_, 4001, 4005, 4001, MF_BYCOMMAND);

        hViewMenu_ = CreateMenu();
        AppendMenuW((HMENU)hViewMenu_, MF_POPUP, (UINT_PTR)hWindowMenu_, L"Size");
        AppendMenuW((HMENU)hViewMenu_, MF_POPUP, (UINT_PTR)hMapZoomMenu_, L"Zoom");
        AppendMenuW((HMENU)hViewMenu_, MF_SEPARATOR, 0, NULL);
        AppendMenuW((HMENU)hViewMenu_, MF_STRING, 2005, L"Full Screen\tF11");
        if (fullscreen) {
            CheckMenuItem((HMENU)hViewMenu_, 2005, MF_BYCOMMAND | MF_CHECKED);
        }
        AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hViewMenu_, L"View");

        HMENU hCheatsMenu = CreateMenu();
        AppendMenuW(hCheatsMenu, MF_STRING, 3001, L"Enable All Cheats");
        AppendMenuW(hCheatsMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hCheatsMenu, MF_STRING, 3002, L"Play Any Mission");
        AppendMenuW(hCheatsMenu, MF_STRING, 3003, L"Add Max Funds ($100M)");
        AppendMenuW(hCheatsMenu, MF_STRING, 3004, L"Unlock All Weapons && Upgrades");
        AppendMenuW(hCheatsMenu, MF_STRING, 3005, L"Equip All V3 Upgrades");
        AppendMenuW(hCheatsMenu, MF_STRING, 3006, L"Equip Top Weapons && Items");

        hDebugMenu_ = CreateMenu();
        AppendMenuW((HMENU)hDebugMenu_, MF_STRING, 5001, L"Show Tile Coordinates");
        AppendMenuW((HMENU)hDebugMenu_, MF_STRING, 5002, L"Show Agent Paths");
        AppendMenuW((HMENU)hDebugMenu_, MF_STRING, 5003, L"Show Stats");
        CheckMenuItem((HMENU)hDebugMenu_, 5001, MF_BYCOMMAND | (showCoordinates_ ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem((HMENU)hDebugMenu_, 5002, MF_BYCOMMAND | (showAgentPaths_ ? MF_CHECKED : MF_UNCHECKED));
        CheckMenuItem((HMENU)hDebugMenu_, 5003, MF_BYCOMMAND | (showStats_ ? MF_CHECKED : MF_UNCHECKED));
        AppendMenuW((HMENU)hDebugMenu_, MF_SEPARATOR, 0, NULL);
        AppendMenuW((HMENU)hDebugMenu_, MF_POPUP, (UINT_PTR)hCheatsMenu, L"Cheats");
        AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hDebugMenu_, L"Debug");

        HMENU hHelpMenu = CreateMenu();
        AppendMenuW(hHelpMenu, MF_STRING, 6001, L"&About");
        AppendMenuW(hMenuBar, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");
        
        SetMenu(hwnd, hMenuBar);
        
        if (!fullscreen) {
            SDL_SetWindowSize(pWindow_, rs_eng::kScreenWidth, rs_eng::kScreenHeight);
            SDL_SetWindowPosition(pWindow_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
        
        SDL_SetWindowsMessageHook(win32MessageHook, this);
    }
#elif defined(__APPLE__)
    macMenuController_ = installMacGameMenus(this, pWindow_);
#endif

    pRenderer_ = SDL_CreateRenderer(pWindow_, NULL);
    if (pRenderer_ == nullptr) {
        throw InitializationFailedException(std::format("Critical error, SDL could not be initialized! SDL Error :  {}", SDL_GetError()));
    }
    SDL_SetRenderVSync(pRenderer_, 1);
    SDL_SetDefaultTextureScaleMode(pRenderer_, SDL_SCALEMODE_NEAREST);

    // initialize the screen to black
    SDL_SetRenderDrawColor(pRenderer_, 0, 0, 0, 255);
    SDL_RenderClear(pRenderer_);
    // in case we are fullscreen and did not specify dimensions
    SDL_SetRenderLogicalPresentation(pRenderer_, rs_eng::kScreenWidth, rs_eng::kScreenHeight, SDL_LOGICAL_PRESENTATION_LETTERBOX);

    // Load the cursor sprites
    if (loadCursorSprites()) {
        SDL_HideCursor();
    }
    hideCursor();
    useMenuCursor();
    mouse_in_window_ = true;

    // Audio initialization
#ifdef HAVE_SDL_MIXER
    audio_ = std::make_unique<SdlMixerAudio>();
#else
    audio_ = std::make_unique<DefaultAudio>();
#endif

    if (!audio_->init()) {
        RSINFO(Log::k_FLG_SND, "SystemSDL", "Init", ("Couldn't initialize Sound System : no sound will be played."))
    }

    // Be sure that we don't receive unusefull text events
    // TextInput are activated when user clicks in a textfield
    if (pWindow_) SDL_StopTextInput(pWindow_);
}

SDL_Window * SystemSDL::createWindow(bool fullscreen) {
#ifdef _WIN32
    SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON, "1");
    SDL_SetHint(SDL_HINT_WINDOWS_INTRESOURCE_ICON_SMALL, "1");
#endif

    SDL_Window *sdlWindow = nullptr;
    if (fullscreen) {
        sdlWindow = SDL_CreateWindow("ReSyndicated - " RS_VERSION_STRING,
                             rs_eng::kScreenWidth, rs_eng::kScreenHeight,
                             SDL_WINDOW_FULLSCREEN);
    } else {
        sdlWindow = SDL_CreateWindow("ReSyndicated - " RS_VERSION_STRING,
                          rs_eng::kScreenWidth, rs_eng::kScreenHeight,
                          SDL_WINDOW_RESIZABLE);
        if (sdlWindow) {
            SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
    }

    if (sdlWindow) {
        std::string iconPath = rs_utl::File::getResyndDataFullPath("resources/logo.png");
        SDL_Surface *iconSurface = IMG_Load(iconPath.c_str());
        if (!iconSurface) {
            iconPath = rs_utl::File::getResyndDataFullPath("resources/logo.ico");
            iconSurface = IMG_Load(iconPath.c_str());
        }
        if (iconSurface) {
            SDL_SetWindowIcon(sdlWindow, iconSurface);
            SDL_DestroySurface(iconSurface);
        }
    }

    return sdlWindow;
}

void SystemSDL::showError(const char *errorMsg) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                                "Error in ReSyndicated",
                                errorMsg,
                                pWindow_);
    RSERR(Log::k_FLG_IO, "SystemSDL", "showError", (errorMsg));
}

#ifdef _WIN32
//! Opens a native Windows file open dialog.
bool SystemSDL::browseFile(const char *title, const char *filterDesc, const char *filterPattern, std::filesystem::path &outPath) {
    HWND hParent = getHwndFromWindow(pWindow_);

    auto toWide = [](const char *str) -> std::wstring {
        if (!str || !*str) return L"";
        int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
        if (len <= 0) return L"";
        std::wstring res(static_cast<size_t>(len - 1), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, str, -1, res.data(), len);
        return res;
    };

    std::wstring wTitle = toWide(title);
    std::wstring wFilterDesc = toWide(filterDesc);
    std::wstring wFilterPat = toWide(filterPattern);

    std::wstring wFilter;
    if (!wFilterDesc.empty() && !wFilterPat.empty()) {
        wFilter.append(wFilterDesc);
        wFilter.push_back(L'\0');
        wFilter.append(wFilterPat);
        wFilter.push_back(L'\0');
    }
    wFilter.append(L"All Files (*.*)");
    wFilter.push_back(L'\0');
    wFilter.append(L"*.*");
    wFilter.push_back(L'\0');
    wFilter.push_back(L'\0');

    wchar_t szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hParent;
    ofn.lpstrFilter = wFilter.c_str();
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = wTitle.empty() ? nullptr : wTitle.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_NOCHANGEDIR;

    bool prevCursor = SDL_CursorVisible();
    SDL_ShowCursor();
    BOOL ok = GetOpenFileNameW(&ofn);
    if (!prevCursor) SDL_HideCursor();

    if (ok) {
        outPath = std::filesystem::path(szFile);
        return true;
    }
    return false;
}
#endif

/*!
 * Clear the screen with the black color.
 * @return True if ok.
 */
bool SystemSDL::clearScreen() {
    SDL_SetRenderDrawColor(pRenderer_, 0x00, 0x00, 0x00, 0xFF);
    return SDL_RenderClear(pRenderer_);
}

void SystemSDL::updateScreen() {        
    if (cursor_visible_ && mouse_in_window_) {
        SDL_FRect dst = {
            static_cast<float>(cursor_x_ - cursor_hs_x_),
            static_cast<float>(cursor_y_ - cursor_hs_y_),
            static_cast<float>(kCursorWidth),
            static_cast<float>(kCursorWidth)
        };
        SDL_FRect src = {
            static_cast<float>(cursor_rect_.x),
            static_cast<float>(cursor_rect_.y),
            static_cast<float>(cursor_rect_.w),
            static_cast<float>(cursor_rect_.h)
        };

        SDL_RenderTexture(pRenderer_, pCursorTexture_, &src, &dst);
        update_cursor_ = false;
    }

    // Flip screen
    SDL_RenderPresent(pRenderer_);
}

bool SystemSDL::resetRenderTarget() {
    if (!SDL_SetRenderTarget(pRenderer_, NULL)) {
        RSERR(Log::k_FLG_GFX, "SystemSDL", "resetRenderTarget", ("Critical error, Could not reset target texture! SDL Error : %s", SDL_GetError()))
        return false;
    }

    return true;
}

/*!
 * Set the viewport
 * @param x Origin of the viewport on the screen
 * @param y Origin of the viewport on the screen
 * @param width Width of the viewport
 * @param height Height of the viewport
 * @return True if setting the viewport is ok
 */
bool SystemSDL::setViewport(int x, int y, int width, int height) {
    SDL_Rect rect {x, y, width, height};
    if (!SDL_SetRenderViewport(pRenderer_, &rect)) {
        RSERR(Log::k_FLG_GFX, "SystemSDL", "setViewport", ("Critical error, Could not set viewport! SDL Error : %s", SDL_GetError()))
        return false;
    }

    return true;
}
    
/*!
 * Reset the viewport to the default one
 * @return True if ok
 */
bool SystemSDL::resetViewport() {
    if (!SDL_SetRenderViewport(pRenderer_, nullptr)) {
        RSERR(Log::k_FLG_GFX, "SystemSDL", "resetViewport", ("Critical error, Could not reset viewport! SDL Error : %s", SDL_GetError()))
        return false;
    }

    return true;
}

bool SystemSDL::isScaleAllowed(int scale) const {
    if (scale < 1 || scale > 4) return false;
    if (!pWindow_) return (scale <= 2);

    SDL_DisplayID displayID = SDL_GetDisplayForWindow(pWindow_);
    if (displayID == 0) return (scale <= 2);

    SDL_Rect usableBounds = {0, 0, 0, 0};
    if (!SDL_GetDisplayUsableBounds(displayID, &usableBounds) || usableBounds.w <= 0 || usableBounds.h <= 0) {
        if (!SDL_GetDisplayBounds(displayID, &usableBounds) || usableBounds.w <= 0 || usableBounds.h <= 0) {
            return (scale <= 2);
        }
    }

    int w = rs_eng::kScreenWidth * scale;
    int h = rs_eng::kScreenHeight * scale;

    return (w <= usableBounds.w && h <= usableBounds.h);
}

#ifdef _WIN32
void SystemSDL::updateWindowMenuScales() {
    if (hWindowMenu_) {
        for (int s = 1; s <= 4; ++s) {
            UINT flags = MF_BYCOMMAND;
            if (isScaleAllowed(s)) {
                flags |= MF_ENABLED;
            } else {
                flags |= (MF_GRAYED | MF_DISABLED);
            }
            EnableMenuItem((HMENU)hWindowMenu_, 2000 + s, flags);
        }
    }
}
#endif

void SystemSDL::setWindowScale(int scale) {
    if (scale < 1 || scale > 4 || !pWindow_) return;

    if (!isScaleAllowed(scale)) {
        LOG(Log::k_FLG_GFX, "SystemSDL", "setWindowScale",
            ("Window scale %dx (%dx%d) exceeds display resolution and was blocked.",
             scale, rs_eng::kScreenWidth * scale, rs_eng::kScreenHeight * scale))
        return;
    }

    int w = rs_eng::kScreenWidth * scale;
    int h = rs_eng::kScreenHeight * scale;
    SDL_SetWindowSize(pWindow_, w, h);
    SDL_SetWindowPosition(pWindow_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

#ifdef _WIN32
    if (hWindowMenu_) {
        int selectedId = 2000 + scale;
        CheckMenuRadioItem((HMENU)hWindowMenu_, 2001, 2006, selectedId, MF_BYCOMMAND);
        updateWindowMenuScales();
    }
#endif
}

void SystemSDL::setWindowDimensions(int width, int height) {
    if (width <= 0 || height <= 0 || !pWindow_) return;

    SDL_DisplayID displayID = SDL_GetDisplayForWindow(pWindow_);
    SDL_Rect usableBounds = {0, 0, 0, 0};
    if (displayID != 0 && SDL_GetDisplayUsableBounds(displayID, &usableBounds) && usableBounds.w > 0 && usableBounds.h > 0) {
        if (width > usableBounds.w || height > usableBounds.h) {
            LOG(Log::k_FLG_GFX, "SystemSDL", "setWindowDimensions",
                ("Requested window size (%dx%d) exceeds display resolution (%dx%d). Clamping.",
                 width, height, usableBounds.w, usableBounds.h))
            width = std::min(width, usableBounds.w);
            height = std::min(height, usableBounds.h);
        }
    }

    SDL_SetWindowSize(pWindow_, width, height);
    SDL_SetWindowPosition(pWindow_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

#ifdef _WIN32
    if (hWindowMenu_) {
        int selectedId = 2006;
        if (width == rs_eng::kScreenWidth && height == rs_eng::kScreenHeight) selectedId = 2001;
        else if (width == rs_eng::kScreenWidth * 2 && height == rs_eng::kScreenHeight * 2) selectedId = 2002;
        else if (width == rs_eng::kScreenWidth * 3 && height == rs_eng::kScreenHeight * 3) selectedId = 2003;
        else if (width == rs_eng::kScreenWidth * 4 && height == rs_eng::kScreenHeight * 4) selectedId = 2004;

        CheckMenuRadioItem((HMENU)hWindowMenu_, 2001, 2006, selectedId, MF_BYCOMMAND);
        updateWindowMenuScales();
    }
#endif
}

void SystemSDL::setMapZoom(double zoomFactor) {
    if (zoomFactor <= 0.0) return;
    if (zoomFactor < 0.5) zoomFactor = 0.5;
    if (zoomFactor > 4.0) zoomFactor = 4.0;

    g_mapZoom = zoomFactor;

#ifdef _WIN32
    if (hMapZoomMenu_) {
        int wmId = 4004;
        if (std::abs(zoomFactor - 0.5) < 0.01) wmId = 4005;
        else if (std::abs(zoomFactor - 1.0) < 0.01) wmId = 4001;
        else if (std::abs(zoomFactor - 1.5) < 0.01) wmId = 4002;
        else if (std::abs(zoomFactor - 2.0) < 0.01) wmId = 4003;

        CheckMenuRadioItem((HMENU)hMapZoomMenu_, 4001, 4005, wmId, MF_BYCOMMAND);
    }
#endif
}

/*!
 * Using the keysym parameter, verify if the given key is a function key (ie
 * a not printable key) returns the corresponding entry in the KeyFunc enumeration.
 * \returns If key code is not a function key, returns KEY_UNKNOWN.
 */
void SystemSDL::fillKeyEvent(const SDL_KeyboardEvent &keyEvt, RS_Event &evtOut) {
    evtOut.type = EVT_KEY_DOWN;
    RS_Key key;
    key.keyCode = kKeyCode_Unknown;
    SDL_Keycode sym = keyEvt.key;
    switch(sym) {
        case SDLK_ESCAPE: key.keyCode = kKeyCode_Escape; break;
        case SDLK_BACKSPACE: key.keyCode = kKeyCode_Backspace; break;
        case SDLK_SPACE: key.keyCode = kKeyCode_Space; break;
        case SDLK_RETURN: key.keyCode = kKeyCode_Return; break;
        case SDLK_DELETE: key.keyCode = kKeyCode_Delete; break;
        case SDLK_UP: key.keyCode = kKeyCode_Up; break;
        case SDLK_DOWN: key.keyCode = kKeyCode_Down; break;
        case SDLK_RIGHT: key.keyCode = kKeyCode_Right; break;
        case SDLK_LEFT: key.keyCode = kKeyCode_Left; break;
        case SDLK_INSERT: key.keyCode = kKeyCode_Insert; break;
        case SDLK_HOME: key.keyCode = kKeyCode_Home; break;
        case SDLK_END: key.keyCode = kKeyCode_End; break;
        case SDLK_PAGEUP: key.keyCode = kKeyCode_PageUp; break;
        case SDLK_PAGEDOWN: key.keyCode = kKeyCode_PageDown; break;
        case SDLK_F1: case SDLK_F2: case SDLK_F3: case SDLK_F4:
        case SDLK_F5: case SDLK_F6: case SDLK_F7: case SDLK_F8:
        case SDLK_F9: case SDLK_F10: case SDLK_F11: case SDLK_F12:
            key.keyCode = static_cast < RS_KeyCode > (kKeyCode_F1 + (sym - SDLK_F1));
            break;
        case SDLK_0: case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
            key.keyCode = static_cast < RS_KeyCode > (kKeyCode_0 + (sym - SDLK_0));
            break;
        case SDLK_A: case SDLK_B: case SDLK_C: case SDLK_D: case SDLK_E:
        case SDLK_F: case SDLK_G: case SDLK_H: case SDLK_I: case SDLK_J:
        case SDLK_K: case SDLK_L: case SDLK_M: case SDLK_N: case SDLK_O:
        case SDLK_P: case SDLK_Q: case SDLK_R: case SDLK_S: case SDLK_T:
        case SDLK_U: case SDLK_V: case SDLK_W: case SDLK_X: case SDLK_Y:
        case SDLK_Z:
            key.keyCode = static_cast < RS_KeyCode > (kKeyCode_A + (sym - SDLK_A));
            break;
        case SDLK_KP_PLUS: key.keyCode = kKeyCode_Plus; break;
        case SDLK_KP_MINUS: key.keyCode = kKeyCode_Minus; break;
        default:
            break;
    }

    evtOut.key.key = key;
    evtOut.key.keyMods = keyModState_;
}

//! Pumps an event from the event queue
/*!
 * Watch the event queue and dispatch events.
 * - keyboard events : when a modifier key is pressed,
 * the system does not dispatch the event to the application:
 * it stores the key state and then passes the complete state
 * when a regular key is pressed. So that the application knows
 * if multiple modifier keys are pressed at the same time (ie Ctrl/Shift)
 */
#ifdef _WIN32
bool SDLCALL SystemSDL::win32MessageHook(void *userdata, MSG *msg) {
    if (msg->message == WM_COMMAND) {
        SystemSDL *sys = static_cast<SystemSDL *>(userdata);
        if (sys) {
            sys->handleMenuCommand(LOWORD(msg->wParam));
            return false;
        }
    }
    return true;
}

#endif

void SystemSDL::handleMenuCommand(int wmId) {
#ifdef _WIN32
    HWND hParent = getHwndFromWindow(pWindow_);
#else
    SDL_Window *hParent = pWindow_;
#endif

    if (wmId == 1001) {
        RS_Event evt;
        evt.quit.type = EVT_QUIT;
        pendingEvents_.push(evt);
    } else if (wmId == 1002) {
        std::string curDir = rs_utl::File::getOriginalDataFolder().string();
        std::string newDir;
#ifdef _WIN32
        bool changed = showSettingsDialog(hParent, curDir, newDir, L"ReSyndicated") &&
                       pathsAreDifferent(curDir, newDir);
#else
        bool changed = chooseMacDataFolder(curDir, newDir);
#endif
        if (changed) {
            rs_utl::File::setOriginalDataFolder(newDir);
            std::string sig = rs_utl::File::computeDataSignature(newDir);
            saveOriginalDataDirToResyndConf(newDir, sig);
            g_Ctx.setDataSignature(sig);
            RS_Event evt;
            evt.type = EVT_RELOAD_ASSETS;
            pendingEvents_.push(evt);
        }
    } else if (wmId >= 2001 && wmId <= 2004) {
        int targetScale = wmId - 2000;
        if (isScaleAllowed(targetScale)) {
            setWindowScale(targetScale);
        }
    } else if (wmId == 2005) {
        toggleFullscreen();
    } else if (wmId == 2006) {
        int curW = rs_eng::kScreenWidth;
        int curH = rs_eng::kScreenHeight;
        if (pWindow_) {
            SDL_GetWindowSize(pWindow_, &curW, &curH);
        }
        SDL_DisplayID dispId = pWindow_ ? SDL_GetDisplayForWindow(pWindow_) : 0;
        SDL_Rect usableBounds = {0, 0, 0, 0};
        int maxW = 3840;
        int maxH = 2160;
        if (dispId != 0 && SDL_GetDisplayUsableBounds(dispId, &usableBounds) && usableBounds.w > 0 && usableBounds.h > 0) {
            maxW = usableBounds.w;
            maxH = usableBounds.h;
        }
        int newW = curW;
        int newH = curH;
        if (showCustomSizeDialog(hParent, newW, newH, rs_eng::kScreenWidth, rs_eng::kScreenHeight, maxW, maxH)) {
            setWindowDimensions(newW, newH);
        }
    } else if (wmId >= 3001 && wmId <= 3006) {
        RS_Event evt;
        evt.type = EVT_CHEAT;
        evt.cheat.cheatId = wmId;
        pendingEvents_.push(evt);
    } else if ((wmId >= 4001 && wmId <= 4003) || wmId == 4005) {
        double zoomVal = 1.0;
        if (wmId == 4005) zoomVal = 0.5;
        else if (wmId == 4002) zoomVal = 1.5;
        else if (wmId == 4003) zoomVal = 2.0;
        setMapZoom(zoomVal);
        RS_Event evt;
        evt.type = EVT_ZOOM;
        evt.zoom.zoomFactor = zoomVal;
        pendingEvents_.push(evt);
    } else if (wmId == 4004) {
        double curZoom = g_mapZoom;
        if (showCustomZoomDialog(hParent, curZoom)) {
            setMapZoom(curZoom);
            RS_Event evt;
            evt.type = EVT_ZOOM;
            evt.zoom.zoomFactor = curZoom;
            pendingEvents_.push(evt);
        }
    } else if (wmId == 5001) {
        setShowCoordinates(!showCoordinates_);
    } else if (wmId == 5002) {
        setShowAgentPaths(!showAgentPaths_);
    } else if (wmId == 5003) {
        setShowStats(!showStats_);
    } else if (wmId == 6001) {
        showGameAboutDialog(hParent);
    }
}

bool SystemSDL::pumpEvents(RS_Event &evtOut) {
    if (!pendingEvents_.empty()) {
        evtOut = pendingEvents_.front();
        pendingEvents_.pop();
        return true;
    }

    SDL_Event evtIn;
    evtOut.type = EVT_NONE;

    while (evtOut.type == EVT_NONE && SDL_PollEvent(&evtIn)) {
        if (pRenderer_) {
            SDL_ConvertEventToRenderCoordinates(pRenderer_, &evtIn);
        }

        switch (evtIn.type) {
        case SDL_EVENT_QUIT:
            evtOut.quit.type = EVT_QUIT;
            break;
        case SDL_EVENT_TEXT_INPUT:
            {
                evtOut.key.type = EVT_KEY_DOWN;
                evtOut.key.key.keyCode = kKeyCode_Text;
                const char* w = evtIn.text.text;
                evtOut.key.key.codePoint = utf8::next(w, evtIn.text.text + strlen(evtIn.text.text));
                evtOut.key.keyMods = keyModState_;
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            {
                switch(evtIn.key.key) {
                    case SDLK_RSHIFT:
                        keyModState_ = keyModState_ | KMD_RSHIFT;
                        break;
                    case SDLK_LSHIFT:
                        keyModState_ = keyModState_ | KMD_LSHIFT;
                        break;
                    case SDLK_RCTRL:
                        keyModState_ = keyModState_ | KMD_RCTRL;
                        break;
                    case SDLK_LCTRL:
                        keyModState_ = keyModState_ | KMD_LCTRL;
                        break;
                    case SDLK_RALT:
                        keyModState_ = keyModState_ | KMD_RALT;
                        break;
                    case SDLK_LALT:
                        keyModState_ = keyModState_ | KMD_LALT;
                        break;
                    case SDLK_F11:
                        toggleFullscreen();
                        break;
                    default:
                        fillKeyEvent(evtIn.key, evtOut);
                        break;
                }
            }
            break;
        case SDL_EVENT_KEY_UP:
            {
                switch(evtIn.key.key) {
                    case SDLK_RSHIFT:
                        keyModState_ = keyModState_ & ~KMD_RSHIFT;
                        break;
                    case SDLK_LSHIFT:
                        keyModState_ = keyModState_ & ~KMD_LSHIFT;
                        break;
                    case SDLK_RCTRL:
                        keyModState_ = keyModState_ & ~KMD_RCTRL;
                        break;
                    case SDLK_LCTRL:
                        keyModState_ = keyModState_ & ~KMD_LCTRL;
                        break;
                    case SDLK_RALT:
                        keyModState_ = keyModState_ & ~KMD_RALT;
                        break;
                    case SDLK_LALT:
                        keyModState_ = keyModState_ & ~KMD_LALT;
                        break;
                    default:
                        break;
                }
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            evtOut.button.type = EVT_MSE_UP;
            evtOut.button.x = cursor_x_ = static_cast<int>(std::round(evtIn.button.x));
            evtOut.button.y = cursor_y_ = static_cast<int>(std::round(evtIn.button.y));
            evtOut.button.button = evtIn.button.button;
            evtOut.button.keyMods = keyModState_;
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            evtOut.button.type = EVT_MSE_DOWN;
            evtOut.button.x = cursor_x_ = static_cast<int>(std::round(evtIn.button.x));
            evtOut.button.y = cursor_y_ = static_cast<int>(std::round(evtIn.button.y));
            evtOut.button.button = evtIn.button.button;
            evtOut.button.keyMods = keyModState_;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            mouse_in_window_ = true;
            update_cursor_ = true;
            evtOut.motion.type = EVT_MSE_MOTION;
            evtOut.motion.x = cursor_x_ = static_cast<int>(std::round(evtIn.motion.x));
            evtOut.motion.y = cursor_y_ = static_cast<int>(std::round(evtIn.motion.y));
            evtOut.motion.state = evtIn.motion.state;
            evtOut.motion.keyMods = keyModState_;
            break;
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            mouse_in_window_ = true;
            {
                float mx = 0.0f, my = 0.0f;
                SDL_GetMouseState(&mx, &my);
                if (pRenderer_) {
                    SDL_RenderCoordinatesFromWindow(pRenderer_, mx, my, &mx, &my);
                }
                cursor_x_ = static_cast<int>(std::round(mx));
                cursor_y_ = static_cast<int>(std::round(my));
                update_cursor_ = true;
            }
            break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            mouse_in_window_ = false;
            break;
#ifdef _WIN32
        case SDL_EVENT_WINDOW_MOVED:
            updateWindowMenuScales();
            break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            {
                int w = evtIn.window.data1;
                int h = evtIn.window.data2;
                int selectedId = 2006;
                if (w == rs_eng::kScreenWidth && h == rs_eng::kScreenHeight) selectedId = 2001;
                else if (w == rs_eng::kScreenWidth * 2 && h == rs_eng::kScreenHeight * 2) selectedId = 2002;
                else if (w == rs_eng::kScreenWidth * 3 && h == rs_eng::kScreenHeight * 3) selectedId = 2003;
                else if (w == rs_eng::kScreenWidth * 4 && h == rs_eng::kScreenHeight * 4) selectedId = 2004;

                CheckMenuRadioItem((HMENU)hWindowMenu_, 2001, 2006, selectedId, MF_BYCOMMAND);
                updateWindowMenuScales();
            }
            break;
#endif
        default:
            break;
        }
    }

    if (evtOut.type == EVT_NONE && !pendingEvents_.empty()) {
        evtOut = pendingEvents_.front();
        pendingEvents_.pop();
    }

    return evtOut.type != EVT_NONE;
}

void SystemSDL::toggleFullscreen() {
    uint64_t flags = SDL_GetWindowFlags(pWindow_);
    bool isFullscreen = (flags & SDL_WINDOW_FULLSCREEN) != 0;
    bool newFullscreen = !isFullscreen;
    SDL_SetWindowFullscreen(pWindow_, newFullscreen);
#ifdef _WIN32
    if (hViewMenu_) {
        CheckMenuItem((HMENU)hViewMenu_, 2005, MF_BYCOMMAND | (newFullscreen ? MF_CHECKED : MF_UNCHECKED));
    }
#endif
}

void SystemSDL::setShowCoordinates(bool show) {
    showCoordinates_ = show;
#ifdef _WIN32
    if (hDebugMenu_) {
        CheckMenuItem((HMENU)hDebugMenu_, 5001, MF_BYCOMMAND | (showCoordinates_ ? MF_CHECKED : MF_UNCHECKED));
    }
#endif
}

void SystemSDL::setShowAgentPaths(bool show) {
    showAgentPaths_ = show;
#ifdef _WIN32
    if (hDebugMenu_) {
        CheckMenuItem((HMENU)hDebugMenu_, 5002, MF_BYCOMMAND | (showAgentPaths_ ? MF_CHECKED : MF_UNCHECKED));
    }
#endif
}

void SystemSDL::setShowStats(bool show) {
    showStats_ = show;
#ifdef _WIN32
    if (hDebugMenu_) {
        CheckMenuItem((HMENU)hDebugMenu_, 5003, MF_BYCOMMAND | (showStats_ ? MF_CHECKED : MF_UNCHECKED));
    }
#endif
}

void SystemSDL::delay(uint32_t msec) {
    SDL_Delay(msec);
}

uint32_t SystemSDL::getTicks() {
    return static_cast<uint32_t>(SDL_GetTicks());
}

bool like(int a, int b) {
    return a == b || a == b - 1 || a == b + 1;
}

/*!
 * Draw a point on the screen at given position
 * @param position 
 * @param color 
 */
void SystemSDL::drawPoint (Point2D position, rs_eng::RSColor color) {
    SDL_SetRenderDrawColor(pRenderer_, color.r, color.g, color.b, color.a);
    SDL_RenderPoint(pRenderer_, static_cast<float>(position.x), static_cast<float>(position.y));
}

/*!
 * Draw a vertical line with given length and color
 * @param start The starting point of the line
 * @param length The length of the line
 * @param color Color of the line
 */
void SystemSDL::drawVLine(Point2D start, int length, rs_eng::RSColor color) {
    SDL_SetRenderDrawColor(pRenderer_, color.r, color.g, color.b, color.a);
    SDL_RenderLine(pRenderer_, static_cast<float>(start.x), static_cast<float>(start.y),
                   static_cast<float>(start.x), static_cast<float>(start.y + length));
}

/*!
 * Draw a horizontal line with given length and color
 * @param start The starting point of the line
 * @param length The length of the line
 * @param color Color of the line
 */
void SystemSDL::drawHLine(Point2D start, int length, rs_eng::RSColor color) {
    SDL_SetRenderDrawColor(pRenderer_, color.r, color.g, color.b, color.a);
    SDL_RenderLine(pRenderer_, static_cast<float>(start.x), static_cast<float>(start.y),
                   static_cast<float>(start.x + length), static_cast<float>(start.y));
}

/*!
 * Draw a vertical line of the given length which is composed of segments of dashLength and space of same size.
 */
void SystemSDL::drawDashedVLine(Point2D start, int length, int dashLength, int dashOffset, rs_eng::RSColor color) {
    dashOffset %= 2 * dashLength;
    Point2D end = start.add(0, length);

    SDL_SetRenderDrawColor(pRenderer_, color.r, color.g, color.b, color.a);
   
    Point2D segmentStart = start;
    Point2D segmentEnd = start;

    if (dashOffset <= dashLength) {
        segmentEnd.y += dashOffset;
    } else {
        segmentStart.y += (dashOffset - dashLength);
        segmentEnd.y = (segmentStart.y + dashLength);
    }
    SDL_RenderLine(pRenderer_, static_cast<float>(segmentStart.x), static_cast<float>(segmentStart.y),
                   static_cast<float>(segmentEnd.x), static_cast<float>(segmentEnd.y));
    int distanceDone = dashOffset;

    int i = 0;
    while (distanceDone < length && i < 100) {
        if (segmentEnd.y + dashLength > end.y) {
            break;
        }
        segmentStart.y = segmentEnd.y + dashLength;
        segmentEnd.y = (segmentStart.y + dashLength > end.y) ? end.y : segmentStart.y + dashLength;
        SDL_RenderLine(pRenderer_, static_cast<float>(segmentStart.x), static_cast<float>(segmentStart.y),
                       static_cast<float>(segmentEnd.x), static_cast<float>(segmentEnd.y));
        distanceDone += dashLength + (segmentEnd.y - segmentStart.y);
        i++;
    }
}

/*!
 * Draw a horizontal line of the given length which is composed of segments of dashLength and space of same size.
 */
void SystemSDL::drawDashedHLine(Point2D start, int length, int dashLength, int dashOffset, rs_eng::RSColor color) {
    dashOffset %= 2 * dashLength;
    Point2D end = start.add(length, 0);

    SDL_SetRenderDrawColor(pRenderer_, color.r, color.g, color.b, color.a);
   
    Point2D segmentStart = start;
    Point2D segmentEnd = start;

    if (dashOffset <= dashLength) {
        segmentEnd.x += dashOffset;
    } else {
        segmentStart.x += (dashOffset - dashLength);
        segmentEnd.x = (segmentStart.x + dashLength);
    }
    SDL_RenderLine(pRenderer_, static_cast<float>(segmentStart.x), static_cast<float>(segmentStart.y),
                   static_cast<float>(segmentEnd.x), static_cast<float>(segmentEnd.y));
    int distanceDone = dashOffset;

    int i = 0;
    while (distanceDone < length && i < 100) {
        if (segmentEnd.x + dashLength > end.x) {
            break;
        }
        segmentStart.x = segmentEnd.x + dashLength;
        segmentEnd.x = (segmentStart.x + dashLength > end.x) ? end.x : segmentStart.x + dashLength;
        SDL_RenderLine(pRenderer_, static_cast<float>(segmentStart.x), static_cast<float>(segmentStart.y),
                       static_cast<float>(segmentEnd.x), static_cast<float>(segmentEnd.y));
        distanceDone += dashLength + (segmentEnd.x - segmentStart.x);
        i++;
    }
}

/*!
 * Draw a line from start to end points with given color.
 */
void SystemSDL::drawLine(Point2D start, Point2D end, rs_eng::RSColor color) {
    SDL_SetRenderDrawColor(pRenderer_, color.r, color.g, color.b, color.a);
    SDL_RenderLine(pRenderer_, static_cast<float>(start.x), static_cast<float>(start.y),
                   static_cast<float>(end.x), static_cast<float>(end.y));
}

/*!
 * Draw a rect with given color
 */
void SystemSDL::drawRect(Point2D pos, int width, int height, rs_eng::RSColor color) {
    SDL_FRect outlineRect = { static_cast<float>(pos.x), static_cast<float>(pos.y),
                              static_cast<float>(width), static_cast<float>(height) };
    SDL_SetRenderDrawColor(pRenderer_, color.r, color.g, color.b, color.a);
    SDL_RenderRect(pRenderer_, &outlineRect);
}

/*!
 * Draw a rect filled with given color
 */
void SystemSDL::drawFillRect(Point2D pos, int width, int height, rs_eng::RSColor color) {
    SDL_FRect outlineRect = { static_cast<float>(pos.x), static_cast<float>(pos.y),
                              static_cast<float>(width), static_cast<float>(height) };
    const bool useBlend = (color.a > 0 && color.a < 255);
    if (useBlend) {
        SDL_SetRenderDrawBlendMode(pRenderer_, SDL_BLENDMODE_BLEND);
    }
    SDL_SetRenderDrawColor(pRenderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(pRenderer_, &outlineRect);
    if (useBlend) {
        SDL_SetRenderDrawBlendMode(pRenderer_, SDL_BLENDMODE_NONE);
    }
}

bool SystemSDL::loadCursorSprites() {
    LOG(Log::k_FLG_GFX, "SystemSDL", "loadCursorSprites", ("loading cursor sprites from file cursors/cursors.png"))
    cursor_rect_.w = cursor_rect_.h = kCursorWidth;

    SDL_Surface* pLoadedSurface = IMG_Load(rs_utl::File::getResyndDataFullPath("cursors/cursors.png").c_str());

    if (!pLoadedSurface) {
        RSERR(Log::k_FLG_GFX, "SystemSDL", "loadCursorSprites", ("Cannot load cursors image : %s", SDL_GetError()))
        return false;
    }

    pCursorTexture_ = SDL_CreateTextureFromSurface(pRenderer_, pLoadedSurface);
    SDL_DestroySurface(pLoadedSurface);

    if (pCursorTexture_ == nullptr) {
        RSERR(Log::k_FLG_GFX, "SystemSDL", "loadCursorSprites", ("Cannot create cursor texture : %s", SDL_GetError()))
        return false;
    }
    SDL_SetTextureScaleMode(pCursorTexture_, SDL_SCALEMODE_NEAREST);

    return true;
}

/*!
 * Returns the mouse pointer coordinates using SDL_GetMouseState.
 * \param point The mouse coordinate.
 * \return See SDL_GetMouseState.
 */
uint32_t SystemSDL::getMousePos(Point2D &point) {
    float wx = 0.0f, wy = 0.0f;
    SDL_MouseButtonFlags state = SDL_GetMouseState(&wx, &wy);
    if (pRenderer_) {
        float lx = wx;
        float ly = wy;
        SDL_RenderCoordinatesFromWindow(pRenderer_, wx, wy, &lx, &ly);
        point.x = static_cast<int>(lx);
        point.y = static_cast<int>(ly);
    } else {
        point.x = static_cast<int>(wx);
        point.y = static_cast<int>(wy);
    }
    return static_cast<uint32_t>(state);
}

void SystemSDL::hideCursor() {
    if (pCursorTexture_ != nullptr) {
        cursor_visible_ = false;
    } else {
        SDL_HideCursor();
    }
}

void SystemSDL::showCursor() {
    if (pCursorTexture_ != nullptr) {
        cursor_visible_ = true;
    } else {
        SDL_ShowCursor();
    }
}

void SystemSDL::useMenuCursor() {
    update_cursor_ = true;
    cursor_rect_.x = cursor_rect_.y = 0;
    cursor_hs_x_ = cursor_hs_y_ = 0;
}

void SystemSDL::usePointerCursor() {
    update_cursor_ = true;
    cursor_rect_.x = 24;
    cursor_rect_.y = 0;
    cursor_hs_x_ = 1;
    cursor_hs_y_ = 1;
}

void SystemSDL::usePointerYellowCursor() {
    update_cursor_ = true;
    cursor_rect_.x = 24;
    cursor_rect_.y = 24;
    cursor_hs_x_ = 1;
    cursor_hs_y_ = 1;
}

void SystemSDL::useTargetCursor() {
    update_cursor_ = true;
    cursor_rect_.x = 48;
    cursor_rect_.y = 0;
    cursor_hs_x_ = cursor_hs_y_ = 10;
}

void SystemSDL::useTargetRedCursor() {
    update_cursor_ = true;
    cursor_rect_.x = 72;
    cursor_rect_.y = 0;
    cursor_hs_x_ = cursor_hs_y_ = 10;
}

void SystemSDL::usePickupCursor() {
    update_cursor_ = true;
    cursor_rect_.x = 0;
    cursor_rect_.y = 24;
    cursor_hs_x_ = 4;
    cursor_hs_y_ = 1;
}

/*!
 * Calls SDL_StartTextInput() to tell SDL3 to send SDL_EVENT_TEXT_INPUT events.
 * This method is called when a textfield widget receives focus.
 */
void SystemSDL::startReceiveText() {
    if (pWindow_) SDL_StartTextInput(pWindow_);
}

/*!
 * Calls SDL_StopTextInput() to stop receiving text input events.
 */
void SystemSDL::stopReceiveText() {
    if (pWindow_) SDL_StopTextInput(pWindow_);
}

std::unique_ptr<RSTexture> SystemSDL::createTexture() {
    return std::make_unique<RSTextureSDL>(pRenderer_);
}

/*!
 * Call SDL_GetPreferredLocales() to get the locales for the system.
 * If there is an error, return English silently.
 */
rs_eng::RS_Lang SystemSDL::getLanguageFromSystem() {
    rs_eng::RS_Lang resLang = rs_eng::ENGLISH;
    int count = 0;
    SDL_Locale **locales = SDL_GetPreferredLocales(&count);
    if (locales != nullptr) {
        for (int i = 0; i < count; ++i) {
            if (locales[i] && locales[i]->language) {
                std::string lang(locales[i]->language);
                if (lang == "en") {
                    resLang = rs_eng::ENGLISH;
                    break;
                } else if (lang == "fr") {
                    resLang = rs_eng::FRENCH;
                    break;
                } else if (lang == "it") {
                    resLang = rs_eng::ITALIAN;
                    break;
                } else if (lang == "de") {
                    resLang = rs_eng::GERMAN;
                    break;
                }
            }
        }
        SDL_free(locales);
    }

    return resLang;
}

namespace {

constexpr auto makeScancodeTable() {
    std::array<SDL_Scancode, 128> table{};
    table.fill(SDL_SCANCODE_UNKNOWN);

    table[kKeyCode_Escape] = SDL_SCANCODE_ESCAPE;
    table[kKeyCode_Backspace] = SDL_SCANCODE_BACKSPACE;
    table[kKeyCode_Space] = SDL_SCANCODE_SPACE;
    table[kKeyCode_Return] = SDL_SCANCODE_RETURN;
    table[kKeyCode_Delete] = SDL_SCANCODE_DELETE;
    table[kKeyCode_Up] = SDL_SCANCODE_UP;
    table[kKeyCode_Down] = SDL_SCANCODE_DOWN;
    table[kKeyCode_Right] = SDL_SCANCODE_RIGHT;
    table[kKeyCode_Left] = SDL_SCANCODE_LEFT;
    table[kKeyCode_Insert] = SDL_SCANCODE_INSERT;
    table[kKeyCode_Home] = SDL_SCANCODE_HOME;
    table[kKeyCode_End] = SDL_SCANCODE_END;
    table[kKeyCode_PageUp] = SDL_SCANCODE_PAGEUP;
    table[kKeyCode_PageDown] = SDL_SCANCODE_PAGEDOWN;

    for (int i = 0; i < 12; ++i) {
        table[kKeyCode_F1 + i] = static_cast<SDL_Scancode>(SDL_SCANCODE_F1 + i);
    }
    for (int i = 0; i < 26; ++i) {
        table[kKeyCode_A + i] = static_cast<SDL_Scancode>(SDL_SCANCODE_A + i);
    }
    for (int i = 0; i <= 4; ++i) {
        table[kKeyCode_0 + i] = static_cast<SDL_Scancode>(SDL_SCANCODE_0 + i);
    }
    table[kKeyCode_Plus] = SDL_SCANCODE_KP_PLUS;
    table[kKeyCode_Minus] = SDL_SCANCODE_KP_MINUS;

    return table;
}

constexpr auto kScancodeLookup = makeScancodeTable();

} // namespace

bool SystemSDL::isKeyPressed(const RS_KeyCode keyCode) {
    if (keyCode < 0 || static_cast<size_t>(keyCode) >= kScancodeLookup.size()) {
        return false;
    }
    SDL_Scancode scancode = kScancodeLookup[keyCode];
    if (scancode == SDL_SCANCODE_UNKNOWN) {
        return false;
    }

    const bool *state = SDL_GetKeyboardState(NULL);
    if (!state) {
        return false;
    }
    if (keyCode == kKeyCode_Plus) {
        return state[SDL_SCANCODE_KP_PLUS] || state[SDL_SCANCODE_EQUALS];
    }
    if (keyCode == kKeyCode_Minus) {
        return state[SDL_SCANCODE_KP_MINUS] || state[SDL_SCANCODE_MINUS];
    }

    return state[scancode];
}

}
