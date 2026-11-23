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

#include "mission_properties_dialog.h"
#include "dialog_theme.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <format>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

#include "rs-utils/io/file.h"
#include "rs-utils/io/dernc.h"

namespace rs_win_editor {

namespace {

std::wstring toWString(const std::string &str) {
    if (str.empty()) return L"";
    int numChars = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
    if (numChars <= 0) {
        return std::wstring(str.begin(), str.end());
    }
    std::wstring result(numChars, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &result[0], numChars);
    return result;
}

std::string fromWString(const std::wstring &wstr) {
    if (wstr.empty()) return "";
    int numBytes = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), nullptr, 0, nullptr, nullptr);
    if (numBytes <= 0) {
        std::string fallback;
        fallback.reserve(wstr.size());
        for (wchar_t wc : wstr) fallback.push_back(static_cast<char>(wc & 0xFF));
        return fallback;
    }
    std::string result(numBytes, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.length()), &result[0], numBytes, nullptr, nullptr);
    return result;
}

std::wstring normalizeLineEndingsToWin(const std::string &s) {
    std::wstring ws = toWString(s);
    std::wstring out;
    out.reserve(ws.size() + 32);
    for (size_t i = 0; i < ws.size(); ++i) {
        if (ws[i] == L'\r') {
            continue;
        }
        if (ws[i] == L'\n') {
            out += L"\r\n";
        } else {
            out += ws[i];
        }
    }
    return out;
}

std::string normalizeLineEndingsToUnix(const std::wstring &ws) {
    std::wstring clean;
    clean.reserve(ws.size());
    for (size_t i = 0; i < ws.size(); ++i) {
        if (ws[i] == L'\r') continue;
        clean += ws[i];
    }
    return fromWString(clean);
}

static int readTechLevelFromGameFile(const std::string &path) {
    if (path.empty() || !std::filesystem::exists(path)) return -1;
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return -1;
    std::streamsize sz = in.tellg();
    if (sz <= 0) return -1;
    in.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(sz));
    if (!in.read(reinterpret_cast<char*>(buffer.data()), sz)) return -1;

    const size_t kTechLevelOffset = 113960 + 11; // mapinfos.unkn1[0]
    if (rnc::isRncCompressed(buffer.data())) {
        size_t unpLen = 0;
        if (rnc::unpackedLength(buffer.data(), unpLen) == rnc::kOk && unpLen > kTechLevelOffset) {
            std::vector<uint8_t> unpacked(unpLen);
            size_t realSize = 0;
            if (rnc::unpack(buffer.data(), unpacked.data(), realSize) >= 0 && realSize > kTechLevelOffset) {
                int tier = unpacked[kTechLevelOffset];
                return (tier >= 0 && tier <= 3) ? tier : 0;
            }
        }
    } else if (buffer.size() > kTechLevelOffset) {
        int tier = buffer[kTechLevelOffset];
        return (tier >= 0 && tier <= 3) ? tier : 0;
    }
    return -1;
}

struct PropDialogState {
    HWND hParent = nullptr;
    HWND hDlg = nullptr;

    HWND hComboLang = nullptr;
    HWND hComboTechLevel = nullptr;
    HWND hStaticTechDesc = nullptr;

    HWND hEditInfo1 = nullptr;
    HWND hEditInfo2 = nullptr;
    HWND hEditInfo3 = nullptr;
    HWND hEditEnh1 = nullptr;
    HWND hEditEnh2 = nullptr;
    HWND hEditEnh3 = nullptr;

    HWND hEditBaseBrief = nullptr;
    HWND hEditInfo1Brief = nullptr;
    HWND hEditInfo2Brief = nullptr;
    HWND hEditInfo3Brief = nullptr;

    HWND hBtnSave = nullptr;
    HWND hBtnReload = nullptr;
    HWND hBtnCancel = nullptr;

    HWND hLabelMissionId = nullptr;
    HWND hLabelBaseBrief = nullptr;
    HWND hLabelInfo1Brief = nullptr;
    HWND hLabelInfo2Brief = nullptr;
    HWND hLabelInfo3Brief = nullptr;

    HFONT hFont = nullptr;
    HFONT hFontBold = nullptr;
    MissionPropertiesData *pData = nullptr;
    bool accepted = false;
};

static void setDefaultBriefingTemplateForLang(MissionBriefingLang &lang) {
    lang.available = true;
    lang.modified = false;
    lang.sessionEdited = false;
    lang.baseBriefing = "MISSION OBJECTIVE:\nInfiltrate the target area and complete all assigned tasks.\nEliminate any hostile syndicate agents.";
    lang.info1Briefing = "INTEL LEVEL 1:\nSecurity forces have been alerted. Patrols frequent the main road.";
    lang.info2Briefing = "INTEL LEVEL 2:\nTarget coordinates confirmed. Heavy vehicle parked nearby.";
    lang.info3Briefing = "TACTICS:\nA single agent can infiltrate the area discreetly. Maintain backup outside if needed.";
    lang.infoCostLines = { "100", "250", "400" };
    lang.enhCostLines = { "100", "250", "400" };
    lang.briefingSections = { lang.baseBriefing, lang.info1Briefing, lang.info2Briefing, lang.info3Briefing };
}

static void setDefaultBriefingTemplate(MissionPropertiesData &data) {
    data.infoCost1 = 100;
    data.infoCost2 = 250;
    data.infoCost3 = 400;
    data.enhCost1 = 100;
    data.enhCost2 = 250;
    data.enhCost3 = 400;
    int curLang = (data.language >= 0 && data.language <= 3) ? data.language : 0;
    setDefaultBriefingTemplateForLang(data.langs[curLang]);
    data.syncLangToActive(curLang);
}

static void updateTechLevelDescription(PropDialogState *pState) {
    if (!pState || !pState->hStaticTechDesc || !pState->hComboTechLevel) return;
    int tier = ComboBox_GetCurSel(pState->hComboTechLevel);
    bool isCampaign = (pState->pData && pState->pData->missionId >= 1 && pState->pData->missionId <= 50 && !pState->pData->gameFilePath.empty());
    std::wstring desc;
    switch (tier) {
        case 0:
            desc = isCampaign ? L"(Dynamic: World Map progression)" : L"(Unaugmented: no cyberware)";
            break;
        case 1:
            desc = L"(All enemy agents receive V1 mods)";
            break;
        case 2:
            desc = L"(All enemy agents receive V2 mods)";
            break;
        case 3:
            desc = L"(All enemy agents receive V3 mods)";
            break;
        default:
            desc = L"";
            break;
    }
    SetWindowTextW(pState->hStaticTechDesc, desc.c_str());
}

void populateFields(PropDialogState *pState) {
    if (!pState || !pState->pData) return;
    const auto &d = *pState->pData;

    ComboBox_SetCurSel(pState->hComboLang, d.language);
    ComboBox_SetCurSel(pState->hComboTechLevel, d.techLevel);
    updateTechLevelDescription(pState);

    SetWindowTextW(pState->hEditInfo1, std::to_wstring(d.infoCost1).c_str());
    SetWindowTextW(pState->hEditInfo2, std::to_wstring(d.infoCost2).c_str());
    SetWindowTextW(pState->hEditInfo3, std::to_wstring(d.infoCost3).c_str());
    SetWindowTextW(pState->hEditEnh1, std::to_wstring(d.enhCost1).c_str());
    SetWindowTextW(pState->hEditEnh2, std::to_wstring(d.enhCost2).c_str());
    SetWindowTextW(pState->hEditEnh3, std::to_wstring(d.enhCost3).c_str());

    SetWindowTextW(pState->hEditBaseBrief, normalizeLineEndingsToWin(d.baseBriefing).c_str());
    SetWindowTextW(pState->hEditInfo1Brief, normalizeLineEndingsToWin(d.info1Briefing).c_str());
    SetWindowTextW(pState->hEditInfo2Brief, normalizeLineEndingsToWin(d.info2Briefing).c_str());
    SetWindowTextW(pState->hEditInfo3Brief, normalizeLineEndingsToWin(d.info3Briefing).c_str());
}

void readFields(PropDialogState *pState) {
    if (!pState || !pState->pData) return;
    auto &d = *pState->pData;

    wchar_t buf[1024];

    d.language = ComboBox_GetCurSel(pState->hComboLang);
    if (d.language < 0 || d.language > 3) d.language = 0;

    d.techLevel = ComboBox_GetCurSel(pState->hComboTechLevel);
    if (d.techLevel < 0 || d.techLevel > 3) d.techLevel = 0;

    GetWindowTextW(pState->hEditInfo1, buf, 64);
    d.infoCost1 = _wtoi(buf);
    GetWindowTextW(pState->hEditInfo2, buf, 64);
    d.infoCost2 = _wtoi(buf);
    GetWindowTextW(pState->hEditInfo3, buf, 64);
    d.infoCost3 = _wtoi(buf);

    GetWindowTextW(pState->hEditEnh1, buf, 64);
    d.enhCost1 = _wtoi(buf);
    GetWindowTextW(pState->hEditEnh2, buf, 64);
    d.enhCost2 = _wtoi(buf);
    GetWindowTextW(pState->hEditEnh3, buf, 64);
    d.enhCost3 = _wtoi(buf);

    int len = GetWindowTextLengthW(pState->hEditBaseBrief);
    std::wstring wtext(len + 1, 0);
    GetWindowTextW(pState->hEditBaseBrief, &wtext[0], len + 1);
    wtext.resize(len);
    d.baseBriefing = normalizeLineEndingsToUnix(wtext);

    len = GetWindowTextLengthW(pState->hEditInfo1Brief);
    wtext.assign(len + 1, 0);
    GetWindowTextW(pState->hEditInfo1Brief, &wtext[0], len + 1);
    wtext.resize(len);
    d.info1Briefing = normalizeLineEndingsToUnix(wtext);

    len = GetWindowTextLengthW(pState->hEditInfo2Brief);
    wtext.assign(len + 1, 0);
    GetWindowTextW(pState->hEditInfo2Brief, &wtext[0], len + 1);
    wtext.resize(len);
    d.info2Briefing = normalizeLineEndingsToUnix(wtext);

    len = GetWindowTextLengthW(pState->hEditInfo3Brief);
    wtext.assign(len + 1, 0);
    GetWindowTextW(pState->hEditInfo3Brief, &wtext[0], len + 1);
    wtext.resize(len);
    d.info3Briefing = normalizeLineEndingsToUnix(wtext);
}

LRESULT CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PropDialogState *pState = reinterpret_cast<PropDialogState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    if (msg == WM_CTLCOLORSTATIC && pState) {
        HWND hChild = reinterpret_cast<HWND>(lParam);
        if (hChild == pState->hLabelBaseBrief || hChild == pState->hLabelInfo1Brief ||
            hChild == pState->hLabelInfo2Brief || hChild == pState->hLabelInfo3Brief ||
            hChild == pState->hLabelMissionId) {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            SetTextColor(hdc, RGB(0, 220, 245)); // Syndicate Cyan for briefing headers
            SetBkMode(hdc, TRANSPARENT);
            return reinterpret_cast<LRESULT>(GetStockObject(NULL_BRUSH));
        }
    }

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<PropDialogState*>(cs->lpCreateParams);
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
            pState->hFontBold = CreateFontW(
                -12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );

            // 1. GroupBox Language & Tech Level
            CreateWindowExW(0, WC_BUTTONW, L"Mission Briefing && Enemy Cyberware",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                16, 12, 592, 98, hWnd, nullptr, hInst, nullptr);

            std::wstring missIdStr = std::format(L"Mission ID: {:02}", pState->pData->missionId);
            pState->hLabelMissionId = CreateWindowExW(0, WC_STATICW, missIdStr.c_str(),
                WS_CHILD | WS_VISIBLE, 32, 36, 130, 20, hWnd, nullptr, hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Briefing Language:",
                WS_CHILD | WS_VISIBLE, 172, 36, 130, 20, hWnd, nullptr, hInst, nullptr);
            pState->hComboLang = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 308, 34, 284, 140, hWnd, nullptr, hInst, nullptr);

            for (int i = 0; i < 4; ++i) {
                std::string fname = MissionPropertiesDialog::getBriefingFilename(pState->pData->missionId, i);
                std::wstring item = std::format(L"{} ({})", MissionPropertiesDialog::getLanguageName(i), toWString(fname));
                ComboBox_AddString(pState->hComboLang, item.c_str());
            }
            ComboBox_SetCurSel(pState->hComboLang, (pState->pData->language >= 0 && pState->pData->language <= 3) ? pState->pData->language : 0);

            // Row 2: Enemy Cyberware
            CreateWindowExW(0, WC_STATICW, L"Enemy Cyberware:",
                WS_CHILD | WS_VISIBLE, 32, 68, 120, 20, hWnd, nullptr, hInst, nullptr);
            pState->hComboTechLevel = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 154, 65, 230, 140, hWnd, nullptr, hInst, nullptr);

            bool isCampaign = (pState->pData && pState->pData->missionId >= 1 && pState->pData->missionId <= 50 && !pState->pData->gameFilePath.empty());
            std::wstring tier0Label = isCampaign ? L"Tier 0: Dynamic (World Progression)" : L"Tier 0: Unaugmented";
            ComboBox_AddString(pState->hComboTechLevel, tier0Label.c_str());
            ComboBox_AddString(pState->hComboTechLevel, L"Tier 1: Basic Cybernetics (V1)");
            ComboBox_AddString(pState->hComboTechLevel, L"Tier 2: Advanced Cybernetics (V2)");
            ComboBox_AddString(pState->hComboTechLevel, L"Tier 3: Apex Cybernetics (V3)");
            ComboBox_SetCurSel(pState->hComboTechLevel, 0);

            pState->hStaticTechDesc = CreateWindowExW(0, WC_STATICW, L"",
                WS_CHILD | WS_VISIBLE, 390, 64, 210, 34, hWnd, nullptr, hInst, nullptr);
            updateTechLevelDescription(pState);

            // 2. GroupBox Intel & Enhancement Costs
            CreateWindowExW(0, WC_BUTTONW, L"Intelligence && Enhancements",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                16, 118, 592, 82, hWnd, nullptr, hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Info 1 ($):", WS_CHILD | WS_VISIBLE, 32, 142, 70, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditInfo1 = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"100",
                WS_CHILD | WS_VISIBLE | ES_NUMBER, 104, 140, 65, 22, hWnd, nullptr, hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Info 2 ($):", WS_CHILD | WS_VISIBLE, 195, 142, 70, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditInfo2 = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"250",
                WS_CHILD | WS_VISIBLE | ES_NUMBER, 268, 140, 65, 22, hWnd, nullptr, hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Info 3 ($):", WS_CHILD | WS_VISIBLE, 358, 142, 70, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditInfo3 = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"400",
                WS_CHILD | WS_VISIBLE | ES_NUMBER, 431, 140, 65, 22, hWnd, nullptr, hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Enh 1 ($):", WS_CHILD | WS_VISIBLE, 32, 170, 70, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditEnh1 = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"100",
                WS_CHILD | WS_VISIBLE | ES_NUMBER, 104, 168, 65, 22, hWnd, nullptr, hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Enh 2 ($):", WS_CHILD | WS_VISIBLE, 195, 170, 70, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditEnh2 = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"250",
                WS_CHILD | WS_VISIBLE | ES_NUMBER, 268, 168, 65, 22, hWnd, nullptr, hInst, nullptr);

            CreateWindowExW(0, WC_STATICW, L"Enh 3 ($):", WS_CHILD | WS_VISIBLE, 358, 170, 70, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditEnh3 = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"400",
                WS_CHILD | WS_VISIBLE | ES_NUMBER, 431, 168, 65, 22, hWnd, nullptr, hInst, nullptr);

            // 3. GroupBox Briefing Texts
            CreateWindowExW(0, WC_BUTTONW, L"Mission Briefing Content",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                16, 208, 592, 442, hWnd, nullptr, hInst, nullptr);

            pState->hLabelBaseBrief = CreateWindowExW(0, WC_STATICW, L"Base Mission Briefing:",
                WS_CHILD | WS_VISIBLE, 32, 230, 250, 16, hWnd, nullptr, hInst, nullptr);
            pState->hEditBaseBrief = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                32, 250, 560, 68, hWnd, nullptr, hInst, nullptr);

            pState->hLabelInfo1Brief = CreateWindowExW(0, WC_STATICW, L"Level 1 Intelligence Intel Text:",
                WS_CHILD | WS_VISIBLE, 32, 326, 250, 16, hWnd, nullptr, hInst, nullptr);
            pState->hEditInfo1Brief = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                32, 346, 560, 62, hWnd, nullptr, hInst, nullptr);

            pState->hLabelInfo2Brief = CreateWindowExW(0, WC_STATICW, L"Level 2 Intelligence Intel Text:",
                WS_CHILD | WS_VISIBLE, 32, 416, 250, 16, hWnd, nullptr, hInst, nullptr);
            pState->hEditInfo2Brief = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                32, 436, 560, 62, hWnd, nullptr, hInst, nullptr);

            pState->hLabelInfo3Brief = CreateWindowExW(0, WC_STATICW, L"Level 3 Intelligence / Tactics Text:",
                WS_CHILD | WS_VISIBLE, 32, 506, 250, 16, hWnd, nullptr, hInst, nullptr);
            pState->hEditInfo3Brief = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, L"",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                32, 526, 560, 114, hWnd, nullptr, hInst, nullptr);

            // 4. Buttons
            pState->hBtnSave = CreateWindowExW(0, WC_BUTTONW, L"Save",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                290, 660, 100, 28, hWnd, reinterpret_cast<HMENU>(IDOK), hInst, nullptr);

            pState->hBtnReload = CreateWindowExW(0, WC_BUTTONW, L"Reset",
                WS_CHILD | WS_VISIBLE,
                400, 660, 100, 28, hWnd, reinterpret_cast<HMENU>(102), hInst, nullptr);

            pState->hBtnCancel = CreateWindowExW(0, WC_BUTTONW, L"Cancel",
                WS_CHILD | WS_VISIBLE,
                508, 660, 100, 28, hWnd, reinterpret_cast<HMENU>(IDCANCEL), hInst, nullptr);

            // Set font on all child controls
            EnumChildWindows(hWnd, [](HWND hChild, LPARAM lp) -> BOOL {
                HFONT font = reinterpret_cast<HFONT>(lp);
                SendMessageW(hChild, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                return TRUE;
            }, reinterpret_cast<LPARAM>(pState->hFont));

            // Set bold font on prominent labels
            if (pState->hFontBold) {
                SendMessageW(pState->hLabelMissionId, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontBold), TRUE);
                SendMessageW(pState->hLabelBaseBrief, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontBold), TRUE);
                SendMessageW(pState->hLabelInfo1Brief, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hLabelInfo1Brief ? pState->hFontBold : pState->hFont), TRUE);
                SendMessageW(pState->hLabelInfo2Brief, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontBold), TRUE);
                SendMessageW(pState->hLabelInfo3Brief, WM_SETFONT, reinterpret_cast<WPARAM>(pState->hFontBold), TRUE);
            }

            populateFields(pState);
            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);

            if (reinterpret_cast<HWND>(lParam) == pState->hComboLang && code == CBN_SELCHANGE) {
                int newLang = ComboBox_GetCurSel(pState->hComboLang);
                int prevLang = pState->pData->language;
                if (newLang != prevLang && newLang >= 0 && newLang <= 3) {
                    readFields(pState);
                    pState->pData->syncActiveToLang(prevLang);
                    pState->pData->langs[prevLang].modified = true;
                    pState->pData->langs[prevLang].sessionEdited = true;

                    if (!pState->pData->langs[newLang].available) {
                        std::string targetFile = MissionPropertiesDialog::getBriefingFilename(pState->pData->missionId, newLang);
                        std::wstring prompt = std::format(
                            L"The {} briefing ({}) has not been created yet.\n\n"
                            L"Would you like to copy the current English briefing as a starting template to translate?",
                            MissionPropertiesDialog::getLanguageName(newLang),
                            toWString(targetFile));
                        int ans = showEditorMessageBox(hWnd, prompt.c_str(), L"Create Language Translation",
                            MB_YESNOCANCEL | MB_ICONQUESTION);
                        if (ans == IDCANCEL) {
                            ComboBox_SetCurSel(pState->hComboLang, prevLang);
                            return 0;
                        }
                        if (ans == IDYES && pState->pData->langs[0].available) {
                            pState->pData->langs[newLang] = pState->pData->langs[0];
                        } else {
                            setDefaultBriefingTemplateForLang(pState->pData->langs[newLang]);
                        }
                        pState->pData->langs[newLang].available = true;
                        pState->pData->langs[newLang].modified = true;
                        pState->pData->langs[newLang].sessionEdited = true;
                    }

                    pState->pData->syncLangToActive(newLang);
                    populateFields(pState);
                }
                return 0;
            }

            if (reinterpret_cast<HWND>(lParam) == pState->hComboTechLevel && code == CBN_SELCHANGE) {
                updateTechLevelDescription(pState);
                return 0;
            }

            if (id == 102) { // Reload
                int curLang = ComboBox_GetCurSel(pState->hComboLang);
                if (curLang < 0 || curLang > 3) curLang = 0;
                pState->pData->language = curLang;

                // Reload all briefing languages from source directory (or fallback paths)
                MissionPropertiesDialog::loadAllBriefingLanguages(
                    pState->pData->missionId,
                    pState->pData->sourceDir,
                    *pState->pData
                );

                // Ensure active language is synced
                if (curLang >= 0 && curLang <= 3 && pState->pData->langs[curLang].available) {
                    pState->pData->syncLangToActive(curLang);
                } else {
                    pState->pData->syncLangToActive(pState->pData->language);
                }

                // Reload tech level from game binary if available, or restore initial tech level
                std::filesystem::path gameFile = pState->pData->gameFilePath;
                if (gameFile.empty() && !pState->pData->sourceDir.empty()) {
                    gameFile = pState->pData->sourceDir / std::format("game{:02}.dat", pState->pData->missionId);
                }
                int diskTier = readTechLevelFromGameFile(gameFile.string());
                if (diskTier < 0) {
                    std::string origGame = rs_utl::File::getOriginalDataFullPath(std::format("game{:02}.dat", pState->pData->missionId));
                    diskTier = readTechLevelFromGameFile(origGame);
                }
                if (diskTier >= 0 && diskTier <= 3) {
                    pState->pData->techLevel = diskTier;
                } else if (pState->pData->initialTechLevel >= 0 && pState->pData->initialTechLevel <= 3) {
                    pState->pData->techLevel = pState->pData->initialTechLevel;
                }

                populateFields(pState);
                InvalidateRect(hWnd, nullptr, TRUE);
                UpdateWindow(hWnd);
                return 0;
            }

            if (id == IDOK) {
                readFields(pState);
                pState->pData->syncActiveToLang(pState->pData->language);
                pState->pData->langs[pState->pData->language].modified = true;
                pState->pData->langs[pState->pData->language].sessionEdited = true;
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
            if (pState && pState->hFont) {
                DeleteObject(pState->hFont);
                pState->hFont = nullptr;
            }
            if (pState && pState->hFontBold) {
                DeleteObject(pState->hFontBold);
                pState->hFontBold = nullptr;
            }
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static bool parseBriefingBufferToLang(const uint8_t *loaded, size_t size, MissionPropertiesData &data, int langIdx) {
    if (!loaded || size == 0 || langIdx < 0 || langIdx > 3) {
        return false;
    }

    const char *buf = reinterpret_cast<const char*>(loaded);
    std::string content(buf, size);

    std::vector<std::string> sections;
    size_t pos = 0;
    while (true) {
        size_t nextPipe = content.find('|', pos);
        if (nextPipe == std::string::npos) {
            sections.push_back(content.substr(pos));
            break;
        }
        sections.push_back(content.substr(pos, nextPipe - pos));
        pos = nextPipe + 1;
        if (pos < content.size() && content[pos] == '\r') pos++;
        if (pos < content.size() && content[pos] == '\n') pos++;
    }

    auto &langSlot = data.langs[langIdx];
    langSlot.available = true;
    langSlot.modified = false;
    langSlot.sessionEdited = false;

    // Parse info costs lines
    langSlot.infoCostLines.clear();
    if (!sections.empty()) {
        std::istringstream iss(sections[0]);
        std::string line;
        while (std::getline(iss, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
            if (!line.empty()) langSlot.infoCostLines.push_back(line);
        }
        if (langSlot.infoCostLines.size() > 0) data.infoCost1 = std::atoi(langSlot.infoCostLines[0].c_str());
        if (langSlot.infoCostLines.size() > 1) data.infoCost2 = std::atoi(langSlot.infoCostLines[1].c_str());
        if (langSlot.infoCostLines.size() > 2) data.infoCost3 = std::atoi(langSlot.infoCostLines[2].c_str());
    }

    // Parse enh costs lines
    langSlot.enhCostLines.clear();
    if (sections.size() > 1) {
        std::istringstream iss(sections[1]);
        std::string line;
        while (std::getline(iss, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.pop_back();
            if (!line.empty()) langSlot.enhCostLines.push_back(line);
        }
        if (langSlot.enhCostLines.size() > 0) data.enhCost1 = std::atoi(langSlot.enhCostLines[0].c_str());
        if (langSlot.enhCostLines.size() > 1) data.enhCost2 = std::atoi(langSlot.enhCostLines[1].c_str());
        if (langSlot.enhCostLines.size() > 2) data.enhCost3 = std::atoi(langSlot.enhCostLines[2].c_str());
    }

    // Parse briefing text sections (sections 2+)
    langSlot.briefingSections.clear();
    for (size_t i = 2; i < sections.size(); ++i) {
        langSlot.briefingSections.push_back(sections[i]);
    }

    auto trimEnd = [](const std::string &s) {
        std::string res = s;
        while (!res.empty() && (res.back() == '\n' || res.back() == '\r')) res.pop_back();
        return res;
    };

    langSlot.baseBriefing = langSlot.briefingSections.size() > 0 ? trimEnd(langSlot.briefingSections[0]) : "";
    langSlot.info1Briefing = langSlot.briefingSections.size() > 1 ? trimEnd(langSlot.briefingSections[1]) : "";
    langSlot.info2Briefing = langSlot.briefingSections.size() > 2 ? trimEnd(langSlot.briefingSections[2]) : "";
    langSlot.info3Briefing = langSlot.briefingSections.size() > 3 ? trimEnd(langSlot.briefingSections[3]) : "";

    if (langIdx == data.language) {
        data.syncLangToActive(langIdx);
    }

    return true;
}

} // namespace

bool MissionPropertiesDialog::loadBriefingLangFromPath(const std::string &path, MissionPropertiesData &data, int langIdx) {
    if (path.empty() || !std::filesystem::exists(path) || langIdx < 0 || langIdx > 3) {
        return false;
    }

    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return false;
    std::streamsize sz = in.tellg();
    if (sz <= 0) return false;
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(static_cast<size_t>(sz));
    if (!in.read(reinterpret_cast<char*>(buffer.data()), sz)) return false;

    if (rnc::isRncCompressed(buffer.data())) {
        size_t unpLen = 0;
        if (rnc::unpackedLength(buffer.data(), unpLen) == rnc::kOk && unpLen > 0) {
            std::vector<uint8_t> unpacked(unpLen);
            size_t realSize = 0;
            if (rnc::unpack(buffer.data(), unpacked.data(), realSize) >= 0 && realSize > 0) {
                return parseBriefingBufferToLang(unpacked.data(), realSize, data, langIdx);
            }
        }
    }

    return parseBriefingBufferToLang(buffer.data(), buffer.size(), data, langIdx);
}

bool MissionPropertiesDialog::loadBriefingDataFromPath(const std::string &path, MissionPropertiesData &data) {
    if (path.empty() || !std::filesystem::exists(path)) {
        setDefaultBriefingTemplate(data);
        return false;
    }
    bool ok = loadBriefingLangFromPath(path, data, data.language);
    if (!ok) {
        setDefaultBriefingTemplate(data);
    }
    return ok;
}

bool MissionPropertiesDialog::loadBriefingData(int missionId, int language, MissionPropertiesData &data, const std::filesystem::path &dir) {
    std::filesystem::path searchDir = !dir.empty() ? dir : data.sourceDir;
    std::string filename = getBriefingFilename(missionId, language);

    if (!searchDir.empty()) {
        std::filesystem::path customCandidate = searchDir / filename;
        if (std::filesystem::exists(customCandidate)) {
            return loadBriefingLangFromPath(customCandidate.string(), data, language);
        }
    }

    std::string fullPath = rs_utl::File::getOriginalDataFullPath(filename);
    if (std::filesystem::exists(fullPath)) {
        return loadBriefingLangFromPath(fullPath, data, language);
    }

    size_t size = 0;
    uint8_t *loaded = rs_utl::File::loadOriginalFile(filename, size);
    if (!loaded || size == 0) {
        setDefaultBriefingTemplate(data);
        return false;
    }

    bool res = parseBriefingBufferToLang(loaded, size, data, language);
    delete[] loaded;
    return res;
}

bool MissionPropertiesDialog::loadAllBriefingLanguages(int missionId, const std::filesystem::path &dir, MissionPropertiesData &data) {
    bool anyLoaded = false;
    for (int i = 0; i < 4; ++i) {
        std::string filename = getBriefingFilename(missionId, i);
        bool loaded = false;

        if (!dir.empty()) {
            std::filesystem::path candidate = dir / filename;
            if (std::filesystem::exists(candidate)) {
                loaded = loadBriefingLangFromPath(candidate.string(), data, i);
            }
        }

        if (!loaded) {
            std::string fullPath = rs_utl::File::getOriginalDataFullPath(filename);
            if (std::filesystem::exists(fullPath)) {
                loaded = loadBriefingLangFromPath(fullPath, data, i);
            }
        }

        if (!loaded) {
            size_t size = 0;
            uint8_t *packed = rs_utl::File::loadOriginalFile(filename, size);
            if (packed && size > 0) {
                loaded = parseBriefingBufferToLang(packed, size, data, i);
                delete[] packed;
            }
        }

        if (loaded) {
            data.langs[i].available = true;
            data.langs[i].modified = false;
            data.langs[i].sessionEdited = false;
            anyLoaded = true;
        } else {
            data.langs[i].available = false;
            data.langs[i].modified = false;
            data.langs[i].sessionEdited = false;
        }
    }

    if (!data.langs[0].available) {
        setDefaultBriefingTemplateForLang(data.langs[0]);
        data.langs[0].available = true;
        data.langs[0].sessionEdited = false;
    }

    if (data.language < 0 || data.language > 3 || !data.langs[data.language].available) {
        data.language = 0;
    }
    data.syncLangToActive(data.language);

    return anyLoaded;
}

bool MissionPropertiesDialog::saveBriefingLangToPath(const std::string &path, const MissionPropertiesData &data, int langIdx) {
    if (path.empty() || langIdx < 0 || langIdx > 3) return false;

    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;

    const auto &langData = data.langs[langIdx];

    std::vector<std::string> outInfoCosts = langData.infoCostLines;
    if (outInfoCosts.empty()) {
        outInfoCosts.push_back(std::to_string(data.infoCost1));
        outInfoCosts.push_back(std::to_string(data.infoCost2));
        if (data.infoCost3 > 0) outInfoCosts.push_back(std::to_string(data.infoCost3));
    } else {
        outInfoCosts[0] = std::to_string(data.infoCost1);
        if (outInfoCosts.size() > 1) outInfoCosts[1] = std::to_string(data.infoCost2);
        else outInfoCosts.push_back(std::to_string(data.infoCost2));
        if (outInfoCosts.size() > 2) outInfoCosts[2] = std::to_string(data.infoCost3);
        else if (data.infoCost3 > 0) outInfoCosts.push_back(std::to_string(data.infoCost3));
    }

    auto updateCostWithSuffix = [](std::string &target, int val) {
        std::string suffix;
        for (char c : target) {
            if (c < '0' || c > '9') suffix += c;
        }
        target = std::to_string(val) + suffix;
    };
    std::vector<std::string> outEnhCosts = langData.enhCostLines;
    if (outEnhCosts.empty()) {
        outEnhCosts.push_back(std::to_string(data.enhCost1));
        outEnhCosts.push_back(std::to_string(data.enhCost2));
        if (data.enhCost3 > 0) outEnhCosts.push_back(std::to_string(data.enhCost3));
    } else {
        updateCostWithSuffix(outEnhCosts[0], data.enhCost1);
        if (outEnhCosts.size() > 1) updateCostWithSuffix(outEnhCosts[1], data.enhCost2);
        else outEnhCosts.push_back(std::to_string(data.enhCost2));
        if (outEnhCosts.size() > 2) updateCostWithSuffix(outEnhCosts[2], data.enhCost3);
        else if (data.enhCost3 > 0) outEnhCosts.push_back(std::to_string(data.enhCost3));
    }

    std::vector<std::string> outSections = langData.briefingSections;

    auto syncSection = [](std::string &rawSec, const std::string &uiSec) {
        auto trimEnd = [](const std::string &s) {
            std::string res = s;
            while (!res.empty() && (res.back() == '\n' || res.back() == '\r')) res.pop_back();
            return res;
        };

        if (trimEnd(rawSec) == trimEnd(uiSec)) {
            return;
        }
        rawSec = uiSec;
        while (!rawSec.empty() && (rawSec.back() == '\n' || rawSec.back() == '\r')) rawSec.pop_back();
        if (!rawSec.empty()) rawSec += "\n\n";
    };

    if (outSections.size() < 1) outSections.resize(1);
    syncSection(outSections[0], langData.baseBriefing);

    if (outSections.size() < 2 && !langData.info1Briefing.empty()) outSections.resize(2);
    if (outSections.size() >= 2) syncSection(outSections[1], langData.info1Briefing);

    if (outSections.size() < 3 && !langData.info2Briefing.empty()) outSections.resize(3);
    if (outSections.size() >= 3) syncSection(outSections[2], langData.info2Briefing);

    if (outSections.size() < 4 && !langData.info3Briefing.empty()) outSections.resize(4);
    if (outSections.size() >= 4) syncSection(outSections[3], langData.info3Briefing);

    std::ostringstream ss;
    for (const auto &line : outInfoCosts) {
        ss << line << "\n";
    }
    ss << "|\n";

    for (const auto &line : outEnhCosts) {
        ss << line << "\n";
    }

    for (size_t i = 0; i < outSections.size(); ++i) {
        if (!outSections[i].empty() && outSections[i].back() != '\n') {
            outSections[i] += '\n';
        }
        ss << "|\n" << outSections[i];
    }

    std::string rawContent = ss.str();
    std::vector<uint8_t> packed;
    rnc::RncRetCode rc = rnc::pack(reinterpret_cast<const uint8_t*>(rawContent.data()), rawContent.size(), packed);
    if (rc == rnc::kOk && !packed.empty()) {
        out.write(reinterpret_cast<const char*>(packed.data()), packed.size());
    } else {
        out.write(rawContent.data(), rawContent.size());
    }

    out.close();
    return true;
}

bool MissionPropertiesDialog::saveBriefingDataToPath(const std::string &path, const MissionPropertiesData &data) {
    return saveBriefingLangToPath(path, data, data.language);
}

bool MissionPropertiesDialog::saveBriefingData(int missionId, int language, const MissionPropertiesData &data, const std::string &customPath) {
    std::string filename = customPath.empty() ?
        rs_utl::File::getOriginalDataFullPath(getBriefingFilename(missionId, language)) :
        customPath;
    return saveBriefingLangToPath(filename, data, language);
}

bool MissionPropertiesDialog::show(HWND hParent, MissionPropertiesData &data) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedMissionPropertiesDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = DialogProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    PropDialogState state;
    state.hParent = hParent;
    state.pData = &data;

    RECT rcParent = {};
    GetWindowRect(hParent, &rcParent);
    int dlgW = 636;
    int dlgH = 736;
    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (dlgX < 0) dlgX = 0;
    if (dlgY < 0) dlgY = 0;

    std::wstring title = std::format(L"Mission {} Properties & Briefing Editor", data.missionId);

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        kClassName,
        title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        dlgX, dlgY, dlgW, dlgH,
        hParent, nullptr, hInstance, &state
    );

    if (!hDlg) return false;

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
