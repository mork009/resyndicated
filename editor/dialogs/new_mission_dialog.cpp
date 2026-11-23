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

#include "new_mission_dialog.h"
#include "dialog_theme.h"
#include "mission_chooser.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <format>
#include <vector>

namespace rs_win_editor {

namespace {

struct NewDlgState {
    HWND hParent = nullptr;
    HWND hDlg = nullptr;
    NewMissionDialogData *pData = nullptr;
    bool accepted = false;

    HWND hEditMissionId = nullptr;
    HWND hComboBaseMap = nullptr;
    HWND hEditTitle = nullptr;
    HFONT hFont = nullptr;
    HFONT hHeaderFont = nullptr;
};

LRESULT CALLBACK NewDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    NewDlgState *pState = reinterpret_cast<NewDlgState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<NewDlgState*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_CREATE: {
            pState->hDlg = hWnd;
            HINSTANCE hInst = GetModuleHandleW(nullptr);

            pState->hFont = CreateFontW(
                -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );

            pState->hHeaderFont = CreateFontW(
                -14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );

            // 1. Mission ID
            CreateWindowExW(0, WC_STATICW, L"Mission ID (1..99):",
                WS_CHILD | WS_VISIBLE, 24, 20, 140, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditMissionId = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW,
                std::to_wstring(pState->pData->missionId).c_str(),
                WS_CHILD | WS_VISIBLE | ES_NUMBER, 170, 18, 60, 22, hWnd, nullptr, hInst, nullptr);

            // 2. Base Map ComboBox
            CreateWindowExW(0, WC_STATICW, L"Base Map Template:",
                WS_CHILD | WS_VISIBLE, 24, 56, 140, 20, hWnd, nullptr, hInst, nullptr);
            pState->hComboBaseMap = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 170, 54, 280, 240, hWnd, nullptr, hInst, nullptr);

            ComboBox_AddString(pState->hComboBaseMap, L"Blank Map");
            const auto &missions = MissionChooserDialog::getMissionList();
            for (const auto &m : missions) {
                if (m.id <= 50) {
                    std::wstring label = std::format(L"[Copy Map {:02}] {} (map{:02}.dat)", m.id, m.name, m.id);
                    ComboBox_AddString(pState->hComboBaseMap, label.c_str());
                }
            }
            ComboBox_SetCurSel(pState->hComboBaseMap, 0);

            // 3. Briefing Title / Codename
            CreateWindowExW(0, WC_STATICW, L"Briefing Codename:",
                WS_CHILD | WS_VISIBLE, 24, 94, 140, 20, hWnd, nullptr, hInst, nullptr);
            int wlen = MultiByteToWideChar(CP_UTF8, 0, pState->pData->briefingTitle.c_str(), -1, nullptr, 0);
            std::wstring initialTitle;
            if (wlen > 1) {
                initialTitle.resize(wlen - 1);
                MultiByteToWideChar(CP_UTF8, 0, pState->pData->briefingTitle.c_str(), -1, initialTitle.data(), wlen);
            } else {
                initialTitle = L"OPERATION NEW DAWN";
            }
            pState->hEditTitle = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW,
                initialTitle.c_str(),
                WS_CHILD | WS_VISIBLE, 170, 92, 280, 22, hWnd, nullptr, hInst, nullptr);

            // 4. Buttons
            CreateWindowExW(0, WC_BUTTONW, L"Create Mission",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 230, 136, 120, 28, hWnd, reinterpret_cast<HMENU>(IDOK), hInst, nullptr);
            CreateWindowExW(0, WC_BUTTONW, L"Cancel",
                WS_CHILD | WS_VISIBLE, 360, 136, 90, 28, hWnd, reinterpret_cast<HMENU>(IDCANCEL), hInst, nullptr);

            // Set fonts
            EnumChildWindows(hWnd, [](HWND hChild, LPARAM lp) -> BOOL {
                HFONT font = reinterpret_cast<HFONT>(lp);
                SendMessageW(hChild, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                return TRUE;
            }, reinterpret_cast<LPARAM>(pState->hFont));

            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);

            if (id == IDOK) {
                wchar_t buf[256] = {};
                GetWindowTextW(pState->hEditMissionId, buf, 64);
                int mId = _wtoi(buf);
                if (mId < 1) mId = 1;
                if (mId > 99) mId = 99;
                pState->pData->missionId = mId;

                int selMap = ComboBox_GetCurSel(pState->hComboBaseMap);
                if (selMap <= 0) {
                    pState->pData->isBlankMap = true;
                    pState->pData->baseMapId = 1;
                } else {
                    pState->pData->isBlankMap = false;
                    pState->pData->baseMapId = selMap; // Maps 1..50
                }

                GetWindowTextW(pState->hEditTitle, buf, 256);
                int mlen = WideCharToMultiByte(CP_UTF8, 0, buf, -1, nullptr, 0, nullptr, nullptr);
                if (mlen > 1) {
                    std::string str(mlen - 1, '\0');
                    WideCharToMultiByte(CP_UTF8, 0, buf, -1, str.data(), mlen, nullptr, nullptr);
                    pState->pData->briefingTitle = str;
                } else {
                    pState->pData->briefingTitle = "OPERATION NEW DAWN";
                }

                pState->accepted = true;
                DestroyWindow(hWnd);
                return 0;
            }

            if (id == IDCANCEL) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_DESTROY: {
            if (pState) {
                if (pState->hFont) {
                    DeleteObject(pState->hFont);
                    pState->hFont = nullptr;
                }
                if (pState->hHeaderFont) {
                    DeleteObject(pState->hHeaderFont);
                    pState->hHeaderFont = nullptr;
                }
            }
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace

bool NewMissionDialog::show(HWND hParent, NewMissionDialogData &outData) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedNewMissionDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = NewDlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    NewDlgState state;
    state.hParent = hParent;
    state.pData = &outData;

    RECT rcParent = {};
    GetWindowRect(hParent, &rcParent);
    int dlgW = 490;
    int dlgH = 220;
    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (dlgX < 0) dlgX = 0;
    if (dlgY < 0) dlgY = 0;

    HWND hTopLevel = GetAncestor(hParent, GA_ROOT);
    if (!hTopLevel) hTopLevel = hParent;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        kClassName,
        L"New Mission",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dlgX, dlgY, dlgW, dlgH,
        hTopLevel, nullptr, hInstance, &state
    );

    if (!hDlg) return false;

    applyDialogTheme(hDlg);

    EnableWindow(hTopLevel, FALSE);
    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    MSG msg = {};
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            DestroyWindow(hDlg);
            break;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hTopLevel, TRUE);
    SetForegroundWindow(hTopLevel);
    SetFocus(hTopLevel);

    return state.accepted;
}

} // namespace rs_win_editor
