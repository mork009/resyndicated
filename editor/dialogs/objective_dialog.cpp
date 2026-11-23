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

#include "objective_dialog.h"
#ifndef RS_TEST_KERNEL
#include "dialog_theme.h"
#include "map_viewport.h"
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <format>
#include <vector>

#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/vehicle.h"

namespace rs_win_editor {

#ifndef RS_TEST_KERNEL
namespace {

static const int IDC_BTN_PICK_TARGET = 1001;
static const int IDC_BTN_PICK_COORD  = 1002;
static const int IDC_BTN_FIND_TARGET = 1003;
static const int IDC_BTN_FIND_COORD  = 1004;

struct ObjTypeItem {
    uint16_t typeId;
    const wchar_t *label;
};

static const ObjTypeItem kObjTypes[] = {
    { 0x01, L"Persuade Target Ped" },
    { 0x02, L"Assassinate Target Ped" },
    { 0x03, L"Protect Target Ped" },
    { 0x0B, L"Combat Sweep (Eliminate Enemy Agents)" },
    { 0x0E, L"Destroy Target Vehicle" },
    { 0x0F, L"Use / Steal Target Vehicle" },
    { 0x10, L"Evacuate Squad & Persuaded" }
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

struct ObjDlgState {
    HWND hTopLevel = nullptr;
    HWND hParent = nullptr;
    HWND hDlg = nullptr;
    WinMapViewport *pViewport = nullptr;
    rs_knl::Mission *pMission = nullptr;
    ObjectiveDialogData *pData = nullptr;
    bool isEdit = false;
    bool accepted = false;

    HWND hStaticType = nullptr;
    HWND hComboType = nullptr;

    HWND hStaticTarget = nullptr;
    HWND hComboTarget = nullptr;
    HWND hBtnPickTarget = nullptr;
    HWND hBtnFindTarget = nullptr;

    HWND hGroupEvac = nullptr;
    HWND hStaticTx = nullptr;
    HWND hEditTx = nullptr;
    HWND hStaticTy = nullptr;
    HWND hEditTy = nullptr;
    HWND hStaticTz = nullptr;
    HWND hEditTz = nullptr;
    HWND hBtnPickCoord = nullptr;
    HWND hBtnFindCoord = nullptr;

    HWND hBtnOk = nullptr;
    HWND hBtnCancel = nullptr;

    HFONT hFont = nullptr;

    std::vector<uint16_t> targetPedIds;
    std::vector<uint16_t> targetVehicleIndices;
    int editingIndex = -1;
};

void syncObjectivePreview(ObjDlgState *pState) {
    if (!pState || !pState->pViewport || !pState->pData) return;
    pState->pViewport->setObjectivePreview(
        pState->editingIndex,
        pState->pData->type,
        pState->pData->targetPedId,
        pState->pData->targetVehicleIdx,
        pState->pData->evacX,
        pState->pData->evacY,
        pState->pData->evacZ
    );
}

void updateDialogLayout(ObjDlgState *pState) {
    if (!pState || !pState->hDlg) return;

    int typeIdx = ComboBox_GetCurSel(pState->hComboType);
    if (typeIdx < 0 || typeIdx >= static_cast<int>(sizeof(kObjTypes)/sizeof(kObjTypes[0]))) {
        typeIdx = 0;
    }
    uint16_t typeId = kObjTypes[typeIdx].typeId;

    bool isPedObj = (typeId == 0x01 || typeId == 0x02 || typeId == 0x03);
    bool isVehObj = (typeId == 0x0E || typeId == 0x0F);
    bool showTarget = isPedObj || isVehObj;
    bool showEvac = (typeId == 0x10);

    int curY = 50;

    if (showTarget) {
        SetWindowPos(pState->hStaticTarget, nullptr, 20, curY + 2, 115, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pState->hComboTarget, nullptr, 140, curY, 216, 200, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pState->hBtnPickTarget, nullptr, 362, curY - 1, 100, 24, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pState->hBtnFindTarget, nullptr, 466, curY - 1, 92, 24, SWP_NOZORDER | SWP_NOACTIVATE);

        ShowWindow(pState->hStaticTarget, SW_SHOW);
        ShowWindow(pState->hComboTarget, SW_SHOW);
        ShowWindow(pState->hBtnPickTarget, SW_SHOW);
        ShowWindow(pState->hBtnFindTarget, SW_SHOW);

        curY += 36;
    } else {
        ShowWindow(pState->hStaticTarget, SW_HIDE);
        ShowWindow(pState->hComboTarget, SW_HIDE);
        ShowWindow(pState->hBtnPickTarget, SW_HIDE);
        ShowWindow(pState->hBtnFindTarget, SW_HIDE);
    }

    if (showEvac) {
        SetWindowPos(pState->hGroupEvac, nullptr, 20, curY, 538, 68, SWP_NOZORDER | SWP_NOACTIVATE);

        int innerY = curY + 28;
        SetWindowPos(pState->hStaticTx, nullptr, 32, innerY + 2, 24, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pState->hEditTx, nullptr, 58, innerY, 36, 22, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pState->hStaticTy, nullptr, 98, innerY + 2, 24, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pState->hEditTy, nullptr, 124, innerY, 36, 22, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pState->hStaticTz, nullptr, 164, innerY + 2, 24, 20, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pState->hEditTz, nullptr, 190, innerY, 36, 22, SWP_NOZORDER | SWP_NOACTIVATE);

        SetWindowPos(pState->hBtnPickCoord, nullptr, 234, innerY - 1, 178, 24, SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowPos(pState->hBtnFindCoord, nullptr, 418, innerY - 1, 130, 24, SWP_NOZORDER | SWP_NOACTIVATE);

        ShowWindow(pState->hGroupEvac, SW_SHOW);
        ShowWindow(pState->hStaticTx, SW_SHOW);
        ShowWindow(pState->hEditTx, SW_SHOW);
        ShowWindow(pState->hStaticTy, SW_SHOW);
        ShowWindow(pState->hEditTy, SW_SHOW);
        ShowWindow(pState->hStaticTz, SW_SHOW);
        ShowWindow(pState->hEditTz, SW_SHOW);
        ShowWindow(pState->hBtnPickCoord, SW_SHOW);
        ShowWindow(pState->hBtnFindCoord, SW_SHOW);

        curY += 76;
    } else {
        ShowWindow(pState->hGroupEvac, SW_HIDE);
        ShowWindow(pState->hStaticTx, SW_HIDE);
        ShowWindow(pState->hEditTx, SW_HIDE);
        ShowWindow(pState->hStaticTy, SW_HIDE);
        ShowWindow(pState->hEditTy, SW_HIDE);
        ShowWindow(pState->hStaticTz, SW_HIDE);
        ShowWindow(pState->hEditTz, SW_HIDE);
        ShowWindow(pState->hBtnPickCoord, SW_HIDE);
        ShowWindow(pState->hBtnFindCoord, SW_HIDE);
    }

    // OK and Cancel buttons
    int btnY = curY + 6;
    SetWindowPos(pState->hBtnOk, nullptr, 345, btnY, 100, 28, SWP_NOZORDER | SWP_NOACTIVATE);
    SetWindowPos(pState->hBtnCancel, nullptr, 452, btnY, 105, 28, SWP_NOZORDER | SWP_NOACTIVATE);

    int targetClientH = btnY + 28 + 16;

    RECT rcWnd = {}, rcCli = {};
    GetWindowRect(pState->hDlg, &rcWnd);
    GetClientRect(pState->hDlg, &rcCli);

    int borderH = (rcWnd.bottom - rcWnd.top) - (rcCli.bottom - rcCli.top);
    int newWndH = targetClientH + borderH;

    RECT rcWork = {};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &rcWork, 0);
    if (rcWnd.top + newWndH > rcWork.bottom) {
        int newTop = rcWork.bottom - newWndH - 10;
        if (newTop < rcWork.top) newTop = rcWork.top;
        SetWindowPos(pState->hDlg, nullptr, rcWnd.left, newTop, rcWnd.right - rcWnd.left, newWndH,
                     SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        SetWindowPos(pState->hDlg, nullptr, 0, 0, rcWnd.right - rcWnd.left, newWndH,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }

    InvalidateRect(pState->hDlg, nullptr, TRUE);
    UpdateWindow(pState->hDlg);
}

void updateTargetCombo(ObjDlgState *pState) {
    if (!pState || !pState->pMission) return;

    ComboBox_ResetContent(pState->hComboTarget);
    pState->targetPedIds.clear();
    pState->targetVehicleIndices.clear();

    int typeIdx = ComboBox_GetCurSel(pState->hComboType);
    if (typeIdx < 0 || typeIdx >= static_cast<int>(sizeof(kObjTypes)/sizeof(kObjTypes[0]))) {
        typeIdx = 0;
    }
    uint16_t typeId = kObjTypes[typeIdx].typeId;

    bool isPedObj = (typeId == 0x01 || typeId == 0x02 || typeId == 0x03);
    bool isVehObj = (typeId == 0x0E || typeId == 0x0F);
    bool isEvac = (typeId == 0x10);

    EnableWindow(pState->hComboTarget, isPedObj || isVehObj);
    EnableWindow(pState->hBtnPickTarget, (isPedObj || isVehObj) && pState->pViewport != nullptr);
    EnableWindow(pState->hBtnFindTarget, (isPedObj || isVehObj) && pState->pViewport != nullptr);
    EnableWindow(pState->hEditTx, isEvac);
    EnableWindow(pState->hEditTy, isEvac);
    EnableWindow(pState->hEditTz, isEvac);
    EnableWindow(pState->hBtnPickCoord, isEvac && pState->pViewport != nullptr);
    EnableWindow(pState->hBtnFindCoord, isEvac && pState->pViewport != nullptr);

    if (isPedObj) {
        SetWindowTextW(pState->hStaticTarget, L"Target Pedestrian:");
        int selIdx = 0;
        for (size_t i = 0; i < pState->pMission->numPeds(); ++i) {
            rs_knl::PedInstance *p = pState->pMission->ped(i);
            if (!p || p->isOurAgent()) continue;
            uint16_t pid = p->id();
            pState->targetPedIds.push_back(pid);

            const wchar_t *typeStr = L"Civilian";
            if (p->type() == rs_knl::PedInstance::kPedTypeGuard) typeStr = L"Guard";
            else if (p->type() == rs_knl::PedInstance::kPedTypePolice) typeStr = L"Police";
            else if (p->type() == rs_knl::PedInstance::kPedTypeAgent) typeStr = L"Agent";
            else if (p->type() == rs_knl::PedInstance::kPedTypeCriminal) typeStr = L"Criminal";

            std::wstring label = std::format(L"Ped #{:03} [{}] at (TX:{:02}, TY:{:02}, TZ:{:02})",
                pid, typeStr, p->tileX(), p->tileY(), p->tileZ());
            ComboBox_AddString(pState->hComboTarget, label.c_str());

            if (pid == pState->pData->targetPedId) {
                selIdx = static_cast<int>(pState->targetPedIds.size() - 1);
            }
        }
        if (pState->targetPedIds.empty()) {
            ComboBox_AddString(pState->hComboTarget, L"(No pedestrians available on map)");
        } else {
            ComboBox_SetCurSel(pState->hComboTarget, selIdx);
        }
    } else if (isVehObj) {
        SetWindowTextW(pState->hStaticTarget, L"Target Vehicle:");
        int selIdx = 0;
        for (size_t i = 0; i < pState->pMission->numVehicles(); ++i) {
            rs_knl::Vehicle *v = pState->pMission->vehicle(i);
            if (!v) continue;
            pState->targetVehicleIndices.push_back(static_cast<uint16_t>(i));

            std::wstring label = std::format(L"Vehicle #{:02} [{}] at (TX:{:02}, TY:{:02}, TZ:{:02})",
                i, getVehicleTypeName(v->getType()), v->tileX(), v->tileY(), v->tileZ());
            ComboBox_AddString(pState->hComboTarget, label.c_str());

            if (i == pState->pData->targetVehicleIdx) {
                selIdx = static_cast<int>(pState->targetVehicleIndices.size() - 1);
            }
        }
        if (pState->targetVehicleIndices.empty()) {
            ComboBox_AddString(pState->hComboTarget, L"(No vehicles available on map)");
        } else {
            ComboBox_SetCurSel(pState->hComboTarget, selIdx);
        }
    } else {
        SetWindowTextW(pState->hStaticTarget, L"Target Entity:");
        ComboBox_AddString(pState->hComboTarget, L"(None required for this objective)");
        ComboBox_SetCurSel(pState->hComboTarget, 0);
    }
}

LRESULT CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    ObjDlgState *pState = reinterpret_cast<ObjDlgState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    bool handled = false;
    LRESULT lr = handleDialogColorMessages(hWnd, msg, wParam, lParam, handled);
    if (handled) return lr;

    switch (msg) {
        case WM_NCCREATE: {
            CREATESTRUCTW *cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
            pState = reinterpret_cast<ObjDlgState*>(cs->lpCreateParams);
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pState));
            return DefWindowProcW(hWnd, msg, wParam, lParam);
        }

        case WM_ERASEBKGND: {
            HDC hdc = reinterpret_cast<HDC>(wParam);
            RECT rc;
            GetClientRect(hWnd, &rc);
            HBRUSH hbr = CreateSolidBrush(kColorDlgBg);
            FillRect(hdc, &rc, hbr);
            DeleteObject(hbr);
            return 1;
        }

        case WM_CREATE: {
            pState->hDlg = hWnd;
            HINSTANCE hInst = GetModuleHandleW(nullptr);

            pState->hFont = CreateFontW(
                -12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );

            // 1. Objective Type
            pState->hStaticType = CreateWindowExW(0, WC_STATICW, L"Objective Type:",
                WS_CHILD | WS_VISIBLE, 20, 16, 115, 20, hWnd, nullptr, hInst, nullptr);
            pState->hComboType = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, 14, 418, 200, hWnd, nullptr, hInst, nullptr);

            int selectTypeIdx = 0;
            for (size_t i = 0; i < sizeof(kObjTypes)/sizeof(kObjTypes[0]); ++i) {
                ComboBox_AddString(pState->hComboType, kObjTypes[i].label);
                if (kObjTypes[i].typeId == pState->pData->type) {
                    selectTypeIdx = static_cast<int>(i);
                }
            }
            ComboBox_SetCurSel(pState->hComboType, selectTypeIdx);

            // 2. Target Entity
            pState->hStaticTarget = CreateWindowExW(0, WC_STATICW, L"Target Entity:",
                WS_CHILD | WS_VISIBLE, 20, 50, 115, 20, hWnd, nullptr, hInst, nullptr);
            pState->hComboTarget = CreateWindowExW(0, WC_COMBOBOXW, L"",
                WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, 48, 216, 200, hWnd, nullptr, hInst, nullptr);
            pState->hBtnPickTarget = CreateWindowExW(0, WC_BUTTONW, L"🎯 Pick on Map",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP, 362, 47, 100, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PICK_TARGET)), hInst, nullptr);
            pState->hBtnFindTarget = CreateWindowExW(0, WC_BUTTONW, L"🔍 Find Target",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP, 466, 47, 92, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_FIND_TARGET)), hInst, nullptr);

            // 3. Evacuate Coordinates GroupBox
            pState->hGroupEvac = CreateWindowExW(0, WC_BUTTONW, L"Evacuate Destination Tile Coordinates (Radius: 2 Tiles / 512 Units)",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 20, 84, 538, 68, hWnd, nullptr, hInst, nullptr);

            pState->hStaticTx = CreateWindowExW(0, WC_STATICW, L"TX:", WS_CHILD | WS_VISIBLE, 32, 114, 24, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditTx = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, std::to_wstring(pState->pData->evacX / 256).c_str(),
                WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_TABSTOP, 58, 112, 36, 22, hWnd, nullptr, hInst, nullptr);

            pState->hStaticTy = CreateWindowExW(0, WC_STATICW, L"TY:", WS_CHILD | WS_VISIBLE, 98, 114, 24, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditTy = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, std::to_wstring(pState->pData->evacY / 256).c_str(),
                WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_TABSTOP, 124, 112, 36, 22, hWnd, nullptr, hInst, nullptr);

            pState->hStaticTz = CreateWindowExW(0, WC_STATICW, L"TZ:", WS_CHILD | WS_VISIBLE, 164, 114, 24, 20, hWnd, nullptr, hInst, nullptr);
            pState->hEditTz = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, std::to_wstring(pState->pData->evacZ / 128).c_str(),
                WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_TABSTOP, 190, 112, 36, 22, hWnd, nullptr, hInst, nullptr);

            pState->hBtnPickCoord = CreateWindowExW(0, WC_BUTTONW, L"🎯 Pick Destination Tile",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP, 234, 111, 178, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_PICK_COORD)), hInst, nullptr);
            pState->hBtnFindCoord = CreateWindowExW(0, WC_BUTTONW, L"🔍 Find Zone",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP, 418, 111, 130, 24, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_FIND_COORD)), hInst, nullptr);

            // 4. Buttons
            pState->hBtnOk = CreateWindowExW(0, WC_BUTTONW, L"OK",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, 345, 166, 100, 28, hWnd, reinterpret_cast<HMENU>(IDOK), hInst, nullptr);
            pState->hBtnCancel = CreateWindowExW(0, WC_BUTTONW, L"Cancel",
                WS_CHILD | WS_VISIBLE | WS_TABSTOP, 452, 166, 105, 28, hWnd, reinterpret_cast<HMENU>(IDCANCEL), hInst, nullptr);

            // Set fonts
            EnumChildWindows(hWnd, [](HWND hChild, LPARAM lp) -> BOOL {
                HFONT font = reinterpret_cast<HFONT>(lp);
                SendMessageW(hChild, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
                return TRUE;
            }, reinterpret_cast<LPARAM>(pState->hFont));

            updateTargetCombo(pState);
            updateDialogLayout(pState);
            syncObjectivePreview(pState);
            applyDialogTheme(hWnd);
            return 0;
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);

            if (id == 0 && code == CBN_SELCHANGE) {
                // Ignore
            }
            if (lParam == reinterpret_cast<LPARAM>(pState->hComboType) && code == CBN_SELCHANGE) {
                int typeSel = ComboBox_GetCurSel(pState->hComboType);
                if (typeSel >= 0 && typeSel < static_cast<int>(sizeof(kObjTypes)/sizeof(kObjTypes[0]))) {
                    pState->pData->type = kObjTypes[typeSel].typeId;
                }
                updateTargetCombo(pState);
                updateDialogLayout(pState);
                int targetSel = ComboBox_GetCurSel(pState->hComboTarget);
                if (pState->pData->type == 0x01 || pState->pData->type == 0x02 || pState->pData->type == 0x03) {
                    if (targetSel >= 0 && targetSel < static_cast<int>(pState->targetPedIds.size())) {
                        pState->pData->targetPedId = pState->targetPedIds[targetSel];
                    }
                } else if (pState->pData->type == 0x0E || pState->pData->type == 0x0F) {
                    if (targetSel >= 0 && targetSel < static_cast<int>(pState->targetVehicleIndices.size())) {
                        pState->pData->targetVehicleIdx = pState->targetVehicleIndices[targetSel];
                    }
                }
                syncObjectivePreview(pState);
                return 0;
            }

            if (lParam == reinterpret_cast<LPARAM>(pState->hComboTarget) && code == CBN_SELCHANGE) {
                int targetSel = ComboBox_GetCurSel(pState->hComboTarget);
                if (pState->pData->type == 0x01 || pState->pData->type == 0x02 || pState->pData->type == 0x03) {
                    if (targetSel >= 0 && targetSel < static_cast<int>(pState->targetPedIds.size())) {
                        pState->pData->targetPedId = pState->targetPedIds[targetSel];
                    }
                } else if (pState->pData->type == 0x0E || pState->pData->type == 0x0F) {
                    if (targetSel >= 0 && targetSel < static_cast<int>(pState->targetVehicleIndices.size())) {
                        pState->pData->targetVehicleIdx = pState->targetVehicleIndices[targetSel];
                    }
                }
                syncObjectivePreview(pState);
                return 0;
            }

            if ((lParam == reinterpret_cast<LPARAM>(pState->hEditTx) ||
                 lParam == reinterpret_cast<LPARAM>(pState->hEditTy) ||
                 lParam == reinterpret_cast<LPARAM>(pState->hEditTz)) && code == EN_CHANGE) {
                wchar_t buf[64] = {};
                GetWindowTextW(pState->hEditTx, buf, 64);
                pState->pData->evacX = _wtoi(buf) * 256 + 128;
                GetWindowTextW(pState->hEditTy, buf, 64);
                pState->pData->evacY = _wtoi(buf) * 256 + 128;
                GetWindowTextW(pState->hEditTz, buf, 64);
                pState->pData->evacZ = _wtoi(buf) * 128;
                if (pState->pData->type == 0x10) {
                    syncObjectivePreview(pState);
                }
                return 0;
            }

            if (id == IDC_BTN_PICK_TARGET && pState->pViewport) {
                int typeSel = ComboBox_GetCurSel(pState->hComboType);
                if (typeSel < 0 || typeSel >= static_cast<int>(sizeof(kObjTypes)/sizeof(kObjTypes[0]))) {
                    typeSel = 0;
                }
                uint16_t typeId = kObjTypes[typeSel].typeId;
                ObjectivePickMode mode = (typeId == 0x0E || typeId == 0x0F) ? ObjectivePickMode::Vehicle : ObjectivePickMode::Ped;

                ShowWindow(hWnd, SW_HIDE);
                EnableWindow(pState->hParent, FALSE);
                if (pState->hTopLevel) {
                    EnableWindow(pState->hTopLevel, TRUE);
                    SetForegroundWindow(pState->hTopLevel);
                }
                SetFocus(pState->pViewport->hwnd());

                pState->pViewport->beginObjectivePicking(mode, [hWnd, pState](const ObjectivePickResult &res) {
                    if (pState->hTopLevel) {
                        EnableWindow(pState->hTopLevel, FALSE);
                    }
                    EnableWindow(pState->hParent, TRUE);
                    ShowWindow(hWnd, SW_SHOW);
                    SetForegroundWindow(hWnd);
                    SetFocus(hWnd);

                    if (res.accepted) {
                        if (res.pedId != 0) {
                            for (size_t i = 0; i < pState->targetPedIds.size(); ++i) {
                                if (pState->targetPedIds[i] == res.pedId) {
                                    ComboBox_SetCurSel(pState->hComboTarget, static_cast<int>(i));
                                    pState->pData->targetPedId = res.pedId;
                                    break;
                                }
                            }
                        } else {
                            for (size_t i = 0; i < pState->targetVehicleIndices.size(); ++i) {
                                if (pState->targetVehicleIndices[i] == res.vehicleIdx) {
                                    ComboBox_SetCurSel(pState->hComboTarget, static_cast<int>(i));
                                    pState->pData->targetVehicleIdx = res.vehicleIdx;
                                    break;
                                }
                            }
                        }
                        syncObjectivePreview(pState);
                    }
                });
                return 0;
            }

            if (id == IDC_BTN_PICK_COORD && pState->pViewport) {
                ShowWindow(hWnd, SW_HIDE);
                EnableWindow(pState->hParent, FALSE);
                if (pState->hTopLevel) {
                    EnableWindow(pState->hTopLevel, TRUE);
                    SetForegroundWindow(pState->hTopLevel);
                }
                SetFocus(pState->pViewport->hwnd());

                pState->pViewport->beginObjectivePicking(ObjectivePickMode::Coordinate, [hWnd, pState](const ObjectivePickResult &res) {
                    if (pState->hTopLevel) {
                        EnableWindow(pState->hTopLevel, FALSE);
                    }
                    EnableWindow(pState->hParent, TRUE);
                    ShowWindow(hWnd, SW_SHOW);
                    SetForegroundWindow(hWnd);
                    SetFocus(hWnd);

                    if (res.accepted) {
                        SetWindowTextW(pState->hEditTx, std::to_wstring(res.tilePos.tx).c_str());
                        SetWindowTextW(pState->hEditTy, std::to_wstring(res.tilePos.ty).c_str());
                        SetWindowTextW(pState->hEditTz, std::to_wstring(res.tilePos.tz).c_str());
                        pState->pData->evacX = res.tilePos.tx * 256 + 128;
                        pState->pData->evacY = res.tilePos.ty * 256 + 128;
                        pState->pData->evacZ = res.tilePos.tz * 128;
                        syncObjectivePreview(pState);
                    }
                });
                return 0;
            }

            if (id == IDC_BTN_FIND_TARGET && pState->pViewport && pState->pMission) {
                int typeSel = ComboBox_GetCurSel(pState->hComboType);
                if (typeSel < 0 || typeSel >= static_cast<int>(sizeof(kObjTypes)/sizeof(kObjTypes[0]))) {
                    typeSel = 0;
                }
                uint16_t typeId = kObjTypes[typeSel].typeId;
                if (typeId == 0x01 || typeId == 0x02 || typeId == 0x03) {
                    int sel = ComboBox_GetCurSel(pState->hComboTarget);
                    if (sel >= 0 && sel < static_cast<int>(pState->targetPedIds.size())) {
                        uint16_t pid = pState->targetPedIds[sel];
                        for (size_t i = 0; i < pState->pMission->numPeds(); ++i) {
                            auto *p = pState->pMission->ped(i);
                            if (p && p->id() == pid) {
                                pState->pViewport->centerOnTile(p->position());
                                InvalidateRect(pState->pViewport->hwnd(), nullptr, FALSE);
                                break;
                            }
                        }
                    }
                } else if (typeId == 0x0E || typeId == 0x0F) {
                    int sel = ComboBox_GetCurSel(pState->hComboTarget);
                    if (sel >= 0 && sel < static_cast<int>(pState->targetVehicleIndices.size())) {
                        uint16_t vidx = pState->targetVehicleIndices[sel];
                        if (vidx < pState->pMission->numVehicles()) {
                            auto *v = pState->pMission->vehicle(vidx);
                            if (v) {
                                pState->pViewport->centerOnTile(v->position());
                                InvalidateRect(pState->pViewport->hwnd(), nullptr, FALSE);
                            }
                        }
                    }
                }
                return 0;
            }

            if (id == IDC_BTN_FIND_COORD && pState->pViewport) {
                wchar_t buf[64] = {};
                GetWindowTextW(pState->hEditTx, buf, 64);
                int tx = _wtoi(buf);
                GetWindowTextW(pState->hEditTy, buf, 64);
                int ty = _wtoi(buf);
                GetWindowTextW(pState->hEditTz, buf, 64);
                int tz = _wtoi(buf);
                pState->pViewport->centerOnTile(rs_knl::TilePoint(tx, ty, tz));
                InvalidateRect(pState->pViewport->hwnd(), nullptr, FALSE);
                return 0;
            }

            if (id == IDOK) {
                int typeSel = ComboBox_GetCurSel(pState->hComboType);
                if (typeSel >= 0 && typeSel < static_cast<int>(sizeof(kObjTypes)/sizeof(kObjTypes[0]))) {
                    pState->pData->type = kObjTypes[typeSel].typeId;
                }

                int targetSel = ComboBox_GetCurSel(pState->hComboTarget);
                if (pState->pData->type == 0x01 || pState->pData->type == 0x02 || pState->pData->type == 0x03) {
                    if (targetSel >= 0 && targetSel < static_cast<int>(pState->targetPedIds.size())) {
                        pState->pData->targetPedId = pState->targetPedIds[targetSel];
                    }
                } else if (pState->pData->type == 0x0E || pState->pData->type == 0x0F) {
                    if (targetSel >= 0 && targetSel < static_cast<int>(pState->targetVehicleIndices.size())) {
                        pState->pData->targetVehicleIdx = pState->targetVehicleIndices[targetSel];
                    }
                }

                wchar_t buf[128];
                GetWindowTextW(pState->hEditTx, buf, 64);
                pState->pData->evacX = _wtoi(buf) * 256 + 128;
                GetWindowTextW(pState->hEditTy, buf, 64);
                pState->pData->evacY = _wtoi(buf) * 256 + 128;
                GetWindowTextW(pState->hEditTz, buf, 64);
                pState->pData->evacZ = _wtoi(buf) * 128;
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
            if (pState && pState->pViewport && pState->pViewport->isObjectivePicking()) {
                pState->pViewport->cancelObjectivePicking();
            }
            if (pState && pState->pViewport) {
                pState->pViewport->clearObjectivePreview();
            }
            if (pState && pState->hTopLevel) {
                EnableWindow(pState->hTopLevel, TRUE);
            }
            if (pState && pState->hParent) {
                EnableWindow(pState->hParent, TRUE);
            }
            if (pState && pState->hFont) {
                DeleteObject(pState->hFont);
                pState->hFont = nullptr;
            }
            return 0;
        }
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

} // namespace
#endif // !RS_TEST_KERNEL

ObjectiveDialogData ObjectiveDialog::extractDataFromObjective(rs_knl::Mission *pMission, rs_knl::ObjectiveDesc *pObj) {
    ObjectiveDialogData d;
    if (!pObj) return d;
    d.msg = pObj->msg;

    if (auto *op = dynamic_cast<rs_knl::ObjPersuade*>(pObj)) {
        d.type = 0x01;
        if (auto *p = dynamic_cast<rs_knl::PedInstance*>(op->target())) {
            d.targetPedId = p->id();
        }
    } else if (auto *oa = dynamic_cast<rs_knl::ObjAssassinate*>(pObj)) {
        d.type = 0x02;
        if (auto *p = dynamic_cast<rs_knl::PedInstance*>(oa->target())) {
            d.targetPedId = p->id();
        }
    } else if (auto *opr = dynamic_cast<rs_knl::ObjProtect*>(pObj)) {
        d.type = 0x03;
        if (auto *p = dynamic_cast<rs_knl::PedInstance*>(opr->target())) {
            d.targetPedId = p->id();
        }
    } else if (dynamic_cast<rs_knl::ObjEliminate*>(pObj)) {
        d.type = 0x0B;
    } else if (auto *odv = dynamic_cast<rs_knl::ObjDestroyVehicle*>(pObj)) {
        d.type = 0x0E;
        if (pMission) {
            for (size_t i = 0; i < pMission->numVehicles(); ++i) {
                if (pMission->vehicle(i) == odv->target()) {
                    d.targetVehicleIdx = static_cast<uint16_t>(i);
                    break;
                }
            }
        }
    } else if (auto *ouv = dynamic_cast<rs_knl::ObjUseVehicle*>(pObj)) {
        d.type = 0x0F;
        if (pMission) {
            for (size_t i = 0; i < pMission->numVehicles(); ++i) {
                if (pMission->vehicle(i) == ouv->target()) {
                    d.targetVehicleIdx = static_cast<uint16_t>(i);
                    break;
                }
            }
        }
    } else if (auto *oev = dynamic_cast<rs_knl::ObjEvacuate*>(pObj)) {
        d.type = 0x10;
        d.evacX = oev->location().x;
        d.evacY = oev->location().y;
        d.evacZ = oev->location().z;
    }

    return d;
}

rs_knl::ObjectiveDesc* ObjectiveDialog::createObjectiveFromData(rs_knl::Mission *pMission, const ObjectiveDialogData &data) {
    if (!pMission) return nullptr;

    rs_knl::ObjectiveDesc *res = nullptr;

    if (data.type == 0x01) { // Persuade
        for (size_t i = 0; i < pMission->numPeds(); ++i) {
            rs_knl::PedInstance *p = pMission->ped(i);
            if (p && p->id() == data.targetPedId) {
                res = new rs_knl::ObjPersuade(p);
                break;
            }
        }
    } else if (data.type == 0x02) { // Assassinate
        for (size_t i = 0; i < pMission->numPeds(); ++i) {
            rs_knl::PedInstance *p = pMission->ped(i);
            if (p && p->id() == data.targetPedId) {
                res = new rs_knl::ObjAssassinate(p);
                break;
            }
        }
    } else if (data.type == 0x03) { // Protect
        for (size_t i = 0; i < pMission->numPeds(); ++i) {
            rs_knl::PedInstance *p = pMission->ped(i);
            if (p && p->id() == data.targetPedId) {
                res = new rs_knl::ObjProtect(p);
                break;
            }
        }
    } else if (data.type == 0x0B) { // Combat sweep
        res = new rs_knl::ObjEliminate(rs_knl::PedInstance::og_dmAgent);
    } else if (data.type == 0x0E) { // Destroy vehicle
        if (data.targetVehicleIdx < pMission->numVehicles()) {
            res = new rs_knl::ObjDestroyVehicle(pMission->vehicle(data.targetVehicleIdx));
        }
    } else if (data.type == 0x0F) { // Use vehicle
        if (data.targetVehicleIdx < pMission->numVehicles()) {
            res = new rs_knl::ObjUseVehicle(pMission->vehicle(data.targetVehicleIdx));
        }
    } else if (data.type == 0x10) { // Evacuate
        std::vector<rs_knl::PedInstance*> peds;
        for (size_t i = 0; i < pMission->numObjectives(); ++i) {
            if (auto *op = dynamic_cast<rs_knl::ObjPersuade*>(pMission->objective(i))) {
                if (auto *p = dynamic_cast<rs_knl::PedInstance*>(op->target())) {
                    peds.push_back(p);
                }
            }
        }
        res = new rs_knl::ObjEvacuate(data.evacX, data.evacY, data.evacZ, peds);
    }

    if (res) {
        res->msg = data.msg;
    }
    return res;
}

#ifndef RS_TEST_KERNEL
bool ObjectiveDialog::show(HWND hParent, WinMapViewport *pViewport, rs_knl::Mission *pMission, ObjectiveDialogData &data, bool isEdit, int editingIndex) {
    static bool s_registered = false;
    const wchar_t *kClassName = L"ReSyndicatedObjectiveDlgClass";
    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    if (!s_registered) {
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = DlgProc;
        wc.hInstance = hInstance;
        wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
        wc.hbrBackground = CreateSolidBrush(kColorDlgBg);
        wc.lpszClassName = kClassName;
        RegisterClassExW(&wc);
        s_registered = true;
    }

    HWND hTopLevel = GetAncestor(hParent, GA_ROOT);
    if (!hTopLevel) hTopLevel = hParent;

    ObjDlgState state;
    state.hTopLevel = hTopLevel;
    state.hParent = hParent;
    state.pViewport = pViewport;
    state.pMission = pMission;
    state.pData = &data;
    state.isEdit = isEdit;
    state.editingIndex = editingIndex;

    RECT rcParent = {};
    GetWindowRect(hParent, &rcParent);
    int dlgW = 580;

    bool isPedObj = (data.type == 0x01 || data.type == 0x02 || data.type == 0x03);
    bool isVehObj = (data.type == 0x0E || data.type == 0x0F);
    bool isEvac = (data.type == 0x10);

    int clientH = 100;
    if (isPedObj || isVehObj) clientH = 136;
    else if (isEvac) clientH = 176;

    int dlgH = clientH + 39;
    int dlgX = rcParent.left + ((rcParent.right - rcParent.left) - dlgW) / 2;
    int dlgY = rcParent.top + ((rcParent.bottom - rcParent.top) - dlgH) / 2;
    if (dlgX < 0) dlgX = 0;
    if (dlgY < 0) dlgY = 0;

    std::wstring title = isEdit ? L"Edit Mission Objective" : L"Add New Mission Objective";

    HWND hDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        kClassName,
        title.c_str(),
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
            if (IsWindowVisible(hDlg)) {
                DestroyWindow(hDlg);
                break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(hTopLevel, TRUE);
    SetForegroundWindow(hTopLevel);
    SetFocus(hTopLevel);

    return state.accepted;
}
#else
bool ObjectiveDialog::show(HWND, WinMapViewport *, rs_knl::Mission *, ObjectiveDialogData &, bool, int) {
    return false;
}
#endif

} // namespace rs_win_editor
