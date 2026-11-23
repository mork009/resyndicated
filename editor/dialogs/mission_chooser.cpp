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

#include "mission_chooser.h"
#include "rs-kernel/mgr/mission_triad.h"
#include "dialog_theme.h"

#include <commctrl.h>
#include <commdlg.h>
#include <algorithm>
#include <format>
#include <cwctype>

namespace rs_win_editor {

static const std::vector<MissionInfo> s_missions = {
    { 1,  L"Western Europe" },
    { 2,  L"Far East" },
    { 3,  L"Mongolia" },
    { 4,  L"Iran" },
    { 5,  L"California" },
    { 6,  L"Iraq" },
    { 7,  L"India" },
    { 8,  L"Northeast Territories" },
    { 9,  L"Kazakhstan" },
    { 10, L"Eastern Europe" },
    { 11, L"Western Australia" },
    { 12, L"Kamchatka" },
    { 13, L"Mozambique" },
    { 14, L"Peru" },
    { 15, L"Central Europe" },
    { 16, L"Greenland" },
    { 17, L"Alaska" },
    { 18, L"Ural" },
    { 19, L"Northern Territories" },
    { 20, L"Scandinavia" },
    { 21, L"Yukon" },
    { 22, L"Siberia" },
    { 23, L"Rockies" },
    { 24, L"Southern States" },
    { 25, L"Indonesia" },
    { 26, L"Mexico" },
    { 27, L"Zaire" },
    { 28, L"Algeria" },
    { 29, L"New England" },
    { 30, L"Argentina" },
    { 31, L"South Africa" },
    { 32, L"Colorado" },
    { 33, L"Sudan" },
    { 34, L"Midwest" },
    { 35, L"Libya" },
    { 36, L"Venezuela" },
    { 37, L"Atlantic Accelerator" },
    { 38, L"Arabia" },
    { 39, L"Northwest Territories" },
    { 40, L"Kenya" },
    { 41, L"Mauritania" },
    { 42, L"Newfoundland" },
    { 43, L"New South Wales" },
    { 44, L"Colombia" },
    { 45, L"Nigeria" },
    { 46, L"Brazil" },
    { 47, L"Uruguay" },
    { 48, L"Pacific Rim" },
    { 49, L"Paraguay" },
    { 50, L"China" },
    { 99, L"Custom / Playtest Mission" }
};

const std::vector<MissionInfo>& MissionChooserDialog::getMissionList() {
    return s_missions;
}

struct DialogState {
    HWND hParent = nullptr;
    HWND hDlg = nullptr;
    HWND hEditFilter = nullptr;
    HWND hListBox = nullptr;
    HWND hBtnOpen = nullptr;
    HWND hBtnBrowse = nullptr;
    HWND hBtnCancel = nullptr;
    HFONT hFont = nullptr;

    int selectedId = 1;
    std::wstring customPath;
    bool accepted = false;
    std::vector<int> filteredIndices;
};

static void populateList(DialogState *pState) {
    SendMessageW(pState->hListBox, LB_RESETCONTENT, 0, 0);
    pState->filteredIndices.clear();

    wchar_t filterBuf[128] = {};
    GetWindowTextW(pState->hEditFilter, filterBuf, 128);
    std::wstring filter(filterBuf);
    std::transform(filter.begin(), filter.end(), filter.begin(), ::towlower);

    int selectIndex = 0;
    for (size_t i = 0; i < s_missions.size(); ++i) {
        const auto &m = s_missions[i];
        std::wstring line = std::format(L"Mission {:02} - {}", m.id, m.name);
        std::wstring lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::towlower);

        if (filter.empty() || lowerLine.find(filter) != std::wstring::npos) {
            int itemIndex = static_cast<int>(SendMessageW(pState->hListBox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(line.c_str())));
            SendMessageW(pState->hListBox, LB_SETITEMDATA, itemIndex, static_cast<LPARAM>(m.id));
            pState->filteredIndices.push_back(static_cast<int>(i));

            if (m.id == pState->selectedId) {
                selectIndex = itemIndex;
            }
        }
    }

    if (SendMessageW(pState->hListBox, LB_GETCOUNT, 0, 0) > 0) {
        SendMessageW(pState->hListBox, LB_SETCURSEL, selectIndex, 0);
    }
}

static void confirmSelection(DialogState *pState) {
    int curSel = static_cast<int>(SendMessageW(pState->hListBox, LB_GETCURSEL, 0, 0));
    if (curSel != LB_ERR) {
        pState->selectedId = static_cast<int>(SendMessageW(pState->hListBox, LB_GETITEMDATA, curSel, 0));
        pState->customPath.clear();
        pState->accepted = true;
        DestroyWindow(pState->hDlg);
    }
}

static void browseForMissionFile(DialogState *pState) {
    OPENFILENAMEW ofn = {};
    wchar_t szFile[MAX_PATH] = L"";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = pState->hDlg;
    ofn.lpstrFilter = L"Syndicate Mission Files (*.dat)\0game*.dat;map*.dat;miss*.dat;*.dat\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Select Syndicate Mission File (gameXX.dat, mapXX.dat, or missXX.dat)";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        rs_knl::MissionTriadPaths paths;
        if (!rs_knl::MissionTriad::resolveFromAnyFile(szFile, paths)) {
            showEditorMessageBox(pState->hDlg,
                               L"Invalid or unrecognized mission file name.\n\n"
                               L"Expected format: gameXX.dat, mapXX.dat, or missXX.dat.",
                               L"Invalid File", MB_ICONERROR | MB_OK);
            return;
        }

        rs_knl::TriadValidationResult res = rs_knl::MissionTriad::validate(paths);
        if (!res.isValid) {
            showEditorMessageBox(pState->hDlg, res.summaryMessage.c_str(), L"Mission Triad Validation Error", MB_ICONERROR | MB_OK);
            return;
        }

        pState->customPath = paths.gameFile.wstring();
        pState->selectedId = paths.missionId;
        pState->accepted = true;
        DestroyWindow(pState->hDlg);
    }
}

LRESULT CALLBACK MissionChooserDialog::DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DialogState *pState = reinterpret_cast<DialogState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<DialogState*>(cs->lpCreateParams);
            pState->hDlg = hWnd;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));

            // Standard Segoe UI font
            pState->hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            // Filter label & edit
            HWND hLblFilter = CreateWindowExW(0, L"STATIC", L"Search missions:", WS_CHILD | WS_VISIBLE,
                                              16, 12, 380, 18, hWnd, nullptr, nullptr, nullptr);
            SendMessageW(hLblFilter, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);

            pState->hEditFilter = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                                                  16, 32, 380, 24, hWnd, reinterpret_cast<HMENU>(101), nullptr, nullptr);
            SendMessageW(pState->hEditFilter, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);

            // ListBox
            pState->hListBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
                                               16, 64, 380, 290, hWnd, reinterpret_cast<HMENU>(102), nullptr, nullptr);
            SendMessageW(pState->hListBox, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);

            // Buttons
            pState->hBtnBrowse = CreateWindowExW(0, L"BUTTON", L"Browse File...", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                                 16, 366, 110, 28, hWnd, reinterpret_cast<HMENU>(103), nullptr, nullptr);
            SendMessageW(pState->hBtnBrowse, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);

            pState->hBtnOpen = CreateWindowExW(0, L"BUTTON", L"Open Mission", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                                               184, 366, 110, 28, hWnd, reinterpret_cast<HMENU>(IDOK), nullptr, nullptr);
            SendMessageW(pState->hBtnOpen, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);

            pState->hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                                 300, 366, 96, 28, hWnd, reinterpret_cast<HMENU>(IDCANCEL), nullptr, nullptr);
            SendMessageW(pState->hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);

            populateList(pState);
            SetFocus(pState->hEditFilter);
            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == 101 && code == EN_CHANGE) {
                populateList(pState);
                return 0;
            }

            if (id == 102 && code == LBN_DBLCLK) {
                confirmSelection(pState);
                return 0;
            }

            if (id == IDOK) {
                confirmSelection(pState);
                return 0;
            }

            if (id == 103) {
                browseForMissionFile(pState);
                return 0;
            }

            if (id == IDCANCEL) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            if (pState && pState->hFont) {
                DeleteObject(pState->hFont);
                pState->hFont = nullptr;
            }
            return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool MissionChooserDialog::show(HWND hParent, int currentId, int &outMissionId, std::wstring &outCustomPath) {
    static bool s_classRegistered = false;
    const wchar_t *kClassName = L"ReSyndicatedMissionChooserDlgClass";

    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_classRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = DialogProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_classRegistered = true;
    }

    DialogState state;
    state.hParent = hParent;
    state.selectedId = currentId;

    // Center over parent
    RECT rcParent = {};
    GetWindowRect(hParent, &rcParent);
    int dlgW = 428;
    int dlgH = 445;
    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (dlgX < 0) dlgX = 0;
    if (dlgY < 0) dlgY = 0;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        kClassName,
        L"Open Syndicate Mission",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dlgX, dlgY, dlgW, dlgH,
        hParent, nullptr, hInstance, &state
    );

    if (!hDlg) return false;

    applyDialogTheme(hDlg);

    // Disable parent for modal behavior
    EnableWindow(hParent, FALSE);
    ShowWindow(hDlg, SW_SHOW);
    UpdateWindow(hDlg);

    // Modal message loop
    MSG msg = {};
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            DestroyWindow(hDlg);
            break;
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            confirmSelection(&state);
            break;
        }

        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Re-enable parent and restore focus
    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    SetFocus(hParent);

    if (state.accepted) {
        outMissionId = state.selectedId;
        outCustomPath = state.customPath;
        return true;
    }

    return false;
}

} // namespace rs_win_editor
