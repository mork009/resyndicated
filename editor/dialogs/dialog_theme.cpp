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

#include "dialog_theme.h"
#include "resource.h"
#include "rs-utils/version.h"
#include <windowsx.h>
#include <uxtheme.h>
#include <vector>
#include <format>
#include <algorithm>

namespace rs_win_editor {

void enableDarkModeForWindow(HWND hWnd) {
    if (!hWnd) return;
    BOOL darkMode = TRUE;
    if (FAILED(DwmSetWindowAttribute(hWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode)))) {
        DwmSetWindowAttribute(hWnd, 19, &darkMode, sizeof(darkMode));
    }
    static auto pfnAllowDarkModeForWindow = []() {
        HMODULE hUxTheme = GetModuleHandleW(L"uxtheme.dll");
        if (!hUxTheme) hUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        return hUxTheme ? reinterpret_cast<BOOL(WINAPI*)(HWND, BOOL)>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(133))) : nullptr;
    }();
    if (pfnAllowDarkModeForWindow) {
        pfnAllowDarkModeForWindow(hWnd, TRUE);
    }
}

void enableDarkModeForControl(HWND hWnd) {
    if (!hWnd) return;
    static auto pfnAllowDarkModeForWindow = []() {
        HMODULE hUxTheme = GetModuleHandleW(L"uxtheme.dll");
        if (!hUxTheme) hUxTheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        return hUxTheme ? reinterpret_cast<BOOL(WINAPI*)(HWND, BOOL)>(GetProcAddress(hUxTheme, MAKEINTRESOURCEA(133))) : nullptr;
    }();
    if (pfnAllowDarkModeForWindow) {
        pfnAllowDarkModeForWindow(hWnd, TRUE);
    }
    wchar_t cls[64] = {};
    GetClassNameW(hWnd, cls, 64);
    if (_wcsicmp(cls, L"ListBox") == 0 || _wcsicmp(cls, L"ComboLBox") == 0) {
        SetWindowTheme(hWnd, L"", L"");
    } else {
        SetWindowTheme(hWnd, L"DarkMode_Explorer", nullptr);
    }
}

// -----------------------------------------------------------------------------
// Subclass: Custom Dark Push Buttons
// -----------------------------------------------------------------------------
struct ThemedButtonData {
    bool isHovered = false;
};

static LRESULT CALLBACK ThemedButtonSubclass(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    ThemedButtonData *pData = reinterpret_cast<ThemedButtonData*>(dwRefData);
    if (!pData && uMsg != WM_NCDESTROY) {
        pData = new ThemedButtonData();
        SetWindowSubclass(hWnd, ThemedButtonSubclass, uIdSubclass, reinterpret_cast<DWORD_PTR>(pData));
    }

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            if (pData && !pData->isHovered) {
                pData->isHovered = true;
                TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        }

        case WM_MOUSELEAVE: {
            if (pData && pData->isHovered) {
                pData->isHovered = false;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        }

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
            LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
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
            LONG_PTR style = GetWindowLongPtrW(hWnd, GWL_STYLE);
            bool isDefault = ((style & 0x0F) == BS_DEFPUSHBUTTON) || (GetDlgCtrlID(hWnd) == IDOK);
            LRESULT state = SendMessageW(hWnd, BM_GETSTATE, 0, 0);
            bool isPressed = (state & BST_PUSHED) != 0;
            bool isFocused = (GetFocus() == hWnd);

            wchar_t text[128] = {};
            GetWindowTextW(hWnd, text, 128);

            bool isAgentActive = (text[0] == L'[');
            bool isDanger = (wcsstr(text, L"Delete") != nullptr || wcsstr(text, L"Clear") != nullptr);

            COLORREF bgCol, borderCol, textCol;
            if (!isEnabled) {
                bgCol = RGB(30, 32, 36);
                borderCol = RGB(46, 50, 56);
                textCol = RGB(110, 118, 128);
            } else if (isPressed) {
                bgCol = RGB(22, 25, 29);
                borderCol = isDanger ? kColorAccentRed : kColorAccentCyan;
                textCol = RGB(255, 255, 255);
            } else if (pData && pData->isHovered) {
                bgCol = isDanger ? RGB(56, 32, 36) : RGB(46, 54, 64);
                borderCol = isDanger ? RGB(245, 80, 90) : (isDefault ? kColorAccentCyan : kColorBorderHighlight);
                textCol = RGB(255, 255, 255);
            } else if (isAgentActive) {
                bgCol = RGB(26, 48, 62);
                borderCol = kColorAccentCyan;
                textCol = RGB(255, 255, 255);
            } else {
                bgCol = isDanger ? RGB(44, 28, 30) : RGB(36, 40, 46);
                borderCol = isDanger ? RGB(140, 50, 55) : (isDefault ? RGB(0, 160, 200) : kColorBorder);
                textCol = kColorText;
            }

            // Fill button background
            HBRUSH hbrBg = CreateSolidBrush(bgCol);
            FillRect(memDC, &rc, hbrBg);
            DeleteObject(hbrBg);

            // Draw border
            HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
            HPEN oldPen = (HPEN)SelectObject(memDC, hPen);
            HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
            Rectangle(memDC, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(memDC, oldPen);
            SelectObject(memDC, oldBrush);
            DeleteObject(hPen);

            // Subtle inner focus rectangle (avoid XOR white inversion)
            if (isFocused && isEnabled && !isAgentActive && !isDefault) {
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

            // Text
            HFONT hFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
            HFONT oldFont = nullptr;
            if (hFont) oldFont = (HFONT)SelectObject(memDC, hFont);

            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, textCol);

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

        case WM_NCDESTROY: {
            if (pData) {
                delete pData;
            }
            RemoveWindowSubclass(hWnd, ThemedButtonSubclass, uIdSubclass);
            break;
        }
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// -----------------------------------------------------------------------------
// Subclass: Custom Dark Group Boxes
// -----------------------------------------------------------------------------
static void PaintThemedGroupBox(HWND hWnd, HDC hdc) {
    RECT rc;
    GetClientRect(hWnd, &rc);
    if (rc.right <= 0 || rc.bottom <= 0) return;

    wchar_t text[256] = {};
    GetWindowTextW(hWnd, text, 256);

    HFONT hFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
    HFONT oldFont = nullptr;
    if (hFont) oldFont = (HFONT)SelectObject(hdc, hFont);

    int textLen = static_cast<int>(wcslen(text));
    SIZE sz = { 0, 16 };
    if (textLen > 0) {
        GetTextExtentPoint32W(hdc, text, textLen, &sz);
    }

    int topY = sz.cy / 2;

    // Erase only the top caption line area (so old text/lines are cleared cleanly without touching the interior controls)
    RECT rcTop = { 0, 0, rc.right, sz.cy + 2 };
    HBRUSH hbrBg = CreateSolidBrush(kColorDlgBg);
    FillRect(hdc, &rcTop, hbrBg);

    HPEN hPen = CreatePen(PS_SOLID, 1, kColorBorder);
    HPEN oldPen = (HPEN)SelectObject(hdc, hPen);

    if (textLen > 0) {
        MoveToEx(hdc, 0, topY, nullptr);
        LineTo(hdc, 10, topY);

        MoveToEx(hdc, 10 + sz.cx + 8, topY, nullptr);
        LineTo(hdc, rc.right - 1, topY);
    } else {
        MoveToEx(hdc, 0, topY, nullptr);
        LineTo(hdc, rc.right - 1, topY);
    }

    LineTo(hdc, rc.right - 1, rc.bottom - 1);
    LineTo(hdc, 0, rc.bottom - 1);
    LineTo(hdc, 0, topY);

    SelectObject(hdc, oldPen);
    DeleteObject(hPen);

    if (textLen > 0) {
        RECT rcText = { 14, 0, 14 + sz.cx, sz.cy };
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, kColorAccentCyan);
        DrawTextW(hdc, text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }

    DeleteObject(hbrBg);
    if (oldFont) SelectObject(hdc, oldFont);
}

static LRESULT CALLBACK ThemedGroupBoxSubclass(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
    switch (uMsg) {
        case WM_SETTEXT: {
            DefWindowProcW(hWnd, WM_SETTEXT, wParam, lParam);
            InvalidateRect(hWnd, nullptr, FALSE);
            UpdateWindow(hWnd);
            return TRUE;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            PaintThemedGroupBox(hWnd, hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_PRINTCLIENT: {
            PaintThemedGroupBox(hWnd, reinterpret_cast<HDC>(wParam));
            return 0;
        }

        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, ThemedGroupBoxSubclass, uIdSubclass);
            break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// -----------------------------------------------------------------------------
// Subclass: Custom Dark Checkboxes
// -----------------------------------------------------------------------------
static LRESULT CALLBACK ThemedCheckBoxSubclass(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
    switch (uMsg) {
        case WM_SETTEXT: {
            DefWindowProcW(hWnd, WM_SETTEXT, wParam, lParam);
            InvalidateRect(hWnd, nullptr, FALSE);
            UpdateWindow(hWnd);
            return TRUE;
        }

        case BM_SETCHECK: {
            LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
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

            HBRUSH hbrBg = CreateSolidBrush(kColorDlgBg);
            FillRect(memDC, &rc, hbrBg);
            DeleteObject(hbrBg);

            bool isChecked = (Button_GetCheck(hWnd) == BST_CHECKED);
            bool isEnabled = (IsWindowEnabled(hWnd) != FALSE);

            int boxY = rc.top + ((rc.bottom - rc.top) - 14) / 2;
            RECT rcBox = { 2, boxY, 16, boxY + 14 };

            HBRUSH hbrBox = CreateSolidBrush(kColorControlBg);
            FillRect(memDC, &rcBox, hbrBox);
            DeleteObject(hbrBox);

            HPEN hPen = CreatePen(PS_SOLID, 1, isChecked ? kColorAccentCyan : kColorBorder);
            HPEN oldPen = (HPEN)SelectObject(memDC, hPen);
            HBRUSH oldBrush = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
            Rectangle(memDC, rcBox.left, rcBox.top, rcBox.right, rcBox.bottom);
            SelectObject(memDC, oldPen);
            SelectObject(memDC, oldBrush);
            DeleteObject(hPen);

            if (isChecked) {
                HPEN hCheckPen = CreatePen(PS_SOLID, 2, kColorAccentCyan);
                HPEN oldCPen = (HPEN)SelectObject(memDC, hCheckPen);
                MoveToEx(memDC, rcBox.left + 3, rcBox.top + 7, nullptr);
                LineTo(memDC, rcBox.left + 6, rcBox.top + 10);
                LineTo(memDC, rcBox.left + 11, rcBox.top + 3);
                SelectObject(memDC, oldCPen);
                DeleteObject(hCheckPen);
            }

            wchar_t text[128] = {};
            GetWindowTextW(hWnd, text, 128);

            HFONT hFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
            HFONT oldFont = nullptr;
            if (hFont) oldFont = (HFONT)SelectObject(memDC, hFont);

            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, isEnabled ? kColorText : kColorTextMuted);

            RECT rcText = rc;
            rcText.left = rcBox.right + 8;
            DrawTextW(memDC, text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            if (oldFont) SelectObject(memDC, oldFont);

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBm);
            DeleteObject(hbm);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, ThemedCheckBoxSubclass, uIdSubclass);
            break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// -----------------------------------------------------------------------------
// Subclass: 1px Dark Border for Edit and ListBox Controls
// -----------------------------------------------------------------------------
static LRESULT CALLBACK ThemedControlBorderSubclass(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/)
{
    switch (uMsg) {
        case WM_NCPAINT: {
            LRESULT res = DefSubclassProc(hWnd, uMsg, wParam, lParam);
            HDC hdc = GetWindowDC(hWnd);
            if (hdc) {
                RECT rc;
                GetWindowRect(hWnd, &rc);
                OffsetRect(&rc, -rc.left, -rc.top);
                HBRUSH hbr = CreateSolidBrush(kColorBorder);
                FrameRect(hdc, &rc, hbr);
                // Also frame inner 1px to cover any 2px 3D bevels from WS_EX_CLIENTEDGE
                InflateRect(&rc, -1, -1);
                HBRUSH hbrInner = CreateSolidBrush(kColorControlBg);
                FrameRect(hdc, &rc, hbrInner);
                DeleteObject(hbrInner);
                DeleteObject(hbr);
                ReleaseDC(hWnd, hdc);
            }
            return res;
        }

        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, ThemedControlBorderSubclass, uIdSubclass);
            break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// -----------------------------------------------------------------------------
// Subclass: Custom Dark ComboBox
// -----------------------------------------------------------------------------
struct ThemedComboData {
    bool isHovered = false;
};

static LRESULT CALLBACK ThemedComboBoxSubclass(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    ThemedComboData *pData = reinterpret_cast<ThemedComboData*>(dwRefData);
    if (!pData && uMsg != WM_NCDESTROY) {
        pData = new ThemedComboData();
        SetWindowSubclass(hWnd, ThemedComboBoxSubclass, uIdSubclass, reinterpret_cast<DWORD_PTR>(pData));
    }

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            if (pData && !pData->isHovered) {
                pData->isHovered = true;
                TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, 0 };
                TrackMouseEvent(&tme);
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        }

        case WM_MOUSELEAVE: {
            if (pData && pData->isHovered) {
                pData->isHovered = false;
                InvalidateRect(hWnd, nullptr, FALSE);
            }
            break;
        }

        case WM_SETFOCUS:
        case WM_KILLFOCUS:
        case WM_ENABLE:
            InvalidateRect(hWnd, nullptr, FALSE);
            break;

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            if (rc.right <= 0 || rc.bottom <= 0) {
                EndPaint(hWnd, &ps);
                return 0;
            }

            COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
            GetComboBoxInfo(hWnd, &cbi);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP hbm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HBITMAP oldBm = static_cast<HBITMAP>(SelectObject(memDC, hbm));

            bool isEnabled = IsWindowEnabled(hWnd) != FALSE;
            bool isFocused = (GetFocus() == hWnd);
            bool isHovered = pData && pData->isHovered;
            bool isDropped = (SendMessageW(hWnd, CB_GETDROPPEDSTATE, 0, 0) != 0);

            // Dark Background
            HBRUSH hbrBg = CreateSolidBrush(kColorControlBg);
            FillRect(memDC, &rc, hbrBg);
            DeleteObject(hbrBg);

            // Dropdown button rect
            RECT rcBtn = cbi.rcButton;
            if (rcBtn.right <= rcBtn.left) {
                rcBtn = { rc.right - 22, 0, rc.right, rc.bottom };
            }

            // Dropdown button background
            COLORREF btnBgCol = (isHovered || isDropped) ? RGB(36, 42, 50) : RGB(24, 28, 34);
            HBRUSH hbrBtn = CreateSolidBrush(btnBgCol);
            FillRect(memDC, &rcBtn, hbrBtn);
            DeleteObject(hbrBtn);

            // Subtle vertical separator line between item area and dropdown button
            HPEN hSepPen = CreatePen(PS_SOLID, 1, RGB(44, 50, 58));
            HPEN hOldPen = static_cast<HPEN>(SelectObject(memDC, hSepPen));
            MoveToEx(memDC, rcBtn.left, rc.top + 2, nullptr);
            LineTo(memDC, rcBtn.left, rc.bottom - 2);
            SelectObject(memDC, hOldPen);
            DeleteObject(hSepPen);

            // Downward Chevron Arrow
            int arrowCx = (rcBtn.left + rcBtn.right) / 2;
            int arrowCy = (rcBtn.top + rcBtn.bottom) / 2;
            POINT pts[3] = {
                { arrowCx - 4, arrowCy - 2 },
                { arrowCx + 4, arrowCy - 2 },
                { arrowCx,     arrowCy + 3 }
            };
            COLORREF arrowCol = !isEnabled ? kColorTextMuted : ((isHovered || isDropped || isFocused) ? RGB(0, 220, 245) : RGB(175, 185, 195));
            HBRUSH hArrowBr = CreateSolidBrush(arrowCol);
            HBRUSH hOldBr = static_cast<HBRUSH>(SelectObject(memDC, hArrowBr));
            HPEN hOldP = static_cast<HPEN>(SelectObject(memDC, GetStockObject(NULL_PEN)));
            Polygon(memDC, pts, 3);
            SelectObject(memDC, hOldP);
            SelectObject(memDC, hOldBr);
            DeleteObject(hArrowBr);

            // Selected text
            RECT rcText = cbi.rcItem;
            if (rcText.right <= rcText.left) {
                rcText = { rc.left + 6, rc.top, rcBtn.left - 4, rc.bottom };
            } else {
                rcText.left += 4;
                rcText.right -= 2;
            }

            wchar_t text[256] = {};
            int curSel = ComboBox_GetCurSel(hWnd);
            if (curSel != CB_ERR) {
                ComboBox_GetLBText(hWnd, curSel, text);
            } else {
                GetWindowTextW(hWnd, text, 256);
            }

            HFONT hFont = reinterpret_cast<HFONT>(SendMessageW(hWnd, WM_GETFONT, 0, 0));
            if (!hFont) hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
            HFONT oldFont = static_cast<HFONT>(SelectObject(memDC, hFont));

            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, isEnabled ? kColorText : kColorTextMuted);
            DrawTextW(memDC, text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(memDC, oldFont);

            // Outer 1px border
            COLORREF borderCol = !isEnabled ? RGB(45, 50, 56) : (isFocused || isDropped ? RGB(0, 220, 245) : (isHovered ? RGB(85, 96, 110) : kColorBorder));
            HPEN hBorderPen = CreatePen(PS_SOLID, 1, borderCol);
            hOldPen = static_cast<HPEN>(SelectObject(memDC, hBorderPen));
            hOldBr = static_cast<HBRUSH>(SelectObject(memDC, GetStockObject(NULL_BRUSH)));
            Rectangle(memDC, rc.left, rc.top, rc.right, rc.bottom);
            SelectObject(memDC, hOldBr);
            SelectObject(memDC, hOldPen);
            DeleteObject(hBorderPen);

            BitBlt(hdc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBm);
            DeleteObject(hbm);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_NCDESTROY:
            if (pData) {
                delete pData;
            }
            RemoveWindowSubclass(hWnd, ThemedComboBoxSubclass, uIdSubclass);
            break;
    }

    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void applyComboBoxTheme(HWND hCombo) {
    if (!hCombo) return;
    enableDarkModeForControl(hCombo);
    SetWindowSubclass(hCombo, ThemedComboBoxSubclass, 107, 0);
    COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
    if (GetComboBoxInfo(hCombo, &cbi)) {
        if (cbi.hwndList) {
            enableDarkModeForControl(cbi.hwndList);
            SetWindowSubclass(cbi.hwndList, ThemedControlBorderSubclass, 108, 0);
        }
        if (cbi.hwndItem) {
            enableDarkModeForControl(cbi.hwndItem);
            SetWindowSubclass(cbi.hwndItem, ThemedControlBorderSubclass, 109, 0);
        }
    }
}

void applyCheckBoxTheme(HWND hBtn) {
    if (!hBtn) return;
    SetWindowSubclass(hBtn, ThemedCheckBoxSubclass, 102, 0);
}

// -----------------------------------------------------------------------------
// Apply Dark Theme to Dialog Controls
// -----------------------------------------------------------------------------
void applyDialogTheme(HWND hDlg) {
    if (!hDlg) return;
    enableDarkModeForWindow(hDlg);

    EnumChildWindows(hDlg, [](HWND hChild, LPARAM) -> BOOL {
        wchar_t cls[64] = {};
        GetClassNameW(hChild, cls, 64);

        if (_wcsicmp(cls, L"Button") == 0) {
            LONG_PTR style = GetWindowLongPtrW(hChild, GWL_STYLE);
            int btnType = static_cast<int>(style & 0x0F);
            if (btnType == BS_GROUPBOX) {
                SetWindowSubclass(hChild, ThemedGroupBoxSubclass, 101, 0);
            } else if (btnType == BS_CHECKBOX || btnType == BS_AUTOCHECKBOX) {
                SetWindowSubclass(hChild, ThemedCheckBoxSubclass, 102, 0);
            } else if (btnType == BS_OWNERDRAW) {
                // Owner-draw buttons handle their own custom rendering via WM_DRAWITEM in the parent dialog
            } else {
                SetWindowSubclass(hChild, ThemedButtonSubclass, 103, 0);
            }
        } else if (_wcsicmp(cls, L"Edit") == 0) {
            enableDarkModeForControl(hChild);
            SetWindowSubclass(hChild, ThemedControlBorderSubclass, 104, 0);
        } else if (_wcsicmp(cls, L"ListBox") == 0) {
            enableDarkModeForControl(hChild);
            SetWindowSubclass(hChild, ThemedControlBorderSubclass, 105, 0);
        } else if (_wcsicmp(cls, L"ComboBox") == 0) {
            applyComboBoxTheme(hChild);
        }

        return TRUE;
    }, 0);
}

// -----------------------------------------------------------------------------
// Handle WM_CTLCOLOR... Messages
// -----------------------------------------------------------------------------
LRESULT handleDialogColorMessages(HWND /*hWnd*/, UINT msg, WPARAM wParam, LPARAM lParam, bool &handled) {
    static HBRUSH s_hbrDlg = CreateSolidBrush(kColorDlgBg);
    static HBRUSH s_hbrControl = CreateSolidBrush(kColorControlBg);

    switch (msg) {
        case WM_CTLCOLORDLG:
            handled = true;
            return reinterpret_cast<LRESULT>(s_hbrDlg);

        case WM_CTLCOLORSTATIC: {
            handled = true;
            HDC hdc = reinterpret_cast<HDC>(wParam);
            HWND hChild = reinterpret_cast<HWND>(lParam);

            wchar_t clsName[32] = {};
            GetClassNameW(hChild, clsName, 32);
            if (_wcsicmp(clsName, L"Edit") == 0) {
                SetTextColor(hdc, kColorText);
                SetBkColor(hdc, kColorControlBg);
                SetBkMode(hdc, OPAQUE);
                return reinterpret_cast<LRESULT>(s_hbrControl);
            }

            SetTextColor(hdc, kColorText);
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(s_hbrDlg);
        }

        case WM_CTLCOLOREDIT: {
            handled = true;
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(245, 248, 252));
            SetBkColor(hdc, kColorControlBg);
            SetBkMode(hdc, OPAQUE);
            return reinterpret_cast<LRESULT>(s_hbrControl);
        }

        case WM_CTLCOLORLISTBOX: {
            handled = true;
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(235, 240, 245));
            SetBkColor(hdc, kColorControlBg);
            SetBkMode(hdc, OPAQUE);
            return reinterpret_cast<LRESULT>(s_hbrControl);
        }

        case WM_CTLCOLORBTN:
            handled = true;
            return reinterpret_cast<LRESULT>(s_hbrDlg);
    }

    handled = false;
    return 0;
}

// -----------------------------------------------------------------------------
// Custom Dark Modal Message Box
// -----------------------------------------------------------------------------
struct MsgBoxState {
    std::wstring text;
    std::wstring caption;
    UINT uType;
    int result = IDCANCEL;
    HFONT hFont = nullptr;
    HFONT hFontBold = nullptr;
};

static LRESULT CALLBACK MsgBoxProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MsgBoxState *pState = reinterpret_cast<MsgBoxState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<MsgBoxState*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_CREATE: {
            enableDarkModeForWindow(hWnd);

            pState->hFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            pState->hFontBold = CreateFontW(-14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            RECT rcClient;
            GetClientRect(hWnd, &rcClient);

            // Determine buttons
            UINT typeMask = (pState->uType & MB_TYPEMASK);
            int btnW = 90;
            int btnH = 28;
            int btnY = rcClient.bottom - btnH - 14;

            if (typeMask == MB_YESNOCANCEL) {
                int startX = rcClient.right - (btnW * 3 + 24) - 16;
                HWND hBtnYes = CreateWindowExW(0, L"BUTTON", L"Yes", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    startX, btnY, btnW, btnH, hWnd, reinterpret_cast<HMENU>(IDYES), nullptr, nullptr);
                HWND hBtnNo = CreateWindowExW(0, L"BUTTON", L"No", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                    startX + btnW + 12, btnY, btnW, btnH, hWnd, reinterpret_cast<HMENU>(IDNO), nullptr, nullptr);
                HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                    startX + (btnW + 12) * 2, btnY, btnW, btnH, hWnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
                SendMessageW(hBtnYes, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
                SendMessageW(hBtnNo, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
                SendMessageW(hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            } else if (typeMask == MB_YESNO) {
                int startX = rcClient.right - (btnW * 2 + 12) - 16;
                HWND hBtnYes = CreateWindowExW(0, L"BUTTON", L"Yes", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    startX, btnY, btnW, btnH, hWnd, reinterpret_cast<HMENU>(IDYES), nullptr, nullptr);
                HWND hBtnNo = CreateWindowExW(0, L"BUTTON", L"No", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                    startX + btnW + 12, btnY, btnW, btnH, hWnd, reinterpret_cast<HMENU>(IDNO), nullptr, nullptr);
                SendMessageW(hBtnYes, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
                SendMessageW(hBtnNo, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            } else if (typeMask == MB_OKCANCEL) {
                int startX = rcClient.right - (btnW * 2 + 12) - 16;
                HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    startX, btnY, btnW, btnH, hWnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
                HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                    startX + btnW + 12, btnY, btnW, btnH, hWnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
                SendMessageW(hBtnOk, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
                SendMessageW(hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            } else { // MB_OK default
                int startX = rcClient.right - btnW - 16;
                HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                    startX, btnY, btnW, btnH, hWnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
                SendMessageW(hBtnOk, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            }

            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            HBRUSH hbrBg = CreateSolidBrush(kColorDlgBg);
            FillRect(hdc, &rc, hbrBg);
            DeleteObject(hbrBg);

            // Draw Icon Badge on Left
            UINT iconMask = (pState->uType & MB_ICONMASK);
            COLORREF badgeColor = kColorAccentCyan;
            const wchar_t *badgeSymbol = L"i";

            if (iconMask == MB_ICONERROR) {
                badgeColor = kColorAccentRed;
                badgeSymbol = L"X";
            } else if (iconMask == MB_ICONWARNING) {
                badgeColor = kColorAccentAmber;
                badgeSymbol = L"!";
            } else if (iconMask == MB_ICONQUESTION) {
                badgeColor = kColorAccentCyan;
                badgeSymbol = L"?";
            }

            int badgeX = 18;
            int badgeY = 20;
            int badgeR = 34;
            RECT rcBadge = { badgeX, badgeY, badgeX + badgeR, badgeY + badgeR };

            HBRUSH hbrBadge = CreateSolidBrush(kColorControlBg);
            FillRect(hdc, &rcBadge, hbrBadge);
            DeleteObject(hbrBadge);

            HPEN hBadgePen = CreatePen(PS_SOLID, 2, badgeColor);
            HPEN oldPen = (HPEN)SelectObject(hdc, hBadgePen);
            HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, rcBadge.left, rcBadge.top, rcBadge.right, rcBadge.bottom);
            SelectObject(hdc, oldPen);
            SelectObject(hdc, oldBrush);
            DeleteObject(hBadgePen);

            HFONT oldFont = (HFONT)SelectObject(hdc, pState->hFontBold);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, badgeColor);
            DrawTextW(hdc, badgeSymbol, -1, &rcBadge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            // Draw Message Text
            SelectObject(hdc, pState->hFont);
            SetTextColor(hdc, kColorText);
            RECT rcText = { badgeX + badgeR + 18, 20, rc.right - 18, rc.bottom - 56 };
            DrawTextW(hdc, pState->text.c_str(), -1, &rcText, DT_LEFT | DT_TOP | DT_WORDBREAK);

            SelectObject(hdc, oldFont);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            pState->result = id;
            DestroyWindow(hWnd);
            return 0;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                pState->result = ((pState->uType & MB_TYPEMASK) == MB_YESNO) ? IDNO : IDCANCEL;
                DestroyWindow(hWnd);
                return 0;
            }
            if (wParam == VK_RETURN) {
                pState->result = ((pState->uType & MB_TYPEMASK) == MB_YESNO) ? IDYES : IDOK;
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            pState->result = ((pState->uType & MB_TYPEMASK) == MB_YESNO) ? IDNO : IDCANCEL;
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            if (pState->hFont) DeleteObject(pState->hFont);
            if (pState->hFontBold) DeleteObject(pState->hFontBold);
            return 0;
    }

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int showEditorMessageBox(HWND hWndParent, const wchar_t *lpText, const wchar_t *lpCaption, UINT uType) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedMsgBoxClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = MsgBoxProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    MsgBoxState state;
    state.text = lpText ? lpText : L"";
    state.caption = lpCaption ? lpCaption : L"ReSyndicated";
    state.uType = uType;
    state.result = ((uType & MB_TYPEMASK) == MB_YESNO) ? IDNO : IDOK;

    // Measure text to size window nicely
    HDC screenDC = GetDC(nullptr);
    HFONT hFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(screenDC, hFont);

    int maxWorkW = GetSystemMetrics(SM_CXSCREEN);
    if (hWndParent && IsWindow(hWndParent)) {
        HMONITOR hMon = MonitorFromWindow(hWndParent, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfoW(hMon, &mi)) {
            maxWorkW = static_cast<int>(mi.rcWork.right - mi.rcWork.left);
        }
    }
    int maxTextW = std::clamp(maxWorkW - 220, 480, 820);

    RECT rcCalc = { 0, 0, maxTextW, 0 };
    DrawTextW(screenDC, state.text.c_str(), -1, &rcCalc, DT_CALCRECT | DT_WORDBREAK);
    SelectObject(screenDC, oldFont);
    DeleteObject(hFont);
    ReleaseDC(nullptr, screenDC);

    int textWidthCalc = static_cast<int>(rcCalc.right - rcCalc.left);
    int textHeightCalc = static_cast<int>(rcCalc.bottom - rcCalc.top);
    int textW = std::clamp(textWidthCalc, 260, maxTextW);
    int dlgW = textW + 110;
    int dlgH = std::max(170, textHeightCalc + 120);

    RECT rcParent = {};
    if (hWndParent && IsWindow(hWndParent)) {
        GetWindowRect(hWndParent, &rcParent);
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
        state.caption.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dlgX, dlgY, dlgW, dlgH,
        hWndParent, nullptr, hInstance, &state
    );

    if (!hDlg) return state.result;

    if (hWndParent && IsWindow(hWndParent)) {
        EnableWindow(hWndParent, FALSE);
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

    if (hWndParent && IsWindow(hWndParent)) {
        EnableWindow(hWndParent, TRUE);
        SetForegroundWindow(hWndParent);
        SetFocus(hWndParent);
    }

    return state.result;
}

// -----------------------------------------------------------------------------
// Dedicated Dialog: Keyboard & Mouse Shortcuts
// -----------------------------------------------------------------------------
static LRESULT CALLBACK ShortcutsDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HFONT s_hFont = nullptr;
    static HFONT s_hFontBold = nullptr;

    switch (msg) {
        case WM_CREATE: {
            enableDarkModeForWindow(hWnd);
            s_hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            s_hFontBold = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            RECT rc;
            GetClientRect(hWnd, &rc);
            HWND hBtnClose = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                rc.right - 106, rc.bottom - 42, 90, 28, hWnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
            SendMessageW(hBtnClose, WM_SETFONT, reinterpret_cast<WPARAM>(s_hFont), TRUE);

            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            HBRUSH hbrBg = CreateSolidBrush(kColorDlgBg);
            FillRect(hdc, &rc, hbrBg);
            DeleteObject(hbrBg);

            HFONT oldFont = (HFONT)SelectObject(hdc, s_hFontBold);
            SetBkMode(hdc, TRANSPARENT);

            struct ShortcutGroup {
                std::wstring groupTitle;
                std::vector<std::pair<std::wstring, std::wstring>> items;
            };

            std::vector<ShortcutGroup> groups = {
                {
                    L"Mission File && Project",
                    {
                        { L"Ctrl + N", L"New Mission" },
                        { L"Ctrl + O", L"Open Mission" },
                        { L"Ctrl + S", L"Save Mission && Map files" },
                        { L"Ctrl + W", L"Close Mission" },
                        { L"F5", L"Playtest Mission in ReSyndicated" }
                    }
                },
                {
                    L"Map Navigation && Layer View",
                    {
                        { L"Middle / Right Click && Drag", L"Pan map viewport" },
                        { L"Mouse Wheel", L"Zoom in / out" },
                        { L"Home", L"Center view on Player's Agents" },
                        { L"Z", L"Toggle Slice View (Z-Isolation)" },
                        { L"PgUp / PgDn", L"Move Active Layer Up / Down" },
                        { L"Shift + PgUp / PgDn", L"Elevate Selected Entity / Tile Layer" }
                    }
                },
                {
                    L"Editing Tools && Actions",
                    {
                        { L"P", L"Pencil Tool (Paint active tile / entity)" },
                        { L"B", L"Box Fill Tool (Area fill tiles)" },
                        { L"V", L"Select / Move Tool (Tile / entity)" },
                        { L"S", L"Search Tool (Find tile / entity on map)" },
                        { L"Ctrl + Z / Ctrl + Y", L"Undo / Redo action" },
                        { L"Del", L"Delete selected" },
                        { L"Esc", L"Deselect any selected tile / entity" },
                        { L"F1", L"Keyboard && Mouse Shortcuts" }
                    }
                }
            };

            int curY = 26;
            for (const auto &g : groups) {
                SelectObject(hdc, s_hFontBold);
                SetTextColor(hdc, kColorAccentCyan);
                RECT rcGrp = { 20, curY, rc.right - 20, curY + 20 };
                DrawTextW(hdc, g.groupTitle.c_str(), -1, &rcGrp, DT_LEFT | DT_SINGLELINE);
                curY += 22;

                SelectObject(hdc, s_hFont);
                for (const auto &item : g.items) {
                    SetTextColor(hdc, kColorText);
                    RECT rcKey = { 32, curY, 190, curY + 18 };
                    DrawTextW(hdc, item.first.c_str(), -1, &rcKey, DT_LEFT | DT_SINGLELINE);

                    SetTextColor(hdc, kColorTextMuted);
                    RECT rcDesc = { 200, curY, rc.right - 20, curY + 18 };
                    DrawTextW(hdc, item.second.c_str(), -1, &rcDesc, DT_LEFT | DT_SINGLELINE);
                    curY += 19;
                }
                curY += 8;
            }

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
            if (s_hFont) DeleteObject(s_hFont);
            if (s_hFontBold) DeleteObject(s_hFontBold);
            return 0;
    }

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void showShortcutsDialog(HWND hParent) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedShortcutsDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = ShortcutsDlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    int dlgW = 640;
    int dlgH = 570;

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
        L"Keyboard & Mouse Shortcuts",
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

// -----------------------------------------------------------------------------
// Dedicated Dialog: Editor Tools & Features Guide
// -----------------------------------------------------------------------------
static LRESULT CALLBACK ToolsGuideDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HFONT s_hFont = nullptr;
    static HFONT s_hFontBold = nullptr;
    static HFONT s_hFontHeader = nullptr;

    switch (msg) {
        case WM_CREATE: {
            enableDarkModeForWindow(hWnd);
            s_hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            s_hFontBold = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            s_hFontHeader = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            RECT rc;
            GetClientRect(hWnd, &rc);
            HWND hBtnClose = CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                rc.right - 106, rc.bottom - 42, 90, 28, hWnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
            SendMessageW(hBtnClose, WM_SETFONT, reinterpret_cast<WPARAM>(s_hFont), TRUE);

            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            HBRUSH hbrBg = CreateSolidBrush(kColorDlgBg);
            FillRect(hdc, &rc, hbrBg);
            DeleteObject(hbrBg);

            HFONT oldFont = (HFONT)SelectObject(hdc, s_hFontHeader);
            SetBkMode(hdc, TRANSPARENT);

            // Title
            SetTextColor(hdc, kColorAccentCyan);
            RECT rcTitle = { 24, 16, rc.right - 24, 38 };
            DrawTextW(hdc, L"ReSyndicated Editor - How-To", -1, &rcTitle, DT_LEFT | DT_SINGLELINE);

            SelectObject(hdc, s_hFont);
            SetTextColor(hdc, kColorTextMuted);
            RECT rcSub = { 24, 38, rc.right - 24, 56 };
            DrawTextW(hdc, L"Operational guide for isometric tile construction, tactical entity placement, and playtesting", -1, &rcSub, DT_LEFT | DT_SINGLELINE);

            struct ToolSection {
                std::wstring sectionTitle;
                std::vector<std::pair<std::wstring, std::wstring>> tools;
            };

            std::vector<ToolSection> sections = {
                {
                    L"Tiles Construction Tools (Tiles Tab)",
                    {
                        { L"Pencil (P)", L"Left-click to stamp the active palette tile onto active layer Z. Drag to paint contiguous surfaces and structures." },
                        { L"Box Fill (B)", L"Click and drag an isometric bounding box to fill rectangular floor plates, walls, or roads at active layer." },
                        { L"Select Tile (V)", L"Click a tile to inspect its properties. Drag or press Shift+PgUp/PgDn (Shift+[/]) to move the selected tile up/down across layers. Displays depth wiregrid with active layer in yellow." },
                        { L"Search Tile (S)", L"Search for all occurrences of the active palette tile across the map with cyan bounding boxes and interactive result navigation." }
                    }
                },
                {
                    L"Entity Placement && Tactical Configuration (Entities Tab)",
                    {
                        { L"Pencil (P)", L"Click on terrain or structures to spawn the selected actor, vehicle, or static object with current orientation on selected layer." },
                        { L"Select Entity (V)", L"Click an entity to inspect and edit health, weapons, and orientation. Drag on map or press Shift+PgUp/PgDn (Shift+[/]) to raise or lower its Z layer." },
                        { L"Search Entity (S)", L"Search for all occurrences of the selected entity type across the map with yellow bounding boxes and interactive result navigation." }
                    }
                },
                {
                    L"Layer Isolation && Map Visualization",
                    {
                        { L"Layer Nav (PgUp/Dn)", L"Press PgUp/PgDn (or [/]) to step the editor's active layer and cursor between Layer 0 and 12. In contrast, Shift+PgUp/PgDn moves the selected tile or entity between layers." },
                        { L"Wirebox Column", L"In Select mode, hovering over tiles displays a 3D wiregrid column: active layer box is highlighted in yellow, ground/roof surface intersection is highlighted in green, and subterranean layers appear faded." },
                        { L"Slice View (Z)", L"Toggle Z-Isolation to slice off upper floors and view/edit interior building layers at active layer Z without obstructions." },
                        { L"Pan && Zoom", L"Middle-click or Right-click drag to pan viewport smoothly. Mouse wheel zooms map (100%, 150%, 200%)." }
                    }
                },
                {
                    L"Playtest && Engine Verification",
                    {
                        { L"Playtest (F5)", L"Launch mission in ReSyndicated automatically with isolated sandbox data without touching original game files." },
                        { L"Configuration", L"Mission -> Playtest Configuration: configure squad weapons, cybernetic mods, window scale, map zoom, and debug flags." }
                    }
                }
            };

            int curY = 64;
            for (const auto &sec : sections) {
                SelectObject(hdc, s_hFontBold);
                SetTextColor(hdc, kColorAccentCyan);
                RECT rcSec = { 24, curY, rc.right - 24, curY + 20 };
                DrawTextW(hdc, sec.sectionTitle.c_str(), -1, &rcSec, DT_LEFT | DT_SINGLELINE);
                curY += 22;

                SelectObject(hdc, s_hFont);
                for (const auto &item : sec.tools) {
                    SetTextColor(hdc, kColorText);
                    RECT rcKey = { 38, curY, 175, curY + 20 };
                    DrawTextW(hdc, item.first.c_str(), -1, &rcKey, DT_LEFT | DT_SINGLELINE);

                    SetTextColor(hdc, kColorTextMuted);
                    RECT rcCalc = { 185, curY, rc.right - 28, 0 };
                    int hDesc = DrawTextW(hdc, item.second.c_str(), -1, &rcCalc, DT_LEFT | DT_WORDBREAK | DT_CALCRECT);
                    DrawTextW(hdc, item.second.c_str(), -1, &rcCalc, DT_LEFT | DT_WORDBREAK);

                    curY += std::max(20, hDesc + 4);
                }
                curY += 6;
            }

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
            if (s_hFont) DeleteObject(s_hFont);
            if (s_hFontBold) DeleteObject(s_hFontBold);
            if (s_hFontHeader) DeleteObject(s_hFontHeader);
            return 0;
    }

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void showToolsGuideDialog(HWND hParent) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedToolsGuideDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = ToolsGuideDlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    int dlgW = 840;
    int dlgH = 800;

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
        L"How-To",
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

// -----------------------------------------------------------------------------
// Dedicated Dialog: About
// -----------------------------------------------------------------------------
static LRESULT CALLBACK AboutDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HFONT s_hFont = nullptr;
    static HFONT s_hFontBold = nullptr;
    static HFONT s_hFontHeader = nullptr;

    switch (msg) {
        case WM_CREATE: {
            enableDarkModeForWindow(hWnd);
            s_hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            s_hFontBold = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            s_hFontHeader = CreateFontW(-18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            RECT rc;
            GetClientRect(hWnd, &rc);
            HWND hBtnOk = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                rc.right - 106, rc.bottom - 42, 90, 28, hWnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
            SendMessageW(hBtnOk, WM_SETFONT, reinterpret_cast<WPARAM>(s_hFont), TRUE);

            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);

            HBRUSH hbrBg = CreateSolidBrush(kColorDlgBg);
            FillRect(hdc, &rc, hbrBg);
            DeleteObject(hbrBg);

            HFONT oldFont = (HFONT)SelectObject(hdc, s_hFontHeader);
            SetBkMode(hdc, TRANSPARENT);

            // Binary Logo
            HICON hIcon = (HICON)LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
            if (hIcon) {
                DrawIconEx(hdc, 24, 20, hIcon, 48, 48, 0, nullptr, DI_NORMAL);
                DestroyIcon(hIcon);
            }

            // Title
            SetTextColor(hdc, kColorAccentCyan);
            RECT rcTitle = { 86, 16, rc.right - 24, 46 };
            DrawTextW(hdc, L"ReSyndicated Editor", -1, &rcTitle, DT_LEFT | DT_SINGLELINE | DT_NOCLIP);

            // App Version
            SelectObject(hdc, s_hFontBold);
            SetTextColor(hdc, kColorText);
            RECT rcVer = { 86, 48, rc.right - 24, 70 };
            DrawTextW(hdc, L"Version " RS_VERSION_WIDE, -1, &rcVer, DT_LEFT | DT_SINGLELINE | DT_NOCLIP);

            // Simple description of the editor
            SelectObject(hdc, s_hFont);
            SetTextColor(hdc, kColorText);

            RECT rcDesc = { 24, 78, rc.right - 24, rc.bottom };
            int hDesc = DrawTextW(hdc, L"ReSyndicated Editor is an advanced visual mission and map editor for the ReSyndicated game engine. "
                           L"It provides multi-layer isometric tilemap construction with 3D depth-tracking wiregrids, "
                           L"interactive ped and vehicle waypoint routing, tactical entity configuration, and sandboxed playtesting.", -1, &rcDesc, DT_LEFT | DT_WORDBREAK);

            // Authors section
            int curY = 78 + hDesc + 14;
            SelectObject(hdc, s_hFontBold);
            SetTextColor(hdc, kColorAccentCyan);
            RECT rcAuthTitle = { 24, curY, rc.right - 24, curY + 22 };
            DrawTextW(hdc, L"Author && Acknowledgments", -1, &rcAuthTitle, DT_LEFT | DT_SINGLELINE | DT_NOCLIP);
            curY += 24;

            SelectObject(hdc, s_hFont);
            SetTextColor(hdc, kColorText);
            RECT rcAuth1 = { 24, curY, rc.right - 24, rc.bottom };
            int hAuth1 = DrawTextW(hdc, L"ReSyndicated Author: mork009 <mork009resynd@gmail.com>",
                      -1, &rcAuth1, DT_LEFT | DT_WORDBREAK);
            curY += hAuth1 + 6;

            SetTextColor(hdc, kColorTextMuted);
            RECT rcAuth2 = { 24, curY, rc.right - 24, rc.bottom };
            int hAuth2 = DrawTextW(hdc, L"ReSyndicated is forked off the open source project, FreeSynd. "
                           L"Special thanks to The FreeSynd Team for building the original project.",
                      -1, &rcAuth2, DT_LEFT | DT_WORDBREAK);
            curY += hAuth2 + 14;

            // License section
            SelectObject(hdc, s_hFontBold);
            SetTextColor(hdc, kColorAccentCyan);
            RECT rcLicTitle = { 24, curY, rc.right - 24, curY + 22 };
            DrawTextW(hdc, L"License", -1, &rcLicTitle, DT_LEFT | DT_SINGLELINE | DT_NOCLIP);
            curY += 24;

            SelectObject(hdc, s_hFont);
            SetTextColor(hdc, kColorTextMuted);
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
            if (s_hFont) DeleteObject(s_hFont);
            if (s_hFontBold) DeleteObject(s_hFontBold);
            if (s_hFontHeader) DeleteObject(s_hFontHeader);
            return 0;
    }

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

void showAboutDialog(HWND hParent) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedAboutDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = AboutDlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
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

} // namespace rs_win_editor
