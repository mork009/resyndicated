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

#include "ai_tactics_dialog.h"
#include "dialog_theme.h"
#include "map_viewport.h"

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <format>
#include <vector>
#include <string>

#include "rs-kernel/model/leveldata.h"
#include "rs-kernel/model/vehicle.h"

namespace rs_win_editor {

namespace {

static const int IDC_COMBO_FACTION     = 1101;
static const int IDC_LIST_TACTICS      = 1102;
static const int IDC_BTN_ADD_NODE      = 1103;
static const int IDC_BTN_DEL_NODE      = 1104;
static const int IDC_BTN_MOVE_UP       = 1105;
static const int IDC_BTN_MOVE_DOWN     = 1106;
static const int IDC_BTN_CLEAR_FACTION = 1107;

static const int IDC_COMBO_MODE        = 1110;
static const int IDC_COMBO_TARGET      = 1111;
static const int IDC_EDIT_P1           = 1112;
static const int IDC_EDIT_P2           = 1113;
static const int IDC_EDIT_P3           = 1114;
static const int IDC_BTN_PICK_MAP      = 1115;
static const int IDC_COMBO_NEXT        = 1116;
static const int IDC_CHECK_ARMED       = 1117;
static const int IDC_COMBO_VEHICLE     = 1119;

static const int IDC_BTN_OK             = IDOK;
static const int IDC_BTN_CANCEL         = IDCANCEL;

static HBRUSH s_hbrDlgBg = nullptr;
static bool s_classRegistered = false;

struct FactionOption {
    uint8_t id;
    const wchar_t *name;
};

static const FactionOption kFactions[] = {
    { 1, L"Faction 1: Blue Syndicate" },
    { 2, L"Faction 2: Orange Syndicate" },
    { 3, L"Faction 3: Yellow Syndicate A" },
    { 4, L"Faction 4: Yellow Syndicate B" },
    { 5, L"Faction 5: Purple Syndicate" },
    { 6, L"Faction 6: Grey Syndicate" },
    { 7, L"Faction 7: Cyan Syndicate" },
    { 255, L"Wildcard: Global / All Hostile Factions" }
};

struct ModeOption {
    uint8_t mode;
    const wchar_t *title;
    const wchar_t *description;
    const wchar_t *p1Label;
    const wchar_t *p2Label;
    const wchar_t *p3Label;
    bool usesP1;
    bool usesP2;
    bool usesP3;
    bool isCoords;
    bool isVehicle;
    bool allowsWildcardTarget;
    bool usesTargetFaction;
    bool allowsArmedCondition;
    const wchar_t *pickPrompt;
    const wchar_t *pickBtnText;
};

static const ModeOption kModes[] = {
    { 1, L"Seek & Destroy",
      L"Squad scenario directive targeting rival factions (excluding player's faction).",
      L"Param 1:", L"Param 2:", L"Param 3:",
      false, false, false, false, false, false, true, false,
      nullptr, L"Pick Target from Map (N/A)" },

    { 2, L"Rival Sweep",
      L"Hunt and eliminate specified target faction (player or rival) across the map.",
      L"Param 1:", L"Param 2:", L"Param 3:",
      false, false, false, false, false, false, true, false,
      nullptr, L"Pick Target from Map (N/A)" },

    { 3, L"Objective Sweep",
      L"DOS: follows mission objective (escort VIP on Assassinate; sweep on Combat). ReSyndicated: hunts Target Faction.",
      L"Param 1:", L"Param 2:", L"Param 3:",
      false, false, false, false, false, true, true, false,
      nullptr, L"Pick Target from Map (N/A)" },

    { 4, L"Stationary Guard",
      L"Hold ground and engage approaching hostiles in self-defense.",
      L"Guard Radius (tiles, 0=12):", L"Param 2:", L"Param 3:",
      true, false, false, false, false, true, true, true,
      nullptr, L"Pick Target from Map (N/A)" },

    { 5, L"Weapon Ambush",
      L"Hold ground until target enters lethal weapon range and line-of-sight.",
      L"Hold Range (tiles, 0=wpn):", L"Param 2:", L"Param 3:",
      true, false, false, false, false, true, true, true,
      nullptr, L"Pick Target from Map (N/A)" },

    { 6, L"Patrol / Search",
      L"Move to target waypoint coordinates.",
      L"Target X (Tile):", L"Target Y (Tile):", L"Target Z (Height):",
      true, true, true, true, false, false, false, false,
      L"🎯 PICK PATROL WAYPOINT  \u2014  Click destination tile on map (Middle/Right drag to pan)",
      L"🎯 Pick Waypoint Coords from Map Viewport" },

    { 7, L"Proximity Tripwire",
      L"Advance when target approaches alert perimeter or timer expires.",
      L"Timer (0=no timeout):", L"Perimeter (tiles, 0=12):", L"Param 3:",
      true, true, false, false, false, false, true, true,
      nullptr, L"Pick Target from Map (N/A)" },

    { 8, L"Board Vehicle",
      L"Seek and enter target vehicle.",
      L"Target Vehicle:", L"Param 2:", L"Param 3:",
      true, false, false, false, true, false, false, false,
      L"🎯 PICK TARGET VEHICLE  \u2014  Click target vehicle on map (Middle/Right drag to pan)",
      L"🎯 Pick Target Vehicle from Map Viewport" },

    { 9, L"Evacuate",
      L"Retreat to designated evacuation coordinates.",
      L"Evac X (Tile):", L"Evac Y (Tile):", L"Evac Z (Height):",
      true, true, true, true, false, false, false, false,
      L"🎯 PICK EVACUATION DESTINATION  \u2014  Click destination tile on map (Middle/Right drag to pan)",
      L"🎯 Pick Evacuation Coords from Map Viewport" },

    { 10, L"Delay Timer",
      L"Wait specified countdown duration before advancing.",
      L"Duration (0.1s ticks):", L"Param 2:", L"Param 3:",
      true, false, false, false, false, false, false, false,
      nullptr, L"Pick Target from Map (N/A)" }
};

struct TargetOption {
    uint8_t id;
    const wchar_t *name;
};

static const TargetOption kTargets[] = {
    { 0, L"Faction 0: Player's Squad" },
    { 1, L"Faction 1: Blue Syndicate" },
    { 2, L"Faction 2: Orange Syndicate" },
    { 3, L"Faction 3: Yellow Syndicate A" },
    { 4, L"Faction 4: Yellow Syndicate B" },
    { 5, L"Faction 5: Purple Syndicate" },
    { 6, L"Faction 6: Grey Syndicate" },
    { 7, L"Faction 7: Cyan Syndicate" },
    { 255, L"Any Hostile / Wildcard" }
};

static const wchar_t* getVehicleTypeName(uint8_t type) {
    if (type == rs_knl::Vehicle::kVehicleTypeRegularCar) return L"Car";
    if (type == rs_knl::Vehicle::kVehicleTypePolice) return L"Police Car";
    if (type == rs_knl::Vehicle::kVehicleTypeMedics) return L"Ambulance";
    if (type == rs_knl::Vehicle::kVehicleTypeFireFighter) return L"Firetruck";
    if (type == rs_knl::Vehicle::kVehicleTypeSmallArmored) return L"Small Armored";
    if (type == rs_knl::Vehicle::kVehicleTypeLargeArmored) return L"Large Armored";
    if (type == rs_knl::Vehicle::kVehicleTypeLargeArmoredDamaged) return L"Damaged Armored";
    if (type == rs_knl::Vehicle::kVehicleTypeTrainHead) return L"Train Engine";
    if (type == rs_knl::Vehicle::kVehicleTypeTrainBody) return L"Train Carriage";
    return L"Vehicle";
}

struct AiDlgState {
    HWND hTopLevel = nullptr;
    HWND hParent = nullptr;
    HWND hDlg = nullptr;
    WinMapViewport *pViewport = nullptr;
    rs_knl::Mission *pMission = nullptr;
    EditorCommandStack *pCommandStack = nullptr;

    LevelData::AITacticsBlock workingTactics{};
    uint8_t currentFactionId = 1;
    int selectedNodeSlot = -1;
    bool isPopulating = false;

    // Controls
    HWND hComboFaction = nullptr;
    HWND hListTactics = nullptr;
    HWND hBtnAddNode = nullptr;
    HWND hBtnDelNode = nullptr;
    HWND hBtnMoveUp = nullptr;
    HWND hBtnMoveDown = nullptr;
    HWND hBtnClearFaction = nullptr;

    HWND hGroupInspector = nullptr;
    HWND hStaticSlot = nullptr;
    HWND hStaticMode = nullptr;
    HWND hComboMode = nullptr;
    HWND hStaticModeDesc = nullptr;
    HWND hStaticTarget = nullptr;
    HWND hComboTarget = nullptr;
    HWND hStaticP1 = nullptr;
    HWND hEditP1 = nullptr;
    HWND hComboVehicle = nullptr;
    HWND hStaticP2 = nullptr;
    HWND hEditP2 = nullptr;
    HWND hStaticP3 = nullptr;
    HWND hEditP3 = nullptr;
    HWND hBtnPickMap = nullptr;
    HWND hStaticNext = nullptr;
    HWND hComboNext = nullptr;
    HWND hCheckArmed = nullptr;
    HWND hStaticWildcardInfo = nullptr;

    HWND hStaticStatus = nullptr;
    HWND hBtnOk = nullptr;
    HWND hBtnCancel = nullptr;
};

static LRESULT CALLBACK StaticLabelSubclass(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    if (uMsg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        HBRUSH hbr = CreateSolidBrush(kColorDlgBg);
        FillRect(hdc, &rc, hbr);
        DeleteObject(hbr);

        wchar_t text[512] = {};
        GetWindowTextW(hWnd, text, 512);
        if (text[0] != L'\0') {
            HFONT hFont = (HFONT)SendMessageW(hWnd, WM_GETFONT, 0, 0);
            HFONT oldFont = hFont ? (HFONT)SelectObject(hdc, hFont) : nullptr;
            SetBkMode(hdc, TRANSPARENT);
            COLORREF col = (dwRefData != 0) ? static_cast<COLORREF>(dwRefData) : kColorText;
            SetTextColor(hdc, col);
            DrawTextW(hdc, text, -1, &rc, DT_LEFT | DT_WORDBREAK);
            if (oldFont) SelectObject(hdc, oldFont);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    if (uMsg == WM_ERASEBKGND) {
        return 1;
    }
    if (uMsg == WM_NCDESTROY) {
        RemoveWindowSubclass(hWnd, StaticLabelSubclass, uIdSubclass);
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static void ensureClassRegistered(HINSTANCE hInst, WNDPROC proc) {
    if (!s_classRegistered) {
        s_hbrDlgBg = CreateSolidBrush(kColorDlgBg);
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc = proc;
        wc.hInstance = hInst;
        wc.hbrBackground = s_hbrDlgBg;
        wc.lpszClassName = L"ReSyndicatedAiTacticsDlgClass";
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassExW(&wc);
        s_classRegistered = true;
    }
}

static void populateVehicleComboBox(HWND hCombo, rs_knl::Mission *pMission, uint16_t selectedVehicleIdx) {
    if (!hCombo) return;
    ComboBox_ResetContent(hCombo);

    if (!pMission || pMission->numVehicles() == 0) {
        int idx = ComboBox_AddString(hCombo, L"(No vehicles on map)");
        ComboBox_SetItemData(hCombo, idx, 0);
        ComboBox_SetCurSel(hCombo, idx);
        return;
    }

    int selectedIdx = 0;
    for (size_t i = 0; i < pMission->numVehicles(); ++i) {
        auto *v = pMission->vehicle(i);
        if (!v) continue;
        std::wstring item = std::format(L"Vehicle #{:02}: {} at ({}, {}, {})",
            i, getVehicleTypeName(v->getType()), v->tileX(), v->tileY(), v->tileZ());
        int idx = ComboBox_AddString(hCombo, item.c_str());
        ComboBox_SetItemData(hCombo, idx, static_cast<LPARAM>(i));
        if (i == selectedVehicleIdx) {
            selectedIdx = idx;
        }
    }
    ComboBox_SetCurSel(hCombo, selectedIdx);
}

std::wstring formatNodeLabel(int nodeIdx, const LevelData::AITactic &e, bool indentChild = false) {
    std::wstring modeStr = L"None";
    for (const auto &m : kModes) {
        if (m.mode == e.mode) {
            modeStr = m.title;
            break;
        }
    }

    uint16_t p1 = rs_utl::READ_LE_UINT16(e.param1);
    uint16_t prev = rs_utl::READ_LE_UINT16(e.prev_tactic);
    uint16_t next = rs_utl::READ_LE_UINT16(e.next_tactic);

    std::wstring detail = L"";
    if (e.mode == LevelData::kAITacticModeDelayTimer) {
        detail = std::format(L" ({:.1f}s)", p1 * 0.1f);
    } else if (e.mode == LevelData::kAITacticModeProximityTripwire) {
        if (next == 0) {
            detail = std::format(L" (Hold Ground / Terminal, p1={})", p1);
        } else {
            uint16_t p2 = rs_utl::READ_LE_UINT16(e.param2);
            if (p2 > 0) {
                detail = std::format(L" (Timer: {:.0f}s, Range: {} tiles)", p1 * 1.35f, p2);
            } else {
                detail = std::format(L" (Timer: {:.0f}s / ~{} ticks)", p1 * 1.35f, p1);
            }
        }
    } else if (e.mode == LevelData::kAITacticModeSeekDestroy || e.mode == LevelData::kAITacticModeRivalSweep ||
               e.mode == LevelData::kAITacticModeObjectiveSweep) {
        detail = std::format(L" (Target F{})", e.sub_mode);
    } else if (e.mode == LevelData::kAITacticModePatrolSearch || e.mode == LevelData::kAITacticModeEvacuate) {
        uint16_t p2 = rs_utl::READ_LE_UINT16(e.param2);
        detail = std::format(L" (At {}, {})", p1, p2);
    } else if (e.mode == LevelData::kAITacticModeBoardVehicle) {
        detail = std::format(L" (Veh #{})", p1);
    }

    std::wstring prefix = (e.mode > 0 && prev == 0) ? L"\U0001F6A9 " : (indentChild ? L"   \u21B3 " : L"");
    std::wstring nextStr = (next > 0 && next < 128) ? std::format(L" -> #{}", next) : L" -> End";
    return std::format(L"{}#{}: [{}]{}{}", prefix, nodeIdx, modeStr, detail, nextStr);
}

std::vector<int> getFactionChain(const LevelData::AITacticsBlock &tactics, uint8_t factionId) {
    std::vector<int> chain;
    std::vector<bool> visited(128, false);

    // Walk every root chain (prev_tactic == 0) in slot order so multiple roots
    // (e.g. Wildcard factions) each group cleanly with their linked child nodes.
    for (int i = 1; i < 128; ++i) {
        const auto &e = tactics.entries[i];
        if (!visited[i] && e.mode > 0 &&
            rs_utl::READ_LE_UINT16(e.prev_tactic) == 0 &&
            (e.faction_id == factionId || (factionId == 255 && e.faction_id == 255))) {
            int cur = i;
            while (cur > 0 && cur < 128 && !visited[cur]) {
                visited[cur] = true;
                chain.push_back(cur);
                cur = rs_utl::READ_LE_UINT16(tactics.entries[cur].next_tactic);
            }
        }
    }

    // Collect any remaining unlinked or cyclic nodes for this faction
    for (int i = 1; i < 128; ++i) {
        const auto &e = tactics.entries[i];
        if (!visited[i] && e.mode > 0 &&
            (e.faction_id == factionId || (factionId == 255 && e.faction_id == 255))) {
            chain.push_back(i);
        }
    }

    return chain;
}

namespace {

constexpr int kInspectorX = 420;
constexpr int kInspectorW = 375;
constexpr int kParamLabelY = 244;
constexpr int kParamEditY = 262;
constexpr int kPickMapY = 294;
constexpr int kNextLabelY = 328;
constexpr int kNextComboY = 346;
constexpr int kCheckArmedY = 382;
constexpr int kWildcardInfoY = 414;
constexpr int kWildcardInfoH = 108;

} // namespace

void updateInspectorFromNode(AiDlgState *pState, int slot) {
    if (!pState || !pState->hGroupInspector) return;
    pState->isPopulating = true;

    bool showWildcardInfo = (pState->currentFactionId == LevelData::kAIFactionWildcard);
    if (pState->hStaticWildcardInfo) {
        ShowWindow(pState->hStaticWildcardInfo, showWildcardInfo ? SW_SHOW : SW_HIDE);
        if (showWildcardInfo) {
            InvalidateRect(pState->hStaticWildcardInfo, nullptr, TRUE);
        }
    }

    if (slot <= 0 || slot >= 128) {
        SetWindowTextW(pState->hStaticSlot, L"No Tactic Node Selected");
        SetWindowTextW(pState->hStaticModeDesc, L"");

        ShowWindow(pState->hStaticMode, SW_HIDE);
        ShowWindow(pState->hComboMode, SW_HIDE);
        ShowWindow(pState->hStaticModeDesc, SW_HIDE);
        ShowWindow(pState->hStaticTarget, SW_HIDE);
        ShowWindow(pState->hComboTarget, SW_HIDE);
        ShowWindow(pState->hStaticP1, SW_HIDE);
        ShowWindow(pState->hEditP1, SW_HIDE);
        ShowWindow(pState->hComboVehicle, SW_HIDE);
        ShowWindow(pState->hStaticP2, SW_HIDE);
        ShowWindow(pState->hEditP2, SW_HIDE);
        ShowWindow(pState->hStaticP3, SW_HIDE);
        ShowWindow(pState->hEditP3, SW_HIDE);
        ShowWindow(pState->hBtnPickMap, SW_HIDE);
        ShowWindow(pState->hStaticNext, SW_HIDE);
        ShowWindow(pState->hComboNext, SW_HIDE);
        ShowWindow(pState->hCheckArmed, SW_HIDE);

        RECT rcInspector = { 400, 50, 815, 530 };
        InvalidateRect(pState->hDlg, &rcInspector, TRUE);
        pState->isPopulating = false;
        return;
    }

    const auto &e = pState->workingTactics.entries[slot];
    std::wstring facLabel = (e.faction_id == LevelData::kAIFactionWildcard)
        ? L"Wildcard 255"
        : std::format(L"Faction {}", e.faction_id);
    std::wstring rootTag = (rs_utl::READ_LE_UINT16(e.prev_tactic) == 0) ? L" \U0001F6A9 Root" : L"";
    SetWindowTextW(pState->hStaticSlot, std::format(L"Tactic Node #{} ({}){}", slot, facLabel, rootTag).c_str());

    // Show common controls
    ShowWindow(pState->hStaticSlot, SW_SHOW);
    ShowWindow(pState->hStaticMode, SW_SHOW);
    ShowWindow(pState->hComboMode, SW_SHOW);
    ShowWindow(pState->hStaticModeDesc, SW_SHOW);
    ShowWindow(pState->hStaticNext, SW_SHOW);
    ShowWindow(pState->hComboNext, SW_SHOW);
    ShowWindow(pState->hCheckArmed, SW_SHOW);

    EnableWindow(pState->hComboMode, TRUE);
    EnableWindow(pState->hComboNext, TRUE);
    EnableWindow(pState->hCheckArmed, TRUE);

    // Mode
    int modeIdx = 0;
    for (size_t i = 0; i < sizeof(kModes) / sizeof(kModes[0]); ++i) {
        if (kModes[i].mode == e.mode) {
            modeIdx = static_cast<int>(i);
            break;
        }
    }
    ComboBox_SetCurSel(pState->hComboMode, modeIdx);
    const auto &modeOpt = kModes[modeIdx];

    // Description text - set text, invalidate and repaint cleanly
    SetWindowTextW(pState->hStaticModeDesc, modeOpt.description);
    InvalidateRect(pState->hStaticModeDesc, nullptr, TRUE);
    UpdateWindow(pState->hStaticModeDesc);

    // Target Faction: show only if mode uses target faction
    if (!modeOpt.usesTargetFaction) {
        ShowWindow(pState->hStaticTarget, SW_HIDE);
        ShowWindow(pState->hComboTarget, SW_HIDE);
    } else {
        ShowWindow(pState->hStaticTarget, SW_SHOW);
        ShowWindow(pState->hComboTarget, SW_SHOW);
        EnableWindow(pState->hComboTarget, TRUE);

        ComboBox_ResetContent(pState->hComboTarget);
        int targetSel = 0;
        for (size_t i = 0; i < sizeof(kTargets) / sizeof(kTargets[0]); ++i) {
            if (!modeOpt.allowsWildcardTarget && kTargets[i].id == 255) {
                continue;
            }
            if (modeOpt.mode == LevelData::kAITacticModeSeekDestroy &&
                kTargets[i].id == LevelData::kAIFactionPlayer) {
                continue;
            }
            int idx = ComboBox_AddString(pState->hComboTarget, kTargets[i].name);
            ComboBox_SetItemData(pState->hComboTarget, idx, kTargets[i].id);
            if (kTargets[i].id == e.sub_mode) {
                targetSel = idx;
            }
        }
        ComboBox_SetCurSel(pState->hComboTarget, targetSel);
    }

    uint16_t p1 = rs_utl::READ_LE_UINT16(e.param1);
    uint16_t p2 = rs_utl::READ_LE_UINT16(e.param2);
    uint16_t p3 = rs_utl::READ_LE_UINT16(e.param3);

    // Parameters display & layout:
    int paramCount = (modeOpt.usesP1 ? 1 : 0) + (modeOpt.usesP2 ? 1 : 0) + (modeOpt.usesP3 ? 1 : 0);

    if (paramCount == 0) {
        // No parameters for this mode (e.g. Seek & Destroy, Objective Sweep)
        ShowWindow(pState->hStaticP1, SW_HIDE);
        ShowWindow(pState->hEditP1, SW_HIDE);
        ShowWindow(pState->hComboVehicle, SW_HIDE);
        ShowWindow(pState->hStaticP2, SW_HIDE);
        ShowWindow(pState->hEditP2, SW_HIDE);
        ShowWindow(pState->hStaticP3, SW_HIDE);
        ShowWindow(pState->hEditP3, SW_HIDE);
    } else if (modeOpt.isVehicle) {
        // Board Vehicle: single full-width vehicle dropdown
        SetWindowTextW(pState->hStaticP1, modeOpt.p1Label);
        SetWindowPos(pState->hStaticP1, nullptr, kInspectorX, kParamLabelY, kInspectorW, 16, SWP_NOZORDER);
        ShowWindow(pState->hStaticP1, SW_SHOW);

        ShowWindow(pState->hEditP1, SW_HIDE);
        SetWindowPos(pState->hComboVehicle, nullptr, kInspectorX, kParamEditY, kInspectorW, 200, SWP_NOZORDER);
        ShowWindow(pState->hComboVehicle, SW_SHOW);
        EnableWindow(pState->hComboVehicle, TRUE);
        populateVehicleComboBox(pState->hComboVehicle, pState->pMission, p1);

        ShowWindow(pState->hStaticP2, SW_HIDE);
        ShowWindow(pState->hEditP2, SW_HIDE);
        ShowWindow(pState->hStaticP3, SW_HIDE);
        ShowWindow(pState->hEditP3, SW_HIDE);
    } else if (paramCount == 1) {
        // 1 Parameter: full-width label so it never truncates
        SetWindowTextW(pState->hStaticP1, modeOpt.p1Label);
        SetWindowPos(pState->hStaticP1, nullptr, kInspectorX, kParamLabelY, kInspectorW, 16, SWP_NOZORDER);
        ShowWindow(pState->hStaticP1, SW_SHOW);

        SetWindowPos(pState->hEditP1, nullptr, kInspectorX, kParamEditY, 120, 24, SWP_NOZORDER);
        SetWindowTextW(pState->hEditP1, std::to_wstring(p1).c_str());
        ShowWindow(pState->hEditP1, SW_SHOW);
        EnableWindow(pState->hEditP1, TRUE);

        ShowWindow(pState->hComboVehicle, SW_HIDE);
        ShowWindow(pState->hStaticP2, SW_HIDE);
        ShowWindow(pState->hEditP2, SW_HIDE);
        ShowWindow(pState->hStaticP3, SW_HIDE);
        ShowWindow(pState->hEditP3, SW_HIDE);
    } else if (paramCount == 2) {
        // 2 Parameters (e.g. Proximity Tripwire: Timer and Perimeter)
        SetWindowTextW(pState->hStaticP1, modeOpt.p1Label);
        SetWindowPos(pState->hStaticP1, nullptr, kInspectorX, kParamLabelY, 180, 16, SWP_NOZORDER);
        ShowWindow(pState->hStaticP1, SW_SHOW);

        SetWindowPos(pState->hEditP1, nullptr, kInspectorX, kParamEditY, 120, 24, SWP_NOZORDER);
        SetWindowTextW(pState->hEditP1, std::to_wstring(p1).c_str());
        ShowWindow(pState->hEditP1, SW_SHOW);
        EnableWindow(pState->hEditP1, TRUE);

        SetWindowTextW(pState->hStaticP2, modeOpt.p2Label);
        SetWindowPos(pState->hStaticP2, nullptr, 610, kParamLabelY, 180, 16, SWP_NOZORDER);
        ShowWindow(pState->hStaticP2, SW_SHOW);

        SetWindowPos(pState->hEditP2, nullptr, 610, kParamEditY, 120, 24, SWP_NOZORDER);
        SetWindowTextW(pState->hEditP2, std::to_wstring(p2).c_str());
        ShowWindow(pState->hEditP2, SW_SHOW);
        EnableWindow(pState->hEditP2, TRUE);

        ShowWindow(pState->hComboVehicle, SW_HIDE);
        ShowWindow(pState->hStaticP3, SW_HIDE);
        ShowWindow(pState->hEditP3, SW_HIDE);
    } else {
        // 3 Parameters (e.g. Rival Sweep, Patrol, Evacuate)
        SetWindowTextW(pState->hStaticP1, modeOpt.p1Label);
        SetWindowPos(pState->hStaticP1, nullptr, kInspectorX, kParamLabelY, 120, 16, SWP_NOZORDER);
        ShowWindow(pState->hStaticP1, SW_SHOW);

        SetWindowPos(pState->hEditP1, nullptr, kInspectorX, kParamEditY, 115, 24, SWP_NOZORDER);
        SetWindowTextW(pState->hEditP1, std::to_wstring(p1).c_str());
        ShowWindow(pState->hEditP1, SW_SHOW);
        EnableWindow(pState->hEditP1, TRUE);

        SetWindowTextW(pState->hStaticP2, modeOpt.p2Label);
        SetWindowPos(pState->hStaticP2, nullptr, 550, kParamLabelY, 120, 16, SWP_NOZORDER);
        ShowWindow(pState->hStaticP2, SW_SHOW);

        SetWindowPos(pState->hEditP2, nullptr, 550, kParamEditY, 115, 24, SWP_NOZORDER);
        SetWindowTextW(pState->hEditP2, std::to_wstring(p2).c_str());
        ShowWindow(pState->hEditP2, SW_SHOW);
        EnableWindow(pState->hEditP2, TRUE);

        SetWindowTextW(pState->hStaticP3, modeOpt.p3Label);
        SetWindowPos(pState->hStaticP3, nullptr, 675, kParamLabelY, 120, 16, SWP_NOZORDER);
        ShowWindow(pState->hStaticP3, SW_SHOW);

        SetWindowPos(pState->hEditP3, nullptr, 675, kParamEditY, 120, 24, SWP_NOZORDER);
        SetWindowTextW(pState->hEditP3, std::to_wstring(p3).c_str());
        ShowWindow(pState->hEditP3, SW_SHOW);
        EnableWindow(pState->hEditP3, TRUE);

        ShowWindow(pState->hComboVehicle, SW_HIDE);
    }

    // Pick Map Button: show only if coordinates or vehicle picking is relevant
    if (modeOpt.isCoords || modeOpt.isVehicle) {
        SetWindowTextW(pState->hBtnPickMap, modeOpt.pickBtnText);
        EnableWindow(pState->hBtnPickMap, TRUE);
        ShowWindow(pState->hBtnPickMap, SW_SHOW);
    } else {
        ShowWindow(pState->hBtnPickMap, SW_HIDE);
    }

    // Next Node ComboBox
    ComboBox_ResetContent(pState->hComboNext);
    int endIdx = ComboBox_AddString(pState->hComboNext, L"0: [End of Chain / Halt]");
    ComboBox_SetItemData(pState->hComboNext, endIdx, 0);

    auto chain = getFactionChain(pState->workingTactics, pState->currentFactionId);
    uint16_t nextTactic = rs_utl::READ_LE_UINT16(e.next_tactic);
    int nextSel = 0;

    for (int node : chain) {
        if (node == slot) continue;
        const auto &ne = pState->workingTactics.entries[node];
        std::wstring itemStr = formatNodeLabel(node, ne);
        int idx = ComboBox_AddString(pState->hComboNext, itemStr.c_str());
        ComboBox_SetItemData(pState->hComboNext, idx, node);
        if (node == nextTactic) {
            nextSel = idx;
        }
    }
    ComboBox_SetCurSel(pState->hComboNext, nextSel);

    // Armed condition checkbox
    if (modeOpt.allowsArmedCondition) {
        SetWindowPos(pState->hCheckArmed, nullptr, kInspectorX, kCheckArmedY, kInspectorW, 22, SWP_NOZORDER);
        Button_SetCheck(pState->hCheckArmed, (e.sub_filter & LevelData::kAITacticFlagArmedRequired) ? BST_CHECKED : BST_UNCHECKED);
        ShowWindow(pState->hCheckArmed, SW_SHOW);
    } else {
        ShowWindow(pState->hCheckArmed, SW_HIDE);
    }

    RECT rcInspector = { 400, 50, 815, 530 };
    InvalidateRect(pState->hDlg, &rcInspector, TRUE);

    pState->isPopulating = false;
}

void refreshTacticsList(AiDlgState *pState, int preferredSelectSlot = -1) {
    if (!pState || !pState->hListTactics) return;
    pState->isPopulating = true;
    ListBox_ResetContent(pState->hListTactics);

    auto chain = getFactionChain(pState->workingTactics, pState->currentFactionId);
    int selectIdx = -1;

    for (size_t i = 0; i < chain.size(); ++i) {
        int slot = chain[i];
        const auto &e = pState->workingTactics.entries[slot];
        std::wstring label = formatNodeLabel(slot, e, true);
        int idx = ListBox_AddString(pState->hListTactics, label.c_str());
        ListBox_SetItemData(pState->hListTactics, idx, slot);

        if (slot == preferredSelectSlot || (preferredSelectSlot == -1 && i == 0)) {
            selectIdx = idx;
        }
    }

    if (selectIdx >= 0) {
        ListBox_SetCurSel(pState->hListTactics, selectIdx);
        pState->selectedNodeSlot = static_cast<int>(ListBox_GetItemData(pState->hListTactics, selectIdx));
    } else {
        pState->selectedNodeSlot = -1;
    }

    // Status label
    size_t totalActive = 0;
    for (int i = 1; i < 128; ++i) {
        if (pState->workingTactics.entries[i].mode > 0) {
            totalActive++;
        }
    }
    std::wstring statusStr;
    if (pState->currentFactionId == 255) {
        statusStr = std::format(L"Total Mission Tactics: {} / 127", totalActive);
    } else {
        int agentCount = pState->pMission ? pState->pMission->countFactionAgents(pState->currentFactionId) : 0;
        statusStr = std::format(L"Placed Agents: {} / 8\tTotal Mission Tactics: {} / 127",
                                agentCount, totalActive);
    }
    SetWindowTextW(pState->hStaticStatus, statusStr.c_str());

    pState->isPopulating = false;
    updateInspectorFromNode(pState, pState->selectedNodeSlot);
}

void saveInspectorToNode(AiDlgState *pState) {
    if (!pState || pState->isPopulating || pState->selectedNodeSlot <= 0 || pState->selectedNodeSlot >= 128) return;

    auto &e = pState->workingTactics.entries[pState->selectedNodeSlot];

    // Mode
    int modeSel = ComboBox_GetCurSel(pState->hComboMode);
    if (modeSel < 0 || modeSel >= static_cast<int>(sizeof(kModes) / sizeof(kModes[0]))) {
        return;
    }
    const auto &modeOpt = kModes[modeSel];
    e.mode = modeOpt.mode;

    // Target Faction
    if (modeOpt.usesTargetFaction) {
        int targetSel = ComboBox_GetCurSel(pState->hComboTarget);
        if (targetSel >= 0) {
            e.sub_mode = static_cast<uint8_t>(ComboBox_GetItemData(pState->hComboTarget, targetSel));
        }
    } else {
        e.sub_mode = 0;
    }

    // P1, P2, P3
    if (modeOpt.mode == LevelData::kAITacticModeRivalSweep ||
        modeOpt.mode == LevelData::kAITacticModeObjectiveSweep) {
        // Mode 2 & 3 target a faction (0 = Player, 1..7 = Rival). param1 is the target faction ID.
        uint16_t tf = (e.sub_mode == 255) ? 0 : static_cast<uint16_t>(e.sub_mode);
        rs_utl::WRITE_LE_UINT16(e.param1, tf);
        rs_utl::WRITE_LE_UINT16(e.param2, 0);
        rs_utl::WRITE_LE_UINT16(e.param3, 0);
    } else if (modeOpt.isVehicle) {
        int vSel = ComboBox_GetCurSel(pState->hComboVehicle);
        uint16_t vIdx = (vSel >= 0) ? static_cast<uint16_t>(ComboBox_GetItemData(pState->hComboVehicle, vSel)) : 0;
        rs_utl::WRITE_LE_UINT16(e.param1, vIdx);
        rs_utl::WRITE_LE_UINT16(e.param2, 0);
        rs_utl::WRITE_LE_UINT16(e.param3, 0);
    } else {
        if (modeOpt.usesP1) {
            wchar_t buf[64];
            GetWindowTextW(pState->hEditP1, buf, 64);
            uint16_t p1 = static_cast<uint16_t>(_wtoi(buf));
            rs_utl::WRITE_LE_UINT16(e.param1, p1);
        } else {
            rs_utl::WRITE_LE_UINT16(e.param1, 0);
        }

        if (modeOpt.usesP2) {
            wchar_t buf[64];
            GetWindowTextW(pState->hEditP2, buf, 64);
            uint16_t p2 = static_cast<uint16_t>(_wtoi(buf));
            rs_utl::WRITE_LE_UINT16(e.param2, p2);
        } else {
            rs_utl::WRITE_LE_UINT16(e.param2, 0);
        }

        if (modeOpt.usesP3) {
            wchar_t buf[64];
            GetWindowTextW(pState->hEditP3, buf, 64);
            uint16_t p3 = static_cast<uint16_t>(_wtoi(buf));
            rs_utl::WRITE_LE_UINT16(e.param3, p3);
        } else {
            rs_utl::WRITE_LE_UINT16(e.param3, 0);
        }
    }

    // Next Node
    int nextSel = ComboBox_GetCurSel(pState->hComboNext);
    if (nextSel >= 0) {
        uint16_t oldNext = rs_utl::READ_LE_UINT16(e.next_tactic);
        uint16_t nextSlot = static_cast<uint16_t>(ComboBox_GetItemData(pState->hComboNext, nextSel));
        if (oldNext > 0 && oldNext < 128 && oldNext != nextSlot) {
            if (rs_utl::READ_LE_UINT16(pState->workingTactics.entries[oldNext].prev_tactic) == pState->selectedNodeSlot) {
                rs_utl::WRITE_LE_UINT16(pState->workingTactics.entries[oldNext].prev_tactic, 0);
            }
        }
        rs_utl::WRITE_LE_UINT16(e.next_tactic, nextSlot);
        if (nextSlot > 0 && nextSlot < 128) {
            rs_utl::WRITE_LE_UINT16(pState->workingTactics.entries[nextSlot].prev_tactic,
                                   static_cast<uint16_t>(pState->selectedNodeSlot));
        }
    }

    // Flags
    uint8_t flags = 0;
    if (IsWindowVisible(pState->hCheckArmed) && Button_GetCheck(pState->hCheckArmed) == BST_CHECKED) {
        flags |= LevelData::kAITacticFlagArmedRequired;
    }
    e.sub_filter = flags;

    // Refresh list item text
    int curSel = ListBox_GetCurSel(pState->hListTactics);
    if (curSel >= 0) {
        int slot = static_cast<int>(ListBox_GetItemData(pState->hListTactics, curSel));
        if (slot == pState->selectedNodeSlot) {
            std::wstring newLabel = formatNodeLabel(slot, e, true);
            pState->isPopulating = true;
            ListBox_DeleteString(pState->hListTactics, curSel);
            ListBox_InsertString(pState->hListTactics, curSel, newLabel.c_str());
            ListBox_SetItemData(pState->hListTactics, curSel, slot);
            ListBox_SetCurSel(pState->hListTactics, curSel);
            pState->isPopulating = false;
        }
    }
}

int findFreeSlot(const LevelData::AITacticsBlock &tactics) {
    for (int i = 1; i < 128; ++i) {
        if (tactics.entries[i].mode == 0) {
            return i;
        }
    }
    return -1;
}

void addNewNode(AiDlgState *pState) {
    int slot = findFreeSlot(pState->workingTactics);
    if (slot == -1) {
        showEditorMessageBox(pState->hDlg, L"Cannot add node: Maximum AI tactic capacity (127 nodes) reached.",
                           L"AI Tactics Limit", MB_ICONWARNING | MB_OK);
        return;
    }

    auto chain = getFactionChain(pState->workingTactics, pState->currentFactionId);

    auto &e = pState->workingTactics.entries[slot];
    memset(&e, 0, sizeof(e));
    e.mode = LevelData::kAITacticModeRivalSweep;
    e.sub_mode = LevelData::kAIFactionPlayer; // 0
    e.faction_id = pState->currentFactionId;
    e.sub_filter = 0;
    rs_utl::WRITE_LE_UINT16(e.param1, LevelData::kAIFactionPlayer);
    if (!chain.empty()) {
        int lastSlot = chain.back();
        rs_utl::WRITE_LE_UINT16(pState->workingTactics.entries[lastSlot].next_tactic, static_cast<uint16_t>(slot));
        rs_utl::WRITE_LE_UINT16(e.prev_tactic, static_cast<uint16_t>(lastSlot));
    } else {
        rs_utl::WRITE_LE_UINT16(e.prev_tactic, 0);
    }
    rs_utl::WRITE_LE_UINT16(e.next_tactic, 0);

    refreshTacticsList(pState, slot);
}

void deleteSelectedNode(AiDlgState *pState) {
    int slot = pState->selectedNodeSlot;
    if (slot <= 0 || slot >= 128) return;

    auto &e = pState->workingTactics.entries[slot];
    uint16_t prev = rs_utl::READ_LE_UINT16(e.prev_tactic);
    uint16_t next = rs_utl::READ_LE_UINT16(e.next_tactic);

    if (prev > 0 && prev < 128) {
        rs_utl::WRITE_LE_UINT16(pState->workingTactics.entries[prev].next_tactic, next);
    }
    if (next > 0 && next < 128) {
        rs_utl::WRITE_LE_UINT16(pState->workingTactics.entries[next].prev_tactic, prev);
    }

    memset(&e, 0, sizeof(e));

    refreshTacticsList(pState, prev > 0 ? prev : next);
}

void moveNode(AiDlgState *pState, bool up) {
    int slot = pState->selectedNodeSlot;
    if (slot <= 0 || slot >= 128) return;

    auto chain = getFactionChain(pState->workingTactics, pState->currentFactionId);
    if (chain.size() < 2) return;

    int idx = -1;
    for (size_t i = 0; i < chain.size(); ++i) {
        if (chain[i] == slot) {
            idx = static_cast<int>(i);
            break;
        }
    }
    if (idx == -1) return;

    int targetIdx = up ? (idx - 1) : (idx + 1);
    if (targetIdx < 0 || targetIdx >= static_cast<int>(chain.size())) return;

    int otherSlot = chain[targetIdx];
    auto &cur = pState->workingTactics.entries[slot];
    auto &oth = pState->workingTactics.entries[otherSlot];

    // Swap node chain positions
    uint16_t curPrev = rs_utl::READ_LE_UINT16(cur.prev_tactic);
    uint16_t curNext = rs_utl::READ_LE_UINT16(cur.next_tactic);
    uint16_t othPrev = rs_utl::READ_LE_UINT16(oth.prev_tactic);
    uint16_t othNext = rs_utl::READ_LE_UINT16(oth.next_tactic);

    if (up) {
        if (othPrev > 0 && othPrev < 128) {
            rs_utl::WRITE_LE_UINT16(pState->workingTactics.entries[othPrev].next_tactic, static_cast<uint16_t>(slot));
        }
        if (curNext > 0 && curNext < 128) {
            rs_utl::WRITE_LE_UINT16(pState->workingTactics.entries[curNext].prev_tactic, static_cast<uint16_t>(otherSlot));
        }
        rs_utl::WRITE_LE_UINT16(cur.prev_tactic, othPrev);
        rs_utl::WRITE_LE_UINT16(cur.next_tactic, static_cast<uint16_t>(otherSlot));
        rs_utl::WRITE_LE_UINT16(oth.prev_tactic, static_cast<uint16_t>(slot));
        rs_utl::WRITE_LE_UINT16(oth.next_tactic, curNext);
    } else {
        if (curPrev > 0 && curPrev < 128) {
            rs_utl::WRITE_LE_UINT16(pState->workingTactics.entries[curPrev].next_tactic, static_cast<uint16_t>(otherSlot));
        }
        if (othNext > 0 && othNext < 128) {
            rs_utl::WRITE_LE_UINT16(pState->workingTactics.entries[othNext].prev_tactic, static_cast<uint16_t>(slot));
        }
        rs_utl::WRITE_LE_UINT16(oth.prev_tactic, curPrev);
        rs_utl::WRITE_LE_UINT16(oth.next_tactic, static_cast<uint16_t>(slot));
        rs_utl::WRITE_LE_UINT16(cur.prev_tactic, static_cast<uint16_t>(otherSlot));
        rs_utl::WRITE_LE_UINT16(cur.next_tactic, othNext);
    }

    refreshTacticsList(pState, slot);
}

void clearCurrentFaction(AiDlgState *pState) {
    if (showEditorMessageBox(pState->hDlg,
                           L"Are you sure you want to clear all tactic nodes for this faction?",
                           L"Clear Faction Tactics", MB_ICONQUESTION | MB_YESNO) != IDYES) {
        return;
    }

    for (int i = 1; i < 128; ++i) {
        if (pState->workingTactics.entries[i].faction_id == pState->currentFactionId) {
            memset(&pState->workingTactics.entries[i], 0, sizeof(LevelData::AITactic));
        }
    }
    refreshTacticsList(pState, -1);
}

LRESULT CALLBACK AiDlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto *pState = reinterpret_cast<AiDlgState*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (msg == WM_ERASEBKGND) {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, s_hbrDlgBg ? s_hbrDlgBg : static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        return 1;
    }



    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    switch (msg) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        int code = HIWORD(wParam);

        if (!pState) break;

        if (id == IDC_COMBO_FACTION && code == CBN_SELCHANGE) {
            int sel = ComboBox_GetCurSel(pState->hComboFaction);
            if (sel >= 0 && sel < static_cast<int>(sizeof(kFactions)/sizeof(kFactions[0]))) {
                pState->currentFactionId = kFactions[sel].id;
                refreshTacticsList(pState, -1);
            }
            return 0;
        }

        if (id == IDC_LIST_TACTICS && code == LBN_SELCHANGE) {
            int sel = ListBox_GetCurSel(pState->hListTactics);
            if (sel >= 0) {
                pState->selectedNodeSlot = static_cast<int>(ListBox_GetItemData(pState->hListTactics, sel));
                updateInspectorFromNode(pState, pState->selectedNodeSlot);
            }
            return 0;
        }

        if (id == IDC_BTN_ADD_NODE) {
            addNewNode(pState);
            return 0;
        }

        if (id == IDC_BTN_DEL_NODE) {
            deleteSelectedNode(pState);
            return 0;
        }

        if (id == IDC_BTN_MOVE_UP) {
            moveNode(pState, true);
            return 0;
        }

        if (id == IDC_BTN_MOVE_DOWN) {
            moveNode(pState, false);
            return 0;
        }

        if (id == IDC_BTN_CLEAR_FACTION) {
            clearCurrentFaction(pState);
            return 0;
        }

        // Mode Combobox selection change
        if (id == IDC_COMBO_MODE && code == CBN_SELCHANGE) {
            int oldModeSel = -1;
            if (pState->selectedNodeSlot > 0 && pState->selectedNodeSlot < 128) {
                uint8_t oldMode = pState->workingTactics.entries[pState->selectedNodeSlot].mode;
                for (size_t i = 0; i < sizeof(kModes) / sizeof(kModes[0]); ++i) {
                    if (kModes[i].mode == oldMode) {
                        oldModeSel = static_cast<int>(i);
                        break;
                    }
                }
            }

            int newModeSel = ComboBox_GetCurSel(pState->hComboMode);
            if (newModeSel >= 0 && newModeSel < static_cast<int>(sizeof(kModes) / sizeof(kModes[0]))) {
                const auto &newModeOpt = kModes[newModeSel];
                if (oldModeSel != newModeSel && pState->selectedNodeSlot > 0 && pState->selectedNodeSlot < 128) {
                    auto &e = pState->workingTactics.entries[pState->selectedNodeSlot];
                    bool wasCoords = (oldModeSel >= 0 && kModes[oldModeSel].isCoords);
                    bool nowCoords = newModeOpt.isCoords;
                    if (wasCoords != nowCoords || !newModeOpt.usesP1) {
                        rs_utl::WRITE_LE_UINT16(e.param1, 0);
                        SetWindowTextW(pState->hEditP1, L"0");
                    }
                    if (!newModeOpt.usesP2) {
                        rs_utl::WRITE_LE_UINT16(e.param2, 0);
                        SetWindowTextW(pState->hEditP2, L"0");
                    }
                    if (!newModeOpt.usesP3) {
                        rs_utl::WRITE_LE_UINT16(e.param3, 0);
                        SetWindowTextW(pState->hEditP3, L"0");
                    }
                    if (!newModeOpt.allowsWildcardTarget && e.sub_mode == 255) {
                        e.sub_mode = (newModeOpt.mode == LevelData::kAITacticModeSeekDestroy) ? 1 : LevelData::kAIFactionPlayer;
                    }
                    if (newModeOpt.mode == LevelData::kAITacticModeSeekDestroy &&
                        e.sub_mode == LevelData::kAIFactionPlayer) {
                        e.sub_mode = 1;
                    }
                }
            }
            saveInspectorToNode(pState);
            updateInspectorFromNode(pState, pState->selectedNodeSlot);
            return 0;
        }

        // Vehicle Combobox selection change
        if (id == IDC_COMBO_VEHICLE && code == CBN_SELCHANGE) {
            int sel = ComboBox_GetCurSel(pState->hComboVehicle);
            if (sel >= 0) {
                uint16_t vIdx = static_cast<uint16_t>(ComboBox_GetItemData(pState->hComboVehicle, sel));
                SetWindowTextW(pState->hEditP1, std::to_wstring(vIdx).c_str());
                saveInspectorToNode(pState);
            }
            return 0;
        }

        // Other inspector edit triggers
        if (id == IDC_COMBO_TARGET && code == CBN_SELCHANGE) {
            saveInspectorToNode(pState);
            return 0;
        }
        if (id == IDC_COMBO_NEXT && code == CBN_SELCHANGE) {
            saveInspectorToNode(pState);
            refreshTacticsList(pState, pState->selectedNodeSlot);
            return 0;
        }

        if ((id == IDC_EDIT_P1 || id == IDC_EDIT_P2 || id == IDC_EDIT_P3) && code == EN_CHANGE) {
            saveInspectorToNode(pState);
            return 0;
        }

        if (id == IDC_CHECK_ARMED) {
            saveInspectorToNode(pState);
            return 0;
        }

        if (id == IDC_BTN_PICK_MAP && pState->pViewport) {
            int modeSel = ComboBox_GetCurSel(pState->hComboMode);
            if (modeSel < 0 || modeSel >= static_cast<int>(sizeof(kModes) / sizeof(kModes[0]))) {
                return 0;
            }
            const auto &modeOpt = kModes[modeSel];
            if (!modeOpt.isCoords && !modeOpt.isVehicle) {
                return 0;
            }

            ObjectivePickMode pickMode = modeOpt.isVehicle ? ObjectivePickMode::Vehicle : ObjectivePickMode::Coordinate;
            std::wstring pickPrompt = modeOpt.pickPrompt ? modeOpt.pickPrompt : L"";

            ShowWindow(hWnd, SW_HIDE);
            if (pState->hTopLevel) {
                EnableWindow(pState->hTopLevel, TRUE);
                SetForegroundWindow(pState->hTopLevel);
            }
            SetFocus(pState->pViewport->hwnd());

            pState->pViewport->beginObjectivePicking(pickMode, [hWnd, pState, modeOpt](const ObjectivePickResult &res) {
                if (pState->hTopLevel) {
                    EnableWindow(pState->hTopLevel, FALSE);
                }
                ShowWindow(hWnd, SW_SHOW);
                SetForegroundWindow(hWnd);
                SetFocus(hWnd);

                if (res.accepted) {
                    if (modeOpt.isVehicle) {
                        uint16_t vIdx = res.vehicleIdx;
                        SetWindowTextW(pState->hEditP1, std::to_wstring(vIdx).c_str());
                        if (pState->hComboVehicle) {
                            int cnt = ComboBox_GetCount(pState->hComboVehicle);
                            for (int i = 0; i < cnt; ++i) {
                                if (ComboBox_GetItemData(pState->hComboVehicle, i) == vIdx) {
                                    ComboBox_SetCurSel(pState->hComboVehicle, i);
                                    break;
                                }
                            }
                        }
                    } else {
                        SetWindowTextW(pState->hEditP1, std::to_wstring(res.tilePos.tx).c_str());
                        SetWindowTextW(pState->hEditP2, std::to_wstring(res.tilePos.ty).c_str());
                        SetWindowTextW(pState->hEditP3, std::to_wstring(res.tilePos.tz).c_str());
                    }
                    saveInspectorToNode(pState);
                    updateInspectorFromNode(pState, pState->selectedNodeSlot);
                }
            }, pickPrompt);
            return 0;
        }

        if (id == IDC_BTN_OK) {
            saveInspectorToNode(pState);
            if (pState->pCommandStack && pState->pMission) {
                pState->pCommandStack->pushAndExecute(
                    std::make_unique<EditAITacticsCommand>(pState->pMission,
                                                          pState->pMission->tactics(),
                                                          pState->workingTactics));
            } else if (pState->pMission) {
                pState->pMission->tactics() = pState->workingTactics;
            }
            DestroyWindow(hWnd);
            return 0;
        }

        if (id == IDC_BTN_CANCEL) {
            DestroyWindow(hWnd);
            return 0;
        }

        break;
    }

    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        return 0;

    default:
        break;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // anonymous namespace

bool AiTacticsDialog::show(HWND hParent, WinMapViewport *pViewport, rs_knl::Mission *pMission,
                          EditorCommandStack *pCommandStack, int preselectedFaction) {
    if (!pMission) return false;

    HINSTANCE hInst = GetModuleHandleW(nullptr);
    ensureClassRegistered(hInst, AiDlgProc);

    AiDlgState state;
    state.hParent = hParent;
    state.hTopLevel = GetAncestor(hParent, GA_ROOT);
    state.pViewport = pViewport;
    state.pMission = pMission;
    state.pCommandStack = pCommandStack;
    state.workingTactics = pMission->tactics();

    if (preselectedFaction >= 1 && preselectedFaction <= 7) {
        state.currentFactionId = static_cast<uint8_t>(preselectedFaction);
    } else {
        state.currentFactionId = 1; // Default to Blue Syndicate
    }

    HWND hOwner = state.hTopLevel ? state.hTopLevel : hParent;
    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"ReSyndicatedAiTacticsDlgClass",
        L"AI Tactics Editor",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 840, 630,
        hOwner, nullptr, hInst, nullptr
    );
    if (!hDlg) return false;

    state.hDlg = hDlg;
    SetWindowLongPtr(hDlg, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&state));

    HFONT hFont = CreateFontW(-12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontBold = CreateFontW(-12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontSmall = CreateFontW(-11, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Top: Faction selection
    CreateWindowExW(0, L"STATIC", L"Select Syndicate Faction:", WS_CHILD | WS_VISIBLE,
                    20, 16, 180, 20, hDlg, nullptr, hInst, nullptr);

    state.hComboFaction = CreateWindowExW(0, L"COMBOBOX", L"",
                                         WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                         200, 14, 420, 240, hDlg,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_FACTION)),
                                         hInst, nullptr);
    applyComboBoxTheme(state.hComboFaction);

    int initialFactionSel = 0;
    for (size_t i = 0; i < sizeof(kFactions)/sizeof(kFactions[0]); ++i) {
        int idx = ComboBox_AddString(state.hComboFaction, kFactions[i].name);
        if (kFactions[i].id == state.currentFactionId) {
            initialFactionSel = idx;
        }
    }
    ComboBox_SetCurSel(state.hComboFaction, initialFactionSel);

    // Left Side: Tactics Execution Chain
    CreateWindowExW(0, L"STATIC", L"Tactics Execution Chain:", WS_CHILD | WS_VISIBLE,
                    20, 48, 280, 20, hDlg, nullptr, hInst, nullptr);

    state.hListTactics = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", L"",
                                        WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY | WS_TABSTOP,
                                        20, 70, 360, 420, hDlg,
                                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LIST_TACTICS)),
                                        hInst, nullptr);

    // Left Side Buttons
    state.hBtnAddNode = CreateWindowExW(0, L"BUTTON", L"+ Add Node", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       20, 500, 85, 26, hDlg,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_ADD_NODE)),
                                       hInst, nullptr);
    state.hBtnDelNode = CreateWindowExW(0, L"BUTTON", L"- Delete", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       110, 500, 80, 26, hDlg,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_DEL_NODE)),
                                       hInst, nullptr);
    state.hBtnMoveUp = CreateWindowExW(0, L"BUTTON", L"▲ Up", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                      195, 500, 60, 26, hDlg,
                                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_MOVE_UP)),
                                      hInst, nullptr);
    state.hBtnMoveDown = CreateWindowExW(0, L"BUTTON", L"▼ Down", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                        260, 500, 60, 26, hDlg,
                                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_MOVE_DOWN)),
                                        hInst, nullptr);
    state.hBtnClearFaction = CreateWindowExW(0, L"BUTTON", L"Clear", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                            325, 500, 55, 26, hDlg,
                                            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CLEAR_FACTION)),
                                            hInst, nullptr);

    // Right Side: Tactic Node Inspector GroupBox
    state.hGroupInspector = CreateWindowExW(0, L"BUTTON", L"Current Tactic Node",
                                           WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_CLIPSIBLINGS,
                                           400, 50, 415, 480, hDlg, nullptr, hInst, nullptr);

    state.hStaticSlot = CreateWindowExW(0, L"STATIC", L"Tactic #", WS_CHILD | WS_VISIBLE,
                                       420, 75, 375, 18, hDlg, nullptr, hInst, nullptr);

    // Tactic Mode
    state.hStaticMode = CreateWindowExW(0, L"STATIC", L"Tactic Mode:", WS_CHILD | WS_VISIBLE,
                                       420, 100, 120, 16, hDlg, nullptr, hInst, nullptr);
    state.hComboMode = CreateWindowExW(0, L"COMBOBOX", L"",
                                      WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                      420, 118, 375, 260, hDlg,
                                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_MODE)),
                                      hInst, nullptr);
    applyComboBoxTheme(state.hComboMode);
    for (const auto &m : kModes) {
        ComboBox_AddString(state.hComboMode, m.title);
    }

    // Dynamic Mode Description Static (wrapped cleanly across 2+ lines)
    state.hStaticModeDesc = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                                           422, 144, 373, 40, hDlg, nullptr, hInst, nullptr);

    // Target Faction
    state.hStaticTarget = CreateWindowExW(0, L"STATIC", L"Target Faction:", WS_CHILD | WS_VISIBLE,
                                         420, 190, 375, 16, hDlg, nullptr, hInst, nullptr);
    state.hComboTarget = CreateWindowExW(0, L"COMBOBOX", L"",
                                        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                        420, 208, 375, 200, hDlg,
                                        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_TARGET)),
                                        hInst, nullptr);
    applyComboBoxTheme(state.hComboTarget);

    // Parameter 1
    state.hStaticP1 = CreateWindowExW(0, L"STATIC", L"Parameter 1:", WS_CHILD | WS_VISIBLE,
                                     kInspectorX, kParamLabelY, kInspectorW, 16, hDlg, nullptr, hInst, nullptr);
    state.hEditP1 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                                   WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
                                   kInspectorX, kParamEditY, 120, 24, hDlg,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_P1)),
                                   hInst, nullptr);

    // Vehicle dropdown (initially hidden, replaces P1 for Board Vehicle mode)
    state.hComboVehicle = CreateWindowExW(0, L"COMBOBOX", L"",
                                         WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP,
                                         kInspectorX, kParamEditY, kInspectorW, 200, hDlg,
                                         reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_VEHICLE)),
                                         hInst, nullptr);
    applyComboBoxTheme(state.hComboVehicle);

    // Parameter 2
    state.hStaticP2 = CreateWindowExW(0, L"STATIC", L"Parameter 2:", WS_CHILD | WS_VISIBLE,
                                     550, kParamLabelY, 120, 16, hDlg, nullptr, hInst, nullptr);
    state.hEditP2 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                                   WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
                                   550, kParamEditY, 115, 24, hDlg,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_P2)),
                                   hInst, nullptr);

    // Parameter 3
    state.hStaticP3 = CreateWindowExW(0, L"STATIC", L"Parameter 3:", WS_CHILD | WS_VISIBLE,
                                     675, kParamLabelY, 120, 16, hDlg, nullptr, hInst, nullptr);
    state.hEditP3 = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
                                   WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
                                   675, kParamEditY, 120, 24, hDlg,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_EDIT_P3)),
                                   hInst, nullptr);

    // Pick from Map
    state.hBtnPickMap = CreateWindowExW(0, L"BUTTON", L"Pick Target / Coords from Map Viewport",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                       kInspectorX, kPickMapY, kInspectorW, 26, hDlg,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PICK_MAP)),
                                       hInst, nullptr);

    // Next Node Transition
    state.hStaticNext = CreateWindowExW(0, L"STATIC", L"Next Tactic (on completion / trigger):",
                                       WS_CHILD | WS_VISIBLE,
                                       kInspectorX, kNextLabelY, kInspectorW, 16, hDlg, nullptr, hInst, nullptr);
    state.hComboNext = CreateWindowExW(0, L"COMBOBOX", L"",
                                      WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                                      kInspectorX, kNextComboY, kInspectorW, 220, hDlg,
                                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_COMBO_NEXT)),
                                      hInst, nullptr);
    applyComboBoxTheme(state.hComboNext);

    // Flags
    state.hCheckArmed = CreateWindowExW(0, L"BUTTON", L"Target must have weapon drawn / armed to trigger",
                                       WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                       kInspectorX, kCheckArmedY, kInspectorW, 22, hDlg,
                                       reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CHECK_ARMED)),
                                       hInst, nullptr);
    applyCheckBoxTheme(state.hCheckArmed);

    state.hStaticWildcardInfo = CreateWindowExW(
        0, L"STATIC",
        L"Wildcard Cycling Claim System:\r\n"
        L"Factions without their own tactics chain claim Wildcard root tactics sequentially in slot order. "
        L"Once all Wildcard roots are claimed, remaining factions wrap around and reuse them in order. "
        L"In other words, multiple Wildcard chains distribute distinct behaviors across factions rather than applying all tactics to all factions.",
        WS_CHILD,
        kInspectorX, kWildcardInfoY, kInspectorW, kWildcardInfoH, hDlg, nullptr, hInst, nullptr);

    // Status bar at bottom
    state.hStaticStatus = CreateWindowExW(0, L"STATIC", L"Ready", WS_CHILD | WS_VISIBLE,
                                         20, 556, 560, 20, hDlg, nullptr, hInst, nullptr);

    state.hBtnOk = CreateWindowExW(0, L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                                  605, 552, 100, 28, hDlg,
                                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_OK)),
                                  hInst, nullptr);

    state.hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                      715, 552, 100, 28, hDlg,
                                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_CANCEL)),
                                      hInst, nullptr);

    // Apply fonts to all controls
    HWND hChild = GetWindow(hDlg, GW_CHILD);
    while (hChild) {
        SendMessageW(hChild, WM_SETFONT, reinterpret_cast<WPARAM>(hFont), TRUE);
        hChild = GetWindow(hChild, GW_HWNDNEXT);
    }
    SendMessageW(state.hStaticSlot, WM_SETFONT, reinterpret_cast<WPARAM>(hFontBold), TRUE);
    SendMessageW(state.hStaticModeDesc, WM_SETFONT, reinterpret_cast<WPARAM>(hFontSmall), TRUE);
    SendMessageW(state.hStaticWildcardInfo, WM_SETFONT, reinterpret_cast<WPARAM>(hFontSmall), TRUE);

    applyDialogTheme(hDlg);
    enableDarkModeForWindow(hDlg);

    SetWindowSubclass(state.hStaticModeDesc, StaticLabelSubclass, 201, RGB(56, 189, 248));
    SetWindowSubclass(state.hStaticP1, StaticLabelSubclass, 202, kColorText);
    SetWindowSubclass(state.hStaticP2, StaticLabelSubclass, 203, kColorText);
    SetWindowSubclass(state.hStaticP3, StaticLabelSubclass, 204, kColorText);
    SetWindowSubclass(state.hStaticSlot, StaticLabelSubclass, 205, kColorAccentCyan);
    SetWindowSubclass(state.hStaticWildcardInfo, StaticLabelSubclass, 206, RGB(148, 163, 184));

    SetWindowPos(state.hGroupInspector, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    // Center dialog over top-level window
    RECT rcParent, rcDlg;
    HWND hCenter = state.hTopLevel ? state.hTopLevel : hParent;
    GetWindowRect(hCenter, &rcParent);
    GetWindowRect(hDlg, &rcDlg);
    int dlgW = rcDlg.right - rcDlg.left;
    int dlgH = rcDlg.bottom - rcDlg.top;
    int x = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int y = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    // Populate initial list
    refreshTacticsList(&state, -1);

    // Run modal message loop
    HWND hDisableTarget = state.hTopLevel ? state.hTopLevel : hParent;
    EnableWindow(hDisableTarget, FALSE);
    ShowWindow(hDlg, SW_SHOW);
    SetForegroundWindow(hDlg);
    SetFocus(state.hListTactics);

    MSG msg;
    while (IsWindow(hDlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            if (state.pViewport && state.pViewport->isObjectivePicking()) {
                state.pViewport->cancelObjectivePicking();
                continue;
            }
            if (IsWindowVisible(hDlg)) {
                DestroyWindow(hDlg);
                break;
            }
        }
        if (state.pViewport && state.pViewport->isObjectivePicking()) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            continue;
        }
        if (!IsDialogMessageW(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    MSG quitMsg;
    while (PeekMessageW(&quitMsg, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE)) {}

    EnableWindow(hDisableTarget, TRUE);
    SetForegroundWindow(hDisableTarget);

    if (hFont) DeleteObject(hFont);
    if (hFontBold) DeleteObject(hFontBold);
    if (hFontSmall) DeleteObject(hFontSmall);

    return true;
}

} // namespace rs_win_editor
