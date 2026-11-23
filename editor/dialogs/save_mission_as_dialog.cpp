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

#include "save_mission_as_dialog.h"
#include "dialog_theme.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlobj.h>
#include <format>
#include <vector>
#include <filesystem>

namespace rs_win_editor {

namespace {

static const int IDC_EDIT_MISSION_ID   = 201;
static const int IDC_EDIT_FOLDER       = 202;
static const int IDC_BTN_BROWSE        = 203;
static const int IDC_RADIO_MAP_CUSTOM  = 204;
static const int IDC_RADIO_MAP_LINKED  = 205;

struct SaveDlgState {
    HWND hParent = nullptr;
    HWND hDlg = nullptr;
    int currentMissionId = 1;
    int currentMapId = 1;
    int currentLanguage = 0;
    std::filesystem::path initialDir;
    SaveMissionAsDialogData *pData = nullptr;
    bool accepted = false;
    bool isCustomMap = true;
    bool hasConflict = false;

    HWND hEditMissionId = nullptr;
    HWND hEditFolder = nullptr;
    HWND hRadioCustomMap = nullptr;
    HWND hRadioLinkedMap = nullptr;

    HWND hGroupTriad = nullptr;
    HWND hStaticGameStatus = nullptr;
    HWND hStaticMapStatus = nullptr;
    HWND hStaticMissStatus[4] = {};
    HWND hStaticWarning = nullptr;
    HWND hBtnSave = nullptr;
    HWND hBtnCancel = nullptr;

    bool availableLanguages[4] = { true, false, false, false };
    bool modifiedLanguages[4] = { false, false, false, false };

    HFONT hFont = nullptr;
    HFONT hFontBold = nullptr;
};

static bool browseForFolder(HWND hOwner, std::wstring &outFolder) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool coInitialized = SUCCEEDED(hr);

    bool success = false;
    IFileOpenDialog *pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));
    if (SUCCEEDED(hr)) {
        DWORD dwOptions = 0;
        if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions))) {
            pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        }
        pFileOpen->SetTitle(L"Select Destination Folder for Mission Triad");
        if (!outFolder.empty()) {
            IShellItem *pDefaultItem = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(outFolder.c_str(), nullptr, IID_PPV_ARGS(&pDefaultItem)))) {
                pFileOpen->SetFolder(pDefaultItem);
                pDefaultItem->Release();
            }
        }
        if (SUCCEEDED(pFileOpen->Show(hOwner))) {
            IShellItem *pItem = nullptr;
            if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
                PWSTR pszFilePath = nullptr;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    outFolder = pszFilePath;
                    CoTaskMemFree(pszFilePath);
                    success = true;
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    } else {
        BROWSEINFOW bi = {};
        bi.hwndOwner = hOwner;
        bi.lpszTitle = L"Select Destination Folder for Mission Triad";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
        if (pidl) {
            wchar_t path[MAX_PATH] = {};
            if (SHGetPathFromIDListW(pidl, path)) {
                outFolder = path;
                success = true;
            }
            CoTaskMemFree(pidl);
        }
    }

    if (coInitialized) {
        CoUninitialize();
    }
    return success;
}

static void updateTriadPreview(SaveDlgState *pState) {
    if (!pState) return;

    wchar_t idBuf[64] = {};
    GetWindowTextW(pState->hEditMissionId, idBuf, 64);
    int mId = _wtoi(idBuf);
    if (mId < 1) mId = 1;
    if (mId > 99) mId = 99;

    wchar_t dirBuf[MAX_PATH] = {};
    GetWindowTextW(pState->hEditFolder, dirBuf, MAX_PATH);
    std::filesystem::path folderPath(dirBuf);

    bool isCustomMap = pState->isCustomMap;
    int mapId = isCustomMap ? mId : pState->currentMapId;

    std::filesystem::path gameFile = folderPath / std::format("game{:02}.dat", mId);
    std::filesystem::path mapFile  = folderPath / std::format("map{:02}.dat", mapId);

    bool gExists = std::filesystem::exists(gameFile);
    bool mExists = std::filesystem::exists(mapFile);
    bool anyMissExists = false;

    std::wstring gText = std::format(L"game{:02}.dat \u2014 {}", mId, gExists ? L"[EXISTS - WILL OVERWRITE]" : L"[NEW FILE]");
    std::wstring mText = std::format(L"map{:02}.dat  \u2014 {} ({})", mapId,
        mExists ? L"[EXISTS - WILL OVERWRITE]" : L"[NEW FILE]",
        isCustomMap ? L"Custom Map Copy" : L"Linked Map");

    SetWindowTextW(pState->hStaticGameStatus, gText.c_str());
    SetWindowTextW(pState->hStaticMapStatus, mText.c_str());

    int curY = 254;
    for (int i = 0; i < 4; ++i) {
        if (!pState->availableLanguages[i]) {
            if (pState->hStaticMissStatus[i]) ShowWindow(pState->hStaticMissStatus[i], SW_HIDE);
            continue;
        }
        std::string missFilename = MissionPropertiesDialog::getBriefingFilename(mId, i);
        std::filesystem::path missFile = folderPath / missFilename;
        bool bExists = std::filesystem::exists(missFile);
        if (bExists) anyMissExists = true;

        std::wstring langTag = MissionPropertiesDialog::getLanguageName(i);
        if (pState->modifiedLanguages[i]) {
            langTag += L" - Edited";
        } else {
            langTag += L" - Original";
        }

        std::wstring bText = std::format(L"{} \u2014 {} ({})",
            missFile.filename().wstring(),
            bExists ? L"[EXISTS - WILL OVERWRITE]" : L"[NEW FILE]",
            langTag);
        SetWindowTextW(pState->hStaticMissStatus[i], bText.c_str());
        SetWindowPos(pState->hStaticMissStatus[i], nullptr, 40, curY, 480, 18, SWP_NOZORDER | SWP_SHOWWINDOW);
        curY += 22;
    }

    SetWindowPos(pState->hStaticWarning, nullptr, 40, curY, 480, 20, SWP_NOZORDER | SWP_SHOWWINDOW);
    if (pState->hGroupTriad) {
        SetWindowPos(pState->hGroupTriad, nullptr, 24, 186, 516, (curY + 28 - 186), SWP_NOZORDER);
    }
    int btnY = curY + 38;
    if (pState->hBtnSave) {
        SetWindowPos(pState->hBtnSave, nullptr, 320, btnY, 110, 28, SWP_NOZORDER);
    }
    if (pState->hBtnCancel) {
        SetWindowPos(pState->hBtnCancel, nullptr, 440, btnY, 100, 28, SWP_NOZORDER);
    }

    pState->hasConflict = (gExists || mExists || anyMissExists);

    if (pState->hasConflict) {
        SetWindowTextW(pState->hStaticWarning, L"\u26A0 Warning: One or more target files already exist in this folder.");
    } else {
        SetWindowTextW(pState->hStaticWarning, L"\u2713 All files are new. Safe to save without overwriting.");
    }

    if (pState->hStaticWarning) InvalidateRect(pState->hStaticWarning, nullptr, TRUE);
    if (pState->hRadioCustomMap) InvalidateRect(pState->hRadioCustomMap, nullptr, TRUE);
    if (pState->hRadioLinkedMap) InvalidateRect(pState->hRadioLinkedMap, nullptr, TRUE);
}

LRESULT CALLBACK SaveDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SaveDlgState *pState = reinterpret_cast<SaveDlgState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    if (msg == WM_CTLCOLORSTATIC && pState && reinterpret_cast<HWND>(lParam) == pState->hStaticWarning) {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        COLORREF color = pState->hasConflict ? RGB(250, 204, 21) : RGB(74, 222, 128);
        SetTextColor(hdc, color);
        SetBkColor(hdc, kColorDlgBg);
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH s_hbrDlg = CreateSolidBrush(kColorDlgBg);
        return reinterpret_cast<LRESULT>(s_hbrDlg);
    }

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<SaveDlgState*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_DRAWITEM: {
            DRAWITEMSTRUCT *pDIS = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (pDIS && pDIS->CtlType == ODT_BUTTON &&
                (pDIS->CtlID == IDC_RADIO_MAP_CUSTOM || pDIS->CtlID == IDC_RADIO_MAP_LINKED)) {
                bool isCustomBtn = (pDIS->CtlID == IDC_RADIO_MAP_CUSTOM);
                bool isActive = (pState && isCustomBtn == pState->isCustomMap);

                HDC hdc = pDIS->hDC;
                RECT rc = pDIS->rcItem;

                COLORREF bgColor = isActive ? RGB(0, 120, 215) : RGB(34, 38, 44);
                COLORREF borderColor = isActive ? RGB(40, 160, 255) : RGB(58, 65, 74);
                COLORREF textColor = isActive ? RGB(255, 255, 255) : RGB(200, 205, 212);

                HBRUSH hBgBrush = CreateSolidBrush(bgColor);
                FillRect(hdc, &rc, hBgBrush);
                DeleteObject(hBgBrush);

                HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
                HGDIOBJ oldPen = SelectObject(hdc, hPen);
                HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
                Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(hPen);

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, textColor);
                HFONT fontToUse = (isActive && pState->hFontBold) ? pState->hFontBold : pState->hFont;
                HGDIOBJ oldFont = SelectObject(hdc, fontToUse);

                wchar_t idBuf[64] = {};
                GetWindowTextW(pState->hEditMissionId, idBuf, 64);
                int mId = _wtoi(idBuf);
                if (mId < 1) mId = 1;
                if (mId > 99) mId = 99;

                std::wstring text;
                if (isCustomBtn) {
                    text = std::format(L"{}  Save custom copy of map (map{:02}.dat matching Mission ID)",
                        isActive ? L"\u25C9" : L"\u25CB", mId);
                } else {
                    text = std::format(L"{}  Keep linked to current map (map{:02}.dat)",
                        isActive ? L"\u25C9" : L"\u25CB", pState ? pState->currentMapId : 1);
                }

                RECT rcText = { rc.left + 14, rc.top, rc.right - 14, rc.bottom };
                DrawTextW(hdc, text.c_str(), -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hdc, oldFont);
                return TRUE;
            }
            break;
        }

        case WM_CREATE: {
            pState->hDlg = hWnd;
            HINSTANCE hInst = GetModuleHandleW(nullptr);

            pState->hFont = CreateFontW(
                -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );

            pState->hFontBold = CreateFontW(
                -12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );

            // 1. Mission ID
            CreateWindowExW(0, WC_STATICW, L"Target Mission ID (1..99):",
                WS_CHILD | WS_VISIBLE, 24, 20, 160, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditMissionId = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW,
                std::to_wstring(pState->currentMissionId).c_str(),
                WS_CHILD | WS_VISIBLE | ES_NUMBER, 190, 18, 60, 22, hWnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_MISSION_ID)), hInst, nullptr);

            // 2. Folder
            CreateWindowExW(0, WC_STATICW, L"Destination Directory:",
                WS_CHILD | WS_VISIBLE, 24, 56, 160, 20, hWnd, nullptr, hInst, nullptr);
            std::wstring initialDirStr = pState->initialDir.wstring();
            pState->hEditFolder = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW,
                initialDirStr.c_str(),
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 190, 54, 260, 22, hWnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_FOLDER)), hInst, nullptr);
            CreateWindowExW(0, WC_BUTTONW, L"Browse...",
                WS_CHILD | WS_VISIBLE, 460, 53, 80, 24, hWnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_BROWSE)), hInst, nullptr);

            // 3. Map Strategy GroupBox
            HWND hGroupMap = CreateWindowExW(0, WC_BUTTONW, L"Map Strategy",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 24, 90, 516, 88, hWnd, nullptr, hInst, nullptr);
            (void)hGroupMap;

            pState->hRadioCustomMap = CreateWindowExW(0, WC_BUTTONW, L"",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP, 38, 112, 488, 28, hWnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_RADIO_MAP_CUSTOM)), hInst, nullptr);

            pState->hRadioLinkedMap = CreateWindowExW(0, WC_BUTTONW, L"",
                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW | WS_TABSTOP, 38, 144, 488, 28, hWnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_RADIO_MAP_LINKED)), hInst, nullptr);

            // 4. Triad Preview GroupBox
            pState->hGroupTriad = CreateWindowExW(0, WC_BUTTONW, L"Syndicate Mission Files && Localization Status",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 24, 186, 516, 170, hWnd, nullptr, hInst, nullptr);

            pState->hStaticGameStatus = CreateWindowExW(0, WC_STATICW, L"",
                WS_CHILD | WS_VISIBLE, 40, 210, 480, 18, hWnd, nullptr, hInst, nullptr);
            pState->hStaticMapStatus = CreateWindowExW(0, WC_STATICW, L"",
                WS_CHILD | WS_VISIBLE, 40, 232, 480, 18, hWnd, nullptr, hInst, nullptr);
            for (int i = 0; i < 4; ++i) {
                pState->hStaticMissStatus[i] = CreateWindowExW(0, WC_STATICW, L"",
                    WS_CHILD | WS_VISIBLE, 40, 254 + i * 22, 480, 18, hWnd, nullptr, hInst, nullptr);
            }
            pState->hStaticWarning = CreateWindowExW(0, WC_STATICW, L"",
                WS_CHILD | WS_VISIBLE, 40, 342, 480, 20, hWnd, nullptr, hInst, nullptr);

            // 5. Buttons
            pState->hBtnSave = CreateWindowExW(0, WC_BUTTONW, L"Save Mission",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 320, 376, 110, 28, hWnd, reinterpret_cast<HMENU>(IDOK), hInst, nullptr);
            pState->hBtnCancel = CreateWindowExW(0, WC_BUTTONW, L"Cancel",
                WS_CHILD | WS_VISIBLE, 440, 376, 100, 28, hWnd, reinterpret_cast<HMENU>(IDCANCEL), hInst, nullptr);

            // Set fonts
            EnumChildWindows(hWnd, [](HWND hChild, LPARAM lp) -> BOOL {
                HFONT font = reinterpret_cast<HFONT>(lp);
                SendMessageW(hChild, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                return TRUE;
            }, reinterpret_cast<LPARAM>(pState->hFont));

            SendMessageW(pState->hStaticWarning, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontBold), TRUE);

            updateTriadPreview(pState);
            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);

            if ((id == IDC_EDIT_MISSION_ID || id == IDC_EDIT_FOLDER) && code == EN_CHANGE) {
                updateTriadPreview(pState);
                return 0;
            }

            if (id == IDC_RADIO_MAP_CUSTOM) {
                if (!pState->isCustomMap) {
                    pState->isCustomMap = true;
                    InvalidateRect(pState->hRadioCustomMap, nullptr, TRUE);
                    InvalidateRect(pState->hRadioLinkedMap, nullptr, TRUE);
                    updateTriadPreview(pState);
                }
                return 0;
            }

            if (id == IDC_RADIO_MAP_LINKED) {
                if (pState->isCustomMap) {
                    pState->isCustomMap = false;
                    InvalidateRect(pState->hRadioCustomMap, nullptr, TRUE);
                    InvalidateRect(pState->hRadioLinkedMap, nullptr, TRUE);
                    updateTriadPreview(pState);
                }
                return 0;
            }

            if (id == IDC_BTN_BROWSE) {
                wchar_t dirBuf[MAX_PATH] = {};
                GetWindowTextW(pState->hEditFolder, dirBuf, MAX_PATH);
                std::wstring folder(dirBuf);
                if (browseForFolder(hWnd, folder)) {
                    SetWindowTextW(pState->hEditFolder, folder.c_str());
                    updateTriadPreview(pState);
                }
                return 0;
            }

            if (id == IDOK) {
                wchar_t idBuf[64] = {};
                GetWindowTextW(pState->hEditMissionId, idBuf, 64);
                int mId = _wtoi(idBuf);
                if (mId < 1) mId = 1;
                if (mId > 99) mId = 99;

                wchar_t dirBuf[MAX_PATH] = {};
                GetWindowTextW(pState->hEditFolder, dirBuf, MAX_PATH);
                std::filesystem::path folderPath(dirBuf);

                if (folderPath.empty() || !std::filesystem::exists(folderPath)) {
                    showEditorMessageBox(hWnd, L"The selected destination directory does not exist. Please specify a valid folder.",
                        L"Invalid Directory", MB_ICONWARNING);
                    return 0;
                }

                bool isCustomMap = pState->isCustomMap;
                int mapId = isCustomMap ? mId : pState->currentMapId;

                pState->pData->targetMissionId = mId;
                pState->pData->targetMapId = mapId;
                pState->pData->targetLanguage = pState->currentLanguage;
                pState->pData->targetDir = folderPath;
                pState->pData->saveCustomMapCopy = isCustomMap;

                std::string missFilename = MissionPropertiesDialog::getBriefingFilename(mId, pState->currentLanguage);
                pState->pData->resolvedPaths.missionId = mId;
                pState->pData->resolvedPaths.mapId = mapId;
                pState->pData->resolvedPaths.gameFile = folderPath / std::format("game{:02}.dat", mId);
                pState->pData->resolvedPaths.mapFile  = folderPath / std::format("map{:02}.dat", mapId);
                pState->pData->resolvedPaths.missFile = folderPath / missFilename;

                // Check overwrite warning across all destination files (game, map, and all active language briefings)
                std::vector<std::wstring> existingFiles;
                if (std::filesystem::exists(pState->pData->resolvedPaths.gameFile)) {
                    existingFiles.push_back(pState->pData->resolvedPaths.gameFile.filename().wstring());
                }
                if (std::filesystem::exists(pState->pData->resolvedPaths.mapFile)) {
                    existingFiles.push_back(pState->pData->resolvedPaths.mapFile.filename().wstring());
                }
                for (int i = 0; i < 4; ++i) {
                    if (pState->availableLanguages[i]) {
                        std::filesystem::path mf = folderPath / MissionPropertiesDialog::getBriefingFilename(mId, i);
                        if (std::filesystem::exists(mf)) {
                            existingFiles.push_back(mf.filename().wstring());
                        }
                    }
                }
                if (!existingFiles.empty()) {
                    std::wstring warn = L"The following destination file(s) already exist and will be overwritten:\n\n";
                    for (const auto &fn : existingFiles) {
                        warn += L" \u2022 " + fn + L"\n";
                    }
                    warn += L"\nAre you sure you want to proceed and overwrite these files?";
                    if (showEditorMessageBox(hWnd, warn.c_str(), L"Confirm Overwrite",
                            MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
                        return 0;
                    }
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
                if (pState->hFontBold) {
                    DeleteObject(pState->hFontBold);
                    pState->hFontBold = nullptr;
                }
            }
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace

bool SaveMissionAsDialog::show(HWND hParent, int currentMissionId, int currentMapId,
                               const MissionPropertiesData &props,
                               const std::filesystem::path &initialDir, SaveMissionAsDialogData &outData) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedSaveMissionAsDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = SaveDlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    SaveDlgState state;
    state.hParent = hParent;
    state.currentMissionId = currentMissionId;
    state.currentMapId = currentMapId;
    state.currentLanguage = props.language;
    state.initialDir = initialDir;
    state.pData = &outData;
    state.isCustomMap = outData.saveCustomMapCopy;
    for (int i = 0; i < 4; ++i) {
        state.availableLanguages[i] = props.langs[i].available;
        state.modifiedLanguages[i] = props.langs[i].modified || props.langs[i].sessionEdited;
        outData.availableLanguages[i] = props.langs[i].available;
        outData.modifiedLanguages[i] = props.langs[i].modified || props.langs[i].sessionEdited;
    }

    RECT rcParent = {};
    GetWindowRect(hParent, &rcParent);
    int dlgW = 580;
    int dlgH = 470;
    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (dlgX < 0) dlgX = 0;
    if (dlgY < 0) dlgY = 0;

    HWND hTopLevel = GetAncestor(hParent, GA_ROOT);
    if (!hTopLevel) hTopLevel = hParent;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        kClassName,
        L"Save Mission As...",
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
