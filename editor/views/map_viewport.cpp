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

#include "map_viewport.h"
#include "tile_rasterizer.h"
#include "sprite_rasterizer.h"
#include "dialog_theme.h"

#include <windows.h>
#include <windowsx.h>
#include <cmath>
#include <algorithm>
#include <format>

#include "rs-engine/gfx/tile.h"
#include "rs-kernel/model/static.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/weapon.h"
#include "rs-kernel/model/leveldata.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/mgr/pedmanager.h"
#include "rs-kernel/mgr/vehiclemanager.h"
#include "rs-kernel/model/objectivedesc.h"
#include "rs-utils/io/file.h"
#include "core/editor_config.h"

namespace rs_win_editor {

static const wchar_t* kViewportClassName = L"ReSyndicatedMapViewportClass";

WinMapViewport::WinMapViewport() = default;

WinMapViewport::~WinMapViewport() {
    if (hDIB_) {
        DeleteObject(hDIB_);
        hDIB_ = nullptr;
    }
}

bool WinMapViewport::create(HWND hWndParent, int x, int y, int w, int h, int id) {
    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = kViewportClassName;
    RegisterClassExW(&wc);

    hWnd_ = CreateWindowExW(
        0, kViewportClassName, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        x, y, w, h,
        hWndParent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        hInstance, this
    );

    recreateDIB(w, h);
    return (hWnd_ != nullptr);
}

LRESULT CALLBACK WinMapViewport::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    WinMapViewport *pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<WinMapViewport*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<WinMapViewport*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->handleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT WinMapViewport::handleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                if (objectivePickMode_ != ObjectivePickMode::None) {
                    if (isHoveringPickCancel_) {
                        SetCursor(LoadCursor(nullptr, IDC_ARROW));
                    } else {
                        SetCursor(LoadCursor(nullptr, IDC_CROSS));
                    }
                    return TRUE;
                }
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            onPaint(hdc);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            onResize(w, h);
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            onMouseMove(x, y, static_cast<UINT>(wParam));
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            SetFocus(hWnd);
            onMouseDown(x, y, 1);
            return 0;
        }

        case WM_LBUTTONUP: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            onMouseUp(x, y, 1);
            return 0;
        }

        case WM_MBUTTONDOWN: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            SetFocus(hWnd);
            onMouseDown(x, y, 2);
            return 0;
        }

        case WM_MBUTTONUP: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            onMouseUp(x, y, 2);
            return 0;
        }

        case WM_RBUTTONDOWN: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            SetFocus(hWnd);
            onMouseDown(x, y, 3);
            return 0;
        }

        case WM_RBUTTONUP: {
            int x = static_cast<short>(LOWORD(lParam));
            int y = static_cast<short>(HIWORD(lParam));
            onMouseUp(x, y, 3);
            return 0;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hWnd, &pt);
            onMouseWheel(delta, pt.x, pt.y);
            return 0;
        }

        case WM_GETDLGCODE:
            return DLGC_WANTALLKEYS | DLGC_WANTTAB;

        case WM_KEYDOWN:
            onKeyDown(wParam, lParam);
            return 0;

        case WM_ERASEBKGND:
            return 1; // Prevent flicker
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool WinMapViewport::isPedVisible(const rs_knl::PedInstance *p) const {
    if (!p) return false;
    auto *ped = const_cast<rs_knl::PedInstance*>(p);
    switch (ped->type()) {
        case rs_knl::PedInstance::kPedTypeCivilian: return filterCivilians_;
        case rs_knl::PedInstance::kPedTypePolice:   return filterPolice_;
        case rs_knl::PedInstance::kPedTypeGuard:    return filterGuards_;
        case rs_knl::PedInstance::kPedTypeCriminal: return filterCriminals_;
        case rs_knl::PedInstance::kPedTypeAgent:    return filterAgents_;
        default: return true;
    }
}

bool WinMapViewport::isBuildingOrStructureTile(rs_knl::Map *pMap, int tx, int ty, int tz, uint8_t tileId) const {
    if (tileId <= 4) return false;

    // 1. All walls, roofs, ramps/stairs, and handrails/windows/railings
    // 0x0A = Wall, 0x0D = Roof, 0x01..0x04 = Slope (Stairs), 0x0C = Handrail
    uint8_t type = TileRasterizer::instance().getTile(tileId).type;
    if (type == 0x0A || type == 0x0D || (type >= 0x01 && type <= 0x04) || type == 0x0C) {
        return true;
    }

    // 2. Specific known flat roof & catwalk structural tiles
    // In Syndicate, flat rooftops and catwalk platforms are made walkable with type 0x05 (Ground).
    static const std::array<bool, 256> kIsRoofGroundTile = []() {
        std::array<bool, 256> arr{};
        for (int id : { 91, 92, 94, 95, 97, 98, 99,
                        180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193,
                        210, 212, 215, 216, 217, 222, 223, 224, 230, 231, 232, 233, 234, 249, 250 }) {
            arr[id] = true;
        }
        return arr;
    }();
    if (kIsRoofGroundTile[tileId]) {
        return true;
    }

    // 3. Elevated structures: Any tile in a column that sits above the column's base terrain tile
    // (e.g. elevated platforms, bridges, towers, yellow hazard platforms, catwalks).
    if (pMap && tz > 0) {
        for (int zi = 0; zi < tz; ++zi) {
            rs_eng::Tile *lowerTile = pMap->getTileAt(tx, ty, zi);
            if (lowerTile && lowerTile->id() > 4) {
                uint8_t lowerId = static_cast<uint8_t>(lowerTile->id());
                uint8_t lowerType = TileRasterizer::instance().getTile(lowerId).type;
                bool lowerIsStructure = (lowerType == 0x0A || lowerType == 0x0D ||
                                         (lowerType >= 0x01 && lowerType <= 0x04) ||
                                         lowerType == 0x0C || kIsRoofGroundTile[lowerId]);
                if (!lowerIsStructure) {
                    // There is a base terrain tile below this layer in this column!
                    return true;
                }
            }
        }
    }

    return false;
}

void WinMapViewport::recreateDIB(int w, int h) {
    if (w <= 0 || h <= 0) return;
    if (hDIB_ && dibWidth_ == w && dibHeight_ == h) return;

    if (hDIB_) {
        DeleteObject(hDIB_);
        hDIB_ = nullptr;
    }

    dibWidth_ = w;
    dibHeight_ = h;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = dibWidth_;
    bmi.bmiHeader.biHeight = -dibHeight_; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(nullptr);
    hDIB_ = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pDIBBits_), nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);
}

void WinMapViewport::setMission(rs_knl::Mission *pMission, int missionId) {
    if (objectivePickMode_ != ObjectivePickMode::None) {
        cancelObjectivePicking();
    }
    pMission_ = pMission;
    missionId_ = missionId;
    activeZ_ = 0;
    explicitSelectedZ_ = 0;
    manualLayerOverride_ = false;
    isolateActiveZ_ = false;
    selectAllFilters(true);

    if (pMission_ && pMission_->map() && pMission_->map()->getTileManager()) {
        int palId = rs_knl::MissionManager::getMissionPaletteId(missionId);
        TileRasterizer::instance().setPalette(palId);
    }

    centerOnSquadSpawn();
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::centerOnTile(const rs_knl::TilePoint &targetPt) {
    if (!pMission_ || !pMission_->map()) {
        return;
    }
    rs_knl::Map *pMap = pMission_->map();
    activeZ_ = targetPt.tz;
    Point2D pt;
    pMap->tileToScreenPoint(targetPt, &pt);

    int viewW = static_cast<int>(dibWidth_ / zoom_);
    int viewH = static_cast<int>(dibHeight_ / zoom_);
    cameraX_ = pt.x - viewW / 2;
    cameraY_ = pt.y - viewH / 2;

    if (cameraX_ < 0) cameraX_ = 0;
    if (cameraY_ < 0) cameraY_ = 0;
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::centerOnSquadSpawn() {
    if (!pMission_ || !pMission_->map()) {
        return;
    }

    rs_knl::Map *pMap = pMission_->map();
    rs_knl::PedInstance *pSquadPed = nullptr;
    for (size_t i = 0; i < pMission_->numPeds(); ++i) {
        auto *p = pMission_->ped(i);
        if (p && p->isOurAgent()) {
            pSquadPed = p;
            break;
        }
    }

    rs_knl::TilePoint targetPt(-1, -1, 0);
    if (pSquadPed) {
        targetPt = rs_knl::TilePoint(pSquadPed->tileX(), pSquadPed->tileY(), pSquadPed->tileZ(), 0, 0);
    } else {
        targetPt = rs_knl::TilePoint(pMap->maxTx() / 2, pMap->maxTy() / 2, 0, 0, 0);
    }

    centerOnTile(targetPt);
}

void WinMapViewport::highlightSearchTile(const rs_knl::TilePoint &tp) {
    highlightedSearchTilePos_ = tp;
    hasHighlightedSearchTile_ = true;
    highlightedSearchEntity_ = nullptr;
    setActiveZ(tp.tz);
    centerOnTile(tp);
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::highlightSearchEntity(rs_knl::ShootableMapObject *pObj) {
    highlightedSearchEntity_ = pObj;
    hasHighlightedSearchTile_ = false;
    if (pObj) {
        setActiveZ(pObj->tileZ());
        centerOnEntity(pObj);
    }
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::clearSearchHighlight() {
    bool hadHighlight = (highlightedSearchEntity_ != nullptr || hasHighlightedSearchTile_);
    highlightedSearchEntity_ = nullptr;
    hasHighlightedSearchTile_ = false;
    if (hadHighlight && hWnd_) {
        InvalidateRect(hWnd_, nullptr, FALSE);
    }
}

void WinMapViewport::setTool(ViewportTool t) {
    if (objectivePickMode_ != ObjectivePickMode::None) {
        toolBeforeObjectivePick_ = t;
        return;
    }
    if (tool_ == t) {
        manualLayerOverride_ = false;
        InvalidateRect(hWnd_, nullptr, FALSE);
        return;
    }
    ViewportTool oldTool = tool_;
    if (oldTool == ViewportTool::RouteTool && t != ViewportTool::RouteTool) {
        rs_knl::PedInstance *pPed = getActiveRoutePed();
        if (pPed) {
            size_t nonResetCount = 0;
            for (const auto &st : pPed->scriptSteps()) {
                if (st.type != LevelData::kScenarioTypeReset) nonResetCount++;
            }
            if (nonResetCount <= 1) {
                pPed->clearScriptSteps();
                pPed->setScriptLooping(false);
                if (pMission_) {
                    rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                }
            }
        }
    }
    if (t == ViewportTool::RouteTool) {
        rs_knl::PedInstance *pPed = getActiveRoutePed();
        if (pPed && pPed->scriptSteps().empty()) {
            rs_knl::Vehicle *v0 = getPedVehicleAtStep(pMission_, pPed, 0);
            rs_knl::TilePoint spawnPos = (v0 != nullptr) ? v0->position() : pPed->position();
            rs_knl::ScriptStep spawnStep;
            spawnStep.type = LevelData::kScenarioTypeGoto;
            spawnStep.targetPos = spawnPos;
            pPed->addScriptStep(spawnStep);
            pPed->setScriptLooping(true);
            if (pMission_) {
                rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
            }
        }
    }
    tool_ = t;
    manualLayerOverride_ = false;
    isPainting_ = false;
    isBoxFilling_ = false;
    isDraggingWaypoint_ = false;
    hoveredWaypointIdx_ = -1;
    draggingWaypointIdx_ = -1;
    if (t != ViewportTool::RouteTool && oldTool != ViewportTool::RouteTool) {
        clearSelection();
    }
    if (toolChangedCallback_) {
        toolChangedCallback_(tool_);
    }
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::beginObjectivePicking(ObjectivePickMode mode, ObjectivePickCallback cb, const std::wstring &customPrompt) {
    if (!hasSavedToolBeforePick_) {
        toolBeforeObjectivePick_ = tool_;
        hasSavedToolBeforePick_ = true;
    }
    objectivePickMode_ = mode;
    objectivePickCallback_ = cb;
    customPickPrompt_ = customPrompt;
    isHoveringPickCancel_ = false;
    isPainting_ = false;
    isBoxFilling_ = false;
    isDraggingTile_ = false;
    isDraggingEntity_ = false;
    isDraggingWaypoint_ = false;
    clearSelection();
    SetFocus(hWnd_);
    if (toolChangedCallback_) {
        toolChangedCallback_(tool_);
    }
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::cancelObjectivePicking() {
    if (objectivePickMode_ == ObjectivePickMode::None) return;
    auto cb = objectivePickCallback_;
    objectivePickMode_ = ObjectivePickMode::None;
    objectivePickCallback_ = nullptr;
    customPickPrompt_.clear();
    isHoveringPickCancel_ = false;
    if (hasSavedToolBeforePick_) {
        tool_ = toolBeforeObjectivePick_;
        hasSavedToolBeforePick_ = false;
    }
    if (toolChangedCallback_) {
        toolChangedCallback_(tool_);
    }
    InvalidateRect(hWnd_, nullptr, FALSE);
    if (cb) {
        ObjectivePickResult res;
        res.accepted = false;
        cb(res);
    }
}

void WinMapViewport::finishObjectivePicking(const ObjectivePickResult &res) {
    auto cb = objectivePickCallback_;
    objectivePickMode_ = ObjectivePickMode::None;
    objectivePickCallback_ = nullptr;
    customPickPrompt_.clear();
    isHoveringPickCancel_ = false;
    if (hasSavedToolBeforePick_) {
        tool_ = toolBeforeObjectivePick_;
        hasSavedToolBeforePick_ = false;
    }
    if (toolChangedCallback_) {
        toolChangedCallback_(tool_);
    }
    InvalidateRect(hWnd_, nullptr, FALSE);
    if (cb) {
        cb(res);
    }
}

void WinMapViewport::setObjectivePreview(int editingIndex, uint16_t type, uint16_t targetPedId, uint16_t targetVehicleIdx, int evacX, int evacY, int evacZ) {
    objPreviewDesc_.active = true;
    objPreviewDesc_.editingIndex = editingIndex;
    objPreviewDesc_.type = type;
    objPreviewDesc_.targetPedId = targetPedId;
    objPreviewDesc_.targetVehicleIdx = targetVehicleIdx;
    objPreviewDesc_.evacX = evacX;
    objPreviewDesc_.evacY = evacY;
    objPreviewDesc_.evacZ = evacZ;
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::clearObjectivePreview() {
    if (objPreviewDesc_.active) {
        objPreviewDesc_.active = false;
        objPreviewDesc_.editingIndex = -1;
        InvalidateRect(hWnd_, nullptr, FALSE);
    }
}

void WinMapViewport::setActiveZ(int z) {
    if (pMission_ && pMission_->map()) {
        if (z < 0) z = 0;
        if (z >= pMission_->map()->maxTz()) z = pMission_->map()->maxTz() - 1;
    }
    activeZ_ = z;
    explicitSelectedZ_ = z;
    manualLayerOverride_ = true;
    if (hWnd_) {
        POINT mousePt;
        if (GetCursorPos(&mousePt) && ScreenToClient(hWnd_, &mousePt)) {
            bool isPickTool = (tool_ == ViewportTool::SelectTile ||
                               tool_ == ViewportTool::SelectEntity);
            currentTilePos_ = isPickTool ? pickTileAtScreen(mousePt.x, mousePt.y) : screenToMapPoint(mousePt.x, mousePt.y);
        }
    }
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::setSliceView(bool isolate) {
    isolateActiveZ_ = isolate;
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::setZoom(float z) {
    if (z < 0.25f) z = 0.25f;
    if (z > 3.0f) z = 3.0f;
    zoom_ = z;
    if (statusCallback_) {
        std::string coords = std::format("TX:{:02} TY:{:02} TZ:{:02}", currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz);
        statusCallback_(coords, "", "");
    }
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::zoomIn() {
    setZoom(zoom_ + 0.25f);
}

void WinMapViewport::zoomOut() {
    setZoom(zoom_ - 0.25f);
}

void WinMapViewport::onResize(int w, int h) {
    recreateDIB(w, h);
    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::onPaint(HDC hdc) {
    if (!pDIBBits_ || dibWidth_ <= 0 || dibHeight_ <= 0) return;

    renderMapToDIB();

    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMem, hDIB_));

    if (!pMission_ || !pMission_->map()) {
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(130, 170, 145));
        HFONT hFont = CreateFontW(
            -20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdcMem, hFont));

        const wchar_t *msg = L"No Mission Loaded\r\n\r\nUse File -> New Mission (Ctrl+N) or Open Mission (Ctrl+O) to begin";
        RECT rcCalc = { 0, 0, dibWidth_, 0 };
        DrawTextW(hdcMem, msg, -1, &rcCalc, DT_CALCRECT | DT_CENTER | DT_NOPREFIX);
        int textH = rcCalc.bottom - rcCalc.top;
        int startY = (dibHeight_ - textH) / 2;
        if (startY < 20) startY = 20;
        RECT rc = { 0, startY, dibWidth_, startY + textH };
        DrawTextW(hdcMem, msg, -1, &rc, DT_CENTER | DT_NOPREFIX);

        SelectObject(hdcMem, hOldFont);
        DeleteObject(hFont);
    } else {
        // High-visibility Layer Z HUD Chip in top-right corner
        std::wstring modeStr = L"MANUAL";
        if (autoLayerSwitchingEnabled_) {
            modeStr = manualLayerOverride_ ? L"MANUAL" : L"AUTO";
        }
        std::wstring zText = std::format(L"LAYER: {} [{}] [{}]", activeZ_,
            modeStr,
            isolateActiveZ_ ? L"SLICE ACTIVE" : L"ALL LAYERS");

        SetBkMode(hdcMem, TRANSPARENT);
        HFONT hFont = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdcMem, hFont));

        RECT rcCalc = { 0, 0, 0, 0 };
        DrawTextW(hdcMem, zText.c_str(), -1, &rcCalc, DT_CALCRECT | DT_NOPREFIX);
        int badgeW = std::max<int>(220, static_cast<int>(rcCalc.right - rcCalc.left) + 24);
        int badgeH = 32;
        int badgeX = dibWidth_ - badgeW - 16;
        int badgeY = 16;

        RECT badgeRc = { badgeX, badgeY, badgeX + badgeW, badgeY + badgeH };
        HBRUSH hBg = CreateSolidBrush(RGB(18, 24, 32));
        HPEN hBorder = CreatePen(PS_SOLID, 2, isolateActiveZ_ ? RGB(255, 185, 40) : RGB(40, 190, 245));
        HBRUSH hOldBr = static_cast<HBRUSH>(SelectObject(hdcMem, hBg));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdcMem, hBorder));
        RoundRect(hdcMem, badgeX, badgeY, badgeX + badgeW, badgeY + badgeH, 8, 8);

        SetTextColor(hdcMem, isolateActiveZ_ ? RGB(255, 210, 60) : RGB(80, 220, 255));
        DrawTextW(hdcMem, zText.c_str(), -1, &badgeRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdcMem, hOldFont);
        DeleteObject(hFont);
        SelectObject(hdcMem, hOldPen);
        DeleteObject(hBorder);
        SelectObject(hdcMem, hOldBr);
        DeleteObject(hBg);

        // Obscured Entities Elevation Badges (Option 1: Roof Indicator & Badges)
        if (!obscuredOverlays_.empty()) {
            HFONT hBadgeFont = CreateFontW(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HFONT hOldBadgeFont = static_cast<HFONT>(SelectObject(hdcMem, hBadgeFont));
            SetBkMode(hdcMem, TRANSPARENT);

            std::vector<RECT> placedBadges;

            for (const auto &item : obscuredOverlays_) {
                if (!item.isSelected) continue;
                std::wstring text;
                if (item.squadSpawnIdx >= 0) {
                    text = std::format(L"SP#{} Layer:{}", item.squadSpawnIdx + 1, item.groundZ);
                } else if (item.isSelected) {
                    text = std::format(L"▼ Layer:{} [SELECTED]", item.groundZ);
                } else if (item.isHovered) {
                    text = std::format(L"▼ Layer:{} [IN]", item.groundZ);
                } else {
                    text = std::format(L"▼ Layer:{}", item.groundZ);
                }

                RECT rcObs = { 0, 0, 0, 0 };
                DrawTextW(hdcMem, text.c_str(), -1, &rcObs, DT_CALCRECT | DT_NOPREFIX);
                int bw = (rcObs.right - rcObs.left) + 12;
                int bh = (rcObs.bottom - rcObs.top) + 6;

                int bx = item.roofAnchorX - (bw / 2);
                int by = item.roofAnchorY - bh - 6;

                RECT obsRc = { bx, by, bx + bw, by + bh };
                bool overlaps = true;
                while (overlaps) {
                    overlaps = false;
                    for (const auto &placed : placedBadges) {
                        RECT isect;
                        if (IntersectRect(&isect, &obsRc, &placed)) {
                            obsRc.top -= (bh + 4);
                            obsRc.bottom -= (bh + 4);
                            overlaps = true;
                            break;
                        }
                    }
                }
                placedBadges.push_back(obsRc);

                COLORREF bgCol = RGB(14, 20, 28);
                COLORREF borderCol = item.isSelected ? RGB(255, 215, 0) : (item.isHovered ? RGB(0, 240, 255) : RGB(0, 180, 215));
                COLORREF textCol = item.isSelected ? RGB(255, 230, 80) : (item.isHovered ? RGB(255, 255, 255) : RGB(120, 230, 255));

                HBRUSH hBgPill = CreateSolidBrush(bgCol);
                HPEN hPenPill = CreatePen(PS_SOLID, item.isSelected ? 2 : 1, borderCol);
                HBRUSH hOldBrPill = static_cast<HBRUSH>(SelectObject(hdcMem, hBgPill));
                HPEN hOldPenPill = static_cast<HPEN>(SelectObject(hdcMem, hPenPill));

                RoundRect(hdcMem, obsRc.left, obsRc.top, obsRc.right, obsRc.bottom, 6, 6);

                SetTextColor(hdcMem, textCol);
                DrawTextW(hdcMem, text.c_str(), -1, &obsRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

                SelectObject(hdcMem, hOldPenPill);
                DeleteObject(hPenPill);
                SelectObject(hdcMem, hOldBrPill);
                DeleteObject(hBgPill);
            }

            SelectObject(hdcMem, hOldBadgeFont);
            DeleteObject(hBadgeFont);
        }

        // Draw 3D wiregrid coordinate badges on selected entity or tile
        drawSelectionBadges(hdcMem);

        // Draw scripted movement route & waypoints for selected ped/vehicle
        if (objectivePickMode_ == ObjectivePickMode::None) {
            drawRouteOverlay(hdcMem);
        }

        // Draw objective target badges, reticles, and evacuation zone
        drawObjectiveAnnotations(hdcMem);

        // Draw objective picking overlay & top banner
        if (objectivePickMode_ != ObjectivePickMode::None) {
            drawObjectivePickOverlay(hdcMem);
        }

        // Draw entity stats overlay
        if (showStats_) {
            drawEntityStatsOverlay(hdcMem);
        }
    }

    // Atomic blit of completely rendered backbuffer to screen DC
    BitBlt(hdc, 0, 0, dibWidth_, dibHeight_, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBmp);
    DeleteDC(hdcMem);
}

void WinMapViewport::renderMapToDIB() {
    // Fill background with dark cyberpunk grid color (#141914)
    std::fill(pDIBBits_, pDIBBits_ + dibWidth_ * dibHeight_, 0xFF141914);

    if (!pMission_ || !pMission_->map()) {
        return;
    }

    rs_knl::Map *pMap = pMission_->map();
    int maxTx = pMap->maxTx();
    int maxTy = pMap->maxTy();
    int maxTz = pMap->maxTz();

    int viewW = static_cast<int>(dibWidth_ / zoom_);
    int viewH = static_cast<int>(dibHeight_ / zoom_);

    int buffer = 256;
    int minScreenX = cameraX_ - buffer;
    int maxScreenX = cameraX_ + viewW + buffer;
    int minScreenY = cameraY_ - buffer;
    int maxScreenY = cameraY_ + viewH + buffer;

    struct ViewportEntityItem {
        const rs_knl::MapObject *obj = nullptr;
        int anchorX = 0;
        int anchorY = 0;
        uint8_t alpha = 255;
        uint32_t outlineCol = 0;
        bool isStatic = false;
        bool isWallMounted = false;
        bool isObscured = false;
        bool drawn = false;
    };

    auto makeTileKey = [](int tx, int ty, int tz) -> uint32_t {
        return (static_cast<uint32_t>(tx & 0xFF) << 16) |
               (static_cast<uint32_t>(ty & 0xFF) << 8) |
               static_cast<uint32_t>(tz & 0xFF);
    };

    obscuredOverlays_.clear();
    std::unordered_map<uint32_t, std::vector<ViewportEntityItem>> entityBuckets;
    int margin = static_cast<int>(128 * zoom_);

    auto checkAndQueueObscured = [&](const rs_knl::MapObject *obj, int drawTx, int drawTy, int drawZ,
                                     int ax, int ay, uint32_t col, bool isSel, bool isVeh) -> bool {
        if (!filterBuildings_) return false;
        int roofZ = getHighestRoofZ(drawTx, drawTy, drawZ);
        bool underRoof = (roofZ > drawZ);
        bool fgObscured = !underRoof && isEntityForegroundObscured(obj, drawTx, drawTy, drawZ);
        if (!underRoof && !fgObscured) return false;

        int roofAx = ax;
        int roofAy = ay - static_cast<int>(26 * zoom_);
        if (underRoof) {
            rs_knl::TilePoint roofTp(drawTx, drawTy, roofZ);
            POINT roofPt = mapPointToScreen(roofTp);
            roofAx = roofPt.x;
            roofAy = roofPt.y;
        }
        bool isHov = (currentTilePos_.tx == obj->tileX() && currentTilePos_.ty == obj->tileY());
        obscuredOverlays_.push_back({ obj, ax, ay, roofAx, roofAy, drawZ, roofZ, col, isSel, isHov, isVeh, -1 });
        return true;
    };

    if (pMission_) {
        // 1. Statics (doors, windows, trees, signs, etc.)
        if (filterBuildings_) {
            for (size_t i = 0; i < pMission_->numStatics(); ++i) {
                rs_knl::Static *s = pMission_->statics(i);
                if (!s) continue;
                if (isolateActiveZ_ && s->tileZ() > activeZ_) continue;

                POINT pt = entityPointToScreen(s->position());
                int ax = pt.x;
                int ay = pt.y;
                if (ax < -margin || ax > dibWidth_ + margin || ay < -margin || ay > dibHeight_ + margin) continue;

                uint8_t a = 255;
                uint32_t col = 0xFF00E5FF;

                bool isWall = s->isWallMounted();
                uint32_t key = makeTileKey(s->tileX(), s->tileY(), s->tileZ());
                entityBuckets[key].push_back({ s, ax, ay, a, col, true, isWall, false, false });
            }
        }

        // 2. Vehicles
        if (filterVehicles_) {
            for (size_t i = 0; i < pMission_->numVehicles(); ++i) {
                rs_knl::Vehicle *v = pMission_->vehicle(i);
                if (!v) continue;
                if (isolateActiveZ_ && v->tileZ() > activeZ_) continue;

                POINT pt = entityPointToScreen(v->position());
                int ax = pt.x;
                int ay = pt.y;
                if (ax < -margin || ax > dibWidth_ + margin || ay < -margin || ay > dibHeight_ + margin) continue;

                uint8_t a = 255;
                uint32_t col = 0xFFFFD030;

                int drawTx = v->tileX();
                int drawTy = v->tileY();
                int drawZ = v->tileZ();
                if (v->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead ||
                    v->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody) {
                    drawZ += 1;
                    int wx = v->tileX() * 256 + v->position().ox;
                    int wy = v->tileY() * 256 + v->position().oy;
                    drawTx = (wx + 128) / 256;
                    drawTy = (wy + 128) / 256;
                }
                bool isSel = (v == targetSelected_);
                bool obs = checkAndQueueObscured(v, drawTx, drawTy, drawZ, ax, ay, col, isSel, true);
                uint32_t key = makeTileKey(drawTx, drawTy, drawZ);
                entityBuckets[key].push_back({ v, ax, ay, a, col, false, false, obs, false });
            }
        }

        // 3. Peds
        for (size_t i = 0; i < pMission_->numPeds(); ++i) {
            rs_knl::PedInstance *p = pMission_->ped(i);
            if (!p) continue;
            if (!isPedVisible(p)) continue;
            if (isolateActiveZ_ && p->tileZ() > activeZ_) continue;

            // Check if ped is inside a vehicle or assigned as driver
            rs_knl::Vehicle *assignedVeh = findAssignedVehicleForPed(p);
            if (assignedVeh && (!isDraggingEntity_ || targetSelected_ != p)) {
                p->setPosition(assignedVeh->position());
            }

            POINT pt = entityPointToScreen(p->position());
            int ax = pt.x;
            int ay = pt.y;
            if (ax < -margin || ax > dibWidth_ + margin || ay < -margin || ay > dibHeight_ + margin) continue;

            bool isSel = (p == targetSelected_);
            uint8_t a = assignedVeh ? (isSel ? 210 : 140) : 255;
            uint32_t col = 0xFF80D0FF;
            if (p->type() == rs_knl::PedInstance::kPedTypeGuard) col = 0xFFC040C0;
            else if (p->type() == rs_knl::PedInstance::kPedTypePolice) col = 0xFF4060FF;
            else if (p->type() == rs_knl::PedInstance::kPedTypeAgent) col = p->isOurAgent() ? 0xFF00FF66 : 0xFFFF3030;
            else if (p->type() == rs_knl::PedInstance::kPedTypeCriminal) col = 0xFFFF8020;

            bool obs = checkAndQueueObscured(p, p->tileX(), p->tileY(), p->tileZ(), ax, ay, col, isSel, false);
            uint32_t key = makeTileKey(p->tileX(), p->tileY(), p->tileZ());
            entityBuckets[key].push_back({ p, ax, ay, a, col, false, false, obs, false });
        }

        // 4. Ground Weapons
        if (filterWeapons_) {
            for (size_t i = 0; i < pMission_->numWeaponsOnGround(); ++i) {
                rs_knl::WeaponInstance *w = pMission_->weaponOnGround(i);
                if (!w) continue;
                if (isolateActiveZ_ && w->tileZ() > activeZ_) continue;

                POINT pt = entityPointToScreen(w->position());
                int ax = pt.x;
                int ay = pt.y;
                if (ax < -margin || ax > dibWidth_ + margin || ay < -margin || ay > dibHeight_ + margin) continue;

                uint8_t a = 255;
                uint32_t col = 0xFF50FF50; // Mint green for ground weapons

                bool isSel = (w == targetSelected_);
                bool obs = checkAndQueueObscured(w, w->tileX(), w->tileY(), w->tileZ(), ax, ay, col, isSel, false);
                uint32_t key = makeTileKey(w->tileX(), w->tileY(), w->tileZ());
                entityBuckets[key].push_back({ w, ax, ay, a, col, false, false, obs, false });
            }
        }

        // Sort items within each bucket: statics & ground weapons first, then dead peds, then vehicles, then alive peds, then depth on same tile
        for (auto &[k, bucket] : entityBuckets) {
            std::stable_sort(bucket.begin(), bucket.end(), [](const ViewportEntityItem &a, const ViewportEntityItem &b) {
                if (a.isStatic != b.isStatic) return a.isStatic > b.isStatic;
                bool aIsWpn = a.obj && a.obj->is(rs_knl::MapObject::kNatureWeapon);
                bool bIsWpn = b.obj && b.obj->is(rs_knl::MapObject::kNatureWeapon);
                if (aIsWpn != bIsWpn) return aIsWpn > bIsWpn;

                bool aIsDeadPed = a.obj && a.obj->is(rs_knl::MapObject::kNaturePed) && 
                    !static_cast<const rs_knl::PedInstance*>(a.obj)->isAlive();
                bool bIsDeadPed = b.obj && b.obj->is(rs_knl::MapObject::kNaturePed) && 
                    !static_cast<const rs_knl::PedInstance*>(b.obj)->isAlive();
                if (aIsDeadPed != bIsDeadPed) return aIsDeadPed > bIsDeadPed;

                bool aIsVeh = a.obj && a.obj->is(rs_knl::MapObject::kNatureVehicle);
                bool bIsVeh = b.obj && b.obj->is(rs_knl::MapObject::kNatureVehicle);
                if (aIsVeh != bIsVeh) return aIsVeh > bIsVeh;
                if (a.obj && b.obj) {
                    return const_cast<rs_knl::MapObject*>(a.obj)->isBehindObjectOnSameTile(const_cast<rs_knl::MapObject*>(b.obj));
                }
                return false;
            });
        }
    }

    auto drawEntitiesAt = [&](int tx, int ty, int tz, bool wallStaticsOnly) {
        if (isolateActiveZ_ && tz > activeZ_) return;
        uint32_t key = makeTileKey(tx, ty, tz);
        auto it = entityBuckets.find(key);
        if (it == entityBuckets.end()) return;
        for (auto &item : it->second) {
            if (item.drawn) continue;
            if (wallStaticsOnly && !item.isWallMounted) continue;
            drawDiamondOutline(pDIBBits_, dibWidth_, dibHeight_, item.anchorX, item.anchorY, item.outlineCol);
            SpriteRasterizer::instance().drawEntity(pDIBBits_, dibWidth_, dibHeight_, item.obj, item.anchorX, item.anchorY, zoom_, item.alpha);
            item.drawn = true;
        }
    };

    int sw = 0;
    int sh = 0;
    int chky = 0;
    int shm = maxTx + maxTy + maxTz * 2 + 8;
    int zr = shm + maxTz + 1;

    for (int inc = 0; inc < zr; ++inc) {
        int ye = sh + inc;
        int ys = ye - maxTz - 2;
        int tile_z = maxTz + 1;
        for (int yb = ys; yb < ye; ++yb) {
            if (yb < 0 || yb < sh || yb >= shm) {
                --tile_z;
                continue;
            }
            int tile_y = yb;
            for (int tile_x = sw; tile_y >= chky && tile_x < maxTx; ++tile_x) {
                if (tile_x < 0 || tile_y >= maxTy) {
                    --tile_y;
                    continue;
                }
                int screen_w = (maxTx + (tile_x - tile_y)) * (rs_eng::Tile::kTileWidth / 2);
                int coord_h = ((maxTz + tile_x + tile_y) - (tile_z - 1)) * (rs_eng::Tile::kTileHeight / 3);
                if (screen_w + 64 >= minScreenX && screen_w <= maxScreenX &&
                    coord_h + 48 >= minScreenY && coord_h <= maxScreenY) {

                    // 1. Draw entities on tile_z - 1 first so tile_z can clip them
                    if (tile_z - 1 >= 0) {
                        int prev_z = tile_z - 1;
                        if (!isolateActiveZ_ || prev_z <= activeZ_) {
                            // If there is a non-wall-mounted entity (ped/car) on prev_z,
                            // draw any wall statics (windows/signs) above it (dz = 1..3) first
                            bool hasNonWallObj = false;
                            uint32_t curKey = makeTileKey(tile_x, tile_y, prev_z);
                            auto curIt = entityBuckets.find(curKey);
                            if (curIt != entityBuckets.end()) {
                                for (const auto &item : curIt->second) {
                                    if (!item.drawn && !item.isWallMounted) {
                                        hasNonWallObj = true;
                                        break;
                                    }
                                }
                            }
                            if (hasNonWallObj) {
                                for (int dz = 1; dz <= 3; ++dz) {
                                    drawEntitiesAt(tile_x, tile_y, prev_z + dz, true);
                                }
                            }
                            drawEntitiesAt(tile_x, tile_y, prev_z, false);
                        }
                    }

                    // 2. Draw static map tile at tile_z
                    if (tile_z < maxTz) {
                        if (!isolateActiveZ_ || tile_z <= activeZ_) {
                            rs_eng::Tile *t = pMap->getTileAt(tile_x, tile_y, tile_z);
                            if (t && t->id() != 0) {
                                bool skipTile = false;
                                if (!filterBuildings_) {
                                    if (isBuildingOrStructureTile(pMap, tile_x, tile_y, tile_z, static_cast<uint8_t>(t->id()))) {
                                        skipTile = true;
                                    }
                                }
                                if (!skipTile) {
                                    int drawX = static_cast<int>((screen_w - cameraX_) * zoom_);
                                    int drawY = static_cast<int>((coord_h - cameraY_) * zoom_);

                                    if (std::abs(zoom_ - 1.0f) < 0.01f) {
                                        TileRasterizer::instance().drawTile(pDIBBits_, dibWidth_, dibHeight_, static_cast<uint8_t>(t->id()), drawX, drawY);
                                    } else {
                                        TileRasterizer::instance().drawTileScaled(pDIBBits_, dibWidth_, dibHeight_, static_cast<uint8_t>(t->id()), drawX, drawY, zoom_);
                                    }
                                }
                            }
                        }
                    }
                }
                --tile_y;
            }
            --tile_z;
        }
    }

    // Render any remaining entities that were outside the tile bounds or on higher layers
    for (auto &[k, bucket] : entityBuckets) {
        for (auto &item : bucket) {
            if (!item.drawn) {
                if (isolateActiveZ_ && item.obj && item.obj->tileZ() > activeZ_) continue;
                drawDiamondOutline(pDIBBits_, dibWidth_, dibHeight_, item.anchorX, item.anchorY, item.outlineCol);
                SpriteRasterizer::instance().drawEntity(pDIBBits_, dibWidth_, dibHeight_, item.obj, item.anchorX, item.anchorY, zoom_, item.alpha);
                item.drawn = true;
            }
        }
    }

    // Option 1: Render Obscured Entities Overlay (Guideline, roof diamond, ground dashed footprint, X-ray ghost sprite)
    auto drawVerticalDashedLine = [&](uint32_t *buf, int bw, int bh, int x, int y1, int y2, uint32_t color) {
        if (x < 0 || x >= bw) return;
        int startY = std::max(0, std::min(y1, y2));
        int endY = std::min(bh - 1, std::max(y1, y2));
        for (int y = startY; y <= endY; ++y) {
            if ((y / 3) % 2 == 0) {
                buf[y * bw + x] = color;
            }
        }
    };

    for (const auto &item : obscuredOverlays_) {
        // Obscured actors/vehicles marker (grid square) underneath them, consistent with visible actors but faded
        uint8_t markerAlpha = item.isSelected ? 220 : (item.isHovered ? 190 : 130);
        drawDiamondOutlineAlpha(pDIBBits_, dibWidth_, dibHeight_, item.groundAnchorX, item.groundAnchorY, item.outlineCol, markerAlpha);

        // Guidelines and roof diamonds are drawn for selected entities under an overhead roof
        if (item.isSelected && item.roofZ > item.groundZ) {
            uint32_t lineCol = 0xFFFFD700;

            // 1. Vertical dashed guideline connecting ground floor to roof surface
            drawVerticalDashedLine(pDIBBits_, dibWidth_, dibHeight_, item.groundAnchorX, item.roofAnchorY, item.groundAnchorY, lineCol);

            // 2. Small dashed diamond on roof surface
            drawSmallDashedDiamond(pDIBBits_, dibWidth_, dibHeight_, item.roofAnchorX, item.roofAnchorY, lineCol);
        }

        // 4. Semi-transparent X-ray ghost sprite through roof (rendered for ALL obscured entities)
        if (item.obj) {
            uint8_t ghostAlpha = item.isSelected ? 210 : (item.isHovered ? 180 : 130);
            SpriteRasterizer::instance().drawEntity(pDIBBits_, dibWidth_, dibHeight_, item.obj, item.groundAnchorX, item.groundAnchorY, zoom_, ghostAlpha);
        }
    }

    // 3D Wiregrid Column for Select, Pencil, BoxFill, and Entity Placement tools (stack of boxes Z=0..12 following mouse)
    bool showWiregridColumn = false;
    if (objectivePickMode_ == ObjectivePickMode::None) {
        if ((tool_ == ViewportTool::SelectTile || tool_ == ViewportTool::SelectEntity) &&
            (!isDraggingTile_ || !hasDraggedTile_) &&
            (!isDraggingEntity_ || !hasDraggedEntity_)) {
            showWiregridColumn = true;
        } else if ((tool_ == ViewportTool::Pencil || (tool_ == ViewportTool::BoxFill && !isBoxFilling_)) && activeTileId_ != 0) {
            showWiregridColumn = true;
        } else if (isEntityPlaceTool(tool_)) {
            showWiregridColumn = true;
        }
    }

    if (showWiregridColumn) {
        drawHoverWiregridColumn(pDIBBits_, dibWidth_, dibHeight_, currentTilePos_);
    }

    // 3D Bounding Wiregrid Box for selected entity or tile
    if (targetSelected_) {
        int heightPx = static_cast<int>(16 * zoom_);
        if (dynamic_cast<rs_knl::PedInstance*>(targetSelected_)) {
            heightPx = static_cast<int>(26 * zoom_);
        } else if (dynamic_cast<rs_knl::Vehicle*>(targetSelected_)) {
            heightPx = static_cast<int>(16 * zoom_);
        } else if (dynamic_cast<rs_knl::WeaponInstance*>(targetSelected_)) {
            heightPx = static_cast<int>(8 * zoom_);
        }
        draw3DBoundingBox(pDIBBits_, dibWidth_, dibHeight_, targetSelected_->position(), heightPx, 0xFFFFD700, true); // Gold
    } else if (hasSelectedTile_) {
        int heightPx = static_cast<int>(16 * zoom_);
        if (isDraggingTile_ && hasDraggedTile_) {
            // Draw dashed wiregrid at original position
            draw3DBoundingBox(pDIBBits_, dibWidth_, dibHeight_, dragTileStartPos_, heightPx, 0x8000E5FF, false);
            // Draw solid wiregrid at dragged destination
            draw3DBoundingBox(pDIBBits_, dibWidth_, dibHeight_, dragTileCurrentPos_, heightPx, 0xFF00E5FF, false);
        } else {
            draw3DBoundingBox(pDIBBits_, dibWidth_, dibHeight_, selectedTilePos_, heightPx, 0xFF00E5FF, false); // Neon Cyan
        }
    }

    // Search Result Highlight Wiregrid Box (vibrant glowing amber, non-selected)
    if (highlightedSearchEntity_) {
        int heightPx = static_cast<int>(16 * zoom_);
        if (dynamic_cast<rs_knl::PedInstance*>(highlightedSearchEntity_)) {
            heightPx = static_cast<int>(26 * zoom_);
        }
        draw3DBoundingBox(pDIBBits_, dibWidth_, dibHeight_, highlightedSearchEntity_->position(), heightPx, 0xFFFFB020, true);
    } else if (hasHighlightedSearchTile_) {
        int heightPx = static_cast<int>(16 * zoom_);
        draw3DBoundingBox(pDIBBits_, dibWidth_, dibHeight_, highlightedSearchTilePos_, heightPx, 0xFFFFB020, false);
    }

    // Box fill live preview
    if (objectivePickMode_ == ObjectivePickMode::None && isBoxFilling_) {
        drawBoxFillOutline(pDIBBits_, dibWidth_, dibHeight_);
    }
}

rs_knl::TilePoint WinMapViewport::screenToMapPointAtZ(int sx, int sy, int targetZ) const {
    if (!pMission_ || !pMission_->map()) return rs_knl::TilePoint(0, 0, 0);

    int worldX = static_cast<int>(sx / zoom_) + cameraX_;
    int worldY = static_cast<int>(sy / zoom_) + cameraY_;

    worldY += targetZ * (rs_eng::Tile::kTileHeight / 3);

    rs_knl::TilePoint tp = pMission_->map()->screenToTilePoint(worldX, worldY);
    tp.tz = targetZ;
    return tp;
}

rs_knl::TilePoint WinMapViewport::screenToMapPoint(int sx, int sy) const {
    return screenToMapPointAtZ(sx, sy, activeZ_);
}

rs_knl::TilePoint WinMapViewport::resolveWalkableTilePoint(int sx, int sy, int referenceZ, bool isVehicle) const {
    if (!pMission_ || !pMission_->map()) return screenToMapPointAtZ(sx, sy, referenceZ);

    auto isWalkableAt = [&](int tx, int ty, int tz) -> bool {
        if (tx < 0 || tx >= pMission_->map()->maxTx() || ty < 0 || ty >= pMission_->map()->maxTy() || tz < 0 || tz >= pMission_->map()->maxTz()) {
            return false;
        }
        if (isVehicle) {
            return pMission_->map()->isTileWalkableByCar(tx, ty, tz);
        }
        return pMission_->isTileWalkable(tx, ty, tz);
    };

    if (!isVehicle) {
        // Continuous sub-tile positioning for on-foot NPCs
        rs_knl::TilePoint refPt = screenToMapPointAtZ(sx, sy, referenceZ);
        if (isWalkableAt(refPt.tx, refPt.ty, refPt.tz)) {
            return refPt;
        }
        for (int dz = 1; dz <= 4; ++dz) {
            if (isWalkableAt(refPt.tx, refPt.ty, referenceZ - dz)) {
                refPt.tz = referenceZ - dz;
                return refPt;
            }
            if (isWalkableAt(refPt.tx, refPt.ty, referenceZ + dz)) {
                refPt.tz = referenceZ + dz;
                return refPt;
            }
        }

        rs_knl::TilePoint picked = pickTileAtScreen(sx, sy);
        rs_knl::TilePoint pickedRay = screenToMapPointAtZ(sx, sy, picked.tz);
        if (isWalkableAt(pickedRay.tx, pickedRay.ty, pickedRay.tz)) {
            return pickedRay;
        }
        if (isWalkableAt(pickedRay.tx, pickedRay.ty, picked.tz + 1)) {
            pickedRay.tz = picked.tz + 1;
            return pickedRay;
        }
        return refPt;
    }

    // Vehicle (Car) road snapping:
    // 1. Raycast screen coordinate directly onto referenceZ (the car's road layer).
    rs_knl::TilePoint refPt = screenToMapPointAtZ(sx, sy, referenceZ);
    if (isWalkableAt(refPt.tx, refPt.ty, refPt.tz)) {
        return refPt;
    }

    // 2. Scan nearby layers around referenceZ along the screen ray.
    for (int dz = 1; dz <= 3; ++dz) {
        if (referenceZ - dz >= 0 && isWalkableAt(refPt.tx, refPt.ty, referenceZ - dz)) {
            refPt.tz = referenceZ - dz;
            return refPt;
        }
        if (referenceZ + dz < pMission_->map()->maxTz() && isWalkableAt(refPt.tx, refPt.ty, referenceZ + dz)) {
            refPt.tz = referenceZ + dz;
            return refPt;
        }
    }

    // 3. Fallback: test screen-picked tile raycast.
    rs_knl::TilePoint picked = pickTileAtScreen(sx, sy);
    rs_knl::TilePoint pickedRay = screenToMapPointAtZ(sx, sy, picked.tz);
    if (isWalkableAt(pickedRay.tx, pickedRay.ty, pickedRay.tz)) {
        return pickedRay;
    }
    for (int dz = 1; dz <= 3; ++dz) {
        if (picked.tz - dz >= 0 && isWalkableAt(pickedRay.tx, pickedRay.ty, picked.tz - dz)) {
            pickedRay.tz = picked.tz - dz;
            return pickedRay;
        }
        if (picked.tz + dz < pMission_->map()->maxTz() && isWalkableAt(pickedRay.tx, pickedRay.ty, picked.tz + dz)) {
            pickedRay.tz = picked.tz + dz;
            return pickedRay;
        }
    }

    return refPt;
}

int WinMapViewport::getReferenceZForRouteStep(rs_knl::PedInstance *pPed, size_t stepIdx, rs_knl::Vehicle *veh) const {
    if (veh && veh->isCar()) {
        int carZ = veh->tileZ();
        if (pMission_ && pMission_->map() && carZ > 0 && pMission_->map()->isTileWalkableByCar(veh->tileX(), veh->tileY(), carZ - 1)) {
            return carZ - 1;
        }
        return carZ;
    }
    if (!pPed) return activeZ_;
    if (stepIdx < pPed->scriptSteps().size()) {
        const auto &sp = pPed->scriptSteps()[stepIdx];
        if (sp.targetPos.tx != 0 || sp.targetPos.ty != 0 || sp.targetPos.tz != 0) {
            return sp.targetPos.tz;
        }
    }
    for (int i = static_cast<int>(stepIdx) - 1; i >= 0; --i) {
        if (i < static_cast<int>(pPed->scriptSteps().size())) {
            const auto &sp = pPed->scriptSteps()[i];
            if (sp.targetPos.tx != 0 || sp.targetPos.ty != 0 || sp.targetPos.tz != 0) {
                return sp.targetPos.tz;
            }
        }
    }
    return pPed->tileZ();
}

rs_knl::Vehicle* WinMapViewport::findAssignedVehicleForPed(rs_knl::Mission *pMission, rs_knl::PedInstance *p) {
    if (!p || !pMission) return nullptr;
    if (p->inVehicle()) return p->inVehicle();
    for (size_t vIdx = 0; vIdx < pMission->numVehicles(); ++vIdx) {
        if (auto *v = pMission->vehicle(vIdx)) {
            if (v->getDriver() == p) return v;
        }
    }
    return nullptr;
}

rs_knl::Vehicle* WinMapViewport::findAssignedVehicleForPed(rs_knl::PedInstance *p) const {
    return findAssignedVehicleForPed(pMission_, p);
}

rs_knl::Vehicle* WinMapViewport::getPedVehicleAtStep(rs_knl::Mission *pMission, rs_knl::PedInstance *p, size_t stepLimit) {
    if (!p || !pMission) return nullptr;
    rs_knl::Vehicle *v = findAssignedVehicleForPed(pMission, p);
    const auto &steps = p->scriptSteps();
    size_t limit = std::min(stepLimit, steps.size());
    for (size_t i = 0; i < limit; ++i) {
        const auto &st = steps[i];
        if (st.type == LevelData::kScenarioTypeUseVehicle || st.type == 2) {
            if (st.targetEntityId >= 0 && static_cast<size_t>(st.targetEntityId) < pMission->numVehicles()) {
                v = pMission->vehicle(st.targetEntityId);
            }
        } else if (st.type == LevelData::kScenarioTypeLeaveVehicle || st.type == 6) {
            v = nullptr;
        }
    }
    return v;
}

rs_knl::PedInstance* WinMapViewport::getActiveRoutePed() const {
    if (!targetSelected_ || !pMission_) return nullptr;
    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(targetSelected_)) {
        if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent) return nullptr;
        return pPed;
    }
    if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(targetSelected_)) {
        return pVeh->getDriver();
    }
    return nullptr;
}

int WinMapViewport::hitTestWaypointAtScreen(int sx, int sy, const std::vector<rs_knl::ScriptStep> &steps) const {
    const int hitRadiusSq = 18 * 18;
    for (size_t i = 0; i < steps.size(); ++i) {
        if (steps[i].type == LevelData::kScenarioTypeGoto ||
            steps[i].type == LevelData::kScenarioTypeTrigger ||
            steps[i].type == 1 || steps[i].type == 8) {
            POINT pt = entityPointToScreen(steps[i].targetPos);
            int dx = pt.x - sx;
            int dy = pt.y - sy;
            if (dx * dx + dy * dy <= hitRadiusSq) {
                return static_cast<int>(i);
            }
        } else if (steps[i].type == LevelData::kScenarioTypeUseVehicle || steps[i].type == 2) {
            if (pMission_ && steps[i].targetEntityId >= 0 && static_cast<size_t>(steps[i].targetEntityId) < pMission_->numVehicles()) {
                rs_knl::Vehicle *v = pMission_->vehicle(steps[i].targetEntityId);
                if (v) {
                    POINT pt = entityPointToScreen(v->position());
                    int dx = pt.x - sx;
                    int dy = pt.y - sy;
                    if (dx * dx + dy * dy <= hitRadiusSq) {
                        return static_cast<int>(i);
                    }
                }
            }
        }
    }
    return -1;
}

std::vector<rs_knl::ShootableMapObject*> WinMapViewport::hitTestAllEntitiesAtScreen(int sx, int sy) const {
    std::vector<rs_knl::ShootableMapObject*> results;
    if (!pMission_) return results;

    struct HitCandidate {
        rs_knl::ShootableMapObject *obj;
        int distSq;
        int priority; // 0 for squad, 1 for peds, 2 for vehicles, 3 for statics
    };
    std::vector<HitCandidate> candidates;
    const int maxDistSq = 48 * 48; // 48-pixel screen radius

    // 1. Check peds (skip in-vehicle drivers so only the vehicle is selectable)
    for (size_t i = 0; i < pMission_->numPeds(); ++i) {
        rs_knl::PedInstance *p = pMission_->ped(i);
        if (!p || !isPedVisible(p)) continue;
        if (findAssignedVehicleForPed(p) != nullptr) continue;
        POINT pt = entityPointToScreen(p->position());
        int dx = pt.x - sx;
        int dy = pt.y - sy;
        int dsq = dx * dx + dy * dy;

        int roofZ = getHighestRoofZ(p->tileX(), p->tileY(), p->tileZ());
        if (roofZ > p->tileZ()) {
            rs_knl::TilePoint roofTp(p->tileX(), p->tileY(), roofZ);
            POINT rpt = mapPointToScreen(roofTp);
            int rdx = rpt.x - sx;
            int rdy = rpt.y - 12 - sy;
            int rdsq = rdx * rdx + rdy * rdy;
            if (rdsq < dsq) dsq = rdsq;
        }

        if (dsq < maxDistSq) {
            candidates.push_back({ p, dsq, (p->isOurAgent() ? 0 : 1) });
        }
    }

    // 3. Check vehicles
    if (filterVehicles_) {
        for (size_t i = 0; i < pMission_->numVehicles(); ++i) {
            rs_knl::Vehicle *v = pMission_->vehicle(i);
            if (!v) continue;
            POINT pt = entityPointToScreen(v->position());
            int dx = pt.x - sx;
            int dy = pt.y - sy;
            int dsq = dx * dx + dy * dy;

            int roofZ = getHighestRoofZ(v->tileX(), v->tileY(), v->tileZ());
            if (roofZ > v->tileZ()) {
                rs_knl::TilePoint roofTp(v->tileX(), v->tileY(), roofZ);
                POINT rpt = mapPointToScreen(roofTp);
                int rdx = rpt.x - sx;
                int rdy = rpt.y - 12 - sy;
                int rdsq = rdx * rdx + rdy * rdy;
                if (rdsq < dsq) dsq = rdsq;
            }

            if (dsq < maxDistSq) {
                candidates.push_back({ v, dsq, 2 });
            }
        }
    }

    // 4. Check statics
    if (filterBuildings_) {
        for (size_t i = 0; i < pMission_->numStatics(); ++i) {
            rs_knl::Static *s = pMission_->statics(i);
            if (!s) continue;
            POINT pt = entityPointToScreen(s->position());
            int dx = pt.x - sx;
            int dy = pt.y - sy;
            int dsq = dx * dx + dy * dy;
            if (dsq < maxDistSq) {
                candidates.push_back({ s, dsq, 3 });
            }
        }
    }

    // 5. Check ground weapons
    if (filterWeapons_) {
        for (size_t i = 0; i < pMission_->numWeaponsOnGround(); ++i) {
            rs_knl::WeaponInstance *w = pMission_->weaponOnGround(i);
            if (!w) continue;
            POINT pt = entityPointToScreen(w->position());
            int dx = pt.x - sx;
            int dy = pt.y - sy;
            int dsq = dx * dx + dy * dy;

            int roofZ = getHighestRoofZ(w->tileX(), w->tileY(), w->tileZ());
            if (roofZ > w->tileZ()) {
                rs_knl::TilePoint roofTp(w->tileX(), w->tileY(), roofZ);
                POINT rpt = mapPointToScreen(roofTp);
                int rdx = rpt.x - sx;
                int rdy = rpt.y - 12 - sy;
                int rdsq = rdx * rdx + rdy * rdy;
                if (rdsq < dsq) dsq = rdsq;
            }

            if (dsq < maxDistSq) {
                candidates.push_back({ w, dsq, 1 });
            }
        }
    }

    // Sort by distSq ascending, then priority
    std::stable_sort(candidates.begin(), candidates.end(), [](const HitCandidate &a, const HitCandidate &b) {
        if (a.distSq != b.distSq) return a.distSq < b.distSq;
        return a.priority < b.priority;
    });

    for (const auto &c : candidates) {
        if (std::find(results.begin(), results.end(), c.obj) == results.end()) {
            results.push_back(c.obj);
        }
    }

    // 5. Fallback to tile coordinate match if empty
    if (results.empty()) {
        rs_knl::TilePoint tp = screenToMapPoint(sx, sy);
        if (filterVehicles_) {
            for (size_t i = 0; i < pMission_->numVehicles(); ++i) {
                rs_knl::Vehicle *v = pMission_->vehicle(i);
                if (v && v->tileX() == tp.tx && v->tileY() == tp.ty) {
                    results.push_back(v);
                    break;
                }
            }
        }
        if (results.empty()) {
            for (size_t i = 0; i < pMission_->numPeds(); ++i) {
                rs_knl::PedInstance *p = pMission_->ped(i);
                if (p && isPedVisible(p) && p->tileX() == tp.tx && p->tileY() == tp.ty && findAssignedVehicleForPed(p) == nullptr) {
                    results.push_back(p);
                    break;
                }
            }
        }
        if (results.empty() && filterBuildings_) {
            for (size_t i = 0; i < pMission_->numStatics(); ++i) {
                rs_knl::Static *s = pMission_->statics(i);
                if (s && s->tileX() == tp.tx && s->tileY() == tp.ty) {
                    results.push_back(s);
                    break;
                }
            }
        }
        if (results.empty() && filterWeapons_) {
            for (size_t i = 0; i < pMission_->numWeaponsOnGround(); ++i) {
                rs_knl::WeaponInstance *w = pMission_->weaponOnGround(i);
                if (w && w->tileX() == tp.tx && w->tileY() == tp.ty) {
                    results.push_back(w);
                    break;
                }
            }
        }
    }

    return results;
}

rs_knl::ShootableMapObject* WinMapViewport::hitTestEntityAtScreen(int sx, int sy) const {
    auto candidates = hitTestAllEntitiesAtScreen(sx, sy);
    return candidates.empty() ? nullptr : candidates[0];
}

bool WinMapViewport::findTileAtScreen(int sx, int sy, rs_knl::TilePoint *outTile) const {
    if (!pMission_ || !pMission_->map()) return false;
    rs_knl::Map *pMap = pMission_->map();

    int worldX = static_cast<int>(sx / zoom_) + cameraX_;
    int worldY = static_cast<int>(sy / zoom_) + cameraY_;

    int topZ = isolateActiveZ_ ? activeZ_ : (pMap->maxTz() - 1);
    int bottomZ = isolateActiveZ_ ? activeZ_ : 0;
    int u_base = static_cast<int>(std::floor(static_cast<float>(worldX) / 32.0f)) - pMap->maxTx();

    for (int z = topZ; z >= bottomZ; --z) {
        int v_base = static_cast<int>(std::floor(static_cast<float>(worldY) / 16.0f)) - pMap->maxTz() + z - 1;

        // At each layer z, at most 3 candidate tiles overlap (worldX, worldY).
        // Iterate vOffset 0..2 (foreground to background order).
        for (int vOffset = 0; vOffset <= 2; ++vOffset) {
            int V = v_base - vOffset;
            for (int uOffset = 0; uOffset <= 1; ++uOffset) {
                int U = u_base - uOffset;
                if ((U & 1) == (V & 1)) {
                    int tx = (V + U) / 2;
                    int ty = (V - U) / 2;
                    if (tx >= 0 && tx < pMap->maxTx() && ty >= 0 && ty < pMap->maxTy()) {
                        rs_eng::Tile *t = pMap->getTileAt(tx, ty, z);
                        if (t && t->id() != 0) {
                            if (!filterBuildings_) {
                                if (isBuildingOrStructureTile(pMap, tx, ty, z, static_cast<uint8_t>(t->id()))) {
                                    continue;
                                }
                            }
                            int screen_w = (pMap->maxTx() + (tx - ty)) * (rs_eng::Tile::kTileWidth / 2);
                            int coord_h = ((pMap->maxTz() + tx + ty) - (z - 1)) * (rs_eng::Tile::kTileHeight / 3);
                            int localX = worldX - screen_w;
                            int localY = worldY - coord_h;
                            if (localX >= 0 && localX < 64 && localY >= 0 && localY < 48) {
                                const auto &winTile = TileRasterizer::instance().getTile(static_cast<uint8_t>(t->id()));
                                if (winTile.pixels[localY * 64 + localX] != 255) {
                                    if (outTile) {
                                        *outTile = rs_knl::TilePoint(tx, ty, z);
                                    }
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return false;
}

rs_knl::TilePoint WinMapViewport::pickTileAtScreen(int sx, int sy) const {
    rs_knl::TilePoint hitTile(0, 0, 0);
    if (findTileAtScreen(sx, sy, &hitTile)) {
        return hitTile;
    }
    return screenToMapPoint(sx, sy);
}

POINT WinMapViewport::mapPointToScreen(const rs_knl::TilePoint &tp) const {
    POINT res = {0, 0};
    if (!pMission_ || !pMission_->map()) return res;

    Point2D p;
    pMission_->map()->tileToScreenPoint(tp, &p);

    int worldX = p.x;
    int worldY = p.y - tp.tz * (rs_eng::Tile::kTileHeight / 3);
    worldY -= (tp.oz * (rs_eng::Tile::kTileHeight / 3)) / 128;

    res.x = static_cast<int>((worldX - cameraX_) * zoom_);
    res.y = static_cast<int>((worldY - cameraY_) * zoom_);
    return res;
}

POINT WinMapViewport::entityPointToScreen(const rs_knl::TilePoint &tp) const {
    POINT res = {0, 0};
    if (!pMission_ || !pMission_->map()) return res;

    Point2D p;
    pMission_->map()->tileToScreenPoint(tp, &p);

    int worldX = p.x;
    int worldY = p.y - tp.tz * (rs_eng::Tile::kTileHeight / 3) + (rs_eng::Tile::kTileHeight / 3);
    worldY -= (tp.oz * (rs_eng::Tile::kTileHeight / 3)) / 128;

    res.x = static_cast<int>((worldX - cameraX_) * zoom_);
    res.y = static_cast<int>((worldY - cameraY_) * zoom_);
    return res;
}

void WinMapViewport::drawDiamondOutline(uint32_t *buf, int bw, int bh, int sx, int sy, uint32_t color) {
    int halfW = static_cast<int>(32 * zoom_);
    int halfH = static_cast<int>(16 * zoom_);

    POINT pts[4] = {
        { sx, sy - halfH },
        { sx + halfW, sy },
        { sx, sy + halfH },
        { sx - halfW, sy }
    };

    auto plotLine = [&](POINT p1, POINT p2) {
        int dx = std::abs(p2.x - p1.x), sx = p1.x < p2.x ? 1 : -1;
        int dy = -std::abs(p2.y - p1.y), sy = p1.y < p2.y ? 1 : -1;
        int err = dx + dy, e2;
        int cx = p1.x, cy = p1.y;
        while (true) {
            if (cx >= 0 && cx < bw && cy >= 0 && cy < bh) {
                buf[cy * bw + cx] = color;
            }
            if (cx == p2.x && cy == p2.y) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; cx += sx; }
            if (e2 <= dx) { err += dx; cy += sy; }
        }
    };

    plotLine(pts[0], pts[1]);
    plotLine(pts[1], pts[2]);
    plotLine(pts[2], pts[3]);
    plotLine(pts[3], pts[0]);
}

void WinMapViewport::drawDiamondOutlineAlpha(uint32_t *buf, int bw, int bh, int sx, int sy, uint32_t color, uint8_t alpha) {
    int halfW = static_cast<int>(32 * zoom_);
    int halfH = static_cast<int>(16 * zoom_);

    POINT pts[4] = {
        { sx, sy - halfH },
        { sx + halfW, sy },
        { sx, sy + halfH },
        { sx - halfW, sy }
    };

    auto plotLineAlpha = [&](POINT p1, POINT p2) {
        int dx = std::abs(p2.x - p1.x), sxStep = p1.x < p2.x ? 1 : -1;
        int dy = -std::abs(p2.y - p1.y), syStep = p1.y < p2.y ? 1 : -1;
        int err = dx + dy, e2;
        int cx = p1.x, cy = p1.y;
        while (true) {
            if (cx >= 0 && cx < bw && cy >= 0 && cy < bh) {
                uint32_t bg = buf[cy * bw + cx];
                uint32_t rb = (color & 0x00FF00FF) * alpha + (bg & 0x00FF00FF) * (256 - alpha);
                uint32_t g  = (color & 0x0000FF00) * alpha + (bg & 0x0000FF00) * (256 - alpha);
                buf[cy * bw + cx] = 0xFF000000 | ((rb >> 8) & 0x00FF00FF) | ((g >> 8) & 0x0000FF00);
            }
            if (cx == p2.x && cy == p2.y) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; cx += sxStep; }
            if (e2 <= dx) { err += dx; cy += syStep; }
        }
    };

    plotLineAlpha(pts[0], pts[1]);
    plotLineAlpha(pts[1], pts[2]);
    plotLineAlpha(pts[2], pts[3]);
    plotLineAlpha(pts[3], pts[0]);
}

void WinMapViewport::drawDashedDiamondOutline(uint32_t *buf, int bw, int bh, int sx, int sy, uint32_t color) {
    int halfW = static_cast<int>(32 * zoom_);
    int halfH = static_cast<int>(16 * zoom_);

    POINT pts[4] = {
        { sx, sy - halfH },
        { sx + halfW, sy },
        { sx, sy + halfH },
        { sx - halfW, sy }
    };

    auto plotDashedLine = [&](POINT p1, POINT p2) {
        int dx = std::abs(p2.x - p1.x), sxStep = p1.x < p2.x ? 1 : -1;
        int dy = -std::abs(p2.y - p1.y), syStep = p1.y < p2.y ? 1 : -1;
        int err = dx + dy, e2;
        int cx = p1.x, cy = p1.y;
        int step = 0;
        while (true) {
            if ((step / 3) % 2 == 0) {
                if (cx >= 0 && cx < bw && cy >= 0 && cy < bh) {
                    buf[cy * bw + cx] = color;
                }
            }
            if (cx == p2.x && cy == p2.y) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; cx += sxStep; }
            if (e2 <= dx) { err += dx; cy += syStep; }
            ++step;
        }
    };

    plotDashedLine(pts[0], pts[1]);
    plotDashedLine(pts[1], pts[2]);
    plotDashedLine(pts[2], pts[3]);
    plotDashedLine(pts[3], pts[0]);
}

void WinMapViewport::drawSmallDashedDiamond(uint32_t *buf, int bw, int bh, int sx, int sy, uint32_t color) {
    int halfW = std::max(6, static_cast<int>(16 * zoom_));
    int halfH = std::max(3, static_cast<int>(8 * zoom_));

    POINT pts[4] = {
        { sx, sy - halfH },
        { sx + halfW, sy },
        { sx, sy + halfH },
        { sx - halfW, sy }
    };

    auto plotDashedLine = [&](POINT p1, POINT p2) {
        int dx = std::abs(p2.x - p1.x), sxStep = p1.x < p2.x ? 1 : -1;
        int dy = -std::abs(p2.y - p1.y), syStep = p1.y < p2.y ? 1 : -1;
        int err = dx + dy, e2;
        int cx = p1.x, cy = p1.y;
        int step = 0;
        while (true) {
            if ((step / 2) % 2 == 0) {
                if (cx >= 0 && cx < bw && cy >= 0 && cy < bh) {
                    buf[cy * bw + cx] = color;
                }
            }
            if (cx == p2.x && cy == p2.y) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; cx += sxStep; }
            if (e2 <= dx) { err += dx; cy += syStep; }
            ++step;
        }
    };

    plotDashedLine(pts[0], pts[1]);
    plotDashedLine(pts[1], pts[2]);
    plotDashedLine(pts[2], pts[3]);
    plotDashedLine(pts[3], pts[0]);
}

int WinMapViewport::getHighestRoofZ(int tx, int ty, int tz) const {
    if (!pMission_ || !pMission_->map()) return -1;
    rs_knl::Map *pMap = pMission_->map();
    int maxZ = isolateActiveZ_ ? std::min(pMap->maxTz(), activeZ_ + 1) : pMap->maxTz();
    int highest = -1;
    for (int z = tz + 1; z < maxZ; ++z) {
        rs_eng::Tile *t = pMap->getTileAt(tx, ty, z);
        if (t && t->id() != 0 && t->notTransparent()) {
            highest = z;
        }
    }
    return highest;
}

bool WinMapViewport::isEntityForegroundObscured(const rs_knl::MapObject *obj, int drawTx, int drawTy, int drawZ) const {
    if (!pMission_ || !pMission_->map() || !obj) return false;
    rs_knl::Map *pMap = pMission_->map();

    uint16_t animId = obj->getDrawAnimation();
    int frameId = obj->getDrawFrame();
    if (animId == 0) return false;

    int minX = 0, minY = 0, maxX = 0, maxY = 0;
    if (!SpriteRasterizer::instance().getAnimFrameBounds(animId, frameId, minX, minY, maxX, maxY)) {
        return false;
    }
    int bw = maxX - minX;
    int bh = maxY - minY;
    if (bw <= 0 || bh <= 0) return false;

    Point2D p;
    pMap->tileToScreenPoint(obj->position(), &p);
    int anchorWorldX = p.x;
    int anchorWorldY = p.y - obj->tileZ() * (rs_eng::Tile::kTileHeight / 3)
                     - (obj->position().oz * (rs_eng::Tile::kTileHeight / 3)) / 128
                     + (rs_eng::Tile::kTileHeight / 3);

    int boxL = anchorWorldX + minX;
    int boxT = anchorWorldY + minY;
    int boxR = anchorWorldX + maxX;
    int boxB = anchorWorldY + maxY;

    int stride = 1;
    while (((bw + stride - 1) / stride) * ((bh + stride - 1) / stride) > 1024) {
        ++stride;
    }
    int gridW = (bw + stride - 1) / stride;
    int gridH = (bh + stride - 1) / stride;
    int totalSamples = gridW * gridH;
    uint8_t covered[1024] = {};
    int coveredSamples = 0;

    const int maxTx = pMap->maxTx();
    const int maxTy = pMap->maxTy();
    const int mapMaxTz = pMap->maxTz();
    const int maxZ = isolateActiveZ_ ? std::min(mapMaxTz, activeZ_ + 1) : mapMaxTz;

    const int entSumXY = drawTx + drawTy;
    const int entSumXYZ = entSumXY + drawZ + 1;

    auto floorDiv = [](int a, int b) -> int {
        int q = a / b;
        int r = a % b;
        if (r != 0 && ((r < 0) != (b < 0))) --q;
        return q;
    };
    int uMin = floorDiv(boxL, rs_eng::Tile::kTileWidth / 2) - maxTx - 2;
    int uMax = floorDiv(boxR - 1, rs_eng::Tile::kTileWidth / 2) - maxTx;

    for (int tz = 0; tz < maxZ; ++tz) {
        int sMin = floorDiv(boxT, rs_eng::Tile::kTileHeight / 3) - mapMaxTz - 4 + tz;
        int sMax = floorDiv(boxB - 1, rs_eng::Tile::kTileHeight / 3) - mapMaxTz - 1 + tz;

        for (int s = sMin; s <= sMax; ++s) {
            const int sumXYZ = s + tz;
            if (sumXYZ < entSumXYZ) continue;

            for (int u = uMin; u <= uMax; ++u) {
                if (((s + u) & 1) != 0) continue;
                int tx = (s + u) >> 1;
                int ty = (s - u) >> 1;
                if (tx < 0 || tx >= maxTx || ty < 0 || ty >= maxTy) continue;

                if (sumXYZ == entSumXYZ) {
                    if (s < entSumXY) continue;
                    if (s == entSumXY && !(tx > drawTx || (tx == drawTx && ty == drawTy && tz == drawZ + 1))) continue;
                }

                rs_eng::Tile *t = pMap->getTileAt(tx, ty, tz);
                if (!t || t->id() == 0 || !t->notTransparent()) continue;
                if (!filterBuildings_ && isBuildingOrStructureTile(pMap, tx, ty, tz, static_cast<uint8_t>(t->id()))) {
                    continue;
                }

                const auto &wTile = TileRasterizer::instance().getTile(static_cast<uint8_t>(t->id()));
                if (!wTile.notAlpha) continue;

                int tileL = (maxTx + u) * (rs_eng::Tile::kTileWidth / 2);
                int tileT = ((mapMaxTz + s) - (tz - 1)) * (rs_eng::Tile::kTileHeight / 3);
                if (tileL >= boxR || tileL + rs_eng::Tile::kTileWidth <= boxL ||
                    tileT >= boxB || tileT + rs_eng::Tile::kTileHeight <= boxT) {
                    continue;
                }

                for (int gy = 0; gy < gridH; ++gy) {
                    int wy = boxT + gy * stride;
                    int ry = wy - tileT;
                    if (ry < 0 || ry >= rs_eng::Tile::kTileHeight) continue;
                    const uint8_t *tRow = &wTile.pixels[ry * rs_eng::Tile::kTileWidth];
                    for (int gx = 0; gx < gridW; ++gx) {
                        int idx = gy * gridW + gx;
                        if (covered[idx]) continue;
                        int wx = boxL + gx * stride;
                        int rx = wx - tileL;
                        if (rx < 0 || rx >= rs_eng::Tile::kTileWidth) continue;
                        if (tRow[rx] != 255) {
                            covered[idx] = 1;
                            ++coveredSamples;
                            if (coveredSamples * 2 >= totalSamples) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

int WinMapViewport::getHighestTileZ(int tx, int ty) const {
    if (!pMission_ || !pMission_->map()) return -1;
    rs_knl::Map *pMap = pMission_->map();
    if (tx < 0 || tx >= pMap->maxTx() || ty < 0 || ty >= pMap->maxTy()) return -1;
    int maxZ = isolateActiveZ_ ? std::min(pMap->maxTz(), activeZ_ + 1) : pMap->maxTz();
    for (int z = maxZ - 1; z >= 0; --z) {
        rs_eng::Tile *t = pMap->getTileAt(tx, ty, z);
        if (t && t->id() != 0 && t->notTransparent()) {
            return z;
        }
    }
    return -1;
}

void WinMapViewport::drawBoxFillOutline(uint32_t *buf, int bw, int bh) {
    int x1 = std::min(boxFillStart_.tx, boxFillCurrent_.tx);
    int x2 = std::max(boxFillStart_.tx, boxFillCurrent_.tx);
    int y1 = std::min(boxFillStart_.ty, boxFillCurrent_.ty);
    int y2 = std::max(boxFillStart_.ty, boxFillCurrent_.ty);

    for (int tx = x1; tx <= x2; ++tx) {
        for (int ty = y1; ty <= y2; ++ty) {
            POINT pt = mapPointToScreen(rs_knl::TilePoint(tx, ty, activeZ_));
            if (activeTileId_ != 0) {
                int drawX = pt.x - static_cast<int>(32 * zoom_);
                int drawY = pt.y - static_cast<int>(16 * zoom_);
                TileRasterizer::instance().drawTileScaledAlpha(buf, bw, bh, activeTileId_, drawX, drawY, zoom_, 150);
            }
            if (tx == x1 || tx == x2 || ty == y1 || ty == y2) {
                drawDiamondOutline(buf, bw, bh, pt.x, pt.y, 0xFF40C0FF);
            }
        }
    }
}

bool WinMapViewport::getEntityPlacementAnim(uint16_t &animId, int &frameId, int &heightPx) const {
    animId = 0;
    frameId = 0;
    heightPx = static_cast<int>(16 * zoom_);

    if (tool_ == ViewportTool::PlaceActor) {
        uint16_t base = (actorBaseAnimToPlace_ != 0) ? actorBaseAnimToPlace_ : rs_knl::PedManager::getDefaultBaseAnim(actorTypeToPlace_);
        int dir8 = ((orientationToPlace_ + 16) / 32) % 8;
        animId = static_cast<uint16_t>(base + dir8);
        frameId = 0;
        heightPx = static_cast<int>(26 * zoom_);
        return true;
    } else if (tool_ == ViewportTool::PlaceVehicle) {
        uint16_t anim = 446;
        if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeSmallArmored) anim = 534;
        else if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeLargeArmored) anim = 400;
        else if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypePolice) anim = 470;
        else if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeFireFighter) anim = 518;
        else if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeMedics) anim = 494;
        else if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeTrainHead) anim = 416;
        else if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeTrainBody) {
            anim = (vehicleBaseAnimToPlace_ != 0) ? vehicleBaseAnimToPlace_ : 428;
        }
        else if (vehicleTypeToPlace_ == 0xFE) anim = 416;

        int ori4 = ((orientationToPlace_ + 32) / 64) % 4;
        if (vehicleHealthToPlace_ <= 0 && vehicleHealthToPlace_ != 0) {
            if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeTrainHead || vehicleTypeToPlace_ == 0xFE) {
                anim = static_cast<uint16_t>(anim + 8 + (ori4 % 4));
            } else if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeTrainBody) {
                anim = static_cast<uint16_t>(anim + 4 + (ori4 % 2));
            } else {
                anim = static_cast<uint16_t>(anim + 12 + (ori4 % 4));
            }
        } else if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeTrainHead ||
                   vehicleTypeToPlace_ == 0xFE) {
            anim = static_cast<uint16_t>(anim + (ori4 % 4));
        } else if (vehicleTypeToPlace_ == rs_knl::Vehicle::kVehicleTypeTrainBody) {
            anim = static_cast<uint16_t>(anim + (ori4 % 2));
        } else {
            anim = static_cast<uint16_t>(anim + (ori4 * 2));
        }
        animId = anim;
        frameId = 0;
        heightPx = static_cast<int>(16 * zoom_);
        return true;
    } else if (tool_ == ViewportTool::PlaceStatic) {
        uint16_t anim = (staticBaseAnimToPlace_ != 0) ? staticBaseAnimToPlace_ : staticIdToPlace_;
        uint8_t st = static_cast<uint8_t>(staticIdToPlace_);
        bool isFixed = false;
        if (st == 0x1F) {
            isFixed = (staticBaseAnimToPlace_ < 572 || staticBaseAnimToPlace_ > 575);
        } else if (st == 0x0A || st == 0x16 || st == 0x05 || st == 0x19 || st == 0x1A) {
            isFixed = true;
        }

        if (!isFixed) {
            if (st == 0x26 && staticBaseAnimToPlace_ == 395) {
                if (orientationToPlace_ == rs_knl::Static::kStaticOrientationEW) anim = 392;
            } else if (orientationToPlace_ == rs_knl::Static::kStaticOrientationEW) {
                anim += 1;
            }
        }
        animId = anim;
        frameId = 0;
        heightPx = static_cast<int>(16 * zoom_);
        return true;
    } else if (tool_ == ViewportTool::PlaceWeapon) {
        rs_knl::Weapon *wDef = rs_knl::WeaponManager::singletonPtr() ? g_weaponMgr.getWeapon(weaponTypeToPlace_) : nullptr;
        uint16_t anim = wDef ? static_cast<uint16_t>(wDef->onGroundAnimationId()) : 0;
        if (anim == 0) {
            switch (weaponTypeToPlace_) {
                case rs_knl::Weapon::Persuadatron: anim = 367; break;
                case rs_knl::Weapon::Pistol:       anim = 368; break;
                case rs_knl::Weapon::GaussGun:     anim = 369; break;
                case rs_knl::Weapon::Shotgun:      anim = 370; break;
                case rs_knl::Weapon::Uzi:          anim = 371; break;
                case rs_knl::Weapon::Minigun:      anim = 372; break;
                case rs_knl::Weapon::Laser:        anim = 373; break;
                case rs_knl::Weapon::Flamer:       anim = 374; break;
                case rs_knl::Weapon::LongRange:    anim = 375; break;
                case rs_knl::Weapon::Scanner:      anim = 376; break;
                case rs_knl::Weapon::MediKit:      anim = 377; break;
                case rs_knl::Weapon::TimeBomb:     anim = 378; break;
                case rs_knl::Weapon::AirStrike:    anim = 378; break;
                case rs_knl::Weapon::AccessCard:   anim = 379; break;
                case rs_knl::Weapon::EnergyShield: anim = 381; break;
                case rs_knl::Weapon::CloneShield:  anim = 381; break;
                default:                           anim = 368; break;
            }
        }
        animId = anim;
        frameId = 0;
        heightPx = static_cast<int>(8 * zoom_);
        return true;
    }
    return false;
}

void WinMapViewport::draw3DBoundingBox(uint32_t *buf, int bw, int bh, const rs_knl::TilePoint &pos, int heightPx, uint32_t wireColor, bool extendsUpward) {
    POINT pt = extendsUpward ? entityPointToScreen(pos) : mapPointToScreen(pos);
    int halfW = static_cast<int>(32 * zoom_);
    int halfH = static_cast<int>(16 * zoom_);

    POINT bBack, bRight, bFront, bLeft;
    POINT tBack, tRight, tFront, tLeft;

    if (extendsUpward) {
        // Entity: pt is at foot/base, box extends upwards (-heightPx)
        bBack  = { pt.x, pt.y - halfH };
        bRight = { pt.x + halfW, pt.y };
        bFront = { pt.x, pt.y + halfH };
        bLeft  = { pt.x - halfW, pt.y };

        tBack  = { pt.x, pt.y - halfH - heightPx };
        tRight = { pt.x + halfW, pt.y - heightPx };
        tFront = { pt.x, pt.y + halfH - heightPx };
        tLeft  = { pt.x - halfW, pt.y - heightPx };
    } else {
        // Tile: pt is at top diamond center, box extends downwards (+heightPx)
        tBack  = { pt.x, pt.y - halfH };
        tRight = { pt.x + halfW, pt.y };
        tFront = { pt.x, pt.y + halfH };
        tLeft  = { pt.x - halfW, pt.y };

        bBack  = { pt.x, pt.y - halfH + heightPx };
        bRight = { pt.x + halfW, pt.y + heightPx };
        bFront = { pt.x, pt.y + halfH + heightPx };
        bLeft  = { pt.x - halfW, pt.y + heightPx };
    }

    auto plotLine = [&](POINT p1, POINT p2, uint32_t col) {
        int dx = std::abs(p2.x - p1.x), sx = p1.x < p2.x ? 1 : -1;
        int dy = -std::abs(p2.y - p1.y), sy = p1.y < p2.y ? 1 : -1;
        int err = dx + dy, e2;
        int cx = p1.x, cy = p1.y;
        while (true) {
            if (cx >= 0 && cx < bw && cy >= 0 && cy < bh) {
                buf[cy * bw + cx] = col;
            }
            if (cx == p2.x && cy == p2.y) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; cx += sx; }
            if (e2 <= dx) { err += dx; cy += sy; }
        }
    };

    auto plotDashedLine = [&](POINT p1, POINT p2, uint32_t col) {
        int dx = std::abs(p2.x - p1.x), sxStep = p1.x < p2.x ? 1 : -1;
        int dy = -std::abs(p2.y - p1.y), syStep = p1.y < p2.y ? 1 : -1;
        int err = dx + dy, e2;
        int cx = p1.x, cy = p1.y;
        int step = 0;
        while (true) {
            if ((step / 3) % 2 == 0) {
                if (cx >= 0 && cx < bw && cy >= 0 && cy < bh) {
                    buf[cy * bw + cx] = col;
                }
            }
            if (cx == p2.x && cy == p2.y) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; cx += sxStep; }
            if (e2 <= dx) { err += dx; cy += syStep; }
            ++step;
        }
    };

    // Draw bottom base diamond (front edges solid, rear edges dashed for depth)
    plotDashedLine(bBack, bRight, wireColor);
    plotDashedLine(bLeft, bBack, wireColor);
    plotLine(bLeft, bFront, wireColor);
    plotLine(bFront, bRight, wireColor);

    // Draw 4 vertical pillars
    plotDashedLine(bBack, tBack, wireColor);
    plotLine(bLeft, tLeft, wireColor);
    plotLine(bFront, tFront, wireColor);
    plotLine(bRight, tRight, wireColor);

    // Draw top diamond (all 4 edges solid)
    plotLine(tBack, tRight, wireColor);
    plotLine(tRight, tFront, wireColor);
    plotLine(tFront, tLeft, wireColor);
    plotLine(tLeft, tBack, wireColor);
}

void WinMapViewport::drawHoverWiregridColumn(uint32_t *buf, int bw, int bh, const rs_knl::TilePoint &pos) {
    if (!pMission_ || !pMission_->map()) return;
    rs_knl::Map *pMap = pMission_->map();
    if (pos.tx < 0 || pos.tx >= pMap->maxTx() || pos.ty < 0 || pos.ty >= pMap->maxTy()) return;

    const int kMaxColumnZ = 12;
    const uint32_t wireColor = 0xFF38BDF8; // Sky blue
    const uint32_t activeYellowColor = 0xFFFACC15; // Bright yellow
    const uint32_t activeTileColor = 0xFF00FF80;   // Neon green for Pencil/BoxFill
    int halfW = static_cast<int>(32 * zoom_);
    int halfH = static_cast<int>(16 * zoom_);
    int heightPx = static_cast<int>(16 * zoom_);

    auto plotLineAlpha = [&](POINT p1, POINT p2, uint32_t col, uint8_t alpha) {
        int dx = std::abs(p2.x - p1.x), sx = p1.x < p2.x ? 1 : -1;
        int dy = -std::abs(p2.y - p1.y), sy = p1.y < p2.y ? 1 : -1;
        int err = dx + dy, e2;
        int cx = p1.x, cy = p1.y;
        while (true) {
            if (cx >= 0 && cx < bw && cy >= 0 && cy < bh) {
                if (alpha == 255) {
                    buf[cy * bw + cx] = col;
                } else {
                    uint32_t bg = buf[cy * bw + cx];
                    uint32_t rb = (col & 0x00FF00FF) * alpha + (bg & 0x00FF00FF) * (256 - alpha);
                    uint32_t g  = (col & 0x0000FF00) * alpha + (bg & 0x0000FF00) * (256 - alpha);
                    buf[cy * bw + cx] = 0xFF000000 | ((rb >> 8) & 0x00FF00FF) | ((g >> 8) & 0x0000FF00);
                }
            }
            if (cx == p2.x && cy == p2.y) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; cx += sx; }
            if (e2 <= dx) { err += dx; cy += sy; }
        }
    };

    auto plotDashedLineAlpha = [&](POINT p1, POINT p2, uint32_t col, uint8_t alpha) {
        int dx = std::abs(p2.x - p1.x), sxStep = p1.x < p2.x ? 1 : -1;
        int dy = -std::abs(p2.y - p1.y), syStep = p1.y < p2.y ? 1 : -1;
        int err = dx + dy, e2;
        int cx = p1.x, cy = p1.y;
        int step = 0;
        while (true) {
            if ((step / 3) % 2 == 0) {
                if (cx >= 0 && cx < bw && cy >= 0 && cy < bh) {
                    if (alpha == 255) {
                        buf[cy * bw + cx] = col;
                    } else {
                        uint32_t bg = buf[cy * bw + cx];
                        uint32_t rb = (col & 0x00FF00FF) * alpha + (bg & 0x00FF00FF) * (256 - alpha);
                        uint32_t g  = (col & 0x0000FF00) * alpha + (bg & 0x0000FF00) * (256 - alpha);
                        buf[cy * bw + cx] = 0xFF000000 | ((rb >> 8) & 0x00FF00FF) | ((g >> 8) & 0x0000FF00);
                    }
                }
            }
            if (cx == p2.x && cy == p2.y) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; cx += sxStep; }
            if (e2 <= dx) { err += dx; cy += syStep; }
            ++step;
        }
    };

    auto drawBox = [&](int z, uint32_t col, uint8_t alpha) {
        // Box z: represents the 3D unit layer z.
        // Top diamond is at mapPointToScreen(tx, ty, z).
        // Bottom diamond is at mapPointToScreen(tx, ty, z - 1) (heightPx lower on screen: +heightPx).
        POINT ptTop = mapPointToScreen(rs_knl::TilePoint(pos.tx, pos.ty, z));

        POINT bBack  = { ptTop.x, ptTop.y - halfH + heightPx };
        POINT bRight = { ptTop.x + halfW, ptTop.y + heightPx };
        POINT bFront = { ptTop.x, ptTop.y + halfH + heightPx };
        POINT bLeft  = { ptTop.x - halfW, ptTop.y + heightPx };

        POINT tBack  = { ptTop.x, ptTop.y - halfH };
        POINT tRight = { ptTop.x + halfW, ptTop.y };
        POINT tFront = { ptTop.x, ptTop.y + halfH };
        POINT tLeft  = { ptTop.x - halfW, ptTop.y };

        // Bottom diamond (rear dashed, front solid)
        plotDashedLineAlpha(bBack, bRight, col, alpha);
        plotDashedLineAlpha(bLeft, bBack, col, alpha);
        plotLineAlpha(bLeft, bFront, col, alpha);
        plotLineAlpha(bFront, bRight, col, alpha);

        // 4 Vertical pillars
        plotDashedLineAlpha(bBack, tBack, col, alpha);
        plotLineAlpha(bLeft, tLeft, col, alpha);
        plotLineAlpha(bFront, tFront, col, alpha);
        plotLineAlpha(bRight, tRight, col, alpha);

        // Top diamond (rear dashed, front solid)
        plotDashedLineAlpha(tBack, tRight, col, alpha);
        plotDashedLineAlpha(tLeft, tBack, col, alpha);
        plotLineAlpha(tLeft, tFront, col, alpha);
        plotLineAlpha(tFront, tRight, col, alpha);
    };

    auto drawHorizontalDiamond = [&](int z, uint32_t col, uint8_t alpha) {
        POINT pt = mapPointToScreen(rs_knl::TilePoint(pos.tx, pos.ty, z));
        POINT dBack  = { pt.x, pt.y - halfH };
        POINT dRight = { pt.x + halfW, pt.y };
        POINT dFront = { pt.x, pt.y + halfH };
        POINT dLeft  = { pt.x - halfW, pt.y };

        plotDashedLineAlpha(dBack, dRight, col, alpha);
        plotDashedLineAlpha(dLeft, dBack, col, alpha);
        plotLineAlpha(dLeft, dFront, col, alpha);
        plotLineAlpha(dFront, dRight, col, alpha);
    };

    bool isTileTool = (tool_ == ViewportTool::Pencil || tool_ == ViewportTool::BoxFill);
    bool isObscured = false;
    int startZ = 0;

    // Identify the layer Z of the tile on which the user is mousing over
    int mousedTileZ = pos.tz;
    rs_eng::Tile *tAtPos = pMap->getTileAt(pos.tx, pos.ty, mousedTileZ);
    if (!tAtPos || tAtPos->id() == 0 || !tAtPos->notTransparent()) {
        mousedTileZ = -1;
        for (int z = kMaxColumnZ; z >= 0; --z) {
            rs_eng::Tile *t = pMap->getTileAt(pos.tx, pos.ty, z);
            if (t && t->id() != 0 && t->notTransparent()) {
                mousedTileZ = z;
                break;
            }
        }
    }

    if (isTileTool) {
        int highestRoofZ = -1;
        for (int z = kMaxColumnZ; z > activeZ_; --z) {
            rs_eng::Tile *t = pMap->getTileAt(pos.tx, pos.ty, z);
            if (t && t->id() != 0 && t->notTransparent()) {
                highestRoofZ = z;
                break;
            }
        }
        if (highestRoofZ > activeZ_) {
            isObscured = true;
            startZ = highestRoofZ + 1;
        } else {
            isObscured = false;
            int highestGroundZ = -1;
            for (int z = activeZ_ - 1; z >= 0; --z) {
                rs_eng::Tile *t = pMap->getTileAt(pos.tx, pos.ty, z);
                if (t && t->id() != 0 && t->notTransparent()) {
                    highestGroundZ = z;
                    break;
                }
            }
            startZ = (highestGroundZ >= 0) ? (highestGroundZ + 1) : 0;
        }
    } else {
        startZ = 0;
    }

    // 1. Draw all standard column wiregrid boxes (blue, or faded blue below ground/roof)
    for (int z = startZ; z <= kMaxColumnZ; ++z) {
        bool isFaded = (!isTileTool && mousedTileZ >= 0 && z < mousedTileZ);
        uint8_t alpha = isFaded ? 65 : 255;
        drawBox(z, wireColor, alpha);
    }

    // 2. Draw the active layer box (activeZ_) so all of its 12 edges (bottom diamond, 4 vertical pillars, top diamond)
    // are prominently highlighted in yellow without being overwritten by adjacent boxes.
    if (activeZ_ >= startZ && activeZ_ <= kMaxColumnZ) {
        bool isActiveFaded = (!isTileTool && mousedTileZ >= 0 && activeZ_ < mousedTileZ);
        uint8_t activeAlpha = isActiveFaded ? 110 : 255;
        uint32_t activeCol = isTileTool ? activeTileColor : activeYellowColor;
        drawBox(activeZ_, activeCol, activeAlpha);
    }

    // 3. Highlight in green the square representing the surface of ground/roof tile that the column intersects with
    if (!isTileTool && mousedTileZ >= 0 && mousedTileZ <= kMaxColumnZ) {
        const uint32_t groundSurfaceGreen = 0xFF00FF80; // Neon green
        drawHorizontalDiamond(mousedTileZ, groundSurfaceGreen, 255);
    }

    // Render floating tile preview inside wiregrid column at activeZ_
    if (isTileTool && activeTileId_ != 0) {
        if (!isObscured && activeZ_ >= 0 && activeZ_ <= kMaxColumnZ) {
            POINT pt = mapPointToScreen(rs_knl::TilePoint(pos.tx, pos.ty, activeZ_));
            int drawX = pt.x - static_cast<int>(32 * zoom_);
            int drawY = pt.y - static_cast<int>(16 * zoom_);
            TileRasterizer::instance().drawTileScaledAlpha(buf, bw, bh, activeTileId_, drawX, drawY, zoom_, 190);
            drawDiamondOutline(buf, bw, bh, pt.x, pt.y, 0xFF00FF80); // Neon green top diamond
        }
    }

    // Render floating entity preview inside wiregrid column (renders even inside buildings / under roof)
    if (isEntityPlaceTool(tool_)) {
        rs_knl::TilePoint spawnPos = currentTilePos_;
        uint16_t anim = 0;
        int frame = 0;
        int hPx = 0;
        if (getEntityPlacementAnim(anim, frame, hPx) && anim != 0) {
            POINT pt = entityPointToScreen(spawnPos);
            int previewFac = -1;
            if (tool_ == ViewportTool::PlaceActor && actorTypeToPlace_ == rs_knl::PedInstance::kPedTypeAgent) {
                previewFac = actorIsOurAgentToPlace_ ? 0 : actorFactionIdToPlace_;
            }
            SpriteRasterizer::instance().drawAnimFrame(buf, bw, bh, anim, frame, pt.x, pt.y, zoom_, 190, previewFac);
            drawDiamondOutline(buf, bw, bh, pt.x, pt.y, 0xFF00FF80); // Neon green placement base diamond
        }
    }
}

void WinMapViewport::drawSelectionBadges(HDC hdc) {
    if (!pMission_ || !pMission_->map()) return;
    rs_knl::Map *pMap = pMission_->map();

    struct BadgeInfo {
        int x;
        int y;
        std::wstring text;
        COLORREF borderColor = RGB(56, 189, 248);
        COLORREF bgColor = RGB(15, 23, 42);
        COLORREF textColor = RGB(241, 245, 249);
    };
    std::vector<BadgeInfo> badges;

    // 1. Hover Wiregrid Column Badges (Select Tool, Pencil, BoxFill, Entity Placement)
    bool showWiregridBadges = false;
    if (objectivePickMode_ == ObjectivePickMode::None) {
        if (currentTilePos_.tx >= 0 && currentTilePos_.tx < pMap->maxTx() &&
            currentTilePos_.ty >= 0 && currentTilePos_.ty < pMap->maxTy()) {
            if ((tool_ == ViewportTool::SelectTile || tool_ == ViewportTool::SelectEntity) &&
                (!isDraggingTile_ || !hasDraggedTile_) &&
                (!isDraggingEntity_ || !hasDraggedEntity_)) {
                showWiregridBadges = true;
            } else if ((tool_ == ViewportTool::Pencil || (tool_ == ViewportTool::BoxFill && !isBoxFilling_)) && activeTileId_ != 0) {
                showWiregridBadges = true;
            } else if (isEntityPlaceTool(tool_)) {
                showWiregridBadges = true;
            }
        }
    }

    if (showWiregridBadges) {
        int surfZ = 0;
        int baseZ = 0;
        const int kMaxColumnZ = 12;
        bool isTileTool = (tool_ == ViewportTool::Pencil || tool_ == ViewportTool::BoxFill);
        bool isObscured = false;
        COLORREF defaultBorderColor = RGB(56, 189, 248); // Sky blue
        COLORREF activeBorderColor = RGB(0, 255, 128);   // Neon green

        // Identify the layer Z of the tile on which the user is mousing over
        int mousedTileZ = currentTilePos_.tz;
        rs_eng::Tile *tAtPos = pMap->getTileAt(currentTilePos_.tx, currentTilePos_.ty, mousedTileZ);
        if (!tAtPos || tAtPos->id() == 0 || !tAtPos->notTransparent()) {
            mousedTileZ = -1;
            for (int z = kMaxColumnZ; z >= 0; --z) {
                rs_eng::Tile *t = pMap->getTileAt(currentTilePos_.tx, currentTilePos_.ty, z);
                if (t && t->id() != 0 && t->notTransparent()) {
                    mousedTileZ = z;
                    break;
                }
            }
        }

        if (isTileTool) {
            int highestRoofZ = -1;
            for (int z = kMaxColumnZ; z > activeZ_; --z) {
                rs_eng::Tile *t = pMap->getTileAt(currentTilePos_.tx, currentTilePos_.ty, z);
                if (t && t->id() != 0 && t->notTransparent()) {
                    highestRoofZ = z;
                    break;
                }
            }
            if (highestRoofZ > activeZ_) {
                isObscured = true;
                surfZ = highestRoofZ + 1;
                baseZ = highestRoofZ;
            } else {
                isObscured = false;
                int highestGroundZ = -1;
                for (int z = activeZ_ - 1; z >= 0; --z) {
                    rs_eng::Tile *t = pMap->getTileAt(currentTilePos_.tx, currentTilePos_.ty, z);
                    if (t && t->id() != 0 && t->notTransparent()) {
                        highestGroundZ = z;
                        break;
                    }
                }
                if (highestGroundZ >= 0) {
                    surfZ = highestGroundZ + 1;
                    baseZ = highestGroundZ;
                } else {
                    surfZ = 0;
                    baseZ = 0;
                }
            }
        } else {
            surfZ = 0;
            baseZ = 0;
        }

        bool hasOverlapSelection = false;
        if (targetSelected_ && targetSelected_->tileX() == currentTilePos_.tx && targetSelected_->tileY() == currentTilePos_.ty) {
            hasOverlapSelection = true;
        } else if (hasSelectedTile_ && selectedTilePos_.tx == currentTilePos_.tx && selectedTilePos_.ty == currentTilePos_.ty) {
            hasOverlapSelection = true;
        }

        POINT ptSurf = mapPointToScreen(rs_knl::TilePoint(currentTilePos_.tx, currentTilePos_.ty, baseZ));
        int halfW = static_cast<int>(32 * zoom_);
        int halfH = static_cast<int>(16 * zoom_);
        int heightPx = static_cast<int>(16 * zoom_);

        // For surfZ = 0 (empty ground), base diamond is at bottom of Box 0 (ptSurf.y + heightPx)
        int surfY = (surfZ > 0) ? ptSurf.y : (ptSurf.y + heightPx);
        POINT bRightSurf = { ptSurf.x + halfW, surfY };
        POINT bFrontSurf = { ptSurf.x, surfY + halfH };
        POINT bLeftSurf  = { ptSurf.x - halfW, surfY };

        // X & Y labels on Bottom-Left and Bottom-Right edges at surface level (only if no selected object badge overlaps)
        if (!hasOverlapSelection) {
            COLORREF xyColor = isTileTool ? activeBorderColor : defaultBorderColor;
            int xMidX = (bLeftSurf.x + bFrontSurf.x) / 2;
            int xMidY = (bLeftSurf.y + bFrontSurf.y) / 2;
            badges.push_back({ xMidX - 44, xMidY + 2, std::format(L"X: {}", currentTilePos_.tx), xyColor });

            int yMidX = (bRightSurf.x + bFrontSurf.x) / 2;
            int yMidY = (bRightSurf.y + bFrontSurf.y) / 2;
            badges.push_back({ yMidX + 8, yMidY + 2, std::format(L"Y: {}", currentTilePos_.ty), xyColor });
        }

        // Z labels on Right Vertical (+Z) pillar for each visible box
        int stepZ = (zoom_ < 0.75f) ? 2 : 1;
        bool activeZBadged = false;
        for (int z = surfZ; z <= kMaxColumnZ; z += stepZ) {
            POINT ptBox = mapPointToScreen(rs_knl::TilePoint(currentTilePos_.tx, currentTilePos_.ty, z));
            POINT bR = { ptBox.x + halfW, ptBox.y + heightPx };
            POINT tR = { ptBox.x + halfW, ptBox.y };
            int badgeX = bR.x + 8;
            int badgeY = (bR.y + tR.y) / 2 - 7;

            if (z == activeZ_) {
                activeZBadged = true;
                if (isTileTool) {
                    badges.push_back({ badgeX, badgeY, std::format(L"Layer: {} (Active)", z), activeBorderColor, RGB(15, 23, 42), RGB(0, 255, 128) });
                } else {
                    // Yellow background badge on the layer label for the current active layer!
                    COLORREF bgYellow = RGB(250, 204, 21);
                    COLORREF textDark = RGB(15, 23, 42);
                    COLORREF borderAmber = RGB(202, 138, 4);
                    badges.push_back({ badgeX, badgeY, std::format(L"Layer: {} (Active)", z), borderAmber, bgYellow, textDark });
                }
            } else if (!isTileTool && z == mousedTileZ) {
                // Ground / roof surface intersection layer: neon green badge border & text
                COLORREF borderGreen = RGB(0, 255, 128);
                COLORREF textGreen   = RGB(0, 255, 128);
                badges.push_back({ badgeX, badgeY, std::format(L"Layer: {} (Surface)", z), borderGreen, RGB(15, 23, 42), textGreen });
            } else if (!isTileTool && mousedTileZ >= 0 && z < mousedTileZ) {
                // Faded layer below moused-over ground/roof
                COLORREF borderFaded = RGB(30, 80, 115);
                COLORREF textFaded   = RGB(100, 116, 139);
                badges.push_back({ badgeX, badgeY, std::format(L"Layer: {}", z), borderFaded, RGB(15, 23, 42), textFaded });
            } else {
                badges.push_back({ badgeX, badgeY, std::format(L"Layer: {}", z), defaultBorderColor, RGB(15, 23, 42), RGB(241, 245, 249) });
            }
        }

        // If in Pencil/BoxFill mode, show activeZ_ status (either at Box activeZ_ or as OBSCURED at surface)
        if (isTileTool) {
            if (isObscured) {
                // Floating tile is obscured below the surface/roof tile
                badges.push_back({ bRightSurf.x + 8, surfY - 7, std::format(L"Layer: {} (OBSCURED)", activeZ_), RGB(245, 158, 11) });
            } else if (!activeZBadged && activeZ_ >= surfZ && activeZ_ <= kMaxColumnZ) {
                // In case stepZ skipped activeZ_, explicitly show activeZ_ badge
                POINT ptBox = mapPointToScreen(rs_knl::TilePoint(currentTilePos_.tx, currentTilePos_.ty, activeZ_));
                POINT bR = { ptBox.x + halfW, ptBox.y + heightPx };
                POINT tR = { ptBox.x + halfW, ptBox.y };
                badges.push_back({ bR.x + 8, (bR.y + tR.y) / 2 - 7, std::format(L"Layer: {} (Active)", activeZ_), activeBorderColor, RGB(15, 23, 42), RGB(0, 255, 128) });
            }
        } else if (!activeZBadged && activeZ_ >= surfZ && activeZ_ <= kMaxColumnZ) {
            // Explicitly show yellow activeZ_ badge if stepZ skipped it
            POINT ptBox = mapPointToScreen(rs_knl::TilePoint(currentTilePos_.tx, currentTilePos_.ty, activeZ_));
            POINT bR = { ptBox.x + halfW, ptBox.y + heightPx };
            POINT tR = { ptBox.x + halfW, ptBox.y };
            COLORREF bgYellow = RGB(250, 204, 21);
            COLORREF textDark = RGB(15, 23, 42);
            COLORREF borderAmber = RGB(202, 138, 4);
            badges.push_back({ bR.x + 8, (bR.y + tR.y) / 2 - 7, std::format(L"Layer: {} (Active)", activeZ_), borderAmber, bgYellow, textDark });
        }
    }

    // 2. Selected Entity or Tile Badges
    if (targetSelected_ || hasSelectedTile_) {
        rs_knl::TilePoint pos;
        int heightPx = 0;
        COLORREF selBorderColor = RGB(255, 215, 0); // Gold for entity

        if (targetSelected_) {
            pos = targetSelected_->position();
            if (dynamic_cast<rs_knl::PedInstance*>(targetSelected_)) {
                heightPx = static_cast<int>(26 * zoom_);
            } else if (dynamic_cast<rs_knl::WeaponInstance*>(targetSelected_)) {
                heightPx = static_cast<int>(8 * zoom_);
            } else {
                heightPx = static_cast<int>(16 * zoom_);
            }
            selBorderColor = RGB(255, 215, 0);
        } else if (hasSelectedTile_) {
            pos = (isDraggingTile_ && hasDraggedTile_) ? dragTileCurrentPos_ : selectedTilePos_;
            heightPx = static_cast<int>(16 * zoom_);
            selBorderColor = RGB(0, 229, 255); // Cyan for tile
        }

        POINT pt = targetSelected_ ? entityPointToScreen(pos) : mapPointToScreen(pos);
        int halfW = static_cast<int>(32 * zoom_);
        int halfH = static_cast<int>(16 * zoom_);

        POINT bRight, bFront, bLeft, tRight;
        if (targetSelected_) {
            // Entity: extends upward from base
            bRight = { pt.x + halfW, pt.y };
            bFront = { pt.x, pt.y + halfH };
            bLeft  = { pt.x - halfW, pt.y };
            tRight = { pt.x + halfW, pt.y - heightPx };
        } else {
            // Tile: extends downward from top surface
            bRight = { pt.x + halfW, pt.y + heightPx };
            bFront = { pt.x, pt.y + halfH + heightPx };
            bLeft  = { pt.x - halfW, pt.y + heightPx };
            tRight = { pt.x + halfW, pt.y };
        }

        int xMidX = (bLeft.x + bFront.x) / 2;
        int xMidY = (bLeft.y + bFront.y) / 2;
        badges.push_back({ xMidX - 44, xMidY + 2, std::format(L"X: {}", pos.tx), selBorderColor });

        int yMidX = (bRight.x + bFront.x) / 2;
        int yMidY = (bRight.y + bFront.y) / 2;
        badges.push_back({ yMidX + 8, yMidY + 2, std::format(L"Y: {}", pos.ty), selBorderColor });

        int zMidX = bRight.x;
        int zMidY = (bRight.y + tRight.y) / 2;
        badges.push_back({ zMidX + 8, zMidY - 8, std::format(L"Layer: {}", pos.tz), selBorderColor });

        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(targetSelected_)) {
            std::wstring moveStr = (pPed->initialLevelState() & LevelData::kPeopleStateWalking) ? L"Patrol" : L"Stationary";
            if (pPed->isOurAgent()) {
                badges.push_back({ pt.x - 50, pt.y - heightPx - 20, std::format(L"Player's Agent #{}", pPed->id() + 1), selBorderColor });
            } else if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent) {
                const wchar_t *fStr = L"Blue";
                int fid = pPed->factionId();
                if (fid == 2) fStr = L"Orange";
                else if (fid >= 3) fStr = L"Yellow";
                badges.push_back({ pt.x - 50, pt.y - heightPx - 20, std::format(L"Ped #{:03} [{}] [{}]", pPed->id(), fStr, moveStr), selBorderColor });
            } else {
                badges.push_back({ pt.x - 40, pt.y - heightPx - 20, std::format(L"Ped #{:03} [{}]", pPed->id(), moveStr), selBorderColor });
            }
        }
    }

    if (badges.empty()) return;

    HFONT hFont = CreateFontW(
        -11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

    HBRUSH hDefaultBgBrush = CreateSolidBrush(RGB(15, 23, 42));
    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hDefaultBgBrush));
    HPEN hOldPen = static_cast<HPEN>(GetCurrentObject(hdc, OBJ_PEN));

    SetBkMode(hdc, TRANSPARENT);

    for (const auto &b : badges) {
        RECT rcCalc = { 0, 0, 0, 0 };
        DrawTextW(hdc, b.text.c_str(), -1, &rcCalc, DT_CALCRECT | DT_SINGLELINE);
        int padX = 5;
        int padY = 2;
        int bw = (rcCalc.right - rcCalc.left) + padX * 2;
        int bh = (rcCalc.bottom - rcCalc.top) + padY * 2;

        HBRUSH hBadgeBrush = CreateSolidBrush(b.bgColor);
        HPEN hPen = CreatePen(PS_SOLID, 1, b.borderColor);
        SelectObject(hdc, hBadgeBrush);
        SelectObject(hdc, hPen);

        RoundRect(hdc, b.x, b.y, b.x + bw, b.y + bh, 4, 4);

        SetTextColor(hdc, b.textColor);
        RECT rcText = { b.x + padX, b.y + padY, b.x + bw - padX, b.y + bh - padY };
        DrawTextW(hdc, b.text.c_str(), -1, &rcText, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hBadgeBrush);
        DeleteObject(hPen);
    }

    SelectObject(hdc, hOldFont);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hFont);
    DeleteObject(hDefaultBgBrush);
}

void WinMapViewport::drawRouteOverlay(HDC hdc) {
    if (!pMission_ || !pMission_->map() || objectivePickMode_ != ObjectivePickMode::None) return;

    rs_knl::PedInstance *pPed = getActiveRoutePed();
    if (!pPed) return;

    const auto &steps = pPed->scriptSteps();
    bool isRouteTool = (tool_ == ViewportTool::RouteTool);
    if (steps.empty() && !isRouteTool) return;

    rs_knl::Vehicle *initialVeh = getPedVehicleAtStep(pMission_, pPed, 0);
    bool startsInVeh = (initialVeh != nullptr);

    POINT startPt = startsInVeh ? entityPointToScreen(initialVeh->position()) : entityPointToScreen(pPed->position());

    struct WaypointNode {
        size_t stepIdx;
        int displayNum;
        POINT screenPos;
        rs_knl::TilePoint mapPos;
        uint8_t type;
        uint32_t param;
        int targetEntityId{-1};
        std::wstring dwellTag;
        bool isVehicleBoard{false};
    };
    std::vector<WaypointNode> waypoints;
    std::wstring spawnDwellTag;
    int displayCounter = 1;
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto &st = steps[i];
        if (st.type == LevelData::kScenarioTypeGoto || st.type == 1) {
            POINT pt;
            if (isDraggingWaypoint_ && static_cast<int>(i) == draggingWaypointIdx_) {
                pt = entityPointToScreen(dragWaypointCurrentPos_);
            } else {
                pt = entityPointToScreen(st.targetPos);
            }
            WaypointNode node;
            node.stepIdx = i;
            node.displayNum = displayCounter++;
            node.screenPos = pt;
            node.mapPos = (isDraggingWaypoint_ && static_cast<int>(i) == draggingWaypointIdx_) ? dragWaypointCurrentPos_ : st.targetPos;
            node.type = st.type;
            node.param = st.param;
            node.targetEntityId = -1;
            node.isVehicleBoard = false;
            waypoints.push_back(node);
        } else if (st.type == LevelData::kScenarioTypeUseVehicle || st.type == 2) {
            if (pMission_ && st.targetEntityId >= 0 && static_cast<size_t>(st.targetEntityId) < pMission_->numVehicles()) {
                rs_knl::Vehicle *targetVeh = pMission_->vehicle(st.targetEntityId);
                if (targetVeh) {
                    POINT pt = entityPointToScreen(targetVeh->position());
                    WaypointNode node;
                    node.stepIdx = i;
                    node.displayNum = displayCounter++;
                    node.screenPos = pt;
                    node.mapPos = targetVeh->position();
                    node.type = st.type;
                    node.param = 0;
                    node.targetEntityId = st.targetEntityId;
                    node.isVehicleBoard = true;
                    waypoints.push_back(node);
                }
            }
        } else if (st.type == LevelData::kScenarioTypeWait || st.type == 5) {
            float sec = (st.param > 0) ? (st.param / 1000.0f) : 5.0f;
            std::wstring tag = std::format(L"{:.1f}s", sec);
            if (waypoints.empty()) {
                spawnDwellTag = tag;
            } else {
                waypoints.back().dwellTag = tag;
            }
        } else if (st.type == LevelData::kScenarioTypeTrainWait || st.type == 10) {
            std::wstring tag = L"Train Wait";
            if (waypoints.empty()) {
                spawnDwellTag = tag;
            } else {
                waypoints.back().dwellTag = tag;
            }
        } else if (st.type == LevelData::kScenarioTypeLeaveVehicle || st.type == 6) {
            std::wstring tag = L"Exit";
            if (waypoints.empty()) {
                spawnDwellTag = tag;
            } else {
                waypoints.back().dwellTag = tag;
            }
        } else if (st.type == LevelData::kScenarioTypeTrigger || st.type == 8) {
            POINT pt;
            if (isDraggingWaypoint_ && static_cast<int>(i) == draggingWaypointIdx_) {
                pt = entityPointToScreen(dragWaypointCurrentPos_);
            } else {
                pt = entityPointToScreen(st.targetPos);
            }
            WaypointNode node;
            node.stepIdx = i;
            node.displayNum = displayCounter++;
            node.screenPos = pt;
            node.mapPos = (isDraggingWaypoint_ && static_cast<int>(i) == draggingWaypointIdx_) ? dragWaypointCurrentPos_ : st.targetPos;
            uint32_t rUnits = (st.param > 0) ? st.param : (6 * 256);
            int rTiles = static_cast<int>(rUnits / 256);
            node.type = st.type;
            node.param = rUnits;
            node.targetEntityId = -1;
            node.dwellTag = std::format(L"Ambush ({} tiles)", rTiles);
            node.isVehicleBoard = false;
            waypoints.push_back(node);
        } else if (st.type == LevelData::kScenarioTypeEscape || st.type == 7) {
            if (st.targetPos.tx != 0 || st.targetPos.ty != 0) {
                POINT pt = entityPointToScreen(st.targetPos);
                WaypointNode node;
                node.stepIdx = i;
                node.displayNum = displayCounter++;
                node.screenPos = pt;
                node.mapPos = st.targetPos;
                node.type = st.type;
                node.param = 0;
                node.targetEntityId = -1;
                node.dwellTag = L"Escape";
                node.isVehicleBoard = false;
                waypoints.push_back(node);
            } else {
                std::wstring tag = L"Escape";
                if (waypoints.empty()) {
                    spawnDwellTag = tag;
                } else {
                    waypoints.back().dwellTag = tag;
                }
            }
        }
    }

    // Helper to draw an arrowhead along vector from p1 to p2
    auto drawArrowHead = [&](POINT p1, POINT p2, COLORREF col) {
        double dx = static_cast<double>(p2.x - p1.x);
        double dy = static_cast<double>(p2.y - p1.y);
        double len = std::sqrt(dx * dx + dy * dy);
        if (len < 16.0) return;
        double midX = (p1.x + p2.x) * 0.5;
        double midY = (p1.y + p2.y) * 0.5;
        double angle = std::atan2(dy, dx);
        double arrowLen = 9.0;
        double arrowAngle = 2.6; // ~150 degrees
        POINT a1 = {
            static_cast<int>(midX + arrowLen * std::cos(angle + arrowAngle)),
            static_cast<int>(midY + arrowLen * std::sin(angle + arrowAngle))
        };
        POINT a2 = {
            static_cast<int>(midX + arrowLen * std::cos(angle - arrowAngle)),
            static_cast<int>(midY + arrowLen * std::sin(angle - arrowAngle))
        };
        HPEN hArrowPen = CreatePen(PS_SOLID, 2, col);
        HPEN hOld = static_cast<HPEN>(SelectObject(hdc, hArrowPen));
        MoveToEx(hdc, a1.x, a1.y, nullptr);
        LineTo(hdc, static_cast<int>(midX), static_cast<int>(midY));
        LineTo(hdc, a2.x, a2.y);
        SelectObject(hdc, hOld);
        DeleteObject(hArrowPen);
    };

    // 1. Draw connecting route segments (road A* path for vehicles, walking line for pedestrians)
    auto getVehicleColors = [](rs_knl::Vehicle *v, bool isLoop, bool isPreview) -> COLORREF {
        if (v && v->isCar()) {
            if (isPreview) return RGB(253, 224, 71); // Light yellow
            if (isLoop)    return RGB(251, 191, 36); // Bright amber
            return RGB(245, 158, 11);               // Gold/Amber
        } else if (v) {
            if (isPreview) return RGB(165, 243, 252); // Light cyan
            if (isLoop)    return RGB(103, 232, 249); // Bright cyan
            return RGB(6, 182, 212);                  // Cyan
        } else {
            if (isPreview) return RGB(134, 239, 172); // Mint / Light green
            if (isLoop)    return RGB(74, 222, 128);  // Bright green
            return RGB(34, 197, 94);                  // Emerald green
        }
    };

    int penW = std::max(2, static_cast<int>(2.0f * zoom_));

    auto drawPathSegment = [&](rs_knl::TilePoint fromTile, rs_knl::TilePoint toTile, rs_knl::Vehicle *segVeh, bool isLoop, bool isPreview) {
        COLORREF col = getVehicleColors(segVeh, isLoop, isPreview);
        bool isCar = (segVeh && segVeh->isCar());
        bool isDashed = isLoop || isPreview;

        if (isCar && pMission_ && pMission_->map()) {
            std::vector<rs_knl::TilePoint> roadPts = rs_knl::GenericCar::calculateRoadPath(pMission_->map(), fromTile, toTile);
            if (roadPts.size() > 1) {
                HPEN hRoadPen = CreatePen(isDashed ? PS_DOT : PS_SOLID, isDashed ? 1 : penW, col);
                HPEN hOld = static_cast<HPEN>(SelectObject(hdc, hRoadPen));
                POINT prevSc = entityPointToScreen(roadPts[0]);
                MoveToEx(hdc, prevSc.x, prevSc.y, nullptr);
                for (size_t k = 1; k < roadPts.size(); ++k) {
                    POINT sc = entityPointToScreen(roadPts[k]);
                    LineTo(hdc, sc.x, sc.y);
                    if (k == roadPts.size() / 2 || k == roadPts.size() - 1) {
                        drawArrowHead(prevSc, sc, col);
                    }
                    prevSc = sc;
                }
                SelectObject(hdc, hOld);
                DeleteObject(hRoadPen);
                return;
            }
        }
        // Direct on-foot / fallback segment
        POINT p1 = entityPointToScreen(fromTile);
        POINT p2 = entityPointToScreen(toTile);
        HPEN hDirectPen = CreatePen(isDashed ? PS_DOT : PS_SOLID, isDashed ? 1 : penW, col);
        HPEN hOld = static_cast<HPEN>(SelectObject(hdc, hDirectPen));
        MoveToEx(hdc, p1.x, p1.y, nullptr);
        LineTo(hdc, p2.x, p2.y);
        drawArrowHead(p1, p2, col);
        SelectObject(hdc, hOld);
        DeleteObject(hDirectPen);
    };

    rs_knl::TilePoint startTilePos = startsInVeh ? initialVeh->position() : pPed->position();
    rs_knl::TilePoint prevTilePos = startTilePos;
    for (size_t i = 0; i < waypoints.size(); ++i) {
        rs_knl::Vehicle *segVeh = getPedVehicleAtStep(pMission_, pPed, waypoints[i].stepIdx);
        drawPathSegment(prevTilePos, waypoints[i].mapPos, segVeh, false, false);
        prevTilePos = waypoints[i].mapPos;
    }

    // If script loops, draw dashed line/road back to the first waypoint (where opcode 0x09 resets to)
    if (pPed->isScriptLooping() && !waypoints.empty()) {
        rs_knl::Vehicle *loopVeh = getPedVehicleAtStep(pMission_, pPed, steps.size());
        rs_knl::TilePoint loopDest = (waypoints.size() > 1) ? waypoints.front().mapPos : startTilePos;
        drawPathSegment(waypoints.back().mapPos, loopDest, loopVeh, true, false);
    }

    // If RouteTool is active and hovering over map (not dragging), draw dashed preview line to mouse
    if (isRouteTool && !isDraggingWaypoint_ && hoveredWaypointIdx_ < 0) {
        rs_knl::Vehicle *nextVeh = getPedVehicleAtStep(pMission_, pPed, steps.size());
        rs_knl::TilePoint lastTile = waypoints.empty() ? startTilePos : waypoints.back().mapPos;
        drawPathSegment(lastTile, currentTilePos_, nextVeh, false, true);

        // Draw ghost "+" badge at previewPt
        POINT previewPt = entityPointToScreen(currentTilePos_);
        COLORREF previewCol = getVehicleColors(nextVeh, false, true);
        HBRUSH hGhostBr = CreateSolidBrush(RGB(30, 41, 59));
        HPEN hGhostCirclePen = CreatePen(PS_SOLID, 1, previewCol);
        HBRUSH hOldGBr = static_cast<HBRUSH>(SelectObject(hdc, hGhostBr));
        HPEN hOldGPen = static_cast<HPEN>(SelectObject(hdc, hGhostCirclePen));
        int gr = 10;
        Ellipse(hdc, previewPt.x - gr, previewPt.y - gr, previewPt.x + gr, previewPt.y + gr);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, previewCol);
        RECT ghostRc = { previewPt.x - gr, previewPt.y - gr, previewPt.x + gr, previewPt.y + gr };
        DrawTextW(hdc, L"+", -1, &ghostRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(hdc, hOldGBr);
        SelectObject(hdc, hOldGPen);
        DeleteObject(hGhostBr);
        DeleteObject(hGhostCirclePen);
    }

    // 2. Draw Start/Spawn Anchor Badge (only if waypoints list is empty)
    if (waypoints.empty()) {
        COLORREF startCol = getVehicleColors(initialVeh, false, false);
        int r = 8;
        HBRUSH hStartBr = CreateSolidBrush(startCol);
        HPEN hStartPen = CreatePen(PS_SOLID, 2, RGB(15, 23, 42));
        HBRUSH hOldBr = static_cast<HBRUSH>(SelectObject(hdc, hStartBr));
        HPEN hOldP = static_cast<HPEN>(SelectObject(hdc, hStartPen));
        Ellipse(hdc, startPt.x - r, startPt.y - r, startPt.x + r, startPt.y + r);
        SelectObject(hdc, hOldBr);
        SelectObject(hdc, hOldP);
        DeleteObject(hStartBr);
        DeleteObject(hStartPen);

        if (!spawnDwellTag.empty()) {
            HFONT hTagFont = CreateFontW(
                -11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );
            HFONT hOldF = static_cast<HFONT>(SelectObject(hdc, hTagFont));
            std::wstring tagText = std::format(L"Start ({})", spawnDwellTag);
            RECT tagCalc = { 0, 0, 0, 0 };
            DrawTextW(hdc, tagText.c_str(), -1, &tagCalc, DT_CALCRECT | DT_SINGLELINE);
            int tagW = (tagCalc.right - tagCalc.left) + 8;
            int tagH = (tagCalc.bottom - tagCalc.top) + 2;
            int tagX = startPt.x - tagW / 2;
            int tagY = startPt.y + r + 2;
            RECT tagRc = { tagX, tagY, tagX + tagW, tagY + tagH };

            HBRUSH hTagBg = CreateSolidBrush(RGB(30, 41, 59));
            HPEN hTagPen = CreatePen(PS_SOLID, 1, startCol);
            SelectObject(hdc, hTagBg);
            SelectObject(hdc, hTagPen);
            RoundRect(hdc, tagX, tagY, tagX + tagW, tagY + tagH, 4, 4);
            SetTextColor(hdc, RGB(226, 232, 240));
            DrawTextW(hdc, tagText.c_str(), -1, &tagRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(hdc, hOldF);
            DeleteObject(hTagFont);
            DeleteObject(hTagBg);
            DeleteObject(hTagPen);
        }
    }

    // 2.5. Draw Ambush Trigger Radius Zones
    for (const auto &wp : waypoints) {
        if (wp.type == LevelData::kScenarioTypeTrigger || wp.type == 8) {
            int rTiles = static_cast<int>((wp.param > 0 ? wp.param : 1536) / 256);
            if (rTiles <= 0) rTiles = 6;

            int radX = static_cast<int>(rTiles * 32.0f * zoom_);
            int radY = static_cast<int>(rTiles * 16.0f * zoom_);

            bool isNodeActive = (isRouteTool && (static_cast<int>(wp.stepIdx) == hoveredWaypointIdx_ ||
                                                 (isDraggingWaypoint_ && static_cast<int>(wp.stepIdx) == draggingWaypointIdx_)));
            COLORREF zoneColor = isNodeActive ? RGB(251, 191, 36) : RGB(217, 119, 6);

            HPEN hZonePen = CreatePen(PS_DOT, 1, zoneColor);
            HPEN hOldP = static_cast<HPEN>(SelectObject(hdc, hZonePen));

            POINT pTop = { wp.screenPos.x, wp.screenPos.y - radY };
            POINT pRight = { wp.screenPos.x + radX, wp.screenPos.y };
            POINT pBottom = { wp.screenPos.x, wp.screenPos.y + radY };
            POINT pLeft = { wp.screenPos.x - radX, wp.screenPos.y };

            MoveToEx(hdc, pTop.x, pTop.y, nullptr);
            LineTo(hdc, pRight.x, pRight.y);
            LineTo(hdc, pBottom.x, pBottom.y);
            LineTo(hdc, pLeft.x, pLeft.y);
            LineTo(hdc, pTop.x, pTop.y);

            SelectObject(hdc, hOldP);
            DeleteObject(hZonePen);

            // Small badge at top apex indicating trigger radius
            std::wstring radBadge = std::format(L"Ambush Zone: {} tiles ({}m)", rTiles, rTiles * 3);
            RECT radCalc = { 0, 0, 0, 0 };
            DrawTextW(hdc, radBadge.c_str(), -1, &radCalc, DT_CALCRECT | DT_SINGLELINE);
            int rbW = (radCalc.right - radCalc.left) + 8;
            int rbH = (radCalc.bottom - radCalc.top) + 2;
            int rbX = pTop.x - rbW / 2;
            int rbY = pTop.y - rbH - 2;
            RECT rbRc = { rbX, rbY, rbX + rbW, rbY + rbH };

            HBRUSH hRbBg = CreateSolidBrush(RGB(20, 24, 30));
            HPEN hRbBorder = CreatePen(PS_SOLID, 1, zoneColor);
            HBRUSH hOldRbB = static_cast<HBRUSH>(SelectObject(hdc, hRbBg));
            HPEN hOldRbP = static_cast<HPEN>(SelectObject(hdc, hRbBorder));
            RoundRect(hdc, rbX, rbY, rbX + rbW, rbY + rbH, 4, 4);
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, isNodeActive ? RGB(254, 240, 138) : RGB(251, 191, 36));
            DrawTextW(hdc, radBadge.c_str(), -1, &rbRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SelectObject(hdc, hOldRbB);
            SelectObject(hdc, hOldRbP);
            DeleteObject(hRbBg);
            DeleteObject(hRbBorder);
        }
    }

    // 3. Draw Waypoint Badges
    HFONT hBadgeFont = CreateFontW(
        -11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hBadgeFont));

    for (const auto &wp : waypoints) {
        bool isHovered = (isRouteTool && static_cast<int>(wp.stepIdx) == hoveredWaypointIdx_);
        bool isDragging = (isRouteTool && isDraggingWaypoint_ && static_cast<int>(wp.stepIdx) == draggingWaypointIdx_);
        int r = (isHovered || isDragging) ? 14 : 12;

        rs_knl::Vehicle *stepVeh = getPedVehicleAtStep(pMission_, pPed, wp.stepIdx);
        if (wp.isVehicleBoard && wp.targetEntityId >= 0 && pMission_ && static_cast<size_t>(wp.targetEntityId) < pMission_->numVehicles()) {
            stepVeh = pMission_->vehicle(wp.targetEntityId);
        }
        COLORREF wpColor = getVehicleColors(stepVeh, false, false);

        COLORREF circleBorder = (isHovered || isDragging) ? RGB(255, 255, 255) : wpColor;
        HBRUSH hBg = CreateSolidBrush(RGB(15, 23, 42));
        HPEN hBorder = CreatePen(PS_SOLID, (isHovered || isDragging) ? 3 : 2, circleBorder);
        HBRUSH hOldBr = static_cast<HBRUSH>(SelectObject(hdc, hBg));
        HPEN hOldP = static_cast<HPEN>(SelectObject(hdc, hBorder));

        Ellipse(hdc, wp.screenPos.x - r, wp.screenPos.y - r, wp.screenPos.x + r, wp.screenPos.y + r);

        // Draw distinctive outer gold halo ring for Waypoint 1 (Spawn Point)
        if (wp.displayNum == 1) {
            HPEN hSpawnRing = CreatePen(PS_SOLID, 2, RGB(251, 191, 36)); // Amber gold ring
            HPEN hOldRing = static_cast<HPEN>(SelectObject(hdc, hSpawnRing));
            HBRUSH hOldRingBr = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
            int spawnR = r + 4;
            Ellipse(hdc, wp.screenPos.x - spawnR, wp.screenPos.y - spawnR, wp.screenPos.x + spawnR, wp.screenPos.y + spawnR);
            SelectObject(hdc, hOldRingBr);
            SelectObject(hdc, hOldRing);
            DeleteObject(hSpawnRing);
        }

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(248, 250, 252));
        RECT numRc = { wp.screenPos.x - r, wp.screenPos.y - r, wp.screenPos.x + r, wp.screenPos.y + r };
        std::wstring label = std::to_wstring(wp.displayNum);
        DrawTextW(hdc, label.c_str(), -1, &numRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, hOldBr);
        SelectObject(hdc, hOldP);
        DeleteObject(hBg);
        DeleteObject(hBorder);

        // Sub-tag for vehicle boarding, dwell, trigger, escape, or spawn point
        std::wstring subTag;
        if (wp.displayNum == 1) {
            if (!wp.dwellTag.empty()) {
                subTag = std::format(L"Spawn: {}", wp.dwellTag);
            } else {
                subTag = L"Spawn";
            }
        } else if (wp.isVehicleBoard) {
            if (pMission_ && wp.targetEntityId >= 0 && static_cast<size_t>(wp.targetEntityId) < pMission_->numVehicles()) {
                rs_knl::Vehicle *v = pMission_->vehicle(wp.targetEntityId);
                subTag = std::format(L"Board Vehicle #{:03}", v ? v->id() : wp.targetEntityId);
            } else {
                subTag = std::format(L"Board Vehicle #{}", wp.targetEntityId);
            }
        } else if (!wp.dwellTag.empty()) {
            subTag = wp.dwellTag;
        }

        if (!subTag.empty()) {
            RECT tagCalc = { 0, 0, 0, 0 };
            DrawTextW(hdc, subTag.c_str(), -1, &tagCalc, DT_CALCRECT | DT_SINGLELINE);
            int tagW = (tagCalc.right - tagCalc.left) + 8;
            int tagH = (tagCalc.bottom - tagCalc.top) + 2;
            int tagX = wp.screenPos.x - tagW / 2;
            int tagY = wp.screenPos.y + r + 2;
            RECT tagRc = { tagX, tagY, tagX + tagW, tagY + tagH };

            HBRUSH hTagBg = CreateSolidBrush(RGB(30, 41, 59));
            HPEN hTagPen = CreatePen(PS_SOLID, 1, wpColor);
            SelectObject(hdc, hTagBg);
            SelectObject(hdc, hTagPen);
            RoundRect(hdc, tagX, tagY, tagX + tagW, tagY + tagH, 4, 4);
            SetTextColor(hdc, RGB(226, 232, 240));
            DrawTextW(hdc, subTag.c_str(), -1, &tagRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            DeleteObject(hTagBg);
            DeleteObject(hTagPen);
        }
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hBadgeFont);
}

void WinMapViewport::toggleSelection() {
    if (tool_ != ViewportTool::SelectTile && tool_ != ViewportTool::SelectEntity) return;

    if (tool_ == ViewportTool::SelectEntity) {
        // In entity select mode, do not toggle to tiles
        return;
    }

    if (targetSelected_) {
        // Switch from entity to tile underneath
        rs_knl::TilePoint pos = targetSelected_->position();
        lastOverlappingEntity_ = targetSelected_;
        targetSelected_ = nullptr;
        hasSelectedTile_ = true;
        selectedTilePos_ = pos;
        if (entitySelectedCallback_) {
            entitySelectedCallback_(nullptr);
        }
        InvalidateRect(hWnd_, nullptr, FALSE);
    } else if (hasSelectedTile_) {
        // Switch from tile to entity on that tile
        rs_knl::ShootableMapObject *candidate = lastOverlappingEntity_;
        if (!candidate && pMission_) {
            for (size_t i = 0; i < pMission_->numPeds(); ++i) {
                if (pMission_->ped(i)->tileX() == selectedTilePos_.tx &&
                    pMission_->ped(i)->tileY() == selectedTilePos_.ty) {
                    candidate = pMission_->ped(i);
                    break;
                }
            }
            if (!candidate) {
                for (size_t i = 0; i < pMission_->numVehicles(); ++i) {
                    if (pMission_->vehicle(i)->tileX() == selectedTilePos_.tx &&
                        pMission_->vehicle(i)->tileY() == selectedTilePos_.ty) {
                        candidate = pMission_->vehicle(i);
                        break;
                    }
                }
            }
            if (!candidate) {
                for (size_t i = 0; i < pMission_->numStatics(); ++i) {
                    if (pMission_->statics(i)->tileX() == selectedTilePos_.tx &&
                        pMission_->statics(i)->tileY() == selectedTilePos_.ty) {
                        candidate = pMission_->statics(i);
                        break;
                    }
                }
            }
        }
        if (candidate) {
            targetSelected_ = candidate;
            hasSelectedTile_ = false;
            lastOverlappingEntity_ = nullptr;
            if (entitySelectedCallback_) {
                entitySelectedCallback_(targetSelected_);
            }
            InvalidateRect(hWnd_, nullptr, FALSE);
        }
    }
}

void WinMapViewport::deleteSelection() {
    if (!pMission_ || (tool_ != ViewportTool::SelectTile && tool_ != ViewportTool::SelectEntity)) return;

    if (targetSelected_) {
        rs_knl::ShootableMapObject *pObjToDelete = targetSelected_;
        clearEntityReferences(pObjToDelete);
        if (entitySelectedCallback_) {
            entitySelectedCallback_(nullptr);
        }

        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pObjToDelete)) {
            if (pCommandStack_) {
                pCommandStack_->executeCommand(std::make_unique<DeletePedCommand>(pMission_, pPed));
                if (commandExecutedCallback_) commandExecutedCallback_();
            } else {
                pMission_->removePed(pPed);
            }
            InvalidateRect(hWnd_, nullptr, FALSE);
        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pObjToDelete)) {
            if (pCommandStack_) {
                pCommandStack_->executeCommand(std::make_unique<DeleteVehicleCommand>(pMission_, pVeh));
                if (commandExecutedCallback_) commandExecutedCallback_();
            } else {
                pMission_->removeVehicle(pVeh);
            }
            InvalidateRect(hWnd_, nullptr, FALSE);
        } else if (auto *pStat = dynamic_cast<rs_knl::Static*>(pObjToDelete)) {
            if (pCommandStack_) {
                pCommandStack_->executeCommand(std::make_unique<DeleteStaticCommand>(pMission_, pStat));
                if (commandExecutedCallback_) commandExecutedCallback_();
            } else {
                pMission_->removeStatic(pStat);
            }
            InvalidateRect(hWnd_, nullptr, FALSE);
        } else if (auto *pWpn = dynamic_cast<rs_knl::WeaponInstance*>(pObjToDelete)) {
            if (pCommandStack_) {
                pCommandStack_->executeCommand(std::make_unique<DeleteWeaponCommand>(pMission_, pWpn));
                if (commandExecutedCallback_) commandExecutedCallback_();
            } else {
                pMission_->removeGroundWeapon(pWpn);
            }
            InvalidateRect(hWnd_, nullptr, FALSE);
        }
    } else if (hasSelectedTile_) {
        if (pMission_->map()) {
            int oldTile = pMission_->map()->getTileIdAt(selectedTilePos_.tx, selectedTilePos_.ty, selectedTilePos_.tz);
            if (oldTile > 0) {
                if (pCommandStack_) {
                    pCommandStack_->executeCommand(std::make_unique<PaintTileCommand>(
                        pMission_->map(), selectedTilePos_.tx, selectedTilePos_.ty, selectedTilePos_.tz,
                        static_cast<uint8_t>(oldTile), static_cast<uint8_t>(0)));
                    if (commandExecutedCallback_) commandExecutedCallback_();
                } else {
                    pMission_->map()->setTileAt(selectedTilePos_.tx, selectedTilePos_.ty, selectedTilePos_.tz, 0);
                }
                InvalidateRect(hWnd_, nullptr, FALSE);
            }
        }
    }
}

void WinMapViewport::moveSelectionZ(int deltaZ) {
    if (!pMission_ || (!targetSelected_ && !hasSelectedTile_)) return;
    int maxZ = pMission_->map() ? (pMission_->map()->maxTz() - 1) : 12;

    if (targetSelected_) {
        rs_knl::TilePoint cur = targetSelected_->position();
        int newZ = cur.tz + deltaZ;
        if (newZ >= 0 && newZ <= maxZ) {
            rs_knl::TilePoint newPos = cur;
            newPos.tz = newZ;
            if (pCommandStack_) {
                pCommandStack_->executeCommand(std::make_unique<MoveEntityCommand>(
                    targetSelected_, cur, newPos, pMission_));
                if (commandExecutedCallback_) commandExecutedCallback_();
            } else {
                targetSelected_->setPosition(newPos.tx, newPos.ty, newPos.tz, newPos.ox, newPos.oy, newPos.oz);
                if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(targetSelected_)) {
                    if (pPed->isOurAgent() && pPed->id() < 4) {
                        pMission_->setSquadSpawnPoint(pPed->id(), newPos, static_cast<uint8_t>(pPed->direction()));
                    }
                }
            }
            setActiveZ(newZ);
            if (entitySelectedCallback_) entitySelectedCallback_(targetSelected_);
            InvalidateRect(hWnd_, nullptr, FALSE);
        }
    } else if (hasSelectedTile_) {
        if (pMission_->map()) {
            int curZ = selectedTilePos_.tz;
            int newZ = curZ + deltaZ;
            if (newZ >= 0 && newZ <= maxZ) {
                if (pCommandStack_) {
                    pCommandStack_->executeCommand(std::make_unique<MoveTileCommand>(
                        pMission_->map(), selectedTilePos_.tx, selectedTilePos_.ty, curZ,
                        selectedTilePos_.tx, selectedTilePos_.ty, newZ, false));
                    if (commandExecutedCallback_) commandExecutedCallback_();
                } else {
                    int tid = pMission_->map()->getTileIdAt(selectedTilePos_.tx, selectedTilePos_.ty, curZ);
                    pMission_->map()->setTileAt(selectedTilePos_.tx, selectedTilePos_.ty, curZ, 0);
                    pMission_->map()->setTileAt(selectedTilePos_.tx, selectedTilePos_.ty, newZ, tid > 0 ? static_cast<uint8_t>(tid) : 0);
                }
                selectedTilePos_.tz = newZ;
                setActiveZ(newZ);
                InvalidateRect(hWnd_, nullptr, FALSE);
            }
        }
    }
}

void WinMapViewport::stepSelectionZ(int deltaZ) {
    if (!pMission_ || !pMission_->map()) return;
    auto *pMap = pMission_->map();
    int maxZ = pMap->maxTz() - 1;

    if (hasSelectedTile_) {
        int curZ = selectedTilePos_.tz;
        int nextZ = -1;
        if (deltaZ > 0) {
            for (int z = curZ + 1; z <= maxZ; ++z) {
                if (pMap->getTileIdAt(selectedTilePos_.tx, selectedTilePos_.ty, z) > 0) {
                    nextZ = z;
                    break;
                }
            }
        } else if (deltaZ < 0) {
            for (int z = curZ - 1; z >= 0; --z) {
                if (pMap->getTileIdAt(selectedTilePos_.tx, selectedTilePos_.ty, z) > 0) {
                    nextZ = z;
                    break;
                }
            }
        }

        if (nextZ != -1) {
            selectedTilePos_.tz = nextZ;
            setActiveZ(nextZ);
            currentTilePos_ = selectedTilePos_;
            int tid = pMap->getTileIdAt(selectedTilePos_.tx, selectedTilePos_.ty, nextZ);
            if (tileSelectedCallback_) {
                tileSelectedCallback_(static_cast<uint8_t>(tid));
            }
            InvalidateRect(hWnd_, nullptr, FALSE);
            return;
        } else {
            hasSelectedTile_ = false;
            int newActiveZ = std::clamp(curZ + deltaZ, 0, maxZ);
            setActiveZ(newActiveZ);
            InvalidateRect(hWnd_, nullptr, FALSE);
            return;
        }
    }

    if (!hasSelectedTile_) {
        int newActiveZ = activeZ_ + deltaZ;
        if (newActiveZ < 0) newActiveZ = 0;
        if (newActiveZ > maxZ) newActiveZ = maxZ;
        setActiveZ(newActiveZ);
    }
}

void WinMapViewport::onKeyDown(WPARAM wParam, LPARAM lParam) {
    (void)lParam;
    if (objectivePickMode_ != ObjectivePickMode::None) {
        if (wParam == VK_ESCAPE) {
            cancelObjectivePicking();
            return;
        } else if (wParam == VK_LEFT) {
            cameraX_ = std::max(0, cameraX_ - 64);
            InvalidateRect(hWnd_, nullptr, FALSE);
            return;
        } else if (wParam == VK_RIGHT) {
            cameraX_ += 64;
            InvalidateRect(hWnd_, nullptr, FALSE);
            return;
        } else if (wParam == VK_UP) {
            cameraY_ = std::max(0, cameraY_ - 64);
            InvalidateRect(hWnd_, nullptr, FALSE);
            return;
        } else if (wParam == VK_DOWN) {
            cameraY_ += 64;
            InvalidateRect(hWnd_, nullptr, FALSE);
            return;
        }
        return;
    }

    if (wParam == VK_TAB) {
        toggleSelection();
    } else if (wParam == VK_DELETE) {
        deleteSelection();
    } else if (wParam == VK_PRIOR) {
        if (GetKeyState(VK_SHIFT) < 0) {
            moveSelectionZ(1);
        } else {
            stepSelectionZ(1);
        }
    } else if (wParam == VK_NEXT) {
        if (GetKeyState(VK_SHIFT) < 0) {
            moveSelectionZ(-1);
        } else {
            stepSelectionZ(-1);
        }
    } else if (wParam == VK_OEM_4) { // '[' or '{'
        if (GetKeyState(VK_SHIFT) < 0) {
            moveSelectionZ(-1);
        } else {
            stepSelectionZ(-1);
        }
    } else if (wParam == VK_OEM_6) { // ']' or '}'
        if (GetKeyState(VK_SHIFT) < 0) {
            moveSelectionZ(1);
        } else {
            stepSelectionZ(1);
        }
    } else if (wParam == VK_ESCAPE) {
        if (tool_ == ViewportTool::RouteTool) {
            setTool(ViewportTool::SelectEntity);
        } else {
            clearSelection();
        }
    } else if (wParam == 'R') {
        if (getActiveRoutePed()) {
            setTool(tool_ == ViewportTool::RouteTool ? ViewportTool::SelectEntity : ViewportTool::RouteTool);
        }
    }
}

void WinMapViewport::onMouseMove(int x, int y, UINT flags) {
    if (!pMission_ || !pMission_->map()) return;

    if (objectivePickMode_ != ObjectivePickMode::None) {
        POINT mousePt = { x, y };
        bool wasHoveringCancel = isHoveringPickCancel_;
        isHoveringPickCancel_ = PtInRect(&pickCancelBtnRect_, mousePt);
        if (isHoveringPickCancel_ != wasHoveringCancel) {
            InvalidateRect(hWnd_, nullptr, FALSE);
        }

        if (isPanning_) {
            cameraX_ = panStartCamX_ - static_cast<int>((x - panStartMouse_.x) / zoom_);
            cameraY_ = panStartCamY_ - static_cast<int>((y - panStartMouse_.y) / zoom_);
            if (cameraX_ < 0) cameraX_ = 0;
            if (cameraY_ < 0) cameraY_ = 0;
            InvalidateRect(hWnd_, nullptr, FALSE);
            return;
        }

        currentTilePos_ = pickTileAtScreen(x, y);
        targetHovered_ = hitTestEntityAtScreen(x, y);

        if (statusCallback_) {
            std::string coords = std::format("TX:{:02} TY:{:02} TZ:{:02} (OX:{:03} OY:{:03})",
                currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz, currentTilePos_.ox, currentTilePos_.oy);
            std::string tileInfo = "";
            std::string hoverInfo;
            if (objectivePickMode_ == ObjectivePickMode::Ped) {
                if (auto *p = dynamic_cast<rs_knl::PedInstance*>(targetHovered_)) {
                    if (!p->isOurAgent()) {
                        hoverInfo = std::format("Click to target Ped #{:03} at ({},{},{})", p->id(), p->tileX(), p->tileY(), p->tileZ());
                    } else {
                        hoverInfo = "Target must not be a squad agent.";
                    }
                } else {
                    hoverInfo = "Click a pedestrian on map to target for objective (or Esc to cancel).";
                }
            } else if (objectivePickMode_ == ObjectivePickMode::Vehicle) {
                if (auto *v = dynamic_cast<rs_knl::Vehicle*>(targetHovered_)) {
                    hoverInfo = std::format("Click to target Vehicle #{:02} at ({},{},{})", v->id(), v->tileX(), v->tileY(), v->tileZ());
                } else {
                    hoverInfo = "Click a vehicle on map to target for objective (or Esc to cancel).";
                }
            } else if (objectivePickMode_ == ObjectivePickMode::Coordinate) {
                hoverInfo = std::format("Click to set Evacuate destination at (TX:{}, TY:{}, TZ:{})", currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz);
            }
            statusCallback_(coords, tileInfo, hoverInfo);
        }

        InvalidateRect(hWnd_, nullptr, FALSE);
        return;
    }

    // 1. Panning with middle mouse or right mouse
    if (isPanning_) {
        cameraX_ = panStartCamX_ - static_cast<int>((x - panStartMouse_.x) / zoom_);
        cameraY_ = panStartCamY_ - static_cast<int>((y - panStartMouse_.y) / zoom_);
        if (cameraX_ < 0) cameraX_ = 0;
        if (cameraY_ < 0) cameraY_ = 0;
        InvalidateRect(hWnd_, nullptr, FALSE);
        return;
    }

    if (isPlacementTool(tool_) && !isPainting_ && !isBoxFilling_) {
        if (autoLayerSwitchingEnabled_ && !manualLayerOverride_) {
            rs_knl::TilePoint hitTile;
            if (findTileAtScreen(x, y, &hitTile)) {
                int maxZ = (pMission_ && pMission_->map()) ? pMission_->map()->maxTz() : 13;
                activeZ_ = std::min(hitTile.tz + 1, maxZ - 1);
                rs_knl::TilePoint subPt = screenToMapPointAtZ(x, y, hitTile.tz);
                currentTilePos_ = rs_knl::TilePoint(hitTile.tx, hitTile.ty, activeZ_, subPt.ox, subPt.oy);
            } else {
                activeZ_ = explicitSelectedZ_;
                currentTilePos_ = screenToMapPointAtZ(x, y, activeZ_);
            }
        } else {
            activeZ_ = explicitSelectedZ_;
            currentTilePos_ = screenToMapPoint(x, y);
        }
    } else {
        bool isPickTool = (tool_ == ViewportTool::SelectTile ||
                           tool_ == ViewportTool::SelectEntity);
        currentTilePos_ = isPickTool ? pickTileAtScreen(x, y) : screenToMapPoint(x, y);
    }

    // 2. Continuous drag-painting
    if (isPainting_ && (flags & MK_LBUTTON) && tool_ == ViewportTool::Pencil) {
        if (pMission_ && pMission_->map()) {
            rs_eng::Tile *t = pMission_->map()->getTileAt(currentTilePos_.tx, currentTilePos_.ty, activeZ_);
            uint8_t oldTile = t ? static_cast<uint8_t>(t->id()) : 0;
            if (oldTile != activeTileId_) {
                if (pCommandStack_) {
                    pCommandStack_->executeCommand(std::make_unique<PaintTileCommand>(
                        pMission_->map(), currentTilePos_.tx, currentTilePos_.ty, activeZ_, oldTile, activeTileId_));
                    if (commandExecutedCallback_) commandExecutedCallback_();
                } else {
                    pMission_->map()->setTileAt(currentTilePos_.tx, currentTilePos_.ty, activeZ_, activeTileId_);
                }
            }
        }
    }

    // 3. Dragging selected entity (Select tool) — preserves entity's original Z and drag offset
    if (isDraggingEntity_ && (flags & MK_LBUTTON) && targetSelected_) {
        int mouseDistSq = (x - dragStartMouse_.x) * (x - dragStartMouse_.x) +
                          (y - dragStartMouse_.y) * (y - dragStartMouse_.y);
        // Only start moving if dragged past 5-pixel threshold
        if (hasDraggedEntity_ || mouseDistSq >= 25) {
            hasDraggedEntity_ = true;
            rs_knl::TilePoint curMouseTile = screenToMapPointAtZ(x, y, dragEntityStartTile_.tz);
            int deltaTx = curMouseTile.tx - dragStartMouseTile_.tx;
            int deltaTy = curMouseTile.ty - dragStartMouseTile_.ty;
            int deltaOx = curMouseTile.ox - dragStartMouseTile_.ox;
            int deltaOy = curMouseTile.oy - dragStartMouseTile_.oy;

            int newTx = dragEntityStartTile_.tx + deltaTx;
            int newTy = dragEntityStartTile_.ty + deltaTy;
            int newOx = dragEntityStartTile_.ox + deltaOx;
            int newOy = dragEntityStartTile_.oy + deltaOy;
            int entityZ = dragEntityStartTile_.tz; // Preserve original entity Z

            // Normalize sub-tile offset (ox, oy in [0, 255])
            while (newOx >= 256) { newOx -= 256; newTx += 1; }
            while (newOx < 0) { newOx += 256; newTx -= 1; }
            while (newOy >= 256) { newOy -= 256; newTy += 1; }
            while (newOy < 0) { newOy += 256; newTy -= 1; }

            if (pMission_ && pMission_->map() &&
                newTx >= 0 && newTx < pMission_->map()->maxTx() &&
                newTy >= 0 && newTy < pMission_->map()->maxTy()) {
                targetSelected_->setPosition(newTx, newTy, entityZ, newOx, newOy, 0);
                if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(targetSelected_)) {
                    if (auto *pDriver = pVeh->getDriver()) {
                        pDriver->setPosition(targetSelected_->position());
                    }
                }
                if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(targetSelected_)) {
                    if (pPed->isOurAgent() && pPed->id() < 4) {
                        pMission_->setSquadSpawnPoint(pPed->id(), rs_knl::TilePoint(newTx, newTy, entityZ, newOx, newOy, 0), static_cast<uint8_t>(pPed->direction()));
                    }
                }
            }
        }
    }

    // 4. Dragging selected tile (Select tool)
    if (isDraggingTile_ && (flags & MK_LBUTTON) && hasSelectedTile_) {
        int mouseDistSq = (x - dragTileStartMouse_.x) * (x - dragTileStartMouse_.x) +
                          (y - dragTileStartMouse_.y) * (y - dragTileStartMouse_.y);
        if (hasDraggedTile_ || mouseDistSq >= 25) {
            hasDraggedTile_ = true;
            rs_knl::TilePoint curMouseTile = screenToMapPointAtZ(x, y, dragTileStartPos_.tz);
            int deltaTx = curMouseTile.tx - dragTileStartMouseTile_.tx;
            int deltaTy = curMouseTile.ty - dragTileStartMouseTile_.ty;
            int newTx = dragTileStartPos_.tx + deltaTx;
            int newTy = dragTileStartPos_.ty + deltaTy;
            if (pMission_ && pMission_->map() &&
                newTx >= 0 && newTx < pMission_->map()->maxTx() &&
                newTy >= 0 && newTy < pMission_->map()->maxTy()) {
                dragTileCurrentPos_ = rs_knl::TilePoint(newTx, newTy, dragTileStartPos_.tz);
            }
        }
    }

    if (isBoxFilling_) {
        boxFillCurrent_ = rs_knl::TilePoint(currentTilePos_.tx, currentTilePos_.ty, activeZ_);
    }

    // 5. Route Tool: dragging or hovering over waypoints
    if (tool_ == ViewportTool::RouteTool) {
        rs_knl::PedInstance *pPed = getActiveRoutePed();
        if (pPed) {
            if (isDraggingWaypoint_ && (flags & MK_LBUTTON)) {
                rs_knl::Vehicle *dragVeh = getPedVehicleAtStep(pMission_, pPed, draggingWaypointIdx_);
                bool isVeh = (dragVeh != nullptr && (dragVeh->isCar() || dragVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead || dragVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody));
                int refZ = getReferenceZForRouteStep(pPed, draggingWaypointIdx_, dragVeh);
                dragWaypointCurrentPos_ = resolveWalkableTilePoint(x, y, refZ, isVeh);
            } else {
                rs_knl::Vehicle *nextVeh = getPedVehicleAtStep(pMission_, pPed, pPed->scriptSteps().size());
                bool isVeh = (nextVeh != nullptr && (nextVeh->isCar() || nextVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead || nextVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody));
                int refZ = getReferenceZForRouteStep(pPed, pPed->scriptSteps().size(), nextVeh);
                currentTilePos_ = resolveWalkableTilePoint(x, y, refZ, isVeh);
                int hit = hitTestWaypointAtScreen(x, y, pPed->scriptSteps());
                if (hit != hoveredWaypointIdx_) {
                    hoveredWaypointIdx_ = hit;
                }
            }
        }
    }

    // Status callback
    if (statusCallback_) {
        std::string coords;
        if (tool_ == ViewportTool::Pencil || tool_ == ViewportTool::BoxFill) {
            coords = std::format("TX:{:02} TY:{:02} TZ:{:02} (Layer:{:02})",
                currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz, activeZ_);
        } else {
            coords = std::format("TX:{:02} TY:{:02} TZ:{:02}",
                currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz);
        }
        int roofZ = getHighestRoofZ(currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz);
        if (roofZ > currentTilePos_.tz) {
            coords += std::format(" [Roof Layer:{}]", roofZ);
        }
        std::string tileInfo = "Tile: #000 [Empty]";
        if (pMission_ && pMission_->map()) {
            rs_eng::Tile *t = pMission_->map()->getTileAt(currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz);
            if (t && t->id() != 0 && t->notTransparent()) {
                bool walkable = pMission_->isTileWalkable(currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz);
                tileInfo = std::format("Tile: #{:03} [{}] [{}]",
                    t->id(),
                    TileRasterizer::instance().getTileTypeName(static_cast<uint8_t>(t->type())),
                    walkable ? "Walkable" : "Not Walkable");
            }
        }
        std::string hoverInfo = "Ready";
        if (tool_ == ViewportTool::RouteTool) {
            if (isDraggingWaypoint_) {
                hoverInfo = std::format("Dragging Waypoint #{} to ({}, {}, {}) - Release to place",
                    draggingWaypointIdx_ + 1, dragWaypointCurrentPos_.tx, dragWaypointCurrentPos_.ty, dragWaypointCurrentPos_.tz);
            } else if (hoveredWaypointIdx_ >= 0) {
                hoverInfo = std::format("Waypoint #{} - Drag to move. Right-click to configure / delete.", hoveredWaypointIdx_ + 1);
            } else {
                hoverInfo = "Route Tool: Left-click to add waypoint. Right-drag to pan map. Press Esc or Done Route to finish.";
            }
        } else if (tool_ == ViewportTool::PlaceActor && actorTypeToPlace_ == rs_knl::PedInstance::kPedTypeAgent && actorIsOurAgentToPlace_) {
            hoverInfo = std::format("Click to place Player's Agent ({} / 4 placed)", pMission_ ? pMission_->countPlayerAgents() : 0);
        } else if (tool_ == ViewportTool::SelectEntity) {
            if (targetSelected_) {
                if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(targetSelected_)) {
                    if (pPed->isOurAgent()) {
                        hoverInfo = std::format("Selected Player's Agent #{} - Drag to move. Shift+PgUp/Dn: Z.", pPed->id() + 1);
                    } else {
                        hoverInfo = "Selected Actor - Drag to move. Shift+PgUp/Dn: Z.";
                    }
                } else if (dynamic_cast<rs_knl::Vehicle*>(targetSelected_)) {
                    hoverInfo = "Selected Vehicle - Drag to move. Shift+PgUp/Dn: Z.";
                } else if (dynamic_cast<rs_knl::WeaponInstance*>(targetSelected_)) {
                    hoverInfo = "Selected Weapon Drop - Drag to move. Del: Delete.";
                } else {
                    hoverInfo = "Selected Entity - Drag to move. Shift+PgUp/Dn: Z.";
                }
            } else {
                hoverInfo = "Select Entity: Click any entity to select. Click empty ground to deselect.";
            }
        } else if (tool_ == ViewportTool::PlaceWeapon) {
            hoverInfo = "Click to place Weapon Drop on map.";
        } else if (tool_ == ViewportTool::SelectTile) {
            if (hasSelectedTile_) {
                int tId = pMission_ && pMission_->map() ? pMission_->map()->getTileIdAt(selectedTilePos_.tx, selectedTilePos_.ty, selectedTilePos_.tz) : 0;
                hoverInfo = std::format("Selected Tile #{} at ({},{},{}) - Drag to move (Ctrl+Drag: copy). Shift+PgUp/Dn: Z.",
                    tId >= 0 ? tId : 0, selectedTilePos_.tx, selectedTilePos_.ty, selectedTilePos_.tz);
            } else {
                hoverInfo = "Select Tile: Click tile to select and inspect.";
            }
        }
        statusCallback_(coords, tileInfo, hoverInfo);
    }

    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::onMouseDown(int x, int y, int button) {
    if (!pMission_ || !pMission_->map()) return;
    SetCapture(hWnd_);

    if (objectivePickMode_ != ObjectivePickMode::None) {
        if (button == 2 || button == 3) {
            isPanning_ = true;
            panStartMouse_ = { x, y };
            panStartCamX_ = cameraX_;
            panStartCamY_ = cameraY_;
            return;
        }

        if (button == 1) {
            POINT mousePt = { x, y };
            if (PtInRect(&pickCancelBtnRect_, mousePt)) {
                cancelObjectivePicking();
                return;
            }

            if (objectivePickMode_ == ObjectivePickMode::Ped) {
                rs_knl::ShootableMapObject *obj = hitTestEntityAtScreen(x, y);
                if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(obj)) {
                    if (!pPed->isOurAgent()) {
                        ObjectivePickResult res;
                        res.accepted = true;
                        res.pedId = pPed->id();
                        res.tilePos = pPed->position();
                        finishObjectivePicking(res);
                        return;
                    }
                }
            } else if (objectivePickMode_ == ObjectivePickMode::Vehicle) {
                rs_knl::ShootableMapObject *obj = hitTestEntityAtScreen(x, y);
                if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(obj)) {
                    uint16_t vIdx = 0;
                    for (size_t vi = 0; vi < pMission_->numVehicles(); ++vi) {
                        if (pMission_->vehicle(vi) == pVeh) {
                            vIdx = static_cast<uint16_t>(vi);
                            break;
                        }
                    }
                    ObjectivePickResult res;
                    res.accepted = true;
                    res.vehicleIdx = vIdx;
                    res.tilePos = pVeh->position();
                    finishObjectivePicking(res);
                    return;
                }
            } else if (objectivePickMode_ == ObjectivePickMode::Coordinate) {
                rs_knl::TilePoint tp = pickTileAtScreen(x, y);
                ObjectivePickResult res;
                res.accepted = true;
                res.tilePos = tp;
                finishObjectivePicking(res);
                return;
            }
        }
        return;
    }

    // In RouteTool: right-clicking directly ON an existing waypoint opens its action context menu.
    // Right-clicking on map space starts panning instead of cancelling route editing!
    if (button == 3 && tool_ == ViewportTool::RouteTool) {
        rs_knl::PedInstance *pPed = getActiveRoutePed();
        if (pPed) {
            int hitIdx = hitTestWaypointAtScreen(x, y, pPed->scriptSteps());
            if (hitIdx >= 0) {
                POINT screenPt = { x, y };
                ClientToScreen(hWnd_, &screenPt);
                HMENU hMenu = CreatePopupMenu();
                std::wstring stepHdr = (hitIdx == 0) ? L"-- WAYPOINT #1 (SPAWN) ACTION --" : std::format(L"-- WAYPOINT #{} ACTION --", hitIdx + 1);
                AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 999, stepHdr.c_str());
                AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);

                UINT uGotoFlags = (pPed->scriptSteps()[hitIdx].type == LevelData::kScenarioTypeGoto) ? MF_CHECKED : MF_UNCHECKED;
                rs_knl::Vehicle *stepVeh = getPedVehicleAtStep(pMission_, pPed, hitIdx);
                std::wstring gotoLabel = L"Goto (Walk)";
                if (stepVeh) {
                    if (stepVeh->isCar()) gotoLabel = std::format(L"Goto (Drive Car #{:03})", stepVeh->id());
                    else if (stepVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead || stepVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody)
                        gotoLabel = std::format(L"Goto (Drive Train #{:03})", stepVeh->id());
                    else gotoLabel = std::format(L"Goto (Drive Vehicle #{:03})", stepVeh->id());
                }
                AppendMenuW(hMenu, MF_STRING | uGotoFlags, 1005, gotoLabel.c_str());

                UINT uWait5Flags = (pPed->scriptSteps()[hitIdx].type == LevelData::kScenarioTypeWait && pPed->scriptSteps()[hitIdx].param == 5000) ? MF_CHECKED : MF_UNCHECKED;
                AppendMenuW(hMenu, MF_STRING | uWait5Flags, 1002, L"Wait 5s (Dwell)");

                UINT uWait10Flags = (pPed->scriptSteps()[hitIdx].type == LevelData::kScenarioTypeWait && pPed->scriptSteps()[hitIdx].param == 10000) ? MF_CHECKED : MF_UNCHECKED;
                AppendMenuW(hMenu, MF_STRING | uWait10Flags, 1003, L"Wait 10s (Dwell)");

                int dwellTime = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeTrainHead).dwell_time;
                std::wstring dwellLabel = std::format(L"Train Station Dwell ({}s)", dwellTime / 1000);
                UINT uTrainWaitFlags = (pPed->scriptSteps()[hitIdx].type == LevelData::kScenarioTypeTrainWait) ? MF_CHECKED : MF_UNCHECKED;
                AppendMenuW(hMenu, MF_STRING | uTrainWaitFlags, 1006, dwellLabel.c_str());

                UINT uTrigFlags = (pPed->scriptSteps()[hitIdx].type == LevelData::kScenarioTypeTrigger) ? MF_CHECKED : MF_UNCHECKED;
                AppendMenuW(hMenu, MF_STRING | uTrigFlags, 1004, L"Proximity Ambush Trigger (6 tiles)");

                HMENU hRadMenu = CreatePopupMenu();
                static const int kRadii[] = { 2, 4, 6, 8, 10, 12, 16 };
                int curRadTiles = (pPed->scriptSteps()[hitIdx].param > 0) ? static_cast<int>(pPed->scriptSteps()[hitIdx].param / 256) : 6;
                for (int r : kRadii) {
                    std::wstring rLabel = std::format(L"{} Tiles ({} units{})", r, r * 256, (r == 6) ? L" - Default" : L"");
                    UINT uRadFlags = (pPed->scriptSteps()[hitIdx].type == LevelData::kScenarioTypeTrigger && curRadTiles == r) ? MF_CHECKED : MF_UNCHECKED;
                    AppendMenuW(hRadMenu, MF_STRING | uRadFlags, 1300 + r, rLabel.c_str());
                }
                AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hRadMenu), L"Ambush Trigger Radius");

                UINT uLeaveFlags = (pPed->scriptSteps()[hitIdx].type == LevelData::kScenarioTypeLeaveVehicle) ? MF_CHECKED : MF_UNCHECKED;
                AppendMenuW(hMenu, MF_STRING | uLeaveFlags, 1007, L"Unboard / Exit Vehicle");

                UINT uEscFlags = (pPed->scriptSteps()[hitIdx].type == LevelData::kScenarioTypeEscape) ? MF_CHECKED : MF_UNCHECKED;
                AppendMenuW(hMenu, MF_STRING | uEscFlags, 1008, L"Target Escape / Evacuate");

                // Vehicles submenu for Board Vehicle
                if (pMission_ && pMission_->numVehicles() > 0) {
                    HMENU hVehMenu = CreatePopupMenu();
                    for (size_t vi = 0; vi < pMission_->numVehicles() && vi < 30; ++vi) {
                        rs_knl::Vehicle *v = pMission_->vehicle(vi);
                        if (!v) continue;
                        const wchar_t *typeName = L"Vehicle";
                        uint8_t vt = v->getType();
                        if (vt == rs_knl::Vehicle::kVehicleTypeRegularCar) typeName = L"Regular Car";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeSmallArmored) typeName = L"Armored Car";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeLargeArmored) typeName = L"Large Armored";
                        else if (vt == rs_knl::Vehicle::kVehicleTypePolice) typeName = L"Police Car";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeFireFighter) typeName = L"Firetruck";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeMedics) typeName = L"Ambulance";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeTrainHead) typeName = L"Train Engine";
                        else if (vt == rs_knl::Vehicle::kVehicleTypeTrainBody) typeName = L"Train Carriage";
                        std::wstring vName = std::format(L"Vehicle #{:03} [{}]", v->id(), typeName);
                        UINT uVehFlags = (pPed->scriptSteps()[hitIdx].type == LevelData::kScenarioTypeUseVehicle &&
                                          pPed->scriptSteps()[hitIdx].targetEntityId == static_cast<int>(vi)) ? MF_CHECKED : MF_UNCHECKED;
                        AppendMenuW(hVehMenu, MF_STRING | uVehFlags, 1100 + static_cast<UINT>(vi), vName.c_str());
                    }
                    AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hVehMenu), L"Board Vehicle");
                }

                AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hMenu, (hitIdx > 0 ? MF_STRING : (MF_STRING | MF_GRAYED)), 1009, L"Move Up in Sequence");
                AppendMenuW(hMenu, ((hitIdx > 0 && hitIdx + 1 < static_cast<int>(pPed->scriptSteps().size())) ? MF_STRING : (MF_STRING | MF_GRAYED)), 1010, L"Move Down in Sequence");
                AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(hMenu, (hitIdx > 0 ? MF_STRING : (MF_STRING | MF_GRAYED)), 1001, L"Delete Waypoint");
                AppendMenuW(hMenu, MF_STRING, 1011, L"Clear Entire Route");

                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, screenPt.x, screenPt.y, 0, hWnd_, nullptr);
                DestroyMenu(hMenu);

                if (cmd > 0) {
                    auto oldSteps = pPed->scriptSteps();
                    auto newSteps = oldSteps;
                    bool isLoop = pPed->isScriptLooping();
                    if (!newSteps.empty() && newSteps.back().type == LevelData::kScenarioTypeReset) {
                        newSteps.pop_back();
                    }
                    if (static_cast<size_t>(hitIdx) < newSteps.size()) {
                        if (cmd == 1001) {
                            if (hitIdx > 0) {
                                newSteps.erase(newSteps.begin() + hitIdx);
                            }
                        } else if (cmd == 1002) {
                            newSteps[hitIdx].type = LevelData::kScenarioTypeWait;
                            newSteps[hitIdx].param = 5000;
                        } else if (cmd == 1003) {
                            newSteps[hitIdx].type = LevelData::kScenarioTypeWait;
                            newSteps[hitIdx].param = 10000;
                        } else if (cmd == 1006) {
                            newSteps[hitIdx].type = LevelData::kScenarioTypeTrainWait;
                            newSteps[hitIdx].param = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeTrainHead).dwell_time;
                            newSteps[hitIdx].targetPos = rs_knl::TilePoint(0, 0, 0);
                        } else if (cmd == 1004) {
                            newSteps[hitIdx].type = LevelData::kScenarioTypeTrigger;
                            if (newSteps[hitIdx].param == 0) newSteps[hitIdx].param = 6 * 256;
                        } else if (cmd >= 1301 && cmd <= 1320) {
                            int rTiles = cmd - 1300;
                            newSteps[hitIdx].type = LevelData::kScenarioTypeTrigger;
                            newSteps[hitIdx].param = rTiles * 256;
                        } else if (cmd == 1005) {
                            newSteps[hitIdx].type = LevelData::kScenarioTypeGoto;
                            newSteps[hitIdx].param = 0;
                        } else if (cmd == 1007) {
                            newSteps[hitIdx].type = LevelData::kScenarioTypeLeaveVehicle;
                            newSteps[hitIdx].targetPos = rs_knl::TilePoint(0, 0, 0);
                            newSteps[hitIdx].param = 0;
                            newSteps[hitIdx].targetEntityId = -1;
                        } else if (cmd == 1008) {
                            newSteps[hitIdx].type = LevelData::kScenarioTypeEscape;
                            newSteps[hitIdx].param = 0;
                        } else if (cmd >= 1100 && cmd < 1150) {
                            int vIdx = cmd - 1100;
                            newSteps[hitIdx].type = LevelData::kScenarioTypeUseVehicle;
                            newSteps[hitIdx].targetEntityId = vIdx;
                            newSteps[hitIdx].targetPos = rs_knl::TilePoint(0, 0, 0);
                        } else if (cmd == 1009 && hitIdx > 0) {
                            std::swap(newSteps[hitIdx], newSteps[hitIdx - 1]);
                        } else if (cmd == 1010 && hitIdx + 1 < static_cast<int>(newSteps.size())) {
                            std::swap(newSteps[hitIdx], newSteps[hitIdx + 1]);
                        } else if (cmd == 1011) {
                            newSteps.clear();
                        }
                        bool newLoop = newSteps.empty() ? false : isLoop;
                        if (pCommandStack_) {
                            pCommandStack_->executeCommand(std::make_unique<ModifyPedRouteCommand>(pMission_, pPed, oldSteps, newSteps, isLoop, newLoop));
                        } else {
                            pPed->setScriptSteps(newSteps);
                            pPed->setScriptLooping(newLoop);
                            rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                        }
                        if (commandExecutedCallback_) commandExecutedCallback_();
                        if (entitySelectedCallback_) entitySelectedCallback_(targetSelected_);
                    }
                }
                InvalidateRect(hWnd_, nullptr, FALSE);
                return;
            }
        }
        // Right-clicking on map space in RouteTool starts panning instead of cancelling route editing!
        isPanning_ = true;
        panStartMouse_ = { x, y };
        panStartCamX_ = cameraX_;
        panStartCamY_ = cameraY_;
        return;
    }

    // Middle or Right click starts panning
    if (button == 2 || button == 3) {
        isPanning_ = true;
        panStartMouse_ = { x, y };
        panStartCamX_ = cameraX_;
        panStartCamY_ = cameraY_;
        return;
    }

    if (button != 1) return;

    if (isPlacementTool(tool_)) {
        if (autoLayerSwitchingEnabled_ && !manualLayerOverride_) {
            rs_knl::TilePoint hitTile;
            if (findTileAtScreen(x, y, &hitTile)) {
                int maxZ = (pMission_ && pMission_->map()) ? pMission_->map()->maxTz() : 13;
                activeZ_ = std::min(hitTile.tz + 1, maxZ - 1);
                rs_knl::TilePoint subPt = screenToMapPointAtZ(x, y, hitTile.tz);
                currentTilePos_ = rs_knl::TilePoint(hitTile.tx, hitTile.ty, activeZ_, subPt.ox, subPt.oy);
            } else {
                activeZ_ = explicitSelectedZ_;
                currentTilePos_ = screenToMapPointAtZ(x, y, activeZ_);
            }
        } else {
            activeZ_ = explicitSelectedZ_;
            currentTilePos_ = screenToMapPoint(x, y);
        }
    } else {
        bool isPickTool = (tool_ == ViewportTool::SelectTile ||
                           tool_ == ViewportTool::SelectEntity);
        currentTilePos_ = isPickTool ? pickTileAtScreen(x, y) : screenToMapPoint(x, y);
    }

    switch (tool_) {
        case ViewportTool::Pencil: {
            isPainting_ = true;
            if (pMission_ && pMission_->map()) {
                rs_eng::Tile *t = pMission_->map()->getTileAt(currentTilePos_.tx, currentTilePos_.ty, activeZ_);
                uint8_t oldTile = t ? static_cast<uint8_t>(t->id()) : 0;
                if (oldTile != activeTileId_) {
                    if (pCommandStack_) {
                        pCommandStack_->executeCommand(std::make_unique<PaintTileCommand>(
                            pMission_->map(), currentTilePos_.tx, currentTilePos_.ty, activeZ_, oldTile, activeTileId_));
                        if (commandExecutedCallback_) commandExecutedCallback_();
                    } else {
                        pMission_->map()->setTileAt(currentTilePos_.tx, currentTilePos_.ty, activeZ_, activeTileId_);
                    }
                }
            }
            break;
        }

        case ViewportTool::BoxFill: {
            isBoxFilling_ = true;
            boxFillStart_ = rs_knl::TilePoint(currentTilePos_.tx, currentTilePos_.ty, activeZ_);
            boxFillCurrent_ = rs_knl::TilePoint(currentTilePos_.tx, currentTilePos_.ty, activeZ_);
            break;
        }

        case ViewportTool::PlaceActor: {
            if (pMission_) {
                if (pMission_->numPeds() >= 256) {
                    showEditorMessageBox(hWnd_,
                        L"Cannot place Actor:\nMap has reached the maximum capacity of 256 pedestrians supported by Syndicate mission data.",
                        L"Pedestrian Capacity Limit", MB_OK | MB_ICONWARNING);
                    break;
                }
                if (actorTypeToPlace_ == rs_knl::PedInstance::kPedTypeAgent && actorIsOurAgentToPlace_) {
                    if (pMission_->countPlayerAgents() >= 4) {
                        showEditorMessageBox(hWnd_,
                            L"Cannot place Player's Agent:\nMap already contains 4 Player's Agents (the maximum allowed).\nTo place another Player's Agent, delete an existing one first.",
                            L"Player's Agent Limit", MB_OK | MB_ICONWARNING);
                        break;
                    }
                    int freeSlot = pMission_->findNextFreePlayerSlot();
                    if (freeSlot < 0) {
                        showEditorMessageBox(hWnd_,
                            L"Cannot place Player's Agent:\nNo available squad slots (0-3).",
                            L"Player's Agent Limit", MB_OK | MB_ICONWARNING);
                        break;
                    }
                    rs_knl::TilePoint spawnPos = currentTilePos_;
                    rs_knl::PedInstance *spawned = nullptr;
                    if (pCommandStack_) {
                        auto cmd = std::make_unique<SpawnPedCommand>(
                            pMission_, rs_knl::PedInstance::kPedTypeAgent, spawnPos, orientationToPlace_, 1,
                            false, 0, 0x10, 0, true, freeSlot);
                        auto *cmdPtr = cmd.get();
                        pCommandStack_->executeCommand(std::move(cmd));
                        spawned = cmdPtr->getSpawnedPed();
                        if (commandExecutedCallback_) commandExecutedCallback_();
                    } else {
                        spawned = pMission_->spawnPlayerAgent(freeSlot, spawnPos, orientationToPlace_);
                    }
                    if (spawned) {
                        targetSelected_ = spawned;
                        if (entitySelectedCallback_) {
                            entitySelectedCallback_(targetSelected_);
                        }
                    }
                    break;
                }
                if (actorTypeToPlace_ == rs_knl::PedInstance::kPedTypeAgent) {
                    int targetFac = (actorFactionIdToPlace_ >= 1 && actorFactionIdToPlace_ <= 7) ? actorFactionIdToPlace_ : 1;
                    if (pMission_->countFactionAgents(targetFac) >= 8) {
                        showEditorMessageBox(hWnd_, std::format(L"Cannot place Enemy Agent:\nFaction {} already has the maximum capacity of 8 agents.", targetFac).c_str(), L"Faction Capacity Limit", MB_OK | MB_ICONWARNING);
                        break;
                    }
                }
                rs_knl::TilePoint spawnPos = currentTilePos_;
                rs_knl::PedInstance *spawned = nullptr;
                if (pCommandStack_) {
                    auto cmd = std::make_unique<SpawnPedCommand>(
                        pMission_, actorTypeToPlace_, spawnPos, orientationToPlace_, actorBaseAnimToPlace_,
                        actorCyberwareOverriddenToPlace_, actorCyberwareTierToPlace_,
                        actorMovementModeToPlace_, actorFactionIdToPlace_);
                    auto *cmdPtr = cmd.get();
                    pCommandStack_->executeCommand(std::move(cmd));
                    spawned = cmdPtr->getSpawnedPed();
                    if (commandExecutedCallback_) commandExecutedCallback_();
                } else {
                    spawned = pMission_->spawnPed(actorTypeToPlace_, spawnPos, orientationToPlace_, actorBaseAnimToPlace_, actorFactionIdToPlace_);
                    if (spawned) {
                        spawned->setInitialLevelState(actorMovementModeToPlace_);
                        if (spawned->scriptSteps().empty()) {
                            spawned->destroyAllActions(true);
                            if (actorMovementModeToPlace_ & LevelData::kPeopleStateWalking) {
                                spawned->addToDefaultActions(new rs_knl::WalkToDirectionAction());
                                spawned->addMovementAction(spawned->defaultAction(), false);
                            }
                        }
                        if (spawned->type() == rs_knl::PedInstance::kPedTypeAgent && !spawned->isOurAgent()) {
                            spawned->setCyberwareOverridden(actorCyberwareOverriddenToPlace_);
                            if (actorCyberwareOverriddenToPlace_) {
                                spawned->applyCyberwareTier(actorCyberwareTierToPlace_);
                            } else if (pMission_) {
                                spawned->applyCyberwareTier(pMission_->techLevel());
                            }
                            if (actorFactionIdToPlace_ >= 0) {
                                spawned->setFactionId(actorFactionIdToPlace_);
                            }
                        }
                    }
                }
                if (spawned) {
                    if (actorHealthToPlace_ > 0) {
                        spawned->setStartHealth(actorHealthToPlace_);
                        spawned->setHealth(actorHealthToPlace_);
                    } else if (actorHealthToPlace_ <= 0 && actorHealthToPlace_ != 0) {
                        spawned->setStartHealth(0);
                        spawned->setHealth(-1);
                        spawned->playDeadAnimation();
                    }
                    if (!actorWeaponsToPlace_.empty()) {
                        for (int wType : actorWeaponsToPlace_) {
                            rs_knl::Weapon *w = g_weaponMgr.getWeapon(static_cast<rs_knl::Weapon::WeaponType>(wType));
                            if (w) {
                                rs_knl::WeaponInstance *wi = rs_knl::WeaponInstance::createInstance(w);
                                if (wi) {
                                    wi->setOwner(spawned);
                                    spawned->addWeapon(wi);
                                }
                            }
                        }
                    }
                }
            }
            break;
        }

        case ViewportTool::PlaceVehicle: {
            if (pMission_) {
                if (pMission_->numVehicles() >= 64) {
                    showEditorMessageBox(hWnd_,
                        L"Cannot place Vehicle:\nMap has reached the maximum capacity of 64 vehicles supported by Syndicate mission data.",
                        L"Vehicle Capacity Limit", MB_OK | MB_ICONWARNING);
                    break;
                }
                rs_knl::Vehicle *spawned = nullptr;
                if (pCommandStack_) {
                    auto cmd = std::make_unique<SpawnVehicleCommand>(
                        pMission_, vehicleTypeToPlace_, currentTilePos_, orientationToPlace_,
                        vehicleHealthToPlace_, vehicleDriverToPlace_, vehicleDriverBaseAnimToPlace_,
                        vehicleBaseAnimToPlace_);
                    auto *cmdPtr = cmd.get();
                    pCommandStack_->executeCommand(std::move(cmd));
                    spawned = cmdPtr->getSpawnedVehicle();
                    if (commandExecutedCallback_) commandExecutedCallback_();
                } else {
                    spawned = pMission_->spawnVehicle(vehicleTypeToPlace_, currentTilePos_, orientationToPlace_, vehicleBaseAnimToPlace_);
                    if (spawned) {
                        if (vehicleHealthToPlace_ > 0) {
                            spawned->setStartHealth(vehicleHealthToPlace_);
                            spawned->setHealth(vehicleHealthToPlace_);
                        } else if (vehicleHealthToPlace_ <= 0 && vehicleHealthToPlace_ != 0) {
                            spawned->setStartHealth(0);
                            spawned->setHealth(-1);
                            spawned->playAnimation(spawned->burntAnimation());
                        }
                    }
                    if (vehicleDriverToPlace_ > 0) {
                        if (spawned && (spawned->isCar() || spawned->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead)) {
                            rs_knl::PedInstance *pDriver = pMission_->spawnPed(
                                static_cast<rs_knl::PedInstance::PedType>(vehicleDriverToPlace_),
                                currentTilePos_, orientationToPlace_, vehicleDriverBaseAnimToPlace_);
                            if (pDriver) {
                                spawned->setDriver(pDriver);
                                pDriver->setPosition(spawned->position());
                            }
                        }
                    }
                }
            }
            break;
        }

        case ViewportTool::PlaceStatic: {
            if (pMission_) {
                if (pMission_->numStatics() >= 400) {
                    showEditorMessageBox(hWnd_,
                        L"Cannot place Static Object:\nMap has reached the maximum capacity of 400 static objects supported by Syndicate mission data.",
                        L"Static Objects Capacity Limit", MB_OK | MB_ICONWARNING);
                    break;
                }
                if (pCommandStack_) {
                    pCommandStack_->executeCommand(std::make_unique<SpawnStaticCommand>(
                        pMission_, staticIdToPlace_, currentTilePos_, staticBaseAnimToPlace_, orientationToPlace_));
                    if (commandExecutedCallback_) commandExecutedCallback_();
                } else {
                    pMission_->spawnStatic(staticIdToPlace_, currentTilePos_, staticBaseAnimToPlace_, orientationToPlace_);
                }
            }
            break;
        }

        case ViewportTool::PlaceWeapon: {
            if (pMission_) {
                if (pMission_->numWeaponsOnGround() >= 512) {
                    showEditorMessageBox(hWnd_,
                        L"Cannot place Weapon Drop:\nMap has reached the maximum capacity of 512 weapons supported by Syndicate mission data.",
                        L"Weapon Capacity Limit", MB_OK | MB_ICONWARNING);
                    break;
                }
                bool hasAR = false;
                EditorPathsConfig pathsCfg;
                if (EditorConfig::loadPathsConfig(pathsCfg) && !pathsCfg.dataDir.empty()) {
                    hasAR = rs_utl::File::hasAmericanRevoltData(pathsCfg.dataDir);
                } else {
                    hasAR = rs_utl::File::hasAmericanRevoltData();
                }
                if (rs_knl::Weapon::isAmericanRevoltOnly(weaponTypeToPlace_) && !hasAR) {
                    showEditorMessageBox(hWnd_,
                        L"Cannot place American Revolt Weapon:\nThe data directory configured in editor settings is not pointing to American Revolt assets.\nConfigure an American Revolt data directory via File -> Settings.",
                        L"American Revolt Assets Required", MB_OK | MB_ICONWARNING);
                    break;
                }
                if (pCommandStack_) {
                    pCommandStack_->executeCommand(std::make_unique<SpawnWeaponCommand>(
                        pMission_, weaponTypeToPlace_, currentTilePos_));
                    if (commandExecutedCallback_) commandExecutedCallback_();
                } else {
                    pMission_->spawnGroundWeapon(weaponTypeToPlace_, currentTilePos_);
                }
            }
            break;
        }

        case ViewportTool::SelectTile: {
            targetSelected_ = nullptr;
            isDraggingEntity_ = false;
            hasDraggedEntity_ = false;

            rs_knl::Map *pMap = (pMission_ && pMission_->map()) ? pMission_->map() : nullptr;

            int tId = -1;
            if (pMap &&
                currentTilePos_.tx >= 0 && currentTilePos_.tx < pMap->maxTx() &&
                currentTilePos_.ty >= 0 && currentTilePos_.ty < pMap->maxTy()) {
                tId = pMap->getTileIdAt(currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz);
            }

            if (tId > 0) {
                hasSelectedTile_ = true;
                selectedTilePos_ = currentTilePos_;
                isDraggingTile_ = true;
                hasDraggedTile_ = false;
                dragTileStartPos_ = currentTilePos_;
                dragTileCurrentPos_ = currentTilePos_;
                dragTileStartMouse_ = { x, y };
                dragTileStartMouseTile_ = screenToMapPointAtZ(x, y, dragTileStartPos_.tz);

                setActiveZ(selectedTilePos_.tz);
                currentTilePos_ = selectedTilePos_;

                if (tileSelectedCallback_) {
                    tileSelectedCallback_(static_cast<uint8_t>(tId));
                }
            } else {
                hasSelectedTile_ = false;
                isDraggingTile_ = false;
            }

            if (entitySelectedCallback_) {
                entitySelectedCallback_(nullptr);
            }
            break;
        }

        case ViewportTool::SelectEntity: {
            auto candidates = hitTestAllEntitiesAtScreen(x, y);
            rs_knl::ShootableMapObject *clickedObj = nullptr;

            if (!candidates.empty()) {
                if (targetSelected_ != nullptr) {
                    auto it = std::find(candidates.begin(), candidates.end(), targetSelected_);
                    if (it != candidates.end()) {
                        size_t nextIdx = (std::distance(candidates.begin(), it) + 1) % candidates.size();
                        clickedObj = candidates[nextIdx];
                    } else {
                        clickedObj = candidates[0];
                    }
                } else {
                    clickedObj = candidates[0];
                }
            }

            if (clickedObj) {
                targetSelected_ = clickedObj;
                hasSelectedTile_ = false;
                lastOverlappingEntity_ = clickedObj;
                lastOverlappingTilePos_ = rs_knl::TilePoint(clickedObj->tileX(), clickedObj->tileY(), clickedObj->tileZ());
                isDraggingEntity_ = true;
                hasDraggedEntity_ = false;
                dragStartMouse_ = { x, y };
                dragStartMouseTile_ = screenToMapPointAtZ(x, y, clickedObj->tileZ());
                dragEntityStartTile_ = targetSelected_->position();
                isDraggingTile_ = false;
                hasDraggedTile_ = false;
            } else {
                // No entity clicked: CLEAR entity selection and NEVER select tiles!
                targetSelected_ = nullptr;
                hasSelectedTile_ = false;
                isDraggingEntity_ = false;
                hasDraggedEntity_ = false;
                isDraggingTile_ = false;
                hasDraggedTile_ = false;
            }

            if (entitySelectedCallback_) {
                entitySelectedCallback_(targetSelected_);
            }
            break;
        }

        case ViewportTool::RouteTool: {
            rs_knl::PedInstance *pPed = getActiveRoutePed();
            if (!pPed) {
                setTool(ViewportTool::SelectEntity);
                break;
            }

            int hitIdx = hitTestWaypointAtScreen(x, y, pPed->scriptSteps());
            if (hitIdx >= 0) {
                isDraggingWaypoint_ = true;
                draggingWaypointIdx_ = hitIdx;
                dragWaypointStartPos_ = pPed->scriptSteps()[hitIdx].targetPos;
                dragWaypointCurrentPos_ = dragWaypointStartPos_;
            } else {
                rs_knl::Vehicle *nextVeh = getPedVehicleAtStep(pMission_, pPed, pPed->scriptSteps().size());
                bool isVeh = (nextVeh != nullptr && (nextVeh->isCar() || nextVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead || nextVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody));
                int refZ = getReferenceZForRouteStep(pPed, pPed->scriptSteps().size(), nextVeh);
                rs_knl::TilePoint clickPt = resolveWalkableTilePoint(x, y, refZ, isVeh);
                rs_knl::ScriptStep newStep;
                newStep.type = LevelData::kScenarioTypeGoto;
                newStep.targetPos = clickPt;
                newStep.targetEntityId = 0;
                newStep.param = 0;

                auto oldSteps = pPed->scriptSteps();
                auto newSteps = oldSteps;
                bool isLoop = pPed->isScriptLooping();
                if (!newSteps.empty() && newSteps.back().type == LevelData::kScenarioTypeReset) {
                    newSteps.pop_back();
                }
                if (newSteps.empty()) {
                    rs_knl::Vehicle *v0 = getPedVehicleAtStep(pMission_, pPed, 0);
                    rs_knl::TilePoint spawnPos = (v0 != nullptr) ? v0->position() : pPed->position();
                    rs_knl::ScriptStep spawnStep;
                    spawnStep.type = LevelData::kScenarioTypeGoto;
                    spawnStep.targetPos = spawnPos;
                    newSteps.push_back(spawnStep);
                }
                newSteps.push_back(newStep);

                if (pCommandStack_) {
                    pCommandStack_->executeCommand(std::make_unique<ModifyPedRouteCommand>(pMission_, pPed, oldSteps, newSteps, isLoop, isLoop));
                } else {
                    pPed->setScriptSteps(newSteps);
                    pPed->setScriptLooping(isLoop);
                    rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                }
                if (commandExecutedCallback_) commandExecutedCallback_();
                if (entitySelectedCallback_) entitySelectedCallback_(targetSelected_);
            }
            break;
        }

        default:
            break;
    }

    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::onMouseUp(int /*x*/, int /*y*/, int /*button*/) {
    ReleaseCapture();
    if (!pMission_ || !pMission_->map()) return;

    if (objectivePickMode_ != ObjectivePickMode::None) {
        if (isPanning_) {
            isPanning_ = false;
        }
        return;
    }

    if (isDraggingWaypoint_) {
        isDraggingWaypoint_ = false;
        rs_knl::PedInstance *pPed = getActiveRoutePed();
        if (pPed && draggingWaypointIdx_ >= 0 && static_cast<size_t>(draggingWaypointIdx_) < pPed->scriptSteps().size()) {
            if (dragWaypointCurrentPos_.tx != dragWaypointStartPos_.tx ||
                dragWaypointCurrentPos_.ty != dragWaypointStartPos_.ty ||
                dragWaypointCurrentPos_.tz != dragWaypointStartPos_.tz) {
                auto oldSteps = pPed->scriptSteps();
                auto newSteps = oldSteps;
                bool isLoop = pPed->isScriptLooping();
                if (!newSteps.empty() && newSteps.back().type == LevelData::kScenarioTypeReset) {
                    newSteps.pop_back();
                }
                if (draggingWaypointIdx_ < static_cast<int>(newSteps.size())) {
                    newSteps[draggingWaypointIdx_].targetPos = dragWaypointCurrentPos_;
                }
                if (draggingWaypointIdx_ == 0) {
                    pPed->setPosition(dragWaypointCurrentPos_);
                    if (pMission_ && pPed->inVehicle()) {
                        pPed->inVehicle()->setPosition(dragWaypointCurrentPos_);
                    }
                }
                if (pCommandStack_) {
                    pCommandStack_->executeCommand(std::make_unique<ModifyPedRouteCommand>(pMission_, pPed, oldSteps, newSteps, isLoop, isLoop));
                } else {
                    pPed->setScriptSteps(newSteps);
                    pPed->setScriptLooping(isLoop);
                    rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
                }
                if (commandExecutedCallback_) commandExecutedCallback_();
                if (entitySelectedCallback_) entitySelectedCallback_(targetSelected_);
            }
        }
        draggingWaypointIdx_ = -1;
    }

    if (isPanning_) {
        isPanning_ = false;
    }

    if (isPainting_) {
        isPainting_ = false;
    }

    if (isDraggingEntity_) {
        isDraggingEntity_ = false;
        if (targetSelected_ && hasDraggedEntity_) {
            hasDraggedEntity_ = false;
            rs_knl::TilePoint destPos = targetSelected_->position();
            if (destPos.tx != dragEntityStartTile_.tx ||
                destPos.ty != dragEntityStartTile_.ty ||
                destPos.ox != dragEntityStartTile_.ox ||
                destPos.oy != dragEntityStartTile_.oy) {

                auto *pPed = dynamic_cast<rs_knl::PedInstance*>(targetSelected_);
                rs_knl::Vehicle *assignedVeh = nullptr;
                if (pPed) {
                    assignedVeh = pPed->inVehicle();
                    if (!assignedVeh && pMission_) {
                        for (size_t vi = 0; vi < pMission_->numVehicles(); ++vi) {
                            if (auto *v = pMission_->vehicle(vi)) {
                                if (v->getDriver() == pPed) { assignedVeh = v; break; }
                            }
                        }
                    }
                }

                if (pPed && assignedVeh) {
                    uint16_t vId = assignedVeh->id();
                    std::wstring msg = std::format(
                        L"This pedestrian is currently the assigned driver of Vehicle #{}.\n\n"
                        L"Do you want to detach the driver from the vehicle?", vId);
                    int res = showEditorMessageBox(hWnd_, msg.c_str(), L"Detach Driver", MB_YESNO | MB_ICONQUESTION);
                    if (res == IDYES) {
                        if (assignedVeh->getDriver() == pPed) {
                            assignedVeh->setDriver(nullptr);
                        } else {
                            assignedVeh->dropPassenger(pPed);
                        }
                        if (pCommandStack_) {
                            pCommandStack_->executeCommand(std::make_unique<MoveEntityCommand>(
                                targetSelected_, dragEntityStartTile_, destPos, pMission_));
                        }
                        if (commandExecutedCallback_) {
                            commandExecutedCallback_();
                        }
                    } else {
                        // Revert ped back to vehicle's position
                        targetSelected_->setPosition(dragEntityStartTile_);
                    }
                } else {
                    if (pCommandStack_) {
                        pCommandStack_->executeCommand(std::make_unique<MoveEntityCommand>(
                            targetSelected_, dragEntityStartTile_, destPos, pMission_));
                    }
                    if (commandExecutedCallback_) {
                        commandExecutedCallback_();
                    }
                }
            }
            if (entitySelectedCallback_) {
                entitySelectedCallback_(targetSelected_);
            }
        }
        hasDraggedEntity_ = false;
    }

    if (isDraggingTile_) {
        isDraggingTile_ = false;
        if (hasSelectedTile_ && hasDraggedTile_) {
            hasDraggedTile_ = false;
            if (dragTileCurrentPos_.tx != dragTileStartPos_.tx ||
                dragTileCurrentPos_.ty != dragTileStartPos_.ty) {
                bool isCopy = (GetKeyState(VK_CONTROL) < 0);
                if (pCommandStack_ && pMission_ && pMission_->map()) {
                    pCommandStack_->executeCommand(std::make_unique<MoveTileCommand>(
                        pMission_->map(),
                        dragTileStartPos_.tx, dragTileStartPos_.ty, dragTileStartPos_.tz,
                        dragTileCurrentPos_.tx, dragTileCurrentPos_.ty, dragTileCurrentPos_.tz,
                        isCopy));
                    if (commandExecutedCallback_) {
                        commandExecutedCallback_();
                    }
                } else if (pMission_ && pMission_->map()) {
                    int srcTile = pMission_->map()->getTileIdAt(
                        dragTileStartPos_.tx, dragTileStartPos_.ty, dragTileStartPos_.tz);
                    if (!isCopy) {
                        pMission_->map()->setTileAt(
                            dragTileStartPos_.tx, dragTileStartPos_.ty, dragTileStartPos_.tz, 0);
                    }
                    pMission_->map()->setTileAt(
                        dragTileCurrentPos_.tx, dragTileCurrentPos_.ty, dragTileCurrentPos_.tz,
                        srcTile > 0 ? static_cast<uint8_t>(srcTile) : 0);
                }
                selectedTilePos_ = dragTileCurrentPos_;
            }
        }
        hasDraggedTile_ = false;
    }

    if (isBoxFilling_) {
        isBoxFilling_ = false;
        if (pMission_ && pMission_->map()) {
            if (pCommandStack_) {
                pCommandStack_->executeCommand(std::make_unique<BoxFillTilesCommand>(
                    pMission_->map(), boxFillStart_.tx, boxFillStart_.ty,
                    boxFillCurrent_.tx, boxFillCurrent_.ty, activeZ_, activeTileId_));
                if (commandExecutedCallback_) commandExecutedCallback_();
            } else {
                int x1 = std::min(boxFillStart_.tx, boxFillCurrent_.tx);
                int x2 = std::max(boxFillStart_.tx, boxFillCurrent_.tx);
                int y1 = std::min(boxFillStart_.ty, boxFillCurrent_.ty);
                int y2 = std::max(boxFillStart_.ty, boxFillCurrent_.ty);
                for (int bx = x1; bx <= x2; ++bx) {
                    for (int by = y1; by <= y2; ++by) {
                        pMission_->map()->setTileAt(bx, by, activeZ_, activeTileId_);
                    }
                }
            }
        }
    }

    InvalidateRect(hWnd_, nullptr, FALSE);
}

void WinMapViewport::onMouseWheel(int delta, int x, int y) {
    float oldZoom = zoom_;
    float newZoom = (delta > 0) ? (oldZoom + 0.25f) : (oldZoom - 0.25f);
    if (newZoom < 0.25f) newZoom = 0.25f;
    if (newZoom > 3.0f) newZoom = 3.0f;

    if (std::abs(newZoom - oldZoom) > 0.01f) {
        // Center zoom on mouse cursor
        int worldX = static_cast<int>(x / oldZoom) + cameraX_;
        int worldY = static_cast<int>(y / oldZoom) + cameraY_;

        zoom_ = newZoom;
        cameraX_ = worldX - static_cast<int>(x / newZoom);
        cameraY_ = worldY - static_cast<int>(y / newZoom);

        if (cameraX_ < 0) cameraX_ = 0;
        if (cameraY_ < 0) cameraY_ = 0;

        InvalidateRect(hWnd_, nullptr, FALSE);
    }
}

void WinMapViewport::drawObjectivePickOverlay(HDC hdc) {
    if (objectivePickMode_ == ObjectivePickMode::None) return;

    // 1. Hover target indicators
    if (objectivePickMode_ == ObjectivePickMode::Ped || objectivePickMode_ == ObjectivePickMode::Vehicle) {
        if (targetHovered_) {
            bool isValid = false;
            std::wstring targetBadgeText;
            if (objectivePickMode_ == ObjectivePickMode::Ped) {
                if (auto *p = dynamic_cast<rs_knl::PedInstance*>(targetHovered_)) {
                    if (!p->isOurAgent()) {
                        isValid = true;
                        const wchar_t *tName = L"Civilian";
                        if (p->type() == rs_knl::PedInstance::kPedTypeGuard) tName = L"Guard";
                        else if (p->type() == rs_knl::PedInstance::kPedTypePolice) tName = L"Police";
                        else if (p->type() == rs_knl::PedInstance::kPedTypeAgent) tName = L"Enemy Agent";
                        else if (p->type() == rs_knl::PedInstance::kPedTypeCriminal) tName = L"Criminal";
                        targetBadgeText = std::format(L"Ped #{:03} [{}]", p->id(), tName);
                    }
                }
            } else if (objectivePickMode_ == ObjectivePickMode::Vehicle) {
                if (auto *v = dynamic_cast<rs_knl::Vehicle*>(targetHovered_)) {
                    isValid = true;
                    targetBadgeText = std::format(L"Vehicle #{:02}", v->id());
                }
            }

            if (isValid) {
                POINT scrPt = entityPointToScreen(targetHovered_->position());
                int tr = static_cast<int>(22 * zoom_);
                if (tr < 14) tr = 14;

                HPEN hTargetPen = CreatePen(PS_SOLID, 2, RGB(0, 240, 255));
                HPEN hOldTP = static_cast<HPEN>(SelectObject(hdc, hTargetPen));
                HBRUSH hOldBr = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(HOLLOW_BRUSH)));

                Ellipse(hdc, scrPt.x - tr, scrPt.y - tr, scrPt.x + tr, scrPt.y + tr);
                MoveToEx(hdc, scrPt.x - tr - 8, scrPt.y, nullptr);
                LineTo(hdc, scrPt.x + tr + 8, scrPt.y);
                MoveToEx(hdc, scrPt.x, scrPt.y - tr - 8, nullptr);
                LineTo(hdc, scrPt.x, scrPt.y + tr + 8);

                SelectObject(hdc, hOldBr);
                SelectObject(hdc, hOldTP);
                DeleteObject(hTargetPen);

                // Badge text
                HFONT hBadgeFont = CreateFontW(
                    -12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
                );
                HFONT hOldBF = static_cast<HFONT>(SelectObject(hdc, hBadgeFont));
                RECT badgeRc = { 0, 0, 0, 0 };
                DrawTextW(hdc, targetBadgeText.c_str(), -1, &badgeRc, DT_CALCRECT | DT_NOPREFIX);
                int bw = (badgeRc.right - badgeRc.left) + 12;
                int bh = (badgeRc.bottom - badgeRc.top) + 6;
                int bx = scrPt.x - bw / 2;
                int by = scrPt.y - tr - bh - 6;
                RECT drawRc = { bx, by, bx + bw, by + bh };

                HBRUSH hBg = CreateSolidBrush(RGB(15, 23, 42));
                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 240, 255));
                SelectObject(hdc, hBg);
                SelectObject(hdc, hPen);
                RoundRect(hdc, drawRc.left, drawRc.top, drawRc.right, drawRc.bottom, 4, 4);

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(0, 240, 255));
                DrawTextW(hdc, targetBadgeText.c_str(), -1, &drawRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

                SelectObject(hdc, hOldBF);
                DeleteObject(hBadgeFont);
                DeleteObject(hBg);
                DeleteObject(hPen);
            }
        }
    } else if (objectivePickMode_ == ObjectivePickMode::Coordinate) {
        POINT scrPt = mapPointToScreen(currentTilePos_);
        int hw = static_cast<int>(32 * zoom_);
        int hh = static_cast<int>(16 * zoom_);
        POINT diamond[5] = {
            { scrPt.x, scrPt.y - hh },
            { scrPt.x + hw, scrPt.y },
            { scrPt.x, scrPt.y + hh },
            { scrPt.x - hw, scrPt.y },
            { scrPt.x, scrPt.y - hh }
        };

        HPEN hTilePen = CreatePen(PS_SOLID, 2, RGB(0, 240, 255));
        HPEN hOldTP = static_cast<HPEN>(SelectObject(hdc, hTilePen));
        Polyline(hdc, diamond, 5);
        SelectObject(hdc, hOldTP);
        DeleteObject(hTilePen);

        // Coordinate text badge
        std::wstring badgePrefix = (!customPickPrompt_.empty() && customPickPrompt_.find(L"SWEEP") != std::wstring::npos) ? L"Sweep Target" : ((!customPickPrompt_.empty() && customPickPrompt_.find(L"PATROL") != std::wstring::npos) ? L"Patrol Waypoint" : L"Evacuate Dest");
        std::wstring coordText = std::format(L"{} (TX:{:02}, TY:{:02}, TZ:{:02})", badgePrefix,
            currentTilePos_.tx, currentTilePos_.ty, currentTilePos_.tz);
        HFONT hBadgeFont = CreateFontW(
            -12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
        HFONT hOldBF = static_cast<HFONT>(SelectObject(hdc, hBadgeFont));
        RECT badgeRc = { 0, 0, 0, 0 };
        DrawTextW(hdc, coordText.c_str(), -1, &badgeRc, DT_CALCRECT | DT_NOPREFIX);
        int bw = (badgeRc.right - badgeRc.left) + 12;
        int bh = (badgeRc.bottom - badgeRc.top) + 6;
        int bx = scrPt.x - bw / 2;
        int by = scrPt.y - hh - bh - 6;
        RECT drawRc = { bx, by, bx + bw, by + bh };

        HBRUSH hBg = CreateSolidBrush(RGB(15, 23, 42));
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 240, 255));
        SelectObject(hdc, hBg);
        SelectObject(hdc, hPen);
        RoundRect(hdc, drawRc.left, drawRc.top, drawRc.right, drawRc.bottom, 4, 4);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 240, 255));
        DrawTextW(hdc, coordText.c_str(), -1, &drawRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        SelectObject(hdc, hOldBF);
        DeleteObject(hBadgeFont);
        DeleteObject(hBg);
        DeleteObject(hPen);
    }

    // 2. Draw Top Bar Banner
    int bannerH = 40;
    RECT bannerRc = { 0, 0, dibWidth_, bannerH };
    HBRUSH hBannerBg = CreateSolidBrush(RGB(15, 23, 42));
    HPEN hBannerBorder = CreatePen(PS_SOLID, 2, RGB(0, 220, 245));
    HBRUSH hOldB = static_cast<HBRUSH>(SelectObject(hdc, hBannerBg));
    HPEN hOldP = static_cast<HPEN>(SelectObject(hdc, hBannerBorder));
    Rectangle(hdc, bannerRc.left - 1, bannerRc.top - 1, bannerRc.right + 1, bannerRc.bottom);
    SelectObject(hdc, hOldB);
    SelectObject(hdc, hOldP);
    DeleteObject(hBannerBg);
    DeleteObject(hBannerBorder);

    std::wstring bannerText;
    if (!customPickPrompt_.empty()) {
        bannerText = customPickPrompt_;
    } else if (objectivePickMode_ == ObjectivePickMode::Ped) {
        bannerText = L"🎯 PICK TARGET PEDESTRIAN  \u2014  Click target entity on map (Middle/Right drag to pan)";
    } else if (objectivePickMode_ == ObjectivePickMode::Vehicle) {
        bannerText = L"🎯 PICK TARGET VEHICLE  \u2014  Click target vehicle on map (Middle/Right drag to pan)";
    } else {
        bannerText = L"🎯 PICK EVACUATION DESTINATION  \u2014  Click destination tile on map (Middle/Right drag to pan)";
    }

    HFONT hTitleFont = CreateFontW(
        -13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    HFONT hOldF = static_cast<HFONT>(SelectObject(hdc, hTitleFont));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(0, 220, 245));
    RECT textRc = { 16, 0, dibWidth_ - 140, bannerH };
    DrawTextW(hdc, bannerText.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    // Cancel Button on Banner
    int btnW = 100;
    int btnH = 26;
    int btnX = dibWidth_ - btnW - 14;
    int btnY = (bannerH - btnH) / 2;
    pickCancelBtnRect_ = { btnX, btnY, btnX + btnW, btnY + btnH };

    COLORREF cancelBg = isHoveringPickCancel_ ? RGB(220, 38, 38) : RGB(30, 41, 59);
    COLORREF cancelBorder = isHoveringPickCancel_ ? RGB(248, 113, 113) : RGB(100, 116, 139);
    HBRUSH hCancelBr = CreateSolidBrush(cancelBg);
    HPEN hCancelPen = CreatePen(PS_SOLID, 1, cancelBorder);
    SelectObject(hdc, hCancelBr);
    SelectObject(hdc, hCancelPen);
    RoundRect(hdc, btnX, btnY, btnX + btnW, btnY + btnH, 4, 4);

    SetTextColor(hdc, RGB(255, 255, 255));
    DrawTextW(hdc, L"Cancel (Esc)", -1, &pickCancelBtnRect_, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

    SelectObject(hdc, hOldF);
    DeleteObject(hTitleFont);
    DeleteObject(hCancelBr);
    DeleteObject(hCancelPen);
}

void WinMapViewport::drawObjectiveAnnotations(HDC hdc) {
    if (!pMission_ || !pMission_->map()) return;

    HFONT hBadgeFont = CreateFontW(-11, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hBadgeFont));
    SetBkMode(hdc, TRANSPARENT);

    std::vector<RECT> placedBadges;

    auto addBadge = [&](int anchorX, int anchorY, const std::wstring &text, COLORREF borderCol, COLORREF bgCol, COLORREF textCol) {
        RECT rcCalc = { 0, 0, 0, 0 };
        DrawTextW(hdc, text.c_str(), -1, &rcCalc, DT_CALCRECT | DT_NOPREFIX);
        int bw = (rcCalc.right - rcCalc.left) + 14;
        int bh = (rcCalc.bottom - rcCalc.top) + 6;
        int bx = anchorX - (bw / 2);
        int by = anchorY - bh - 6;

        RECT rcBadge = { bx, by, bx + bw, by + bh };
        bool overlaps = true;
        while (overlaps) {
            overlaps = false;
            for (const auto &placed : placedBadges) {
                RECT isect;
                if (IntersectRect(&isect, &rcBadge, &placed)) {
                    rcBadge.top -= (bh + 4);
                    rcBadge.bottom -= (bh + 4);
                    overlaps = true;
                    break;
                }
            }
        }
        placedBadges.push_back(rcBadge);

        HBRUSH hBg = CreateSolidBrush(bgCol);
        HPEN hPen = CreatePen(PS_SOLID, 1, borderCol);
        SelectObject(hdc, hBg);
        SelectObject(hdc, hPen);
        RoundRect(hdc, rcBadge.left, rcBadge.top, rcBadge.right, rcBadge.bottom, 6, 6);

        SetTextColor(hdc, textCol);
        DrawTextW(hdc, text.c_str(), -1, &rcBadge, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

        DeleteObject(hPen);
        DeleteObject(hBg);
    };

    auto drawTargetReticle = [&](POINT groundPt, int entityHeight, COLORREF color) {
        int cx = groundPt.x;
        int cy = groundPt.y - static_cast<int>(entityHeight * zoom_ / 2);
        int r = static_cast<int>(18 * zoom_);
        if (r < 12) r = 12;
        int len = r / 2;

        HPEN hPen = CreatePen(PS_SOLID, 2, color);
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));

        // Top-left bracket
        MoveToEx(hdc, cx - r, cy - r + len, nullptr);
        LineTo(hdc, cx - r, cy - r);
        LineTo(hdc, cx - r + len, cy - r);

        // Top-right bracket
        MoveToEx(hdc, cx + r - len, cy - r, nullptr);
        LineTo(hdc, cx + r, cy - r);
        LineTo(hdc, cx + r, cy - r + len);

        // Bottom-left bracket
        MoveToEx(hdc, cx - r, cy + r - len, nullptr);
        LineTo(hdc, cx - r, cy + r);
        LineTo(hdc, cx - r + len, cy + r);

        // Bottom-right bracket
        MoveToEx(hdc, cx + r - len, cy + r, nullptr);
        LineTo(hdc, cx + r, cy + r);
        LineTo(hdc, cx + r, cy + r - len);

        // Ground diamond
        int halfW = static_cast<int>(20 * zoom_);
        int halfH = static_cast<int>(10 * zoom_);
        MoveToEx(hdc, groundPt.x, groundPt.y - halfH, nullptr);
        LineTo(hdc, groundPt.x + halfW, groundPt.y);
        LineTo(hdc, groundPt.x, groundPt.y + halfH);
        LineTo(hdc, groundPt.x - halfW, groundPt.y);
        LineTo(hdc, groundPt.x, groundPt.y - halfH);

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    };

    auto renderSingleObjective = [&](int objNum, uint16_t type, uint16_t targetPedId, uint16_t targetVehicleIdx, int evacX, int evacY, int evacZ) {
        if (type == 0x01 || type == 0x02 || type == 0x03) {
            rs_knl::PedInstance *pPed = nullptr;
            for (size_t p = 0; p < pMission_->numPeds(); ++p) {
                if (pMission_->ped(p) && pMission_->ped(p)->id() == targetPedId) {
                    pPed = pMission_->ped(p);
                    break;
                }
            }
            if (pPed) {
                POINT pt = entityPointToScreen(pPed->position());
                if (type == 0x01) { // Persuade
                    drawTargetReticle(pt, 28, RGB(6, 182, 212));
                    std::wstring txt = std::format(L"\U0001F9E0 OBJ #{}: PERSUADE (Ped #{})", objNum, pPed->id());
                    addBadge(pt.x, pt.y - static_cast<int>(32 * zoom_), txt, RGB(6, 182, 212), RGB(8, 24, 32), RGB(165, 243, 252));
                } else if (type == 0x02) { // Assassinate
                    drawTargetReticle(pt, 28, RGB(244, 63, 94));
                    std::wstring txt = std::format(L"\U0001F3AF OBJ #{}: ASSASSINATE (Ped #{})", objNum, pPed->id());
                    addBadge(pt.x, pt.y - static_cast<int>(32 * zoom_), txt, RGB(244, 63, 94), RGB(28, 10, 16), RGB(254, 205, 211));
                } else if (type == 0x03) { // Protect
                    drawTargetReticle(pt, 28, RGB(34, 197, 94));
                    std::wstring txt = std::format(L"\U0001F6E1 OBJ #{}: PROTECT (Ped #{})", objNum, pPed->id());
                    addBadge(pt.x, pt.y - static_cast<int>(32 * zoom_), txt, RGB(34, 197, 94), RGB(10, 28, 16), RGB(187, 247, 208));
                }
            }
        } else if (type == 0x0E || type == 0x0F) {
            if (targetVehicleIdx < pMission_->numVehicles()) {
                auto *pVeh = pMission_->vehicle(targetVehicleIdx);
                if (pVeh) {
                    POINT pt = entityPointToScreen(pVeh->position());
                    if (type == 0x0E) { // Destroy vehicle
                        drawTargetReticle(pt, 24, RGB(249, 115, 22));
                        std::wstring txt = std::format(L"\U0001F4A5 OBJ #{}: DESTROY VEHICLE", objNum);
                        addBadge(pt.x, pt.y - static_cast<int>(30 * zoom_), txt, RGB(249, 115, 22), RGB(32, 16, 8), RGB(254, 215, 170));
                    } else if (type == 0x0F) { // Use vehicle
                        drawTargetReticle(pt, 24, RGB(168, 85, 247));
                        std::wstring txt = std::format(L"\U0001F697 OBJ #{}: USE VEHICLE", objNum);
                        addBadge(pt.x, pt.y - static_cast<int>(30 * zoom_), txt, RGB(168, 85, 247), RGB(24, 12, 34), RGB(233, 213, 255));
                    }
                }
            }
        } else if (type == 0x10) { // Evacuate
            int cwx = evacX;
            int cwy = evacY;
            int cwz = evacZ;

            // Generate 32 polygon vertices around 512-world-unit radius circle
            const int kSegs = 32;
            std::vector<POINT> poly;
            poly.reserve(kSegs);
            int minX = 999999, maxX = -999999, minY = 999999, maxY = -999999;
            for (int s = 0; s < kSegs; ++s) {
                double rad = (s * 360.0 / kSegs) * 3.14159265358979323846 / 180.0;
                int wx = cwx + static_cast<int>(512.0 * cos(rad));
                int wy = cwy + static_cast<int>(512.0 * sin(rad));
                int wz = cwz;

                rs_knl::TilePoint tp;
                tp.tx = wx / 256;
                tp.ty = wy / 256;
                tp.tz = wz / 128;
                tp.ox = wx % 256;
                tp.oy = wy % 256;
                tp.oz = wz % 128;
                if (tp.ox < 0) { tp.ox += 256; tp.tx--; }
                if (tp.oy < 0) { tp.oy += 256; tp.ty--; }
                if (tp.oz < 0) { tp.oz += 128; tp.tz--; }

                POINT sc = entityPointToScreen(tp);
                poly.push_back(sc);
                if (sc.x < minX) minX = sc.x;
                if (sc.x > maxX) maxX = sc.x;
                if (sc.y < minY) minY = sc.y;
                if (sc.y > maxY) maxY = sc.y;
            }

            // Translucent area fill via AlphaBlend
            int bbW = (maxX - minX) + 8;
            int bbH = (maxY - minY) + 8;
            if (bbW > 0 && bbH > 0 && minX < dibWidth_ && maxX >= 0 && minY < dibHeight_ && maxY >= 0) {
                HDC hdcMask = CreateCompatibleDC(hdc);
                HBITMAP hBmpMask = CreateCompatibleBitmap(hdc, bbW, bbH);
                HBITMAP hOldBmp = static_cast<HBITMAP>(SelectObject(hdcMask, hBmpMask));

                RECT rcBox = { 0, 0, bbW, bbH };
                FillRect(hdcMask, &rcBox, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

                std::vector<POINT> localPoly;
                localPoly.reserve(kSegs);
                for (const auto &p : poly) {
                    localPoly.push_back({ p.x - (minX - 4), p.y - (minY - 4) });
                }

                HBRUSH hZoneBr = CreateSolidBrush(RGB(16, 185, 129));
                HPEN hZonePen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
                SelectObject(hdcMask, hZoneBr);
                SelectObject(hdcMask, hZonePen);
                Polygon(hdcMask, localPoly.data(), static_cast<int>(localPoly.size()));

                BLENDFUNCTION bf = {};
                bf.BlendOp = AC_SRC_OVER;
                bf.SourceConstantAlpha = 50; // Translucent emerald-cyan wash
                AlphaBlend(hdc, minX - 4, minY - 4, bbW, bbH, hdcMask, 0, 0, bbW, bbH, bf);

                SelectObject(hdcMask, hOldBmp);
                DeleteObject(hBmpMask);
                DeleteObject(hZoneBr);
                DeleteObject(hZonePen);
                DeleteDC(hdcMask);
            }

            // Glowing boundary perimeter line
            HPEN hPerimPen = CreatePen(PS_SOLID, 2, RGB(52, 211, 153));
            HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPerimPen));
            HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
            Polygon(hdc, poly.data(), static_cast<int>(poly.size()));

            // Center Point Marker
            rs_knl::TilePoint centerTp(cwx / 256, cwy / 256, cwz / 128, cwx % 256, cwy % 256, cwz % 128);
            POINT centerPt = entityPointToScreen(centerTp);

            HPEN hCrossPen = CreatePen(PS_SOLID, 1, RGB(110, 231, 183));
            SelectObject(hdc, hCrossPen);
            int crossR = static_cast<int>(12 * zoom_);
            MoveToEx(hdc, centerPt.x - crossR, centerPt.y, nullptr);
            LineTo(hdc, centerPt.x + crossR, centerPt.y);
            MoveToEx(hdc, centerPt.x, centerPt.y - crossR, nullptr);
            LineTo(hdc, centerPt.x, centerPt.y + crossR);

            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPerimPen);
            DeleteObject(hCrossPen);

            std::wstring txt = std::format(L"\U0001F681 OBJ #{}: EVACUATION ZONE (2-Tile Radius)", objNum);
            addBadge(centerPt.x, minY - 4, txt, RGB(16, 185, 129), RGB(6, 26, 16), RGB(167, 243, 208));
        }
    };

    // Render existing objectives (skipping the one being edited, if any)
    for (size_t i = 0; i < pMission_->numObjectives(); ++i) {
        if (objPreviewDesc_.active && objPreviewDesc_.editingIndex == static_cast<int>(i)) {
            continue; // Skip stale target of the objective currently being edited
        }
        auto *obj = pMission_->objective(i);
        if (!obj) continue;

        int num = static_cast<int>(i + 1);
        if (auto *oa = dynamic_cast<rs_knl::ObjAssassinate*>(obj)) {
            if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(oa->target())) {
                renderSingleObjective(num, 0x02, pPed->id(), 0, 0, 0, 0);
            }
        } else if (auto *op = dynamic_cast<rs_knl::ObjPersuade*>(obj)) {
            if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(op->target())) {
                renderSingleObjective(num, 0x01, pPed->id(), 0, 0, 0, 0);
            }
        } else if (auto *opr = dynamic_cast<rs_knl::ObjProtect*>(obj)) {
            if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(opr->target())) {
                renderSingleObjective(num, 0x03, pPed->id(), 0, 0, 0, 0);
            }
        } else if (auto *odv = dynamic_cast<rs_knl::ObjDestroyVehicle*>(obj)) {
            if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(odv->target())) {
                uint16_t vIdx = 0;
                for (size_t vi = 0; vi < pMission_->numVehicles(); ++vi) {
                    if (pMission_->vehicle(vi) == pVeh) { vIdx = static_cast<uint16_t>(vi); break; }
                }
                renderSingleObjective(num, 0x0E, 0, vIdx, 0, 0, 0);
            }
        } else if (auto *ouv = dynamic_cast<rs_knl::ObjUseVehicle*>(obj)) {
            if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(ouv->target())) {
                uint16_t vIdx = 0;
                for (size_t vi = 0; vi < pMission_->numVehicles(); ++vi) {
                    if (pMission_->vehicle(vi) == pVeh) { vIdx = static_cast<uint16_t>(vi); break; }
                }
                renderSingleObjective(num, 0x0F, 0, vIdx, 0, 0, 0);
            }
        } else if (auto *oev = dynamic_cast<rs_knl::ObjEvacuate*>(obj)) {
            renderSingleObjective(num, 0x10, 0, 0, oev->location().x, oev->location().y, oev->location().z);
        }
    }

    // Render live objective preview if active (either for editing an existing objective or adding a new one)
    if (objPreviewDesc_.active) {
        int previewNum = (objPreviewDesc_.editingIndex >= 0) ?
            (objPreviewDesc_.editingIndex + 1) :
            static_cast<int>(pMission_->numObjectives() + 1);
        renderSingleObjective(previewNum, objPreviewDesc_.type, objPreviewDesc_.targetPedId,
                              objPreviewDesc_.targetVehicleIdx, objPreviewDesc_.evacX,
                              objPreviewDesc_.evacY, objPreviewDesc_.evacZ);
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hBadgeFont);
}

void WinMapViewport::drawEntityStatsOverlay(HDC hdc) {
    if (!pMission_ || !pMission_->map()) return;

    HFONT hFont = CreateFontW(
        -10, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
    SetBkMode(hdc, TRANSPARENT);

    struct StatBadge {
        int anchorX;
        int anchorY;
        std::wstring text;
        COLORREF borderColor;
        COLORREF bgColor;
        COLORREF textColor;
    };
    std::vector<StatBadge> badges;

    const int margin = 80;

    auto getVehicleTypeName = [](uint8_t type) -> std::wstring {
        if (type == rs_knl::Vehicle::kVehicleTypeRegularCar) return L"Car";
        if (type == rs_knl::Vehicle::kVehicleTypeSmallArmored) return L"Armored Car";
        if (type == rs_knl::Vehicle::kVehicleTypeLargeArmored ||
            type == rs_knl::Vehicle::kVehicleTypeLargeArmoredDamaged) return L"Large Armored";
        if (type == rs_knl::Vehicle::kVehicleTypePolice) return L"Police Car";
        if (type == rs_knl::Vehicle::kVehicleTypeFireFighter) return L"Firetruck";
        if (type == rs_knl::Vehicle::kVehicleTypeMedics) return L"Ambulance";
        if (type == rs_knl::Vehicle::kVehicleTypeTrainHead) return L"Train Engine";
        if (type == rs_knl::Vehicle::kVehicleTypeTrainBody) return L"Train Carriage";
        return L"Vehicle";
    };

    auto getWeaponTypeName = [](rs_knl::Weapon::WeaponType type) -> std::wstring {
        switch (type) {
            case rs_knl::Weapon::Pistol:       return L"Pistol";
            case rs_knl::Weapon::Shotgun:      return L"Shotgun";
            case rs_knl::Weapon::Uzi:          return L"Uzi";
            case rs_knl::Weapon::Minigun:      return L"Minigun";
            case rs_knl::Weapon::Laser:        return L"Laser Gun";
            case rs_knl::Weapon::Flamer:       return L"Flamer";
            case rs_knl::Weapon::LongRange:    return L"Long Range";
            case rs_knl::Weapon::GaussGun:     return L"Gauss Gun";
            case rs_knl::Weapon::Scanner:      return L"Scanner";
            case rs_knl::Weapon::MediKit:      return L"Medikit";
            case rs_knl::Weapon::TimeBomb:     return L"Time Bomb";
            case rs_knl::Weapon::AccessCard:   return L"Access Card";
            case rs_knl::Weapon::EnergyShield: return L"Energy Shield";
            case rs_knl::Weapon::Persuadatron: return L"Persuadertron";
            case rs_knl::Weapon::AirStrike:    return L"Air Raid Com";
            case rs_knl::Weapon::CloneShield:  return L"Clone Shield";
            default:                           return L"Weapon";
        }
    };

    auto getWeaponName = [&](rs_knl::WeaponInstance *w) -> std::wstring {
        if (!w) return L"Weapon";
        if (w->getClass() && w->getClass()->getName()) {
            std::string n = w->getClass()->getName();
            if (!n.empty() && n != "?") {
                return std::wstring(n.begin(), n.end());
            }
        }
        return getWeaponTypeName(w->type());
    };

    // 1. Vehicles
    if (filterVehicles_) {
        for (size_t i = 0; i < pMission_->numVehicles(); ++i) {
            rs_knl::Vehicle *v = pMission_->vehicle(i);
            if (!v) continue;
            if (isolateActiveZ_ && v->tileZ() > activeZ_) continue;

            POINT pt = entityPointToScreen(v->position());
            if (pt.x < -margin || pt.x > dibWidth_ + margin || pt.y < -margin || pt.y > dibHeight_ + margin) continue;

            std::wstring typeName = getVehicleTypeName(v->getType());
            std::wstring title = std::format(L"{} #{}", typeName, v->id());
            int hp = v->health();

            std::wstring hpStr;
            if (hp <= 0) {
                hpStr = L"HP: 0 [BURNT]";
            } else {
                hpStr = std::format(L"HP: {}", hp);
            }

            std::wstring fullText = title + L"\n" + hpStr;
            if (!v->passengers().empty()) {
                fullText += std::format(L" [Pass: {}]", v->passengers().size());
            }

            COLORREF borderCol = RGB(245, 185, 40); // Amber
            COLORREF bgCol = RGB(16, 22, 30);
            COLORREF textCol = RGB(255, 235, 180);

            badges.push_back({ pt.x, pt.y - static_cast<int>(18 * zoom_), fullText, borderCol, bgCol, textCol });
        }
    }

    // 2. Peds
    for (size_t i = 0; i < pMission_->numPeds(); ++i) {
        rs_knl::PedInstance *p = pMission_->ped(i);
        if (!p || !isPedVisible(p)) continue;
        if (isolateActiveZ_ && p->tileZ() > activeZ_) continue;

        rs_knl::Vehicle *assignedVeh = findAssignedVehicleForPed(p);
        if (assignedVeh && (!isDraggingEntity_ || targetSelected_ != p)) {
            p->setPosition(assignedVeh->position());
        }

        POINT pt = entityPointToScreen(p->position());
        if (pt.x < -margin || pt.x > dibWidth_ + margin || pt.y < -margin || pt.y > dibHeight_ + margin) continue;

        std::wstring typeStr;
        COLORREF borderCol = RGB(140, 180, 200);

        if (p->isOurAgent()) {
            typeStr = std::format(L"Player Agent #{}", p->id() + 1);
            borderCol = RGB(40, 210, 255); // Cyan
        } else if (p->type() == rs_knl::PedInstance::kPedTypeAgent) {
            const wchar_t *fStr = L"Blue";
            int fid = p->factionId();
            if (fid == 2) fStr = L"Orange";
            else if (fid == 3 || fid == 4) fStr = L"Yellow";
            else if (fid == 5) fStr = L"Purple";
            else if (fid == 6) fStr = L"Grey";
            else if (fid == 7) fStr = L"Cyan";
            typeStr = std::format(L"Enemy Agent ({}) #{}", fStr, p->id());
            borderCol = RGB(255, 60, 60); // Red
        } else if (p->type() == rs_knl::PedInstance::kPedTypePolice) {
            typeStr = std::format(L"Police #{}", p->id());
            borderCol = RGB(70, 130, 255); // Blue
        } else if (p->type() == rs_knl::PedInstance::kPedTypeGuard) {
            typeStr = std::format(L"Guard #{}", p->id());
            borderCol = RGB(200, 80, 220); // Purple
        } else if (p->type() == rs_knl::PedInstance::kPedTypeCriminal) {
            typeStr = std::format(L"Criminal #{}", p->id());
            borderCol = RGB(255, 130, 30); // Orange
        } else {
            typeStr = std::format(L"Civilian #{}", p->id());
            borderCol = RGB(140, 180, 200); // Slate
        }

        if (!p->isAlive() || p->health() <= 0) {
            typeStr += L" [DEAD]";
        }

        int hp = p->health();
        std::wstring hpStr = std::format(L"HP: {}", hp);

        std::wstring wpnStr;
        if (!p->isOurAgent() && p->numWeapons() > 0) {
            for (size_t wIdx = 0; wIdx < p->numWeapons(); ++wIdx) {
                rs_knl::WeaponInstance *w = p->weapon(wIdx);
                if (!w) continue;
                if (!wpnStr.empty()) wpnStr += L", ";
                wpnStr += getWeaponName(w);
            }
        }

        std::wstring fullText = typeStr + L"\n" + hpStr;
        if (!wpnStr.empty()) {
            fullText += L"\n" + wpnStr;
        }

        COLORREF bgCol = RGB(14, 20, 28);
        COLORREF textCol = RGB(240, 245, 250);

        badges.push_back({ pt.x, pt.y - static_cast<int>(20 * zoom_), fullText, borderCol, bgCol, textCol });
    }

    // 3. Ground Weapons
    if (filterWeapons_) {
        for (size_t i = 0; i < pMission_->numWeaponsOnGround(); ++i) {
            rs_knl::WeaponInstance *w = pMission_->weaponOnGround(i);
            if (!w) continue;
            if (isolateActiveZ_ && w->tileZ() > activeZ_) continue;

            POINT pt = entityPointToScreen(w->position());
            if (pt.x < -margin || pt.x > dibWidth_ + margin || pt.y < -margin || pt.y > dibHeight_ + margin) continue;

            std::wstring text = getWeaponName(w);

            COLORREF borderCol = RGB(50, 220, 110); // Emerald green
            COLORREF bgCol = RGB(14, 24, 20);
            COLORREF textCol = RGB(200, 255, 220);

            badges.push_back({ pt.x, pt.y - static_cast<int>(12 * zoom_), text, borderCol, bgCol, textCol });
        }
    }

    // Render badges with deconfliction
    std::vector<RECT> placedRects;

    for (const auto &b : badges) {
        RECT rcCalc = { 0, 0, 0, 0 };
        DrawTextW(hdc, b.text.c_str(), -1, &rcCalc, DT_CALCRECT | DT_NOPREFIX);
        int padX = 6;
        int padY = 4;
        int bw = (rcCalc.right - rcCalc.left) + padX * 2;
        int bh = (rcCalc.bottom - rcCalc.top) + padY * 2;

        int bx = b.anchorX - (bw / 2);
        int by = b.anchorY - bh;

        RECT badgeRc = { bx, by, bx + bw, by + bh };

        bool overlaps = true;
        int deconflictAttempts = 0;
        while (overlaps && deconflictAttempts < 10) {
            overlaps = false;
            for (const auto &placed : placedRects) {
                RECT isect;
                if (IntersectRect(&isect, &badgeRc, &placed)) {
                    int overlapShift = (placed.bottom - badgeRc.top) + 2;
                    if (overlapShift < 4) overlapShift = 4;
                    badgeRc.top -= overlapShift;
                    badgeRc.bottom -= overlapShift;
                    overlaps = true;
                    ++deconflictAttempts;
                    break;
                }
            }
        }
        placedRects.push_back(badgeRc);

        HBRUSH hBg = CreateSolidBrush(b.bgColor);
        HPEN hBorder = CreatePen(PS_SOLID, 1, b.borderColor);
        HBRUSH hOldBr = static_cast<HBRUSH>(SelectObject(hdc, hBg));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hBorder));

        RoundRect(hdc, badgeRc.left, badgeRc.top, badgeRc.right, badgeRc.bottom, 6, 6);

        SetTextColor(hdc, b.textColor);
        RECT rcText = { badgeRc.left + padX, badgeRc.top + padY, badgeRc.right - padX, badgeRc.bottom - padY };
        DrawTextW(hdc, b.text.c_str(), -1, &rcText, DT_NOPREFIX | DT_LEFT);

        SelectObject(hdc, hOldPen);
        DeleteObject(hBorder);
        SelectObject(hdc, hOldBr);
        DeleteObject(hBg);
    }

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

} // namespace rs_win_editor
