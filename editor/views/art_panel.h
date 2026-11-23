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

#ifndef EDITOR_ART_PANEL_H_
#define EDITOR_ART_PANEL_H_

#include <windows.h>
#include <commctrl.h>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

#include "map_viewport.h"
#include "tile_rasterizer.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/vehicle.h"

namespace rs_win_editor {

class WinArtPanel {
public:
    WinArtPanel();
    ~WinArtPanel();

    bool create(HWND hWndParent, int x, int y, int w, int h, int id);
    HWND hwnd() const { return hWnd_; }

    void onResize(int x, int y, int w, int h);
    void refreshPalette();

    uint8_t activeTileId() const { return activeTileId_; }
    void setActiveTileId(uint8_t id);

    void setViewport(WinMapViewport *pViewport) { pViewport_ = pViewport; }
    void setMission(rs_knl::Mission *pMission, int missionId = 0);

    void updateProperties(const std::string &info);
    void setActiveTab(int tabIndex);
    int currentTab() const { return currentTab_; }
    void updateToolSelection();

    using TabChangedCallback = std::function<void(int)>;
    void setTabChangedCallback(TabChangedCallback cb) { tabChangedCallback_ = std::move(cb); }
    void setOnSplitterChangedCallback(std::function<void()> cb) { onSplitterChangedCallback_ = std::move(cb); }
    int entityGridHeight() const { return entityGridHeight_; }
    int getClampedEntityGridHeight(int clientHeight) const;
    void setEntityGridHeight(int h);
    int tileGridHeight() const { return tileGridHeight_; }
    int getClampedTileGridHeight(int clientHeight) const;
    void setTileGridHeight(int h);

    void activatePencilTool();
    void activateBoxFillTool();
    void activateSelectTool();
    void activateEntityPlaceTool();
    void activateSearchTool();
    void refreshTileInspector();

    bool isTileSearchActive() const { return isTileSearchActive_; }
    bool isEntitySearchActive() const { return isEntitySearchActive_; }
    void performTileSearch(uint8_t tileId);
    void performEntitySearch(int templateIdx);

    void selectEntity(rs_knl::ShootableMapObject *pObj);
    void clearSelectedEntity();
    rs_knl::ShootableMapObject* selectedEntity() const { return pSelectedEntity_; }
    void refreshObjectivesList();
    void refreshInspector();
    void populateFactionCombo(bool isPlayerAgent, int factionId);
    void refreshRouteUI();
    void applyInspectorChanges();
    void revertInspectorChanges();
    bool confirmDiscardEntityEdits();

    void refreshActionsList();
    void refreshWeaponsGrid();
    void toggleWeapon(int weaponIndex);
    const std::set<int>& selectedWeapons() const { return selectedWeapons_; }
    int hoveredWeaponIndex() const { return hoveredWeaponIdx_; }
    void setHoveredWeaponIndex(int idx) { hoveredWeaponIdx_ = idx; }
    void setWeaponsLabelText(const std::wstring &text);
    void refreshWeaponIcons();
    void cleanupWeaponIcons();
    HBITMAP getWeaponIcon(int index) const;

    enum class EntityKind {
        Actor,
        Vehicle,
        Static,
        Weapon
    };

    struct EntityItem {
        std::string name;
        std::wstring shortLabel;
        int typeId;
        uint16_t baseAnim;
        uint16_t drawAnim;
        int defaultHealth;
        EntityKind kind;
        mutable HBITMAP hThumb = nullptr;
    };

    const std::vector<const EntityItem*>& getEntityItemsForCategory(int category) const;
    const EntityItem* selectedEntityTemplateItem() const;

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK TileGridProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EntityGridProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK SearchResultsProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK InspectPaneProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EntityPreviewProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    LRESULT handleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleTileGridMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleEntityGridMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleSearchResultsMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleInspectPaneMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleEntityPreviewMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void drawEntityPreview(HDC hdc);
    void drawTilePreview(HDC hdc);
    void updateEntityPreview();
    void rotatePreviewEntity(int step);
    void togglePreviewAnimation();

    void createChildControls();
    void layoutControls();
    void layoutTileInspectPane();
    void layoutEntInspectPane();
    void updateTabVisibility(int tabIndex);
    void applyEntityPlacementFromSelection();
    void setSelectedEntityGridIndex(int idx);
    void updateTileGridScroll();
    int getTileGridColumns() const;
    void drawTileGrid(HDC hdc);

    void updateEntityGridScroll();
    int getEntityGridColumns() const;
    void drawEntityGrid(HDC hdc);
    void refreshEntityThumbnails();
    void cleanupEntityThumbnails();
    void updateEntityTypeComboLabels();

    void markEntityDirty();
    void markEntityClean();
    void populateDriverCombo(rs_knl::Vehicle *pVeh = nullptr);
    void populateCyberwareCombo(HWND hCombo, bool isOverridden, int overrideTier);

public:
    struct SearchRow {
        std::wstring idStr;
        std::wstring typeStr;
        std::wstring coordStr;
        bool hasTypeColor = false;
        COLORREF typeColor = 0;
        rs_knl::TilePoint pos;
        rs_knl::ShootableMapObject *pEntity = nullptr;
    };

private:
    HWND hWnd_ = nullptr;
    HWND hTabControl_ = nullptr;
    HWND hToolTip_ = nullptr;

    // Tab 0: Tiles
    HWND hComboCategory_ = nullptr;
    HWND hBtnPencil_ = nullptr;
    HWND hBtnBoxFill_ = nullptr;
    HWND hBtnTileSelect_ = nullptr;
    HWND hBtnTileSearch_ = nullptr;
    HWND hTileGrid_ = nullptr;

    // Tab 0 Lower Pane: Tile Inspector
    HWND hTileInspectPane_ = nullptr;
    int tileInspectScrollY_ = 0;
    HWND hTilePreviewBox_ = nullptr;
    HWND hTileInspectStaticType_ = nullptr;
    HWND hTileInspectValueType_ = nullptr;
    HWND hTileInspectStaticSurface_ = nullptr;
    HWND hTileInspectValueSurface_ = nullptr;
    HWND hTileInspectStaticPos_ = nullptr;
    HWND hTileInspectValuePos_ = nullptr;
    HWND hTileInspectStaticPlacements_ = nullptr;
    HWND hTileInspectValuePlacements_ = nullptr;

    // Tab 1: Entities
    HWND hComboEntityType_ = nullptr;
    HWND hBtnEntPlace_ = nullptr;
    HWND hBtnEntSelect_ = nullptr;
    HWND hBtnEntSearch_ = nullptr;
    HWND hEntityGrid_ = nullptr;

    // Search Result Table Windows
    HWND hTileSearchResults_ = nullptr;
    HWND hEntSearchResults_ = nullptr;

    // Tab 1 Lower Pane: Entity Inspector
    HWND hEntInspectPane_ = nullptr;
    int entInspectScrollY_ = 0;
    HWND hEditProperties_ = nullptr; // Fallback / mission summary when no entity selected
    HWND hInspectHeader_ = nullptr;
    HWND hInspectStaticType_ = nullptr;
    HWND hInspectValueType_ = nullptr;
    HWND hInspectComboType_ = nullptr;
    HWND hInspectStaticHealth_ = nullptr;
    HWND hInspectEditHealth_ = nullptr;
    HWND hInspectCheckDead_ = nullptr;
    HWND hInspectStaticOrient_ = nullptr;
    HWND hInspectComboOrient_ = nullptr;
    HWND hInspectStaticPos_ = nullptr;
    HWND hInspectValuePos_ = nullptr;
    HWND hInspectStaticLoc_ = nullptr;
    HWND hInspectValueLoc_ = nullptr;
    HWND hEntPreviewBox_ = nullptr;
    HWND hInspectBtnRotLeft_ = nullptr;
    HWND hInspectBtnPlayAnim_ = nullptr;
    HWND hInspectBtnRotRight_ = nullptr;
    HWND hInspectStaticDriver_ = nullptr;
    HWND hInspectComboDriver_ = nullptr;
    HWND hInspectStaticCyberware_ = nullptr;
    HWND hInspectComboCyberware_ = nullptr;
    HWND hInspectStaticMoveMode_ = nullptr;
    HWND hInspectComboMoveMode_ = nullptr;
    HWND hInspectStaticFaction_ = nullptr;
    HWND hInspectComboFaction_ = nullptr;
    HWND hInspectBtnAiTactics_ = nullptr;
    HWND hInspectBtnPlaytestConfig_ = nullptr;
    HWND hInspectStaticWeapons_ = nullptr;
    HWND hInspectWeaponsGrid_ = nullptr;
    HWND hInspectStaticRoute_ = nullptr;
    HWND hInspectBtnEditRoute_ = nullptr;
    HWND hInspectCheckLoop_ = nullptr;
    HWND hInspectBtnClearRoute_ = nullptr;
    HWND hInspectListActions_ = nullptr;
    HWND hInspectBtnAddAction_ = nullptr;
    HWND hInspectBtnDelAction_ = nullptr;
    HWND hInspectBtnMoveUp_ = nullptr;
    HWND hInspectBtnMoveDown_ = nullptr;
    HWND hInspectBtnApply_ = nullptr;
    HWND hInspectBtnCancel_ = nullptr;
    HWND hInspectBtnDelete_ = nullptr;
    rs_knl::ShootableMapObject *pSelectedEntity_ = nullptr;

    // Entity Preview State
    int previewDir_ = 0;
    int previewFrame_ = 0;
    bool previewAnimating_ = false;

    // Tab 2: Objectives
    HWND hListObjectives_ = nullptr;
    HWND hBtnAddObj_ = nullptr;
    HWND hBtnEditObj_ = nullptr;
    HWND hBtnDelObj_ = nullptr;
    HWND hBtnClearObj_ = nullptr;

    WinMapViewport *pViewport_ = nullptr;
    rs_knl::Mission *pMission_ = nullptr;

    // State
    int currentTab_ = 0;
    TabChangedCallback tabChangedCallback_;
    uint8_t activeTileId_ = 0;
    TileCategory currentCategory_ = TileCategory::All;
    std::vector<uint8_t> categoryTiles_;
    int gridScrollY_ = 0;

    // Entity Grid State
    int entityCategory_ = 0; // 0=All, 1=Actors, 2=Vehicles, 3=Statics
    int selectedEntityGridIdx_ = 0;
    int entityGridScrollY_ = 0;
    int entityGridHeight_ = 224;
    bool isDraggingEntitySplitter_ = false;
    int splitterDragStartY_ = 0;
    int initialSplitterGridHeight_ = 224;
    std::function<void()> onSplitterChangedCallback_;

    // Dirty state & snapshot for selected entity
    bool isPopulatingInspector_ = false;
    bool isEntityDirty_ = false;
    int origHealth_ = 100;
    uint8_t origDir_ = 0;
    rs_knl::PedInstance *origDriver_ = nullptr;
    int origDriverChoice_ = 0;
    int activeDriverChoice_ = 0;
    bool origCyberwareOverridden_ = false;
    int origCyberwareTier_ = 0;
    bool activeCyberwareOverridden_ = false;
    int activeCyberwareTier_ = 0;
    uint8_t origMovementMode_ = 0x10;
    uint8_t activeMovementMode_ = 0x10;
    int origFactionId_ = -1;
    int activeFactionId_ = -1;
    std::vector<int> origWeapons_;
    std::vector<rs_knl::ScriptStep> origSteps_;
    bool origLoop_ = false;
    std::set<int> selectedWeapons_;
    int hoveredWeaponIdx_ = -1;

    // Tiles Splitter State
    int tileGridHeight_ = 224;
    bool isDraggingTileSplitter_ = false;

    // Search Mode State & Results
    bool isTileSearchActive_ = false;
    bool isEntitySearchActive_ = false;
    std::vector<SearchRow> tileSearchResults_;
    std::vector<SearchRow> entSearchResults_;
    std::wstring tileSearchTitle_;
    std::wstring entSearchTitle_;
    int tileSearchScrollY_ = 0;
    int entSearchScrollY_ = 0;
    int tileSearchHoverRow_ = -1;
    int entSearchHoverRow_ = -1;
    bool tileSearchHoverTarget_ = false;
    bool entSearchHoverTarget_ = false;

    // Orientation: 0=N, 32=NE, 64=E, 96=SE, 128=S, 160=SW, 192=W, 224=NW
    uint8_t activeOrientation_ = 0;
    size_t activeEntityTemplateIdx_ = 0;
    int templateAgentType_ = 0; // 0 = Player's Agent, 1 = Enemy Agent
    int activeMissionId_ = 0;

    HBRUSH hBgBrush_ = nullptr;
    HBRUSH hInputBgBrush_ = nullptr;
    HBRUSH hBtnBrush_ = nullptr;
    HBITMAP hBmpPencil_ = nullptr;
    HBITMAP hBmpBox_ = nullptr;
    HBITMAP hBmpSelect_ = nullptr;
    HBITMAP hBmpSearch_ = nullptr;
    HBITMAP hBmpTarget_ = nullptr;
    HBITMAP hWeaponIcons_[16] = { nullptr };
    HFONT hFont_ = nullptr;
    HFONT hBoldFont_ = nullptr;
    HFONT hHeaderFont_ = nullptr;
};

} // namespace rs_win_editor

#endif // EDITOR_ART_PANEL_H_
