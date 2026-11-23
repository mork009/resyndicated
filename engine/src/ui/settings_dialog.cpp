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

#include "rs-engine/ui/settings_dialog.h"
#include "rs-utils/io/file.h"
#include "rs-utils/io/resyndconfig.h"
#include "rs-utils/log/log.h"

#include <shobjidl.h>
#include <shlobj.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <format>
#include <algorithm>
#include <fstream>
#include <iostream>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

namespace fs = std::filesystem;

namespace rs_eng {

namespace {

constexpr COLORREF kColorDlgBg           = RGB(28, 30, 34);    // #1C1E22
constexpr COLORREF kColorControlBg       = RGB(20, 22, 25);    // #141619
constexpr COLORREF kColorBorder          = RGB(58, 65, 74);    // #3A414A
constexpr COLORREF kColorBorderHighlight = RGB(85, 98, 115);   // #556273
constexpr COLORREF kColorText            = RGB(225, 230, 236); // #E1E6EC
constexpr COLORREF kColorTextMuted       = RGB(160, 168, 178); // #A0A8B2
constexpr COLORREF kColorAccentCyan      = RGB(0, 220, 245);   // Syndicate Cyan
constexpr COLORREF kColorGreen           = RGB(34, 197, 94);   // Emerald
constexpr COLORREF kColorRed             = RGB(239, 68, 68);   // Crimson

void enableDarkMode(HWND hWnd) {
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

bool fileExistsInDir(const fs::path &dir, const std::string &filename) {
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return false;

    fs::path p1 = dir / filename;
    if (fs::exists(p1, ec)) return true;

    std::string upper = filename;
    for (char &c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    fs::path p2 = dir / upper;
    if (fs::exists(p2, ec)) return true;

    std::string lower = filename;
    for (char &c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    fs::path p3 = dir / lower;
    if (fs::exists(p3, ec)) return true;

    for (const auto &entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        std::string fname = entry.path().filename().string();
        if (_stricmp(fname.c_str(), filename.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

int countMissionsInDir(const fs::path &dir) {
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return 0;

    int count = 0;
    for (const auto &entry : fs::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file(ec)) continue;
        std::string fname = entry.path().filename().string();
        for (char &c : fname) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (fname.rfind("game", 0) == 0 && fname.size() >= 8 && fname.rfind(".dat") == fname.size() - 4) {
            count++;
        }
    }
    return count;
}

} // anonymous namespace

DataValidationResult verifyDataDirectories(const fs::path &originalDir) {
    DataValidationResult res;
    res.originalDir = originalDir;

    res.hasAmericanRevolt = rs_utl::File::hasAmericanRevoltData(originalDir);

    // 1. Tile Graphics (hblk01.dat)
    AssetCheckItem cTile;
    cTile.description = "Tile Graphics (hblk01.dat)";
    cTile.filename = "hblk01.dat";
    {
        size_t sz = 0;
        std::unique_ptr<uint8_t[]> data(rs_utl::File::loadOriginalFileFromDir(originalDir, "hblk01.dat", sz));
        if (data && sz >= 6144) {
            bool validOffsets = true;
            bool hasSubtiles = false;
            for (int i = 0; i < 1536; ++i) {
                uint32_t off = rs_utl::READ_LE_UINT32(data.get() + i * 4);
                if (off >= 6144) {
                    if (static_cast<size_t>(off) + 320 > sz) {
                        validOffsets = false;
                        break;
                    }
                    hasSubtiles = true;
                }
            }
            if (validOffsets && hasSubtiles) {
                cTile.isPresent = true;
                cTile.detail = "Verified format (256 tiles, 1536 subtiles)";
            } else {
                cTile.isPresent = false;
                cTile.detail = "Corrupted subtile index table";
            }
        } else {
            cTile.isPresent = false;
            cTile.detail = (data == nullptr) ? "File not found" : "Corrupted tile data";
        }
    }
    res.originalChecks.push_back(cTile);

    // 2. Palette Files (hpal01.dat, hpal04.dat)
    AssetCheckItem cPal;
    cPal.description = "Palette Files (hpal01.dat, hpal04.dat)";
    cPal.filename = "hpal01.dat";
    {
        size_t sz1 = 0, sz4 = 0;
        std::unique_ptr<uint8_t[]> pal1(rs_utl::File::loadOriginalFileFromDir(originalDir, "hpal01.dat", sz1));
        std::unique_ptr<uint8_t[]> pal4(rs_utl::File::loadOriginalFileFromDir(originalDir, "hpal04.dat", sz4));
        if (pal1 && pal4 && sz1 >= 768 && sz4 >= 768) {
            cPal.isPresent = true;
            cPal.detail = "Verified format (256-color RGB palettes)";
        } else {
            cPal.isPresent = false;
            cPal.detail = (!pal1 || !pal4) ? "File(s) missing" : "Corrupted color palette";
        }
    }
    res.originalChecks.push_back(cPal);

    // 3. Sprite Data (hspr-0.dat/tab or hspr-0-d.dat/tab)
    AssetCheckItem cSpr;
    cSpr.description = "Gameplay Sprites (hspr-0 / hspr-0-d)";
    cSpr.filename = "hspr-0.dat";
    {
        size_t tabSz = 0, datSz = 0;
        std::unique_ptr<uint8_t[]> tab(rs_utl::File::loadOriginalFileFromDir(originalDir, "hspr-0.tab", tabSz));
        std::unique_ptr<uint8_t[]> dat(rs_utl::File::loadOriginalFileFromDir(originalDir, "hspr-0.dat", datSz));
        if (tab && dat && tabSz >= 6 && tabSz % 6 == 0 && datSz > 0) {
            size_t count = tabSz / 6;
            bool validSprites = true;
            for (size_t i = 0; i < count; ++i) {
                uint32_t off = rs_utl::READ_LE_UINT32(tab.get() + i * 6);
                uint8_t w = tab[i * 6 + 4];
                uint8_t h = tab[i * 6 + 5];
                if (w > 0 && h > 0) {
                    size_t sprBytes = static_cast<size_t>(h) * ((static_cast<size_t>(w) + 7) / 8) * 5;
                    if (static_cast<size_t>(off) + sprBytes > datSz) {
                        validSprites = false;
                        break;
                    }
                }
            }
            if (validSprites && count > 50) {
                cSpr.isPresent = true;
                cSpr.detail = res.hasAmericanRevolt
                    ? std::format("Verified format ({} sprites, American Revolt active)", count)
                    : std::format("Verified format ({} sprites)", count);
            } else {
                cSpr.isPresent = false;
                cSpr.detail = "Corrupted sprite table entries";
            }
        } else {
            cSpr.isPresent = false;
            cSpr.detail = (!tab || !dat) ? "File(s) missing" : "Corrupted sprite data";
        }
    }
    res.originalChecks.push_back(cSpr);

    // 4. Menu & UI Sprites (mspr-0.dat/tab or mspr-0-d.dat/tab)
    AssetCheckItem cMenuSpr;
    cMenuSpr.description = "Menu & UI Sprites (mspr-0 / mspr-0-d)";
    cMenuSpr.filename = "mspr-0.dat";
    {
        size_t tabSz = 0, datSz = 0;
        std::unique_ptr<uint8_t[]> tab(rs_utl::File::loadOriginalFileFromDir(originalDir, "mspr-0.tab", tabSz));
        std::unique_ptr<uint8_t[]> dat(rs_utl::File::loadOriginalFileFromDir(originalDir, "mspr-0.dat", datSz));
        if (tab && dat && tabSz >= 6 && tabSz % 6 == 0 && datSz > 0) {
            size_t count = tabSz / 6;
            bool validSprites = true;
            for (size_t i = 0; i < count; ++i) {
                uint32_t off = rs_utl::READ_LE_UINT32(tab.get() + i * 6);
                uint8_t w = tab[i * 6 + 4];
                uint8_t h = tab[i * 6 + 5];
                if (w > 0 && h > 0) {
                    size_t sprBytes = static_cast<size_t>(h) * ((static_cast<size_t>(w) + 7) / 8) * 5;
                    if (static_cast<size_t>(off) + sprBytes > datSz) {
                        validSprites = false;
                        break;
                    }
                }
            }
            if (validSprites && count > 10) {
                cMenuSpr.isPresent = true;
                cMenuSpr.detail = res.hasAmericanRevolt
                    ? std::format("Verified format ({} sprites, American Revolt active)", count)
                    : std::format("Verified format ({} sprites)", count);
            } else {
                cMenuSpr.isPresent = false;
                cMenuSpr.detail = "Corrupted menu sprite table";
            }
        } else {
            cMenuSpr.isPresent = false;
            cMenuSpr.detail = (!tab || !dat) ? "File(s) missing" : "Corrupted menu sprite data";
        }
    }
    res.originalChecks.push_back(cMenuSpr);

    // 5. Animation Sequences (*.ani)
    AssetCheckItem cAni;
    cAni.description = "Animation Sequences (*.ani)";
    cAni.filename = "hele-0.ani";
    {
        size_t eleSz = 0, fraSz = 0, staSz = 0;
        std::unique_ptr<uint8_t[]> ele(rs_utl::File::loadOriginalFileFromDir(originalDir, "hele-0.ani", eleSz));
        std::unique_ptr<uint8_t[]> fra(rs_utl::File::loadOriginalFileFromDir(originalDir, "hfra-0.ani", fraSz));
        std::unique_ptr<uint8_t[]> sta(rs_utl::File::loadOriginalFileFromDir(originalDir, "hsta-0.ani", staSz));
        if (ele && fra && sta &&
            eleSz >= 10 && eleSz % 10 == 0 &&
            fraSz >= 8 && fraSz % 8 == 0 &&
            staSz >= 2 && staSz % 2 == 0) {
            size_t numElements = eleSz / 10;
            size_t numFrames = fraSz / 8;
            size_t numStates = staSz / 2;
            cAni.isPresent = true;
            cAni.detail = std::format("Verified format ({} elements, {} frames, {} anims)", numElements, numFrames, numStates);
        } else {
            cAni.isPresent = false;
            cAni.detail = (!ele || !fra || !sta) ? "File(s) missing" : "Corrupted animation data";
        }
    }
    res.originalChecks.push_back(cAni);

    // 6. Collision Data (col01.dat)
    AssetCheckItem cCol;
    cCol.description = "Collision Data (col01.dat)";
    cCol.filename = "col01.dat";
    {
        size_t sz = 0;
        std::unique_ptr<uint8_t[]> data(rs_utl::File::loadOriginalFileFromDir(originalDir, "col01.dat", sz));
        if (data && sz >= 256) {
            cCol.isPresent = true;
            cCol.detail = "Verified format (256 tile collision types)";
        } else {
            cCol.isPresent = false;
            cCol.detail = (data == nullptr) ? "File not found" : "Corrupted collision data";
        }
    }
    res.originalChecks.push_back(cCol);

    // 7. Missions & Maps (game*.dat, map*.dat)
    AssetCheckItem cMis;
    cMis.description = "Missions & Maps (game*.dat, map*.dat)";
    cMis.filename = "game01.dat";

    std::vector<std::string> gameFiles;
    std::vector<std::string> mapFiles;
    std::error_code dirEc;
    for (const auto &entry : fs::directory_iterator(originalDir, dirEc)) {
        if (dirEc) break;
        if (!entry.is_regular_file(dirEc)) continue;
        std::string fname = entry.path().filename().string();
        std::string lower = fname;
        for (char &c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        if (lower.rfind("game", 0) == 0 && lower.size() >= 8 && lower.rfind(".dat") == lower.size() - 4) {
            gameFiles.push_back(fname);
        } else if (lower.rfind("map", 0) == 0 && lower.size() >= 7 && lower.rfind(".dat") == lower.size() - 4) {
            mapFiles.push_back(fname);
        }
    }

    int mCount = static_cast<int>(gameFiles.size());
    int mapCount = static_cast<int>(mapFiles.size());
    res.missionCount = mCount;

    if (gameFiles.empty() || mapFiles.empty()) {
        cMis.isPresent = false;
        cMis.detail = gameFiles.empty() ? "No mission level files (game*.dat) found" : "No map files (map*.dat) found";
    } else {
        bool allValid = true;
        std::string corruptDetail;

        for (const auto &gName : gameFiles) {
            size_t gSz = 0;
            std::unique_ptr<uint8_t[]> gDat(rs_utl::File::loadOriginalFileFromDir(originalDir, gName.c_str(), gSz));
            if (!gDat || gSz < 34816) {
                allValid = false;
                corruptDetail = std::format("Corrupted or truncated mission data in {}", gName);
                break;
            }
            if (gSz >= 114060) {
                uint16_t mapNum = rs_utl::READ_LE_UINT16(gDat.get() + 113960);
                if (mapNum > 0) {
                    std::string expectedMap = std::format("map{:02}.dat", mapNum);
                    bool mapFound = false;
                    for (const auto &mName : mapFiles) {
                        if (_stricmp(mName.c_str(), expectedMap.c_str()) == 0) {
                            mapFound = true;
                            break;
                        }
                    }
                    if (!mapFound) {
                        allValid = false;
                        corruptDetail = std::format("Mission {} references missing map {}", gName, expectedMap);
                        break;
                    }
                }
            }
        }

        if (allValid) {
            for (const auto &mName : mapFiles) {
                size_t mSz = 0;
                std::unique_ptr<uint8_t[]> mDat(rs_utl::File::loadOriginalFileFromDir(originalDir, mName.c_str(), mSz));
                if (!mDat || mSz < 12) {
                    allValid = false;
                    corruptDetail = std::format("Corrupted map data in {}", mName);
                    break;
                }
                uint32_t maxX = rs_utl::READ_LE_UINT32(mDat.get() + 0);
                uint32_t maxY = rs_utl::READ_LE_UINT32(mDat.get() + 4);
                uint32_t maxZ = rs_utl::READ_LE_UINT32(mDat.get() + 8);
                if (maxX == 0 || maxX > 512 || maxY == 0 || maxY > 512 || maxZ == 0 || maxZ > 128) {
                    allValid = false;
                    corruptDetail = std::format("Invalid map grid dimensions in {}", mName);
                    break;
                }
                if (mSz < 12 + (maxX * maxY * 4)) {
                    allValid = false;
                    corruptDetail = std::format("Truncated tile column table in {}", mName);
                    break;
                }
            }
        }

        if (allValid) {
            cMis.isPresent = true;
            cMis.detail = std::format("Verified format across all {} missions and {} maps", mCount, mapCount);
        } else {
            cMis.isPresent = false;
            cMis.detail = corruptDetail;
        }
    }
    res.originalChecks.push_back(cMis);

    res.missingCount = 0;
    for (const auto &c : res.originalChecks) {
        if (!c.isPresent) res.missingCount++;
    }

    // 2. ReSyndicated Bundled Assets
    std::string lngPath = rs_utl::File::getResyndDataFullPath("lang/english.lng");
    std::string wepPath = rs_utl::File::getResyndDataFullPath("ref/weapons.dat");
    std::string chrPath = rs_utl::File::getResyndDataFullPath("ref/characters.dat");
    std::string resPath = rs_utl::File::getResyndDataFullPath("ref/research.dat");
    res.bundledDataValid = fs::exists(lngPath) && fs::exists(wepPath) && fs::exists(chrPath) && fs::exists(resPath);

    res.isValid = (res.missingCount == 0) && res.bundledDataValid;

    if (res.isValid) {
        res.summaryMessage = res.hasAmericanRevolt
            ? std::format(L"\x2713 Syndicate: American Revolt assets verified ({} mission levels detected).", mCount > 0 ? mCount : 1)
            : std::format(L"\x2713 Original Syndicate (Base Game) assets verified ({} mission levels detected).", mCount > 0 ? mCount : 1);
    } else if (!res.bundledDataValid) {
        res.summaryMessage = L"\x2717 Bundled ReSyndicated core data files are missing or incomplete.";
    } else {
        res.summaryMessage = std::format(
            L"\x2717 Missing {} required Syndicate asset file(s). Please specify a valid Syndicate data directory.",
            res.missingCount
        );
    }

    return res;
}

bool browseForFolder(HWND hOwner, const std::wstring &title, std::wstring &outFolder) {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    bool coInitialized = SUCCEEDED(hr) || hr == S_FALSE;

    bool success = false;
    IFileOpenDialog *pFileOpen = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));
    if (SUCCEEDED(hr)) {
        DWORD dwOptions = 0;
        if (SUCCEEDED(pFileOpen->GetOptions(&dwOptions))) {
            pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
        }
        if (!title.empty()) {
            pFileOpen->SetTitle(title.c_str());
        }
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
        bi.lpszTitle = title.c_str();
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

namespace {

struct SettingsDialogState {
    HWND hDlg = nullptr;
    HWND hEditOriginalPath = nullptr;
    HWND hBtnBrowseOriginal = nullptr;
    HWND hLblEdition = nullptr;
    HWND hVerifyPanelOriginal = nullptr;
    HWND hLblSummary = nullptr;
    HWND hBtnOk = nullptr;
    HWND hBtnCancel = nullptr;
    HFONT hFont = nullptr;
    HFONT hFontBold = nullptr;
    HFONT hFontHeader = nullptr;

    bool isSetupFlow = true;
    std::wstring appTitle = L"ReSyndicated";
    std::wstring initialOriginalPath;
    std::wstring currentOriginalPath;
    DataValidationResult lastResult;
    bool accepted = false;
};

static void updateDialogValidation(SettingsDialogState *pState) {
    if (!pState || !pState->hEditOriginalPath) return;

    wchar_t bufOrig[MAX_PATH] = {};
    GetWindowTextW(pState->hEditOriginalPath, bufOrig, MAX_PATH);
    fs::path pOrig(bufOrig);

    pState->lastResult = verifyDataDirectories(pOrig);

    if (pState->hLblEdition) {
        std::wstring editionText;
        if (!pState->lastResult.isValid) {
            editionText = L"Detected Assets:  None (invalid or incomplete data directory)";
        } else if (pState->lastResult.hasAmericanRevolt) {
            editionText = L"Detected Assets:  Syndicate: American Revolt (Expansion Data Disk)";
        } else {
            editionText = L"Detected Assets:  Original Syndicate (Base Game)";
        }
        SetWindowTextW(pState->hLblEdition, editionText.c_str());
        InvalidateRect(pState->hLblEdition, nullptr, TRUE);
    }

    if (pState->hVerifyPanelOriginal) {
        InvalidateRect(pState->hVerifyPanelOriginal, nullptr, FALSE);
    }

    if (pState->hLblSummary) {
        SetWindowTextW(pState->hLblSummary, pState->lastResult.summaryMessage.c_str());
        InvalidateRect(pState->hLblSummary, nullptr, TRUE);
    }

    if (pState->hBtnOk) {
        EnableWindow(pState->hBtnOk, pState->lastResult.isValid ? TRUE : FALSE);
    }
}

static const wchar_t *kVerifyPanelClassName = L"ReSyndicatedSettingsVerifyPanel";

static LRESULT CALLBACK VerifyPanelWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsDialogState *pState = reinterpret_cast<SettingsDialogState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            RECT rcClient;
            GetClientRect(hWnd, &rcClient);
            int w = rcClient.right - rcClient.left;
            int h = rcClient.bottom - rcClient.top;

            if (w <= 0 || h <= 0) {
                EndPaint(hWnd, &ps);
                return 0;
            }

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBm = CreateCompatibleBitmap(hdc, w, h);
            HBITMAP oldBm = (HBITMAP)SelectObject(memDC, memBm);

            HBRUSH hbrBg = CreateSolidBrush(kColorDlgBg);
            FillRect(memDC, &rcClient, hbrBg);
            DeleteObject(hbrBg);

            if (pState && !pState->lastResult.originalChecks.empty()) {
                int count = static_cast<int>(pState->lastResult.originalChecks.size());
                int colWidth = (w - 24) / 2;

                SetBkMode(memDC, TRANSPARENT);

                for (int i = 0; i < count; ++i) {
                    const auto &check = pState->lastResult.originalChecks[i];

                    // 4 items in left column (0..3), 3 items in right column (4..6)
                    int col = (i < 4) ? 0 : 1;
                    int row = (i < 4) ? i : (i - 4);
                    int itemX = col * (colWidth + 24);
                    int itemY = row * 20;

                    RECT rcMark = { itemX, itemY, itemX + 16, itemY + 20 };
                    SelectObject(memDC, pState->hFontBold ? pState->hFontBold : pState->hFont);

                    if (check.isPresent) {
                        SetTextColor(memDC, kColorGreen);
                        DrawTextW(memDC, L"\x2713", -1, &rcMark, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                    } else {
                        SetTextColor(memDC, kColorRed);
                        DrawTextW(memDC, L"\x2717", -1, &rcMark, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
                    }

                    if (pState->hFont) {
                        SelectObject(memDC, pState->hFont);
                    }
                    SetTextColor(memDC, check.isPresent ? kColorText : RGB(248, 113, 113));

                    std::wstring descStr(check.description.begin(), check.description.end());
                    RECT rcText = { itemX + 18, itemY, itemX + colWidth, itemY + 20 };
                    DrawTextW(memDC, descStr.c_str(), -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
                }
            }

            BitBlt(hdc, 0, 0, w, h, memDC, 0, 0, SRCCOPY);

            SelectObject(memDC, oldBm);
            DeleteObject(memBm);
            DeleteDC(memDC);

            EndPaint(hWnd, &ps);
            return 0;
        }

        default:
            return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

static LRESULT CALLBACK SettingsButtonSubclass(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (uMsg) {
        case WM_MOUSEMOVE:
        case WM_MOUSELEAVE:
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

            bool isEnabled = IsWindowEnabled(hWnd) != 0;
            bool isDefault = (GetWindowLongPtrW(hWnd, GWL_STYLE) & BS_DEFPUSHBUTTON) != 0;
            bool isHovered = false;
            POINT pt;
            if (GetCursorPos(&pt) && ScreenToClient(hWnd, &pt)) {
                isHovered = PtInRect(&rc, pt);
            }
            bool isPressed = (SendMessageW(hWnd, BM_GETSTATE, 0, 0) & BST_PUSHED) != 0;

            COLORREF bgCol, borderCol, textCol;
            if (!isEnabled) {
                bgCol = RGB(30, 32, 36);
                borderCol = RGB(46, 50, 56);
                textCol = RGB(110, 118, 128);
            } else if (isPressed) {
                bgCol = RGB(16, 18, 21);
                borderCol = isDefault ? kColorAccentCyan : kColorBorderHighlight;
                textCol = RGB(255, 255, 255);
            } else if (isHovered) {
                bgCol = RGB(45, 52, 62);
                borderCol = isDefault ? kColorAccentCyan : kColorBorderHighlight;
                textCol = RGB(255, 255, 255);
            } else {
                bgCol = RGB(32, 36, 42);
                borderCol = isDefault ? kColorAccentCyan : kColorBorder;
                textCol = isDefault ? RGB(255, 255, 255) : kColorText;
            }

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP hbm = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
            HBITMAP oldBm = (HBITMAP)SelectObject(memDC, hbm);

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

            HFONT hFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
            HFONT oldFont = hFont ? (HFONT)SelectObject(memDC, hFont) : nullptr;

            SetBkMode(memDC, TRANSPARENT);
            SetTextColor(memDC, textCol);

            wchar_t text[128] = {};
            GetWindowTextW(hWnd, text, 128);

            RECT rcText = rc;
            if (isPressed && isEnabled) {
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

        case WM_NCDESTROY:
            RemoveWindowSubclass(hWnd, SettingsButtonSubclass, uIdSubclass);
            break;
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static bool pathsAreDifferentInternal(const std::wstring &p1, const std::wstring &p2) {
    if (p1.empty() && p2.empty()) return false;
    if (p1.empty() || p2.empty()) return true;
    std::error_code ec1, ec2;
    fs::path path1 = fs::weakly_canonical(fs::path(p1), ec1);
    fs::path path2 = fs::weakly_canonical(fs::path(p2), ec2);
    if (!ec1 && !ec2) {
        return _wcsicmp(path1.c_str(), path2.c_str()) != 0;
    }
    return _wcsicmp(p1.c_str(), p2.c_str()) != 0;
}

struct ReloadConfirmState {
    HWND hDlg = nullptr;
    HWND hLblHeader = nullptr;
    HFONT hFont = nullptr;
    HFONT hFontHeader = nullptr;
    bool isEditor = false;
    bool accepted = false;
};

static LRESULT CALLBACK ReloadConfirmDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ReloadConfirmState *pState = reinterpret_cast<ReloadConfirmState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    if (msg == WM_CTLCOLORSTATIC) {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND hCtrl = reinterpret_cast<HWND>(lParam);
        if (pState && hCtrl == pState->hLblHeader) {
            SetTextColor(hdc, RGB(255, 185, 40)); // Amber warning accent
        } else {
            SetTextColor(hdc, kColorText);
        }
        SetBkColor(hdc, kColorDlgBg);
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH s_hbrConfirmBg = CreateSolidBrush(kColorDlgBg);
        return reinterpret_cast<LRESULT>(s_hbrConfirmBg);
    }

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<ReloadConfirmState*>(cs->lpCreateParams);
            pState->hDlg = hWnd;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_CREATE: {
            pState->hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            pState->hFontHeader = CreateFontW(-15, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            HINSTANCE hInst = GetModuleHandleW(nullptr);

            const wchar_t *headerText = pState->isEditor
                ? L"\x26A0 Reload Editor Assets & Mission?"
                : L"\x26A0 Reload Game Assets & Session?";
            pState->hLblHeader = CreateWindowExW(0, L"STATIC", headerText,
                WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
                20, 16, 440, 24, hWnd, nullptr, hInst, nullptr);
            SendMessageW(pState->hLblHeader, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontHeader), TRUE);

            const wchar_t *bodyText = pState->isEditor
                ? L"Changing the game data directory will reload all editor assets and reload the currently open mission from disk.\r\n\r\nAny unsaved edits made to the current mission will be lost.\r\n\r\nDo you want to proceed and reload?"
                : L"Changing the game data directory will immediately reload all game assets and return to the main menu.\r\n\r\nAny unsaved progress in an active mission or campaign session will be lost.\r\n\r\nDo you want to proceed and reload the game?";
            HWND hBody = CreateWindowExW(0, L"STATIC", bodyText,
                WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
                20, 48, 440, 96, hWnd, nullptr, hInst, nullptr);
            SendMessageW(hBody, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);

            HWND hBtnProceed = CreateWindowExW(0, L"BUTTON", L"Proceed",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                258, 154, 98, 30, hWnd, reinterpret_cast<HMENU>(IDOK), hInst, nullptr);
            SendMessageW(hBtnProceed, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            SetWindowSubclass(hBtnProceed, SettingsButtonSubclass, 201, 0);

            HWND hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                366, 154, 94, 30, hWnd, reinterpret_cast<HMENU>(IDCANCEL), hInst, nullptr);
            SendMessageW(hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            SetWindowSubclass(hBtnCancel, SettingsButtonSubclass, 202, 0);

            enableDarkMode(hWnd);
            return 0;
        }

        case WM_CLOSE: {
            pState->accepted = false;
            DestroyWindow(hWnd);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK) {
                pState->accepted = true;
                DestroyWindow(hWnd);
                return 0;
            }
            if (id == IDCANCEL) {
                pState->accepted = false;
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                pState->accepted = false;
                DestroyWindow(hWnd);
                return 0;
            }
            if (wParam == VK_RETURN) {
                pState->accepted = true;
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_DESTROY: {
            if (pState->hFont) { DeleteObject(pState->hFont); pState->hFont = nullptr; }
            if (pState->hFontHeader) { DeleteObject(pState->hFontHeader); pState->hFontHeader = nullptr; }
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool showReloadConfirmDialog(HWND hParent, const std::wstring &appTitle) {
    HINSTANCE hInst = GetModuleHandleW(nullptr);
    static bool s_confirmClassRegistered = false;
    if (!s_confirmClassRegistered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = ReloadConfirmDlgProc;
        wc.hInstance = hInst;
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = L"ReSyndicatedReloadConfirmDialogClass";
        RegisterClassExW(&wc);
        s_confirmClassRegistered = true;
    }

    ReloadConfirmState state;
    state.isEditor = (appTitle.find(L"Editor") != std::wstring::npos);

    DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
    RECT rcDlg = { 0, 0, 480, 200 };
    AdjustWindowRectEx(&rcDlg, dwStyle, FALSE, WS_EX_DLGMODALFRAME);
    int dlgW = rcDlg.right - rcDlg.left;
    int dlgH = rcDlg.bottom - rcDlg.top;

    RECT rcParent = {};
    if (hParent && IsWindow(hParent)) {
        GetWindowRect(hParent, &rcParent);
    } else {
        rcParent.right = GetSystemMetrics(SM_CXSCREEN);
        rcParent.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    int posX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int posY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (posX < 0) posX = 0;
    if (posY < 0) posY = 0;

    std::wstring winTitle = appTitle + L" - Confirm Reload";
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"ReSyndicatedReloadConfirmDialogClass", winTitle.c_str(),
        dwStyle, posX, posY, dlgW, dlgH,
        hParent, nullptr, hInst, &state);
    if (!hDlg) return false;

    if (hParent) {
        EnableWindow(hParent, FALSE);
    }

    MSG msg;
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (hParent) {
        EnableWindow(hParent, TRUE);
        SetForegroundWindow(hParent);
    }

    return state.accepted;
}

static LRESULT CALLBACK SettingsDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    SettingsDialogState *pState = reinterpret_cast<SettingsDialogState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    if (msg == WM_CTLCOLORSTATIC) {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        HWND hCtrl = reinterpret_cast<HWND>(lParam);
        if (pState && hCtrl == pState->hLblSummary) {
            SetTextColor(hdc, pState->lastResult.isValid ? kColorGreen : kColorRed);
        } else if (pState && hCtrl == pState->hLblEdition) {
            if (!pState->lastResult.isValid) {
                SetTextColor(hdc, kColorRed);
            } else if (pState->lastResult.hasAmericanRevolt) {
                SetTextColor(hdc, kColorAccentCyan);
            } else {
                SetTextColor(hdc, RGB(220, 225, 235));
            }
        } else {
            SetTextColor(hdc, kColorText);
        }
        SetBkColor(hdc, kColorDlgBg);
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH s_hbrDlg = CreateSolidBrush(kColorDlgBg);
        return reinterpret_cast<LRESULT>(s_hbrDlg);
    }

    if (msg == WM_CTLCOLOREDIT) {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdc, kColorText);
        SetBkColor(hdc, kColorControlBg);
        static HBRUSH s_hbrEdit = CreateSolidBrush(kColorControlBg);
        return reinterpret_cast<LRESULT>(s_hbrEdit);
    }

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<SettingsDialogState*>(cs->lpCreateParams);
            pState->hDlg = hWnd;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_CREATE: {
            pState->hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            pState->hFontBold = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            pState->hFontHeader = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            HINSTANCE hInst = GetModuleHandleW(nullptr);

            // Title
            std::wstring titleText = pState->isSetupFlow
                ? (pState->appTitle + L" Setup")
                : (pState->appTitle + L" Settings");
            HWND hTitle = CreateWindowExW(0, L"STATIC", titleText.c_str(),
                WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 18, 14, 644, 24, hWnd, nullptr, hInst, nullptr);
            SendMessageW(hTitle, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontHeader), TRUE);

            // Description
            HWND hDesc = CreateWindowExW(0, L"STATIC",
                L"Choose the directory containing Syndicate game data files (original Syndicate, American Revolt, Syndicate Plus, or GOG).",
                WS_CHILD | WS_VISIBLE, 18, 40, 644, 18, hWnd, nullptr, hInst, nullptr);
            SendMessageW(hDesc, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);

            // Section 1: Syndicate Game Data
            HWND hLabelOrig = CreateWindowExW(0, L"STATIC",
                L"Syndicate Game Data Directory:",
                WS_CHILD | WS_VISIBLE, 18, 66, 644, 18, hWnd, nullptr, hInst, nullptr);
            SendMessageW(hLabelOrig, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontBold), TRUE);

            pState->hEditOriginalPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", pState->currentOriginalPath.c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
                18, 88, 544, 26, hWnd, reinterpret_cast<HMENU>(101), hInst, nullptr);
            SendMessageW(pState->hEditOriginalPath, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            SetWindowTheme(pState->hEditOriginalPath, L"DarkMode_Explorer", nullptr);

            pState->hBtnBrowseOriginal = CreateWindowExW(0, L"BUTTON", L"Browse...",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                570, 88, 92, 26, hWnd, reinterpret_cast<HMENU>(102), hInst, nullptr);
            SendMessageW(pState->hBtnBrowseOriginal, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            SetWindowSubclass(pState->hBtnBrowseOriginal, SettingsButtonSubclass, 101, 0);

            // Detected Asset Pack Edition Indicator
            pState->hLblEdition = CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
                18, 120, 644, 20, hWnd, reinterpret_cast<HMENU>(106), hInst, nullptr);
            SendMessageW(pState->hLblEdition, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontBold), TRUE);

            // Original Game Data Verification Checklist (7 items across 2 columns)
            pState->hVerifyPanelOriginal = CreateWindowExW(0, kVerifyPanelClassName, L"",
                WS_CHILD | WS_VISIBLE,
                18, 146, 644, 84, hWnd, reinterpret_cast<HMENU>(201), hInst, pState);

            // Summary Label
            pState->hLblSummary = CreateWindowExW(0, L"STATIC", L"",
                WS_CHILD | WS_VISIBLE | SS_NOPREFIX,
                18, 238, 644, 20, hWnd, reinterpret_cast<HMENU>(107), hInst, nullptr);
            SendMessageW(pState->hLblSummary, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontBold), TRUE);

            // Action Buttons
            std::wstring okText = pState->isSetupFlow ? L"Confirm" : L"Save";
            pState->hBtnOk = CreateWindowExW(0, L"BUTTON", okText.c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                464, 270, 100, 30, hWnd, reinterpret_cast<HMENU>(IDOK), hInst, nullptr);
            SendMessageW(pState->hBtnOk, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            SetWindowSubclass(pState->hBtnOk, SettingsButtonSubclass, 102, 0);

            std::wstring cancelText = pState->isSetupFlow ? L"Exit" : L"Cancel";
            pState->hBtnCancel = CreateWindowExW(0, L"BUTTON", cancelText.c_str(),
                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                574, 270, 88, 30, hWnd, reinterpret_cast<HMENU>(IDCANCEL), hInst, nullptr);
            SendMessageW(pState->hBtnCancel, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFont), TRUE);
            SetWindowSubclass(pState->hBtnCancel, SettingsButtonSubclass, 103, 0);

            updateDialogValidation(pState);
            enableDarkMode(hWnd);
            return 0;
        }

        case WM_CLOSE: {
            pState->accepted = false;
            DestroyWindow(hWnd);
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);

            if (id == 101 && code == EN_CHANGE) {
                updateDialogValidation(pState);
                return 0;
            }

            if (id == 102) {
                std::wstring folder = pState->currentOriginalPath;
                if (browseForFolder(hWnd, L"Select Syndicate Game Data Directory", folder)) {
                    pState->currentOriginalPath = folder;
                    SetWindowTextW(pState->hEditOriginalPath, folder.c_str());
                    updateDialogValidation(pState);
                }
                return 0;
            }

            if (id == IDOK) {
                if (pState->lastResult.isValid) {
                    wchar_t bufOrig[MAX_PATH] = {};
                    GetWindowTextW(pState->hEditOriginalPath, bufOrig, MAX_PATH);
                    std::wstring newPath(bufOrig);
                    if (!pState->isSetupFlow && pathsAreDifferentInternal(pState->initialOriginalPath, newPath)) {
                        if (!showReloadConfirmDialog(hWnd, pState->appTitle)) {
                            return 0;
                        }
                    }
                    pState->currentOriginalPath = newPath;
                    pState->accepted = true;
                    DestroyWindow(hWnd);
                }
                return 0;
            }

            if (id == IDCANCEL) {
                pState->accepted = false;
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                pState->accepted = false;
                DestroyWindow(hWnd);
                return 0;
            }
            if (wParam == VK_RETURN && pState->lastResult.isValid) {
                wchar_t bufOrig[MAX_PATH] = {};
                GetWindowTextW(pState->hEditOriginalPath, bufOrig, MAX_PATH);
                std::wstring newPath(bufOrig);
                if (!pState->isSetupFlow && pathsAreDifferentInternal(pState->initialOriginalPath, newPath)) {
                    if (!showReloadConfirmDialog(hWnd, pState->appTitle)) {
                        return 0;
                    }
                }
                pState->currentOriginalPath = newPath;
                pState->accepted = true;
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_DESTROY: {
            if (pState->hFont) { DeleteObject(pState->hFont); pState->hFont = nullptr; }
            if (pState->hFontBold) { DeleteObject(pState->hFontBold); pState->hFontBold = nullptr; }
            if (pState->hFontHeader) { DeleteObject(pState->hFontHeader); pState->hFontHeader = nullptr; }
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool runSettingsModal(HWND hParent, SettingsDialogState &state) {
    HINSTANCE hInst = GetModuleHandleW(nullptr);

    static bool s_classesRegistered = false;
    if (!s_classesRegistered) {
        WNDCLASSEXW wcDlg = {};
        wcDlg.cbSize = sizeof(WNDCLASSEXW);
        wcDlg.lpfnWndProc = SettingsDlgProc;
        wcDlg.hInstance = hInst;
        wcDlg.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wcDlg.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wcDlg.lpszClassName = L"ReSyndicatedSettingsDialogClass";
        RegisterClassExW(&wcDlg);

        WNDCLASSEXW wcPanel = {};
        wcPanel.cbSize = sizeof(WNDCLASSEXW);
        wcPanel.lpfnWndProc = VerifyPanelWndProc;
        wcPanel.hInstance = hInst;
        wcPanel.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wcPanel.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wcPanel.lpszClassName = kVerifyPanelClassName;
        RegisterClassExW(&wcPanel);

        s_classesRegistered = true;
    }

    DWORD dwStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;
    RECT rcDlg = { 0, 0, 680, 318 };
    AdjustWindowRectEx(&rcDlg, dwStyle, FALSE, WS_EX_DLGMODALFRAME);
    int dlgW = rcDlg.right - rcDlg.left;
    int dlgH = rcDlg.bottom - rcDlg.top;

    RECT rcParent = {};
    if (hParent && IsWindow(hParent)) {
        GetWindowRect(hParent, &rcParent);
    } else {
        rcParent.right = GetSystemMetrics(SM_CXSCREEN);
        rcParent.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    int posX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int posY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (posX < 0) posX = 0;
    if (posY < 0) posY = 0;

    std::wstring winTitle = state.isSetupFlow
        ? (state.appTitle + L" - Setup")
        : (state.appTitle + L" - Settings");

    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME,
        L"ReSyndicatedSettingsDialogClass", winTitle.c_str(),
        dwStyle, posX, posY, dlgW, dlgH,
        hParent, nullptr, hInst, &state);

    if (!hDlg) return false;

    if (hParent) {
        EnableWindow(hParent, FALSE);
    }

    MSG msg;
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (hParent) {
        EnableWindow(hParent, TRUE);
        SetForegroundWindow(hParent);
    }

    return state.accepted;
}

} // anonymous namespace

bool showDataDirSetupDialog(HWND hParent, const std::string &initialPath, std::string &outConfiguredPath, const std::wstring &appTitle) {
    SettingsDialogState state;
    state.isSetupFlow = true;
    state.appTitle = appTitle;
    state.initialOriginalPath = fs::path(initialPath).wstring();
    state.currentOriginalPath = state.initialOriginalPath;

    if (runSettingsModal(hParent, state)) {
        outConfiguredPath = fs::path(state.currentOriginalPath).string();
        return true;
    }
    return false;
}

bool showSettingsDialog(HWND hParent, const std::string &currentPath, std::string &outNewDataDir, const std::wstring &appTitle) {
    SettingsDialogState state;
    state.isSetupFlow = false;
    state.appTitle = appTitle;
    state.initialOriginalPath = fs::path(currentPath).wstring();
    state.currentOriginalPath = state.initialOriginalPath;

    if (runSettingsModal(hParent, state)) {
        outNewDataDir = fs::path(state.currentOriginalPath).string();
        return true;
    }
    return false;
}

bool saveOriginalDataDirToResyndConf(const std::string &dataDir, const std::string &dataSignature) {
    if (dataDir.empty()) return false;

    std::string sig = dataSignature;
    if (sig.empty()) {
        sig = rs_utl::File::computeDataSignature(dataDir);
    }

    rs_utl::File::getOrCreateUserConfFolder("");
    fs::path confPath;
    rs_utl::File::getResyndConfFullPath(confPath);

    rs_utl::ResyndConfig cfg;
    cfg.load(confPath);
    cfg.setString("Paths", "data_dir", dataDir);
    if (!sig.empty()) {
        cfg.setString("Paths", "data_signature", sig);
    }
    return cfg.save(confPath);
}

bool pathsAreDifferent(const std::string &p1, const std::string &p2) {
    if (p1.empty() && p2.empty()) return false;
    if (p1.empty() || p2.empty()) return true;
    std::error_code ec1, ec2;
    fs::path path1 = fs::weakly_canonical(fs::path(p1), ec1);
    fs::path path2 = fs::weakly_canonical(fs::path(p2), ec2);
    if (!ec1 && !ec2) {
        return _wcsicmp(path1.c_str(), path2.c_str()) != 0;
    }
    return _stricmp(p1.c_str(), p2.c_str()) != 0;
}

bool pathsAreDifferent(const std::wstring &p1, const std::wstring &p2) {
    return pathsAreDifferentInternal(p1, p2);
}

} // namespace rs_eng
