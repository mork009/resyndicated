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

#include "playtest_loadout_dialog.h"
#include "editor_config.h"
#include "dialog_theme.h"
#include "sprite_rasterizer.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "rs-utils/io/file.h"
#include "rs-utils/log/log.h"

namespace rs_win_editor {

struct WeaponCatalogItem {
    const wchar_t *displayName;
    const char *idName;
    bool isArOnly;
};

static const WeaponCatalogItem kWeapons[] = {
    { L"Pistol", "pistol", false },
    { L"Minigun", "minigun", false },
    { L"Laser Gun", "laser", false },
    { L"Gauss Gun", "gauss", false },
    { L"Shotgun", "shotgun", false },
    { L"Uzi Submachine Gun", "uzi", false },
    { L"Flamer", "flamer", false },
    { L"Long Range Rifle", "longrange", false },
    { L"Energy Shield", "shield", false },
    { L"MediKit", "medikit", false },
    { L"Persuadatron", "persuadatron", false },
    { L"Scanner", "scanner", false },
    { L"Time Bomb", "timebomb", false },
    { L"Access Card", "accesscard", false },
    { L"Air Raid Com (AR)", "airstrike", true },
    { L"Clone Shield (AR)", "cloneshield", true }
};
static const int kNumWeapons = static_cast<int>(sizeof(kWeapons) / sizeof(kWeapons[0]));

struct CompanyColorInfo {
    const wchar_t *name;
    uint8_t paletteIndex;
};
static const CompanyColorInfo kCompanyColors[8] = {
    { L"Red", 6 },
    { L"Blue Grey", 7 },
    { L"Orange", 14 },
    { L"Light Orange", 3 },
    { L"Yellow", 11 },
    { L"White Blue", 12 },
    { L"Light Brown", 13 },
    { L"Dark Blue", 15 }
};

static const wchar_t *kModLevels[] = {
    L"None (Standard)",
    L"Version 1 (V1)",
    L"Version 2 (V2)",
    L"Version 3 (V3)"
};

PlaytestLoadoutData PlaytestLoadoutDialog::getDefaultLoadout() {
    return EditorConfig::getDefaultLoadout();
}

std::string PlaytestLoadoutDialog::getEffectiveLoadoutPath() {
    std::filesystem::path editorConf = EditorConfig::getConfigPath();
    if (!editorConf.empty() && std::filesystem::exists(editorConf)) {
        return editorConf.string();
    }

    EditorConfig::ensureDefaultLoadoutSection();
    if (!editorConf.empty() && std::filesystem::exists(editorConf)) {
        return editorConf.string();
    }

    return editorConf.string();
}

bool PlaytestLoadoutDialog::loadFromConfigFile(const std::string &path, PlaytestLoadoutData &data) {
    if (path.empty()) return false;
    if (std::filesystem::path(path) == EditorConfig::getConfigPath()) {
        return EditorConfig::loadLoadoutConfig(data);
    }

    std::ifstream file(path);
    if (!file.is_open()) return false;

    data = getDefaultLoadout();

    std::vector<std::pair<std::string, std::string>> entries;
    std::string line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;
        entries.push_back({ line.substr(0, eqPos), line.substr(eqPos + 1) });
    }

    EditorConfig::entriesToLoadoutData(entries, data);
    sanitizeLoadout(data);
    return true;
}

bool PlaytestLoadoutDialog::sanitizeLoadout(PlaytestLoadoutData &data) {
    return EditorConfig::sanitizeLoadout(data);
}

bool PlaytestLoadoutDialog::isAmericanRevoltWeapon(const std::string &weaponName) {
    return EditorConfig::isAmericanRevoltWeapon(weaponName);
}

bool PlaytestLoadoutDialog::saveToConfigFile(const std::string &path, const PlaytestLoadoutData &data) {
    if (path.empty()) return false;

    // Safety guard: NEVER modify sample_loadout.conf under any circumstances
    if (path.find("sample_loadout.conf") != std::string::npos) {
        RSERR(Log::k_FLG_IO, "PlaytestLoadoutDialog", "saveToConfigFile",
              ("Blocked attempt to overwrite sample_loadout.conf: %s", path.c_str()));
        return false;
    }

    std::filesystem::path p(path);
    if (p == EditorConfig::getConfigPath()) {
        return EditorConfig::saveLoadoutConfig(data);
    }

    std::error_code ec;
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path(), ec);
    }

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "# ReSyndicated Playtest Squad Loadout & Mods Configuration\n";
    file << "# Generated by ReSyndicated Editor\n\n";

    auto entries = EditorConfig::loadoutDataToEntries(data);
    for (const auto &kv : entries) {
        file << kv.first << "=" << kv.second << "\n";
    }

    file.close();

    // Also copy to %TEMP%/resynd_playtest/playtest_loadout.conf
    std::filesystem::path tempLoadout = std::filesystem::temp_directory_path() / "resynd_playtest" / "playtest_loadout.conf";
    if (tempLoadout != p) {
        std::filesystem::create_directories(tempLoadout.parent_path(), ec);
        std::filesystem::copy_file(p, tempLoadout, std::filesystem::copy_options::overwrite_existing, ec);
    }

    return true;
}

// -------------------------------------------------------------------------
// Win32 Dialog Implementation
// -------------------------------------------------------------------------

struct LoadoutDlgState {
    HWND hParent = nullptr;
    HWND hDlg = nullptr;
    PlaytestLoadoutData *pData = nullptr;
    int currentAgentIdx = 0;
    bool accepted = false;

    HFONT hFont = nullptr;
    HFONT hBoldFont = nullptr;

    HWND hEditSquadSize = nullptr;
    HWND hBtnAgent[4] = {};
    HWND hGroupAgent = nullptr;
    HWND hCheckActive = nullptr;

    HWND hComboModLegs = nullptr;
    HWND hComboModArms = nullptr;
    HWND hComboModChest = nullptr;
    HWND hComboModHeart = nullptr;
    HWND hComboModEyes = nullptr;
    HWND hComboModBrain = nullptr;

    HWND hBtnModsNone = nullptr;
    HWND hBtnModsV1 = nullptr;
    HWND hBtnModsV2 = nullptr;
    HWND hBtnModsV3 = nullptr;

    HWND hListAvail = nullptr;
    HWND hListEquipped = nullptr;
    HWND hLabelEquippedCount = nullptr;
    HWND hBtnAddWpn = nullptr;
    HWND hBtnRemoveWpn = nullptr;
    HWND hBtnClearWpns = nullptr;

    HWND hBtnPresetDefault = nullptr;
    HWND hBtnPresetAssault = nullptr;
    HWND hBtnPresetStealth = nullptr;
    HWND hBtnCopyAll = nullptr;

    // Display, Zoom & Debug Options
    HWND hComboWindowScale = nullptr;
    HWND hComboMapZoom = nullptr;
    HWND hCheckFullscreen = nullptr;
    HWND hCheckNoSound = nullptr;

    HWND hComboLogOption = nullptr;
    HWND hEditCustomLogFlags = nullptr;
    HWND hCheckShowCoords = nullptr;
    HWND hCheckShowPaths = nullptr;
    HWND hCheckShowStats = nullptr;

    // Faction & Syndicate Attributes
    HWND hGroupFaction = nullptr;
    HWND hEditCompanyName = nullptr;
    HWND hEditFunds = nullptr;
    HWND hComboColor = nullptr;
    HWND hStaticLogoPreview = nullptr;
    HWND hBtnLogoPrev = nullptr;
    HWND hBtnLogoNext = nullptr;
    HWND hLabelLogoIndex = nullptr;
    HBITMAP hCurrentLogoBmp = nullptr;
    int currentFactionColor = 0;
    int currentFactionLogo = 0;
};

static const int IDC_STATIC_SQUAD_SIZE = 2001;
static const int IDC_BTN_AGENT0       = 2002;
static const int IDC_BTN_AGENT1       = 2003;
static const int IDC_BTN_AGENT2       = 2004;
static const int IDC_BTN_AGENT3       = 2005;
static const int IDC_CHECK_ACTIVE     = 2006;
static const int IDC_COMBO_MOD_LEGS   = 2007;
static const int IDC_COMBO_MOD_ARMS   = 2008;
static const int IDC_COMBO_MOD_CHEST  = 2009;
static const int IDC_COMBO_MOD_HEART  = 2010;
static const int IDC_COMBO_MOD_EYES   = 2011;
static const int IDC_COMBO_MOD_BRAIN  = 2012;
static const int IDC_LIST_AVAIL       = 2013;
static const int IDC_LIST_EQUIPPED    = 2014;
static const int IDC_BTN_ADD_WPN      = 2015;
static const int IDC_BTN_REM_WPN      = 2016;
static const int IDC_BTN_CLEAR_WPN    = 2017;
static const int IDC_BTN_PRE_DEFAULT  = 2018;
static const int IDC_BTN_PRE_ASSAULT  = 2019;
static const int IDC_BTN_PRE_STEALTH  = 2020;
static const int IDC_BTN_COPY_ALL     = 2021;
static const int IDC_BTN_MODS_NONE    = 2022;
static const int IDC_BTN_MODS_V1      = 2023;
static const int IDC_BTN_MODS_V2      = 2024;
static const int IDC_BTN_MODS_V3      = 2025;

// Launch options control IDs
static const int IDC_COMBO_WIN_SCALE   = 2030;
static const int IDC_COMBO_MAP_ZOOM    = 2032;
static const int IDC_CHECK_FULLSCREEN  = 2033;
static const int IDC_CHECK_NO_SOUND    = 2034;
static const int IDC_COMBO_LOG_OPTION  = 2035;
static const int IDC_EDIT_LOG_FLAGS    = 2036;
static const int IDC_CHECK_SHOW_COORDS = 2037;
static const int IDC_CHECK_SHOW_PATHS  = 2038;
static const int IDC_CHECK_SHOW_STATS  = 2039;

// Faction & Syndicate control IDs
static const int IDC_GROUP_FACTION      = 2040;
static const int IDC_EDIT_COMP_NAME     = 2041;
static const int IDC_EDIT_FUNDS         = 2042;
static const int IDC_COMBO_COLOR        = 2045;
static const int IDC_STATIC_LOGO_PREV   = 2048;
static const int IDC_BTN_LOGO_PREV      = 2049;
static const int IDC_BTN_LOGO_NEXT      = 2050;
static const int IDC_STATIC_LOGO_LABEL  = 2051;

static void syncFromUIToCurrentAgent(LoadoutDlgState *pState) {
    if (!pState || !pState->pData) return;
    int idx = pState->currentAgentIdx;
    if (idx < 0 || idx >= 4) return;

    auto &ag = pState->pData->agents[idx];
    ag.active = (Button_GetCheck(pState->hCheckActive) == BST_CHECKED);
    ag.modLegs = ComboBox_GetCurSel(pState->hComboModLegs);
    ag.modArms = ComboBox_GetCurSel(pState->hComboModArms);
    ag.modChest = ComboBox_GetCurSel(pState->hComboModChest);
    ag.modHeart = ComboBox_GetCurSel(pState->hComboModHeart);
    ag.modEyes = ComboBox_GetCurSel(pState->hComboModEyes);
    ag.modBrain = ComboBox_GetCurSel(pState->hComboModBrain);

    if (ag.modLegs < 0) ag.modLegs = 0;
    if (ag.modArms < 0) ag.modArms = 0;
    if (ag.modChest < 0) ag.modChest = 0;
    if (ag.modHeart < 0) ag.modHeart = 0;
    if (ag.modEyes < 0) ag.modEyes = 0;
    if (ag.modBrain < 0) ag.modBrain = 0;

    ag.weapons.clear();
    int cnt = ListBox_GetCount(pState->hListEquipped);
    for (int i = 0; i < cnt && i < 8; ++i) {
        LPARAM lp = ListBox_GetItemData(pState->hListEquipped, i);
        if (lp >= 0 && lp < kNumWeapons) {
            ag.weapons.push_back(kWeapons[lp].idName);
        }
    }
}

static void updateActiveSquadCountUI(LoadoutDlgState *pState) {
    if (!pState || !pState->pData || !pState->hEditSquadSize) return;
    int count = 0;
    for (int i = 0; i < 4; ++i) {
        if (pState->pData->agents[i].active) {
            count++;
        }
    }
    pState->pData->activeSquadSize = count;
    std::wstring text = std::to_wstring(count);
    SetWindowTextW(pState->hEditSquadSize, text.c_str());
}

static void setAllModsForCurrentAgent(LoadoutDlgState *pState, int modLevel) {
    if (!pState || !pState->pData) return;
    int idx = pState->currentAgentIdx;
    if (idx < 0 || idx >= 4) return;
    auto &ag = pState->pData->agents[idx];
    ag.modLegs = modLevel;
    ag.modArms = modLevel;
    ag.modChest = modLevel;
    ag.modHeart = modLevel;
    ag.modEyes = modLevel;
    ag.modBrain = modLevel;

    ComboBox_SetCurSel(pState->hComboModLegs, modLevel);
    ComboBox_SetCurSel(pState->hComboModArms, modLevel);
    ComboBox_SetCurSel(pState->hComboModChest, modLevel);
    ComboBox_SetCurSel(pState->hComboModHeart, modLevel);
    ComboBox_SetCurSel(pState->hComboModEyes, modLevel);
    ComboBox_SetCurSel(pState->hComboModBrain, modLevel);
}

static void syncFromUIToLaunchSettings(LoadoutDlgState *pState) {
    if (!pState || !pState->pData) return;
    auto &ls = pState->pData->launch;

    int scaleSel = ComboBox_GetCurSel(pState->hComboWindowScale);
    if (scaleSel >= 0 && scaleSel <= 3) {
        ls.windowScale = scaleSel + 1; // 1x, 2x, 3x, 4x
    } else {
        ls.windowScale = 2; // Default 2x
    }

    int zoomSel = ComboBox_GetCurSel(pState->hComboMapZoom);
    float zooms[] = { 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };
    if (zoomSel >= 0 && zoomSel < 5) {
        ls.mapZoom = zooms[zoomSel];
    } else {
        ls.mapZoom = 1.0f;
    }

    ls.fullscreen = (Button_GetCheck(pState->hCheckFullscreen) == BST_CHECKED);
    ls.noSound = (Button_GetCheck(pState->hCheckNoSound) == BST_CHECKED);

    int logSel = ComboBox_GetCurSel(pState->hComboLogOption);
    const char *logOpts[] = { "None", "ALL", "GAME", "KERNEL", "Custom" };
    if (logSel >= 0 && logSel < 5) {
        ls.logOption = logOpts[logSel];
    } else {
        ls.logOption = "None";
    }

    wchar_t logBuf[128] = {};
    GetWindowTextW(pState->hEditCustomLogFlags, logBuf, 128);
    char cLogBuf[128] = {};
    WideCharToMultiByte(CP_UTF8, 0, logBuf, -1, cLogBuf, sizeof(cLogBuf), nullptr, nullptr);
    ls.customLogFlags = cLogBuf;

    ls.showCoordinates = (Button_GetCheck(pState->hCheckShowCoords) == BST_CHECKED);
    ls.showAgentPaths = (Button_GetCheck(pState->hCheckShowPaths) == BST_CHECKED);
    ls.showStats = (Button_GetCheck(pState->hCheckShowStats) == BST_CHECKED);
}

static void syncLaunchSettingsToUI(LoadoutDlgState *pState) {
    if (!pState || !pState->pData) return;
    const auto &ls = pState->pData->launch;

    if (ls.windowScale >= 1 && ls.windowScale <= 4) {
        ComboBox_SetCurSel(pState->hComboWindowScale, ls.windowScale - 1);
    } else {
        ComboBox_SetCurSel(pState->hComboWindowScale, 1); // 2x default
    }

    float zooms[] = { 1.0f, 1.5f, 2.0f, 2.5f, 3.0f };
    int zoomSel = 0;
    for (int i = 0; i < 5; ++i) {
        if (std::abs(ls.mapZoom - zooms[i]) < 0.05f) {
            zoomSel = i;
            break;
        }
    }
    ComboBox_SetCurSel(pState->hComboMapZoom, zoomSel);

    Button_SetCheck(pState->hCheckFullscreen, ls.fullscreen ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(pState->hCheckNoSound, ls.noSound ? BST_CHECKED : BST_UNCHECKED);

    const char *logOpts[] = { "None", "ALL", "GAME", "KERNEL", "Custom" };
    int logSel = 0;
    for (int i = 0; i < 5; ++i) {
        if (ls.logOption == logOpts[i]) {
            logSel = i;
            break;
        }
    }
    ComboBox_SetCurSel(pState->hComboLogOption, logSel);
    EnableWindow(pState->hEditCustomLogFlags, (logSel == 4) ? TRUE : FALSE);
    SetWindowTextA(pState->hEditCustomLogFlags, ls.customLogFlags.c_str());

    Button_SetCheck(pState->hCheckShowCoords, ls.showCoordinates ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(pState->hCheckShowPaths, ls.showAgentPaths ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(pState->hCheckShowStats, ls.showStats ? BST_CHECKED : BST_UNCHECKED);
}

static void updateLogoPreviewUI(LoadoutDlgState *pState) {
    if (!pState || !pState->hStaticLogoPreview) return;
    int logoId = std::clamp(pState->currentFactionLogo, 0, 39);
    int colorIdx = std::clamp(pState->currentFactionColor, 0, 7);

    HBITMAP hNewBmp = SpriteRasterizer::instance().createLogoBitmap(logoId, colorIdx, 64, 64);
    if (hNewBmp) {
        HBITMAP hOld = reinterpret_cast<HBITMAP>(SendMessageW(pState->hStaticLogoPreview, STM_SETIMAGE, IMAGE_BITMAP, reinterpret_cast<LPARAM>(hNewBmp)));
        if (hOld && hOld != hNewBmp) {
            DeleteObject(hOld);
        }
        if (pState->hCurrentLogoBmp && pState->hCurrentLogoBmp != hNewBmp) {
            DeleteObject(pState->hCurrentLogoBmp);
        }
        pState->hCurrentLogoBmp = hNewBmp;
    }

    if (pState->hLabelLogoIndex) {
        std::wstring logoText = std::format(L"Logo {} / 40", logoId + 1);
        SetWindowTextW(pState->hLabelLogoIndex, logoText.c_str());
    }

    if (pState->hComboColor) {
        ComboBox_SetCurSel(pState->hComboColor, colorIdx);
    }
}

static void syncFactionToUI(LoadoutDlgState *pState) {
    if (!pState || !pState->pData) return;
    const auto &fac = pState->pData->faction;

    pState->currentFactionColor = std::clamp(fac.color, 0, 7);
    pState->currentFactionLogo = std::clamp(fac.logo, 0, 39);

    if (pState->hEditCompanyName) {
        std::wstring wName(fac.companyName.begin(), fac.companyName.end());
        SetWindowTextW(pState->hEditCompanyName, wName.c_str());
    }

    if (pState->hEditFunds) {
        std::wstring wFunds = std::to_wstring((std::max)(0, fac.funds));
        SetWindowTextW(pState->hEditFunds, wFunds.c_str());
    }

    updateLogoPreviewUI(pState);
}

static void syncFromUIToFaction(LoadoutDlgState *pState) {
    if (!pState || !pState->pData) return;
    auto &fac = pState->pData->faction;

    fac.color = std::clamp(pState->currentFactionColor, 0, 7);
    fac.logo = std::clamp(pState->currentFactionLogo, 0, 39);

    if (pState->hEditCompanyName) {
        wchar_t buf[64] = {};
        GetWindowTextW(pState->hEditCompanyName, buf, 64);
        char cBuf[64] = {};
        WideCharToMultiByte(CP_UTF8, 0, buf, -1, cBuf, sizeof(cBuf), nullptr, nullptr);
        std::string s = cBuf;
        if (!s.empty()) {
            fac.companyName = s.substr(0, 16);
        } else {
            fac.companyName = "EUROCORP";
        }
    }

    if (pState->hEditFunds) {
        wchar_t buf[64] = {};
        GetWindowTextW(pState->hEditFunds, buf, 64);
        fac.funds = (std::max)(0, _wtoi(buf));
    }
}

static void updateEquippedUI(LoadoutDlgState *pState) {
    if (!pState || !pState->pData) return;
    int cnt = ListBox_GetCount(pState->hListEquipped);
    std::wstring countStr = std::format(L"Equipped ({}/8):", cnt);
    SetWindowTextW(pState->hLabelEquippedCount, countStr.c_str());
    EnableWindow(pState->hBtnAddWpn, cnt < 8 ? TRUE : FALSE);
}

static void syncFromCurrentAgentToUI(LoadoutDlgState *pState) {
    if (!pState || !pState->pData) return;
    int idx = pState->currentAgentIdx;
    if (idx < 0 || idx >= 4) return;
    const auto &ag = pState->pData->agents[idx];

    // Update Agent group title
    std::wstring groupTitle = std::format(L"Agent {} Loadout", idx + 1);
    SetWindowTextW(pState->hGroupAgent, groupTitle.c_str());

    // Update Agent button labels (remove redundant brackets)
    for (int i = 0; i < 4; ++i) {
        std::wstring btnText = std::format(L"Agent {}", i + 1);
        SetWindowTextW(pState->hBtnAgent[i], btnText.c_str());
        InvalidateRect(pState->hBtnAgent[i], nullptr, FALSE);
    }
    InvalidateRect(pState->hGroupAgent, nullptr, FALSE);

    Button_SetCheck(pState->hCheckActive, ag.active ? BST_CHECKED : BST_UNCHECKED);

    ComboBox_SetCurSel(pState->hComboModLegs, std::clamp(ag.modLegs, 0, 3));
    ComboBox_SetCurSel(pState->hComboModArms, std::clamp(ag.modArms, 0, 3));
    ComboBox_SetCurSel(pState->hComboModChest, std::clamp(ag.modChest, 0, 3));
    ComboBox_SetCurSel(pState->hComboModHeart, std::clamp(ag.modHeart, 0, 3));
    ComboBox_SetCurSel(pState->hComboModEyes, std::clamp(ag.modEyes, 0, 3));
    ComboBox_SetCurSel(pState->hComboModBrain, std::clamp(ag.modBrain, 0, 3));

    ListBox_ResetContent(pState->hListEquipped);
    bool hasAR = rs_utl::File::hasAmericanRevoltData();
    for (const auto &wId : ag.weapons) {
        int catIdx = -1;
        for (int c = 0; c < kNumWeapons; ++c) {
            if (wId == kWeapons[c].idName) {
                catIdx = c;
                break;
            }
        }
        if (catIdx >= 0) {
            if (kWeapons[catIdx].isArOnly && !hasAR) {
                continue;
            }
            int row = ListBox_AddString(pState->hListEquipped, kWeapons[catIdx].displayName);
            ListBox_SetItemData(pState->hListEquipped, row, catIdx);
        }
    }

    updateEquippedUI(pState);
    updateActiveSquadCountUI(pState);
    for (int i = 0; i < 4; ++i) {
        InvalidateRect(pState->hBtnAgent[i], nullptr, TRUE);
    }
}

static void addSelectedWeapon(LoadoutDlgState *pState) {
    int curSel = ListBox_GetCurSel(pState->hListAvail);
    if (curSel < 0) return;
    int catIdx = static_cast<int>(ListBox_GetItemData(pState->hListAvail, curSel));
    if (catIdx < 0 || catIdx >= kNumWeapons) return;

    int curCount = ListBox_GetCount(pState->hListEquipped);
    if (curCount >= 8) return;

    int row = ListBox_AddString(pState->hListEquipped, kWeapons[catIdx].displayName);
    ListBox_SetItemData(pState->hListEquipped, row, catIdx);
    ListBox_SetCurSel(pState->hListEquipped, row);

    updateEquippedUI(pState);
}

static void removeSelectedWeapon(LoadoutDlgState *pState) {
    int curSel = ListBox_GetCurSel(pState->hListEquipped);
    if (curSel < 0) return;

    ListBox_DeleteString(pState->hListEquipped, curSel);
    int newCount = ListBox_GetCount(pState->hListEquipped);
    if (newCount > 0) {
        int nextSel = (curSel < newCount) ? curSel : (newCount - 1);
        ListBox_SetCurSel(pState->hListEquipped, nextSel);
    }

    updateEquippedUI(pState);
}

static LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    LoadoutDlgState *pState = reinterpret_cast<LoadoutDlgState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<LoadoutDlgState*>(cs->lpCreateParams);
            pState->hDlg = hWnd;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_CREATE: {
            HINSTANCE hInst = GetModuleHandleW(nullptr);

            SetWindowTextW(hWnd, L"Playtest Configuration");

            NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
            SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);
            pState->hFont = CreateFontIndirectW(&ncm.lfMessageFont);

            LOGFONTW lfBold = ncm.lfMessageFont;
            lfBold.lfWeight = FW_BOLD;
            pState->hBoldFont = CreateFontIndirectW(&lfBold);

            // 1. Top Section - Squad Size & Agent Tabs
            CreateWindowExW(0, WC_STATICW, L"Active Squad Size:",
                WS_CHILD | WS_VISIBLE, 20, 16, 115, 20, hWnd, nullptr, hInst, nullptr);

            pState->hEditSquadSize = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"4",
                WS_CHILD | WS_VISIBLE | ES_CENTER | ES_READONLY | WS_DISABLED,
                138, 15, 26, 20, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATIC_SQUAD_SIZE)), hInst, nullptr);

            int btnX = 245;
            for (int i = 0; i < 4; ++i) {
                std::wstring t = std::format(L"Agent {}", i + 1);
                pState->hBtnAgent[i] = CreateWindowExW(0, WC_BUTTONW, t.c_str(),
                    WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                    btnX, 12, 60, 26, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_AGENT0 + i)), hInst, nullptr);
                btnX += 65;
            }

            // 2. Main Agent Group Box
            pState->hGroupAgent = CreateWindowExW(0, WC_BUTTONW, L"Agent 1 Loadout",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_CLIPSIBLINGS,
                20, 46, 510, 452, hWnd, nullptr, hInst, nullptr);

            pState->hCheckActive = CreateWindowExW(0, WC_BUTTONW, L"Agent Active in Squad",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                36, 66, 180, 22, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHECK_ACTIVE)), hInst, nullptr);

            pState->hBtnCopyAll = CreateWindowExW(0, WC_BUTTONW, L"Copy to All Agents",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                365, 64, 150, 26, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_COPY_ALL)), hInst, nullptr);

            // 2a. Cybernetic Mods Group Box
            CreateWindowExW(0, WC_BUTTONW, L"Cybernetic Modifications",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_CLIPSIBLINGS,
                36, 94, 478, 138, hWnd, nullptr, hInst, nullptr);

            // Mod Combos: Col 1 (Legs, Arms, Chest)
            CreateWindowExW(0, WC_STATICW, L"Legs:", WS_CHILD | WS_VISIBLE, 50, 116, 45, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboModLegs = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 100, 113, 135, 120, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MOD_LEGS)), hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Arms:", WS_CHILD | WS_VISIBLE, 50, 143, 45, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboModArms = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 100, 140, 135, 120, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MOD_ARMS)), hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Chest:", WS_CHILD | WS_VISIBLE, 50, 170, 45, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboModChest = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 100, 167, 135, 120, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MOD_CHEST)), hInst, nullptr);

            // Mod Combos: Col 2 (Heart, Eyes, Brain)
            CreateWindowExW(0, WC_STATICW, L"Heart:", WS_CHILD | WS_VISIBLE, 275, 116, 45, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboModHeart = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 325, 113, 135, 120, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MOD_HEART)), hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Eyes:", WS_CHILD | WS_VISIBLE, 275, 143, 45, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboModEyes = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 325, 140, 135, 120, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MOD_EYES)), hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Brain:", WS_CHILD | WS_VISIBLE, 275, 170, 45, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboModBrain = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 325, 167, 135, 120, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MOD_BRAIN)), hInst, nullptr);

            // Mod Toggle Buttons: "All No Mods", "All V1", "All V2", "All V3"
            pState->hBtnModsNone = CreateWindowExW(0, WC_BUTTONW, L"All No Mods",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                50, 198, 75, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_MODS_NONE)), hInst, nullptr);

            pState->hBtnModsV1 = CreateWindowExW(0, WC_BUTTONW, L"All V1",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                135, 198, 75, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_MODS_V1)), hInst, nullptr);

            pState->hBtnModsV2 = CreateWindowExW(0, WC_BUTTONW, L"All V2",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                220, 198, 75, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_MODS_V2)), hInst, nullptr);

            pState->hBtnModsV3 = CreateWindowExW(0, WC_BUTTONW, L"All V3",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                305, 198, 75, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_MODS_V3)), hInst, nullptr);

            HWND modCombos[] = {
                pState->hComboModLegs, pState->hComboModArms, pState->hComboModChest,
                pState->hComboModHeart, pState->hComboModEyes, pState->hComboModBrain
            };
            for (HWND hC : modCombos) {
                for (int m = 0; m < 4; ++m) {
                    ComboBox_AddString(hC, kModLevels[m]);
                }
            }

            // 2b. Weapons & Items Group Box
            CreateWindowExW(0, WC_BUTTONW, L"Weapons && Equipment (Max 8)",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_CLIPSIBLINGS,
                36, 238, 478, 250, hWnd, nullptr, hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Available Items:", WS_CHILD | WS_VISIBLE, 50, 258, 140, 18, hWnd, nullptr, hInst, nullptr);
            pState->hListAvail = CreateWindowExW(0, WC_LISTBOXW, L"",
                WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
                50, 278, 160, 160, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LIST_AVAIL)), hInst, nullptr);

            bool hasAR = rs_utl::File::hasAmericanRevoltData();
            for (int i = 0; i < kNumWeapons; ++i) {
                if (kWeapons[i].isArOnly && !hasAR) {
                    continue;
                }
                int row = ListBox_AddString(pState->hListAvail, kWeapons[i].displayName);
                ListBox_SetItemData(pState->hListAvail, row, i);
            }
            ListBox_SetCurSel(pState->hListAvail, 0);

            pState->hBtnAddWpn = CreateWindowExW(0, WC_BUTTONW, L"Add ->",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                222, 320, 66, 26, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_ADD_WPN)), hInst, nullptr);

            pState->hBtnRemoveWpn = CreateWindowExW(0, WC_BUTTONW, L"<- Del",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                222, 355, 66, 26, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_REM_WPN)), hInst, nullptr);

            pState->hLabelEquippedCount = CreateWindowExW(0, WC_STATICW, L"Equipped (0/8):",
                WS_CHILD | WS_VISIBLE, 300, 258, 120, 18, hWnd, nullptr, hInst, nullptr);

            pState->hListEquipped = CreateWindowExW(0, WC_LISTBOXW, L"",
                WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
                300, 278, 150, 160, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LIST_EQUIPPED)), hInst, nullptr);

            pState->hBtnClearWpns = CreateWindowExW(0, WC_BUTTONW, L"Clear",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                458, 320, 48, 26, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CLEAR_WPN)), hInst, nullptr);

            // Presets
            pState->hBtnPresetDefault = CreateWindowExW(0, WC_BUTTONW, L"Pistol Only",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                50, 450, 85, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRE_DEFAULT)), hInst, nullptr);

            pState->hBtnPresetAssault = CreateWindowExW(0, WC_BUTTONW, L"Heavy Assault",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                145, 450, 100, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRE_ASSAULT)), hInst, nullptr);

            pState->hBtnPresetStealth = CreateWindowExW(0, WC_BUTTONW, L"Stealth && Persuade",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                255, 450, 135, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PRE_STEALTH)), hInst, nullptr);

            // 3. Right Side Section: Syndicate Faction & Economy
            pState->hGroupFaction = CreateWindowExW(0, WC_BUTTONW, L"Syndicate Faction && Economy",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_CLIPSIBLINGS,
                545, 46, 315, 208, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GROUP_FACTION)), hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Name:", WS_CHILD | WS_VISIBLE, 560, 69, 50, 18, hWnd, nullptr, hInst, nullptr);
            pState->hEditCompanyName = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"EUROCORP",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                615, 66, 230, 22, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_COMP_NAME)), hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Funds ($):", WS_CHILD | WS_VISIBLE, 560, 97, 65, 18, hWnd, nullptr, hInst, nullptr);
            pState->hEditFunds = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"100000",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER,
                630, 94, 215, 22, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_FUNDS)), hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Color:", WS_CHILD | WS_VISIBLE, 560, 126, 45, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboColor = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                615, 123, 230, 140, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_COLOR)), hInst, nullptr);
            for (int c = 0; c < 8; ++c) {
                ComboBox_AddString(pState->hComboColor, kCompanyColors[c].name);
            }

            CreateWindowExW(0, WC_STATICW, L"Logo:", WS_CHILD | WS_VISIBLE, 560, 168, 42, 18, hWnd, nullptr, hInst, nullptr);

            pState->hBtnLogoPrev = CreateWindowExW(0, WC_BUTTONW, L"<",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                608, 168, 24, 32, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_LOGO_PREV)), hInst, nullptr);

            pState->hStaticLogoPreview = CreateWindowExW(WS_EX_STATICEDGE, WC_STATICW, L"",
                WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE,
                638, 152, 64, 64, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATIC_LOGO_PREV)), hInst, nullptr);

            pState->hBtnLogoNext = CreateWindowExW(0, WC_BUTTONW, L">",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                708, 168, 24, 32, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_LOGO_NEXT)), hInst, nullptr);

            pState->hLabelLogoIndex = CreateWindowExW(0, WC_STATICW, L"Logo 1 / 40",
                WS_CHILD | WS_VISIBLE,
                740, 174, 105, 20, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATIC_LOGO_LABEL)), hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"* Air Raid Com requires $50,000 funds.",
                WS_CHILD | WS_VISIBLE,
                560, 226, 285, 18, hWnd, nullptr, hInst, nullptr);

            // 4. Right Side Section: Launch & Debug Options
            CreateWindowExW(0, WC_BUTTONW, L"Launch && Debug Options",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_CLIPSIBLINGS,
                545, 260, 315, 238, hWnd, nullptr, hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Scale:", WS_CHILD | WS_VISIBLE, 560, 282, 42, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboWindowScale = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                605, 279, 90, 120, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_WIN_SCALE)), hInst, nullptr);
            ComboBox_AddString(pState->hComboWindowScale, L"1x (640)");
            ComboBox_AddString(pState->hComboWindowScale, L"2x (1280)");
            ComboBox_AddString(pState->hComboWindowScale, L"3x (1920)");
            ComboBox_AddString(pState->hComboWindowScale, L"4x (2560)");

            CreateWindowExW(0, WC_STATICW, L"Zoom:", WS_CHILD | WS_VISIBLE, 705, 282, 42, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboMapZoom = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                750, 279, 95, 120, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MAP_ZOOM)), hInst, nullptr);
            ComboBox_AddString(pState->hComboMapZoom, L"1.0x (Def)");
            ComboBox_AddString(pState->hComboMapZoom, L"1.5x");
            ComboBox_AddString(pState->hComboMapZoom, L"2.0x");
            ComboBox_AddString(pState->hComboMapZoom, L"2.5x");
            ComboBox_AddString(pState->hComboMapZoom, L"3.0x");

            pState->hCheckFullscreen = CreateWindowExW(0, WC_BUTTONW, L"Fullscreen",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                560, 309, 135, 22, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHECK_FULLSCREEN)), hInst, nullptr);

            pState->hCheckNoSound = CreateWindowExW(0, WC_BUTTONW, L"Disable Sound",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                705, 309, 140, 22, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHECK_NO_SOUND)), hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Log (-l):", WS_CHILD | WS_VISIBLE, 560, 338, 55, 18, hWnd, nullptr, hInst, nullptr);
            pState->hComboLogOption = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
                620, 335, 95, 120, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_LOG_OPTION)), hInst, nullptr);
            ComboBox_AddString(pState->hComboLogOption, L"None");
            ComboBox_AddString(pState->hComboLogOption, L"ALL");
            ComboBox_AddString(pState->hComboLogOption, L"GAME");
            ComboBox_AddString(pState->hComboLogOption, L"KERNEL");
            ComboBox_AddString(pState->hComboLogOption, L"Custom");

            pState->hEditCustomLogFlags = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                725, 335, 120, 22, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_LOG_FLAGS)), hInst, nullptr);

            pState->hCheckShowCoords = CreateWindowExW(0, WC_BUTTONW, L"Show Tile Coordinates",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                560, 368, 275, 20, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHECK_SHOW_COORDS)), hInst, nullptr);

            pState->hCheckShowPaths = CreateWindowExW(0, WC_BUTTONW, L"Show Agent Paths",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                560, 394, 275, 20, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHECK_SHOW_PATHS)), hInst, nullptr);

            pState->hCheckShowStats = CreateWindowExW(0, WC_BUTTONW, L"Show Stats Overlay",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                560, 420, 275, 20, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHECK_SHOW_STATS)), hInst, nullptr);

            // 5. Bottom Row: Save, Cancel
            CreateWindowExW(0, WC_BUTTONW, L"Save",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                595, 514, 150, 30, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDOK)), hInst, nullptr);

            CreateWindowExW(0, WC_BUTTONW, L"Cancel",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                755, 514, 100, 30, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDCANCEL)), hInst, nullptr);

            // Apply font to all child controls
            EnumChildWindows(hWnd, [](HWND hChild, LPARAM lp) -> BOOL {
                HFONT font = reinterpret_cast<HFONT>(lp);
                SendMessageW(hChild, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                return TRUE;
            }, reinterpret_cast<LPARAM>(pState->hFont));

            syncFromCurrentAgentToUI(pState);
            syncLaunchSettingsToUI(pState);
            syncFactionToUI(pState);
            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_DRAWITEM: {
            DRAWITEMSTRUCT *pDIS = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            if (pDIS && pDIS->CtlType == ODT_BUTTON && pDIS->CtlID >= IDC_BTN_AGENT0 && pDIS->CtlID <= IDC_BTN_AGENT3) {
                int agentIdx = pDIS->CtlID - IDC_BTN_AGENT0;
                bool isActive = (pState && agentIdx == pState->currentAgentIdx);

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
                HFONT fontToUse = (isActive && pState->hBoldFont) ? pState->hBoldFont : pState->hFont;
                HGDIOBJ oldFont = SelectObject(hdc, fontToUse);

                std::wstring text = std::format(L"Agent {}", agentIdx + 1);
                DrawTextW(hdc, text.c_str(), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hdc, oldFont);
                return TRUE;
            }
            break;
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);

            if (id == IDC_COMBO_LOG_OPTION && code == CBN_SELCHANGE) {
                int sel = ComboBox_GetCurSel(pState->hComboLogOption);
                EnableWindow(pState->hEditCustomLogFlags, sel == 4 ? TRUE : FALSE);
                return 0;
            }

            if (id == IDC_CHECK_ACTIVE) {
                pState->pData->agents[pState->currentAgentIdx].active = (Button_GetCheck(pState->hCheckActive) == BST_CHECKED);
                updateActiveSquadCountUI(pState);
                return 0;
            }

            if (id >= IDC_BTN_AGENT0 && id <= IDC_BTN_AGENT3) {
                int newAgentIdx = id - IDC_BTN_AGENT0;
                if (newAgentIdx != pState->currentAgentIdx) {
                    syncFromUIToCurrentAgent(pState);
                    pState->currentAgentIdx = newAgentIdx;
                    syncFromCurrentAgentToUI(pState);
                    for (int i = 0; i < 4; ++i) {
                        InvalidateRect(pState->hBtnAgent[i], nullptr, TRUE);
                    }
                }
                return 0;
            }

            if (id == IDC_BTN_MODS_NONE) {
                setAllModsForCurrentAgent(pState, 0);
                return 0;
            }

            if (id == IDC_BTN_MODS_V1) {
                setAllModsForCurrentAgent(pState, 1);
                return 0;
            }

            if (id == IDC_BTN_MODS_V2) {
                setAllModsForCurrentAgent(pState, 2);
                return 0;
            }

            if (id == IDC_BTN_MODS_V3) {
                setAllModsForCurrentAgent(pState, 3);
                return 0;
            }

            if (id == IDC_LIST_AVAIL && code == LBN_DBLCLK) {
                addSelectedWeapon(pState);
                return 0;
            }

            if (id == IDC_LIST_EQUIPPED && code == LBN_DBLCLK) {
                removeSelectedWeapon(pState);
                return 0;
            }

            if (id == IDC_BTN_ADD_WPN) {
                addSelectedWeapon(pState);
                return 0;
            }

            if (id == IDC_BTN_REM_WPN) {
                removeSelectedWeapon(pState);
                return 0;
            }

            if (id == IDC_BTN_CLEAR_WPN) {
                ListBox_ResetContent(pState->hListEquipped);
                updateEquippedUI(pState);
                return 0;
            }

            if (id == IDC_BTN_PRE_DEFAULT) {
                ListBox_ResetContent(pState->hListEquipped);
                int row = ListBox_AddString(pState->hListEquipped, L"Pistol");
                ListBox_SetItemData(pState->hListEquipped, row, 0);
                updateEquippedUI(pState);
                return 0;
            }

            if (id == IDC_BTN_PRE_ASSAULT) {
                ListBox_ResetContent(pState->hListEquipped);
                const char *preset[] = { "minigun", "laser", "shield", "medikit", "gauss" };
                for (const char *wId : preset) {
                    for (int c = 0; c < kNumWeapons; ++c) {
                        if (std::string(wId) == kWeapons[c].idName) {
                            int row = ListBox_AddString(pState->hListEquipped, kWeapons[c].displayName);
                            ListBox_SetItemData(pState->hListEquipped, row, c);
                            break;
                        }
                    }
                }
                updateEquippedUI(pState);
                return 0;
            }

            if (id == IDC_BTN_PRE_STEALTH) {
                ListBox_ResetContent(pState->hListEquipped);
                const char *preset[] = { "persuadatron", "scanner", "shield", "medikit", "pistol" };
                for (const char *wId : preset) {
                    for (int c = 0; c < kNumWeapons; ++c) {
                        if (std::string(wId) == kWeapons[c].idName) {
                            int row = ListBox_AddString(pState->hListEquipped, kWeapons[c].displayName);
                            ListBox_SetItemData(pState->hListEquipped, row, c);
                            break;
                        }
                    }
                }
                updateEquippedUI(pState);
                return 0;
            }

            if (id == IDC_BTN_COPY_ALL) {
                syncFromUIToCurrentAgent(pState);
                const auto &src = pState->pData->agents[pState->currentAgentIdx];
                for (int i = 0; i < 4; ++i) {
                    pState->pData->agents[i].modLegs = src.modLegs;
                    pState->pData->agents[i].modArms = src.modArms;
                    pState->pData->agents[i].modChest = src.modChest;
                    pState->pData->agents[i].modHeart = src.modHeart;
                    pState->pData->agents[i].modEyes = src.modEyes;
                    pState->pData->agents[i].modBrain = src.modBrain;
                    pState->pData->agents[i].weapons = src.weapons;
                }
                return 0;
            }

            if (id == IDC_COMBO_COLOR && code == CBN_SELCHANGE) {
                int sel = ComboBox_GetCurSel(pState->hComboColor);
                if (sel >= 0 && sel < 8) {
                    pState->currentFactionColor = sel;
                    updateLogoPreviewUI(pState);
                }
                return 0;
            }

            if (id == IDC_BTN_LOGO_PREV) {
                pState->currentFactionLogo = (pState->currentFactionLogo + 39) % 40;
                updateLogoPreviewUI(pState);
                return 0;
            }

            if (id == IDC_BTN_LOGO_NEXT) {
                pState->currentFactionLogo = (pState->currentFactionLogo + 1) % 40;
                updateLogoPreviewUI(pState);
                return 0;
            }

            if (id == IDOK) {
                syncFromUIToCurrentAgent(pState);
                syncFromUIToLaunchSettings(pState);
                syncFromUIToFaction(pState);

                static constexpr uint8_t kColours[8] = { 6, 7, 14, 3, 11, 12, 13, 15 };
                SpriteRasterizer::setPlayerFactionColor(kColours[pState->pData->faction.color]);

                // Validation 1: At least 1 active agent
                bool anyActive = false;
                for (int i = 0; i < 4; ++i) {
                    if (pState->pData->agents[i].active) {
                        anyActive = true;
                        break;
                    }
                }
                if (!anyActive) {
                    showEditorMessageBox(hWnd,
                        L"At least one agent must be active in the squad for playtesting.\n\nPlease enable at least one agent.",
                        L"Invalid Squad Loadout", MB_ICONWARNING | MB_OK);
                    return 0;
                }

                // Validation 2: Weapon count check (<= 8 per agent)
                for (int i = 0; i < 4; ++i) {
                    if (pState->pData->agents[i].weapons.size() > 8) {
                        std::wstring warnMsg = std::format(L"Agent {} has more than 8 weapons equipped.\n\nPlease reduce weapons to at most 8.", i + 1);
                        showEditorMessageBox(hWnd, warnMsg.c_str(), L"Invalid Loadout", MB_ICONWARNING | MB_OK);
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
            if (pState->hCurrentLogoBmp) {
                DeleteObject(pState->hCurrentLogoBmp);
                pState->hCurrentLogoBmp = nullptr;
            }
            if (pState->hFont) DeleteObject(pState->hFont);
            if (pState->hBoldFont) DeleteObject(pState->hBoldFont);
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

bool PlaytestLoadoutDialog::show(HWND hParent, PlaytestLoadoutData &data) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedPlaytestLoadoutDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = DlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    sanitizeLoadout(data);

    LoadoutDlgState state;
    state.hParent = hParent;
    state.pData = &data;
    state.currentAgentIdx = 0;

    RECT rcParent = {};
    GetWindowRect(hParent, &rcParent);
    int dlgW = 890;
    int dlgH = 615;
    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (dlgX < 0) dlgX = 0;
    if (dlgY < 0) dlgY = 0;

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        kClassName,
        L"Playtest Configuration",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dlgX, dlgY, dlgW, dlgH,
        hParent, nullptr, hInstance, &state
    );

    if (!hDlg) return false;

    SetWindowTextW(hDlg, L"Playtest Configuration");
    applyDialogTheme(hDlg);

    EnableWindow(hParent, FALSE);
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

    EnableWindow(hParent, TRUE);
    SetForegroundWindow(hParent);
    SetFocus(hParent);

    return state.accepted;
}

} // namespace rs_win_editor
