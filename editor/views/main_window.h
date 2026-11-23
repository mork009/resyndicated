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

#ifndef EDITOR_MAIN_WINDOW_H_
#define EDITOR_MAIN_WINDOW_H_

#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <memory>
#include <string>
#include <filesystem>

#include <uxtheme.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef WM_UAHDRAWMENU
#define WM_UAHDESTROYWINDOW    0x0090
#define WM_UAHDRAWMENU         0x0091
#define WM_UAHDRAWMENUITEM     0x0092
#define WM_UAHINITMENU         0x0093
#define WM_UAHMEASUREMENUITEM  0x0094
#define WM_UAHNCPAINTMENUPOPUP 0x0095

typedef struct tagUAHMENU {
    HMENU hmenu;
    HDC hdc;
    DWORD dwFlags;
} UAHMENU;

typedef struct tagUAHMENUITEM {
    int iPosition;
    UAHMENU uahMenu;
} UAHMENUITEM;

typedef struct tagUAHDRAWMENUITEM {
    DRAWITEMSTRUCT dis;
    UAHMENU uahMenu;
    int iPosition;
} UAHDRAWMENUITEM;
#endif

#include "dialog_theme.h"

#include "map_viewport.h"
#include "art_panel.h"
#include "editor_commands.h"
#include "mission_properties_dialog.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/mgr/missionmanager.h"

namespace rs_eng {
    class TileManager;
    class AnimationManager;
    class AppContext;
    class System;
}

namespace rs_knl {
    class ModManager;
    class WeaponManager;
    class AgentManager;
}

namespace rs_win_editor {

class MainWindow {
public:
    MainWindow();
    ~MainWindow();

    bool create(HINSTANCE hInstance, int nCmdShow, int width = 1600, int height = 960);
    HWND hwnd() const { return hWnd_; }

    void newMission();
    void loadMission(int missionId, const std::string &customPath = "");
    void closeMission();
    void saveMission();
    void saveMissionAs();
    void openMissionDialog();
    void playtestMission();
    void configurePlaytestLoadout();
    bool validateMissionBeforeAction(const std::wstring &actionName);
    void validateMission();
    void showMissionProperties();
    void showAiTacticsDialog();
    void showSettingsDialog();
    bool reloadAssets(const std::string &newResyndDir, const std::string &newOriginalDir);
    bool reloadAssets(const std::string &newDataDir) { return reloadAssets(newDataDir, newDataDir); }
    bool isModified() const;
    bool hasActiveMission() const { return pMission_ != nullptr; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT handleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void createToolbar();
    void createStatusBar();
    void updateStatusBarParts();
    void layoutChildren();
    void updateWindowTitle();
    void updateUIState();
    void updateStatusBar(const std::string &coords, const std::string &tileInfo, const std::string &hoverInfo);
    void saveCurrentWindowPlacement();
    int getEffectiveArtPanelWidth() const;

private:
    HWND hWnd_ = nullptr;
    HWND hToolbar_ = nullptr;
    HWND hStatusBar_ = nullptr;

    WinMapViewport mapViewport_;
    WinArtPanel artPanel_;
    EditorCommandStack commandStack_;

    std::unique_ptr<rs_eng::System> pSystem_;
    std::unique_ptr<rs_eng::AppContext> pAppContext_;
    std::unique_ptr<rs_eng::AnimationManager> pAnimManager_;
    std::unique_ptr<rs_knl::ModManager> pModManager_;
    std::unique_ptr<rs_knl::WeaponManager> pWeaponManager_;
    std::unique_ptr<rs_knl::AgentManager> pAgentManager_;
    std::unique_ptr<rs_eng::TileManager> pTileManager_;
    std::unique_ptr<rs_knl::MissionManager> pMissionManager_;
    rs_knl::Mission *pMission_ = nullptr;
    int currentMissionId_ = 1;
    size_t savedUndoCount_ = 0;
    MissionPropertiesData currentProps_;
    std::filesystem::path activeGamePath_;
    std::filesystem::path activeMapPath_;
    std::filesystem::path activeMissPath_;

    int toolbarHeight_ = 36;
    int statusBarHeight_ = 24;
    int artPanelWidth_ = 320;
    int splitterWidth_ = 6;
    bool isDraggingSplitter_ = false;
    int dragStartMouseX_ = 0;
    int dragStartArtPanelWidth_ = 320;

    HBRUSH hBgBrush_ = nullptr;
};

} // namespace rs_win_editor

#endif // EDITOR_MAIN_WINDOW_H_
