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

#ifndef EDITOR_MAP_VIEWPORT_H_
#define EDITOR_MAP_VIEWPORT_H_

#include <windows.h>
#include <cstdint>
#include <string>
#include <functional>
#include <memory>
#include <vector>

#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/map.h"
#include "rs-kernel/model/position.h"
#include "rs-kernel/model/weapon.h"
#include "editor_commands.h"

namespace rs_win_editor {

enum class ViewportTool {
    Pencil = 0,
    BoxFill,
    SelectTile,
    SelectEntity,
    PlaceActor,
    PlaceVehicle,
    PlaceStatic,
    PlaceWeapon,
    RouteTool
};

enum class ObjectivePickMode {
    None,
    Ped,
    Vehicle,
    Coordinate
};

struct ObjectivePickResult {
    bool accepted = false;
    uint16_t pedId = 0;
    uint16_t vehicleIdx = 0;
    rs_knl::TilePoint tilePos;
};

using ObjectivePickCallback = std::function<void(const ObjectivePickResult&)>;

struct ObjectivePreviewDesc {
    bool active = false;
    int editingIndex = -1; // -1 for new objective, >= 0 for existing objective index
    uint16_t type = 0x01;
    uint16_t targetPedId = 0;
    uint16_t targetVehicleIdx = 0;
    int evacX = 0;
    int evacY = 0;
    int evacZ = 0;
};

class WinMapViewport {
public:
    WinMapViewport();
    ~WinMapViewport();

    bool create(HWND hWndParent, int x, int y, int w, int h, int id);
    HWND hwnd() const { return hWnd_; }

    void setMission(rs_knl::Mission *pMission, int missionId);
    rs_knl::Mission* mission() const { return pMission_; }

    void setCommandStack(EditorCommandStack *pStack) { pCommandStack_ = pStack; }
    EditorCommandStack* commandStack() const { return pCommandStack_; }

    // Tool & Mode settings
    ViewportTool tool() const { return tool_; }
    void setTool(ViewportTool t);

    uint8_t activeTileId() const { return activeTileId_; }
    void setActiveTileId(uint8_t id) { activeTileId_ = id; }

    int activeZ() const { return activeZ_; }
    void setActiveZ(int z);

    int explicitSelectedZ() const { return explicitSelectedZ_; }
    bool isAutoLayerSwitchingEnabled() const { return autoLayerSwitchingEnabled_; }
    void setAutoLayerSwitchingEnabled(bool enable) {
        autoLayerSwitchingEnabled_ = enable;
        if (enable) manualLayerOverride_ = false;
        if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE);
    }
    void toggleAutoLayerSwitching() {
        setAutoLayerSwitchingEnabled(!autoLayerSwitchingEnabled_);
    }
    bool isManualLayerOverride() const { return manualLayerOverride_; }
    void setManualLayerOverride(bool manual) { manualLayerOverride_ = manual; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    bool isSliceView() const { return isolateActiveZ_; }
    void setSliceView(bool isolate);

    // Stats overlay
    bool isShowStats() const { return showStats_; }
    void setShowStats(bool s) { showStats_ = s; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    // View filters
    bool isFilterBuildings() const { return filterBuildings_; }
    void setFilterBuildings(bool f) { filterBuildings_ = f; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    bool isFilterVehicles() const { return filterVehicles_; }
    void setFilterVehicles(bool f) { filterVehicles_ = f; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    bool isFilterCivilians() const { return filterCivilians_; }
    void setFilterCivilians(bool f) { filterCivilians_ = f; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    bool isFilterPolice() const { return filterPolice_; }
    void setFilterPolice(bool f) { filterPolice_ = f; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    bool isFilterGuards() const { return filterGuards_; }
    void setFilterGuards(bool f) { filterGuards_ = f; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    bool isFilterCriminals() const { return filterCriminals_; }
    void setFilterCriminals(bool f) { filterCriminals_ = f; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    bool isFilterAgents() const { return filterAgents_; }
    void setFilterAgents(bool f) { filterAgents_ = f; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    bool isFilterWeapons() const { return filterWeapons_; }
    void setFilterWeapons(bool f) { filterWeapons_ = f; if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE); }

    void selectAllFilters(bool enable) {
        filterBuildings_ = enable;
        filterVehicles_ = enable;
        filterCivilians_ = enable;
        filterPolice_ = enable;
        filterGuards_ = enable;
        filterCriminals_ = enable;
        filterAgents_ = enable;
        filterWeapons_ = enable;
        if (hWnd_) InvalidateRect(hWnd_, nullptr, FALSE);
    }

    bool isPedVisible(const rs_knl::PedInstance *p) const;
    bool isBuildingOrStructureTile(rs_knl::Map *pMap, int tx, int ty, int tz, uint8_t tileId) const;

    float zoom() const { return zoom_; }
    void setZoom(float z);
    void zoomIn();
    void zoomOut();

    void centerOnSquadSpawn();
    void centerOnTile(const rs_knl::TilePoint &targetPt);
    void centerOnEntity(rs_knl::ShootableMapObject *pObj) { if (pObj) centerOnTile(pObj->position()); }

    // Callbacks for status updates
    using StatusCallback = std::function<void(const std::string &coords, const std::string &tileInfo, const std::string &hoverInfo)>;
    void setStatusCallback(StatusCallback cb) { statusCallback_ = cb; }

    using TileSelectedCallback = std::function<void(uint8_t tileId)>;
    void setTileSelectedCallback(TileSelectedCallback cb) { tileSelectedCallback_ = cb; }

    using EntitySelectedCallback = std::function<void(rs_knl::ShootableMapObject *pObj)>;
    void setEntitySelectedCallback(EntitySelectedCallback cb) { entitySelectedCallback_ = cb; }
    rs_knl::ShootableMapObject* selectedEntity() const { return targetSelected_; }
    bool hasSelectedEntity() const { return targetSelected_ != nullptr; }
    rs_knl::PedInstance* getActiveRoutePed() const;
    void setSelectedEntity(rs_knl::ShootableMapObject *pObj) {
        targetSelected_ = pObj;
        if (targetSelected_) hasSelectedTile_ = false;
        InvalidateRect(hWnd_, nullptr, FALSE);
    }

    void clearEntityReferences(rs_knl::ShootableMapObject *pObj) {
        if (!pObj) return;
        if (targetSelected_ == pObj) {
            targetSelected_ = nullptr;
        }
        if (targetHovered_ == pObj) {
            targetHovered_ = nullptr;
        }
        if (lastOverlappingEntity_ == pObj) {
            lastOverlappingEntity_ = nullptr;
        }
        if (highlightedSearchEntity_ == pObj) {
            highlightedSearchEntity_ = nullptr;
        }
    }

    bool hasSelectedTile() const { return hasSelectedTile_; }
    rs_knl::TilePoint selectedTile() const { return selectedTilePos_; }
    void setSelectedTile(const rs_knl::TilePoint &tp) {
        hasSelectedTile_ = true;
        selectedTilePos_ = tp;
        targetSelected_ = nullptr;
        if (entitySelectedCallback_) {
            entitySelectedCallback_(nullptr);
        }
        InvalidateRect(hWnd_, nullptr, FALSE);
    }
    void clearSelection() {
        bool hadEntity = (targetSelected_ != nullptr);
        bool hadTile = hasSelectedTile_;
        targetSelected_ = nullptr;
        hasSelectedTile_ = false;
        lastOverlappingEntity_ = nullptr;
        clearSearchHighlight();
        if (hadEntity && entitySelectedCallback_) {
            entitySelectedCallback_(nullptr);
        }
        if (hadEntity || hadTile) {
            InvalidateRect(hWnd_, nullptr, FALSE);
        }
    }

    void highlightSearchEntity(rs_knl::ShootableMapObject *pObj);
    void highlightSearchTile(const rs_knl::TilePoint &tp);
    void clearSearchHighlight();
    rs_knl::ShootableMapObject* highlightedSearchEntity() const { return highlightedSearchEntity_; }
    bool hasHighlightedSearchTile() const { return hasHighlightedSearchTile_; }
    rs_knl::TilePoint highlightedSearchTile() const { return highlightedSearchTilePos_; }

    void toggleSelection();
    void deleteSelection();
    void moveSelectionZ(int deltaZ);
    void stepSelectionZ(int deltaZ);

    using CommandExecutedCallback = std::function<void()>;
    void setCommandExecutedCallback(CommandExecutedCallback cb) { commandExecutedCallback_ = cb; }
    void notifyCommandExecuted() { if (commandExecutedCallback_) commandExecutedCallback_(); }

    using ToolChangedCallback = std::function<void(ViewportTool tool)>;
    void setToolChangedCallback(ToolChangedCallback cb) { toolChangedCallback_ = cb; }

    // Objective Picking & Live Preview Mode
    void beginObjectivePicking(ObjectivePickMode mode, ObjectivePickCallback cb, const std::wstring &customPrompt = L"");
    void cancelObjectivePicking();
    void finishObjectivePicking(const ObjectivePickResult &res);
    bool isObjectivePicking() const { return objectivePickMode_ != ObjectivePickMode::None; }
    ObjectivePickMode objectivePickMode() const { return objectivePickMode_; }
    void setObjectivePreview(int editingIndex, uint16_t type, uint16_t targetPedId, uint16_t targetVehicleIdx, int evacX, int evacY, int evacZ);
    void clearObjectivePreview();
    bool isObjectivePreviewActive() const { return objPreviewDesc_.active; }

    // Placement template parameters
    void setActorPlacement(rs_knl::PedInstance::PedType type, uint8_t orientation, uint16_t baseAnim = 0,
                           int health = 0, const std::vector<int> &weapons = {},
                           bool cyberwareOverridden = false, int cyberwareTier = 0,
                           uint8_t movementMode = 0x10, int factionId = -1,
                           bool isOurAgent = false, int squadSlot = -1) {
        actorTypeToPlace_ = type;
        orientationToPlace_ = orientation;
        actorBaseAnimToPlace_ = baseAnim;
        actorHealthToPlace_ = health;
        actorWeaponsToPlace_ = weapons;
        actorCyberwareOverriddenToPlace_ = cyberwareOverridden;
        actorCyberwareTierToPlace_ = cyberwareTier;
        actorMovementModeToPlace_ = movementMode;
        actorFactionIdToPlace_ = factionId;
        actorIsOurAgentToPlace_ = isOurAgent;
        actorSquadSlotToPlace_ = squadSlot;
    }
    void setVehiclePlacement(uint8_t type, uint8_t orientation, int health = 0, int driver = 0, uint16_t driverBaseAnim = 0, uint16_t baseAnim = 0) {
        vehicleTypeToPlace_ = type;
        orientationToPlace_ = orientation;
        vehicleHealthToPlace_ = health;
        vehicleDriverToPlace_ = driver;
        vehicleDriverBaseAnimToPlace_ = driverBaseAnim;
        vehicleBaseAnimToPlace_ = baseAnim;
    }
    void setStaticPlacement(uint16_t staticId, uint16_t baseAnim = 0, uint8_t orientation = 0) {
        staticIdToPlace_ = staticId;
        staticBaseAnimToPlace_ = baseAnim;
        orientationToPlace_ = orientation;
    }
    void setWeaponPlacement(rs_knl::Weapon::WeaponType wType) {
        weaponTypeToPlace_ = wType;
    }
    rs_knl::Weapon::WeaponType weaponTypeToPlace() const {
        return weaponTypeToPlace_;
    }

    // Static route & vehicle state helpers
    static rs_knl::Vehicle* findAssignedVehicleForPed(rs_knl::Mission *pMission, rs_knl::PedInstance *p);
    static rs_knl::Vehicle* getPedVehicleAtStep(rs_knl::Mission *pMission, rs_knl::PedInstance *p, size_t stepLimit);

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void onPaint(HDC hdc);
    void onResize(int w, int h);
    void onMouseMove(int x, int y, UINT flags);
    void onMouseDown(int x, int y, int button);
    void onMouseUp(int x, int y, int button);
    void onMouseWheel(int delta, int x, int y);
    void onKeyDown(WPARAM wParam, LPARAM lParam);

    void recreateDIB(int w, int h);
    void renderMapToDIB();
    rs_knl::TilePoint screenToMapPoint(int sx, int sy) const;
    rs_knl::TilePoint screenToMapPointAtZ(int sx, int sy, int targetZ) const;
    rs_knl::TilePoint resolveWalkableTilePoint(int sx, int sy, int referenceZ, bool isVehicle = false) const;
    int getReferenceZForRouteStep(rs_knl::PedInstance *pPed, size_t stepIdx, rs_knl::Vehicle *veh) const;
    rs_knl::ShootableMapObject* hitTestEntityAtScreen(int sx, int sy) const;
    std::vector<rs_knl::ShootableMapObject*> hitTestAllEntitiesAtScreen(int sx, int sy) const;
    rs_knl::TilePoint pickTileAtScreen(int sx, int sy) const;
    bool findTileAtScreen(int sx, int sy, rs_knl::TilePoint *outTile = nullptr) const;
    POINT mapPointToScreen(const rs_knl::TilePoint &tp) const;
    POINT entityPointToScreen(const rs_knl::TilePoint &tp) const;

    void drawDiamondOutline(uint32_t *buf, int bw, int bh, int sx, int sy, uint32_t color);
    void drawDiamondOutlineAlpha(uint32_t *buf, int bw, int bh, int sx, int sy, uint32_t color, uint8_t alpha);
    void drawDashedDiamondOutline(uint32_t *buf, int bw, int bh, int sx, int sy, uint32_t color);
    void drawSmallDashedDiamond(uint32_t *buf, int bw, int bh, int sx, int sy, uint32_t color);
    void drawBoxFillOutline(uint32_t *buf, int bw, int bh);
    bool getEntityPlacementAnim(uint16_t &animId, int &frameId, int &heightPx) const;
    static bool isEntityPlaceTool(ViewportTool t) {
        return (t == ViewportTool::PlaceActor ||
                t == ViewportTool::PlaceVehicle ||
                t == ViewportTool::PlaceStatic ||
                t == ViewportTool::PlaceWeapon);
    }
    static bool isPlacementTool(ViewportTool t) {
        return (t == ViewportTool::Pencil ||
                t == ViewportTool::BoxFill ||
                isEntityPlaceTool(t));
    }
    void draw3DBoundingBox(uint32_t *buf, int bw, int bh, const rs_knl::TilePoint &pos, int heightPx, uint32_t wireColor, bool extendsUpward = true);
    void drawHoverWiregridColumn(uint32_t *buf, int bw, int bh, const rs_knl::TilePoint &pos);
    void drawSelectionBadges(HDC hdcMem);
    void drawRouteOverlay(HDC hdcMem);
    void drawObjectiveAnnotations(HDC hdcMem);
    void drawObjectivePickOverlay(HDC hdc);
    void drawEntityStatsOverlay(HDC hdcMem);
    int hitTestWaypointAtScreen(int sx, int sy, const std::vector<rs_knl::ScriptStep> &steps) const;

    int getHighestRoofZ(int tx, int ty, int tz) const;
    bool isEntityForegroundObscured(const rs_knl::MapObject *obj, int drawTx, int drawTy, int drawZ) const;
    rs_knl::Vehicle* findAssignedVehicleForPed(rs_knl::PedInstance *p) const;
    int getHighestTileZ(int tx, int ty) const;

    struct ObscuredOverlayItem {
        const rs_knl::MapObject *obj = nullptr;
        int groundAnchorX = 0;
        int groundAnchorY = 0;
        int roofAnchorX = 0;
        int roofAnchorY = 0;
        int groundZ = 0;
        int roofZ = 0;
        uint32_t outlineCol = 0;
        bool isSelected = false;
        bool isHovered = false;
        bool isVehicle = false;
        int squadSpawnIdx = -1;
    };

    std::vector<ObscuredOverlayItem> obscuredOverlays_;
    HWND hWnd_ = nullptr;
    rs_knl::Mission *pMission_ = nullptr;
    int missionId_ = 1;
    EditorCommandStack *pCommandStack_ = nullptr;

    // DIB Section
    HBITMAP hDIB_ = nullptr;
    uint32_t *pDIBBits_ = nullptr;
    int dibWidth_ = 0;
    int dibHeight_ = 0;

    // Camera & Zoom
    int cameraX_ = 0;
    int cameraY_ = 0;
    float zoom_ = 1.0f;
    int activeZ_ = 0;
    int explicitSelectedZ_ = 0;
    bool autoLayerSwitchingEnabled_ = true;
    bool manualLayerOverride_ = false;
    bool isolateActiveZ_ = false;

    // Filters & Stats display
    bool showStats_ = false;
    bool filterBuildings_ = true;
    bool filterVehicles_ = true;
    bool filterCivilians_ = true;
    bool filterPolice_ = true;
    bool filterGuards_ = true;
    bool filterCriminals_ = true;
    bool filterAgents_ = true;
    bool filterWeapons_ = true;

    // Panning state
    bool isPanning_ = false;
    POINT panStartMouse_ = {0, 0};
    int panStartCamX_ = 0;
    int panStartCamY_ = 0;

    // Editing tools
    ViewportTool tool_ = ViewportTool::SelectTile;
    uint8_t activeTileId_ = 0;
    bool isPainting_ = false;
    bool isBoxFilling_ = false;
    rs_knl::TilePoint boxFillStart_;
    rs_knl::TilePoint boxFillCurrent_;

    // Spawning parameters
    rs_knl::PedInstance::PedType actorTypeToPlace_ = rs_knl::PedInstance::kPedTypeCivilian;
    uint16_t actorBaseAnimToPlace_ = 0;
    int actorHealthToPlace_ = 0;
    std::vector<int> actorWeaponsToPlace_;
    bool actorCyberwareOverriddenToPlace_ = false;
    int actorCyberwareTierToPlace_ = 0;
    uint8_t actorMovementModeToPlace_ = 0x10;
    int actorFactionIdToPlace_ = -1;
    bool actorIsOurAgentToPlace_ = false;
    int actorSquadSlotToPlace_ = -1;
    uint8_t vehicleTypeToPlace_ = 0;
    int vehicleHealthToPlace_ = 0;
    int vehicleDriverToPlace_ = 0;
    uint16_t vehicleDriverBaseAnimToPlace_ = 0;
    uint16_t vehicleBaseAnimToPlace_ = 0;
    uint16_t staticIdToPlace_ = 1;
    uint16_t staticBaseAnimToPlace_ = 0;
    uint8_t orientationToPlace_ = 0;
    rs_knl::Weapon::WeaponType weaponTypeToPlace_ = rs_knl::Weapon::Pistol;

    // Hover, Selection & Dragging
    rs_knl::TilePoint currentTilePos_;
    rs_knl::ShootableMapObject *targetHovered_ = nullptr;
    rs_knl::ShootableMapObject *targetSelected_ = nullptr;
    bool isDraggingEntity_ = false;
    bool hasDraggedEntity_ = false;
    POINT dragStartMouse_ = {0, 0};
    rs_knl::TilePoint dragStartMouseTile_;
    rs_knl::TilePoint dragEntityStartTile_;

    // Tile Selection & Dragging
    bool hasSelectedTile_ = false;
    rs_knl::TilePoint selectedTilePos_;
    rs_knl::ShootableMapObject *lastOverlappingEntity_ = nullptr;
    rs_knl::TilePoint lastOverlappingTilePos_;

    // Search Results Highlighting
    rs_knl::ShootableMapObject *highlightedSearchEntity_ = nullptr;
    bool hasHighlightedSearchTile_ = false;
    rs_knl::TilePoint highlightedSearchTilePos_;

    bool isDraggingTile_ = false;
    bool hasDraggedTile_ = false;
    POINT dragTileStartMouse_ = {0, 0};
    rs_knl::TilePoint dragTileStartMouseTile_;
    rs_knl::TilePoint dragTileStartPos_;
    rs_knl::TilePoint dragTileCurrentPos_;

    // Waypoint & Route Editing
    int hoveredWaypointIdx_ = -1;
    int draggingWaypointIdx_ = -1;
    bool isDraggingWaypoint_ = false;
    rs_knl::TilePoint dragWaypointStartPos_;
    rs_knl::TilePoint dragWaypointCurrentPos_;

    // Objective Picking State
    ObjectivePickMode objectivePickMode_ = ObjectivePickMode::None;
    ObjectivePickCallback objectivePickCallback_;
    std::wstring customPickPrompt_;
    RECT pickCancelBtnRect_ = { 0, 0, 0, 0 };
    bool isHoveringPickCancel_ = false;
    ObjectivePreviewDesc objPreviewDesc_;
    ViewportTool toolBeforeObjectivePick_ = ViewportTool::SelectTile;
    bool hasSavedToolBeforePick_ = false;

    // Callbacks
    StatusCallback statusCallback_;
    TileSelectedCallback tileSelectedCallback_;
    EntitySelectedCallback entitySelectedCallback_;
    CommandExecutedCallback commandExecutedCallback_;
    ToolChangedCallback toolChangedCallback_;
};

} // namespace rs_win_editor

#endif // EDITOR_MAP_VIEWPORT_H_
