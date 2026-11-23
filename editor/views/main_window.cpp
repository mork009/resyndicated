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

#include "main_window.h"
#include "icon_manager.h"
#include "tile_rasterizer.h"
#include "sprite_rasterizer.h"
#include "playtest_manager.h"
#include "playtest_loadout_dialog.h"
#include "mission_chooser.h"
#include "rs-kernel/mgr/mission_triad.h"
#include "new_mission_dialog.h"
#include "save_mission_as_dialog.h"
#include "editor_config.h"
#include "editor_settings_dialog.h"
#include "ai_tactics_dialog.h"
#include "core/mission_validator.h"
#include "resource.h"

#include <windowsx.h>
#include <format>
#include <vector>
#include <filesystem>
#include <fstream>
#include <commdlg.h>

#include "rs-utils/io/file.h"
#include "rs-utils/version.h"
#include "rs-engine/system/system.h"
#include "rs-engine/appcontext.h"
#include "rs-engine/gfx/tilemanager.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/animationloader.h"
#include "rs-kernel/mgr/modmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/agentmanager.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/model/leveldata.h"

namespace rs_win_editor {

namespace {

std::wstring utf8ToWide(const std::string &str) {
    if (str.empty()) return L"";
    int numChars = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), nullptr, 0);
    if (numChars <= 0) {
        return std::wstring(str.begin(), str.end());
    }
    std::wstring result(numChars, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.length()), &result[0], numChars);
    return result;
}

} // namespace

static const wchar_t* kMainWindowClassName = L"ReSyndicatedMainWindowClass";

MainWindow::MainWindow() {
    hBgBrush_ = CreateSolidBrush(RGB(28, 30, 34));
}

MainWindow::~MainWindow() {
    if (pMissionManager_) {
        pMissionManager_->destroyMission();
    }
    pMission_ = nullptr;
    if (hBgBrush_) {
        DeleteObject(hBgBrush_);
        hBgBrush_ = nullptr;
    }
}

bool MainWindow::create(HINSTANCE hInstance, int nCmdShow, int width, int height) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    HICON hAppIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    HICON hAppIconSm = reinterpret_cast<HICON>(LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON,
                                                          GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
    wc.hInstance = hInstance;
    wc.hIcon = hAppIcon ? hAppIcon : LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = hAppIconSm;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = hBgBrush_;
    wc.lpszMenuName = MAKEINTRESOURCEW(IDR_MAIN_MENU);
    wc.lpszClassName = kMainWindowClassName;
    RegisterClassExW(&wc);

    // Load persisted window configuration if available
    EditorWindowConfig winCfg;
    bool hasSavedConfig = EditorConfig::loadWindowConfig(winCfg);

    int posX = 0;
    int posY = 0;
    int winW = width;
    int winH = height;
    bool startMaximized = false;

    if (hasSavedConfig && winCfg.width >= 640 && winCfg.height >= 480) {
        RECT testRc = { winCfg.x, winCfg.y, winCfg.x + winCfg.width, winCfg.y + winCfg.height };
        HMONITOR hMon = MonitorFromRect(&testRc, MONITOR_DEFAULTTONULL);
        if (hMon != NULL) {
            posX = winCfg.x;
            posY = winCfg.y;
            winW = winCfg.width;
            winH = winCfg.height;
            startMaximized = winCfg.maximized;
        } else {
            int screenW = GetSystemMetrics(SM_CXSCREEN);
            int screenH = GetSystemMetrics(SM_CYSCREEN);
            posX = std::max(0, (screenW - width) / 2);
            posY = std::max(0, (screenH - height) / 2);
        }
    } else {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        posX = std::max(0, (screenW - width) / 2);
        posY = std::max(0, (screenH - height) / 2);
    }

    if (hasSavedConfig) {
        if (winCfg.rightPaneWidth >= 240) {
            artPanelWidth_ = winCfg.rightPaneWidth;
        } else if (winCfg.rightPaneWidth > 0) {
            artPanelWidth_ = std::max(240, winCfg.rightPaneWidth);
        } else if (winCfg.leftPaneWidth > 0 && winW > winCfg.leftPaneWidth + splitterWidth_ + 240) {
            artPanelWidth_ = winW - winCfg.leftPaneWidth - splitterWidth_;
        }
    }

    hWnd_ = CreateWindowExW(
        WS_EX_APPWINDOW,
        kMainWindowClassName,
        L"ReSyndicated Editor - " RS_VERSION_WIDE,
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        posX, posY, winW, winH,
        nullptr, nullptr, hInstance, this
    );

    if (!hWnd_) return false;

    enableDarkModeForWindow(hWnd_);

    if (hAppIcon) {
        SendMessage(hWnd_, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hAppIcon));
    }
    if (hAppIconSm) {
        SendMessage(hWnd_, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hAppIconSm));
    }

    // Initialize icon manager
    IconManager::instance().initialize(24);

    // Initialize tile rasterizer with palette 1
    TileRasterizer::instance().load(1);
    SpriteRasterizer::instance().load();
    SpriteRasterizer::instance().loadMenuSprites();
    SpriteRasterizer::instance().loadLogos();

    // Initialize player faction color from configuration
    PlaytestLoadoutData initLoadout = PlaytestLoadoutDialog::getDefaultLoadout();
    EditorConfig::loadLoadoutConfig(initLoadout);
    static constexpr uint8_t kColours[8] = { 6, 7, 14, 3, 11, 12, 13, 15 };
    int colIdx = std::clamp(initLoadout.faction.color, 0, 7);
    SpriteRasterizer::setPlayerFactionColor(kColours[colIdx]);

    // Initialize System and AppContext
    pSystem_ = rs_eng::System::createSystem();
    pAppContext_ = std::make_unique<rs_eng::AppContext>();

    // Initialize animation manager
    pAnimManager_ = std::make_unique<rs_eng::AnimationManager>();
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(*pAnimManager_);

    // Initialize mod, weapon, and agent managers
    pModManager_ = std::make_unique<rs_knl::ModManager>();
    pModManager_->reset();

    pWeaponManager_ = std::make_unique<rs_knl::WeaponManager>();
    pWeaponManager_->reset();

    pAgentManager_ = std::make_unique<rs_knl::AgentManager>();
    pAgentManager_->setModManager(pModManager_.get());
    pAgentManager_->setWeaponManager(pWeaponManager_.get());
    pAgentManager_->reset();

    // Initialize tile and mission managers
    pTileManager_ = std::make_unique<rs_eng::TileManager>();
    pTileManager_->loadTiles();
    pMissionManager_ = std::make_unique<rs_knl::MissionManager>(pTileManager_.get());

    createToolbar();
    createStatusBar();

    // Create child viewport and art panel
    RECT rc;
    GetClientRect(hWnd_, &rc);
    int effectiveArtW = getEffectiveArtPanelWidth();
    int viewW = rc.right - effectiveArtW - splitterWidth_;
    int viewH = rc.bottom - toolbarHeight_ - statusBarHeight_;

    mapViewport_.create(hWnd_, 0, toolbarHeight_, viewW, viewH, IDC_VIEWPORT);
    mapViewport_.setCommandStack(&commandStack_);

    artPanel_.create(hWnd_, viewW + splitterWidth_, toolbarHeight_, effectiveArtW, viewH, IDC_ARTPANEL);
    artPanel_.setViewport(&mapViewport_);
    artPanel_.setEntityGridHeight(winCfg.entityGridHeight);
    artPanel_.setTileGridHeight(winCfg.tileGridHeight);
    artPanel_.setOnSplitterChangedCallback([this]() {
        saveCurrentWindowPlacement();
    });
    artPanel_.setTabChangedCallback([this](int) {
        updateUIState();
    });

    // Setup callbacks
    mapViewport_.setStatusCallback([this](const std::string &coords, const std::string &tileInfo, const std::string &hoverInfo) {
        updateStatusBar(coords, tileInfo, hoverInfo);
        updateWindowTitle();
    });

    mapViewport_.setTileSelectedCallback([this](uint8_t tid) {
        artPanel_.setActiveTab(0);
        artPanel_.setActiveTileId(tid);
    });

    mapViewport_.setEntitySelectedCallback([this](rs_knl::ShootableMapObject *pObj) {
        artPanel_.selectEntity(pObj);
    });

    mapViewport_.setCommandExecutedCallback([this]() {
        artPanel_.refreshObjectivesList();
        updateUIState();
        updateWindowTitle();
    });

    mapViewport_.setToolChangedCallback([this](ViewportTool) {
        updateUIState();
    });

    layoutChildren();
    int showCmd = startMaximized ? SW_SHOWMAXIMIZED : nCmdShow;
    ShowWindow(hWnd_, showCmd);
    UpdateWindow(hWnd_);

    // Start with blank main window
    closeMission();
    return true;
}

void MainWindow::createToolbar() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    hToolbar_ = CreateWindowExW(
        0, TOOLBARCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER | CCS_NORESIZE,
        0, 0, 0, toolbarHeight_,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_TOOLBAR)),
        hInst, nullptr
    );

    SendMessage(hToolbar_, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(hToolbar_, TB_SETBITMAPSIZE, 0, MAKELPARAM(24, 24));
    SendMessage(hToolbar_, TB_SETBUTTONSIZE, 0, MAKELPARAM(32, 32));
    SendMessage(hToolbar_, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(IconManager::instance().toolbarImageList()));

    TBBUTTON btns[] = {
        { static_cast<int>(IconId::New), IDM_FILE_NEW, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { static_cast<int>(IconId::Open), IDM_FILE_OPEN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { static_cast<int>(IconId::Save), IDM_FILE_SAVE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { static_cast<int>(IconId::Undo), IDM_EDIT_UNDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { static_cast<int>(IconId::Redo), IDM_EDIT_REDO, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { static_cast<int>(IconId::Slice), IDM_VIEW_SLICE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { static_cast<int>(IconId::LayerUp), IDM_LAYER_UP, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { static_cast<int>(IconId::LayerDown), IDM_LAYER_DOWN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { static_cast<int>(IconId::Spawns), IDM_VIEW_CENTER_SQUAD, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { static_cast<int>(IconId::Playtest), IDM_MISSION_PLAYTEST, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { 0, 0, 0, BTNS_SEP, {0}, 0, 0 },
        { static_cast<int>(IconId::ZoomOut), IDM_VIEW_ZOOM_OUT, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { static_cast<int>(IconId::Zoom100), IDM_VIEW_ZOOM_100, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 },
        { static_cast<int>(IconId::ZoomIn), IDM_VIEW_ZOOM_IN, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0 }
    };

    SendMessage(hToolbar_, TB_ADDBUTTONS, sizeof(btns) / sizeof(TBBUTTON), reinterpret_cast<LPARAM>(btns));
    SendMessage(hToolbar_, TB_AUTOSIZE, 0, 0);

    RECT tbRc;
    GetWindowRect(hToolbar_, &tbRc);
    toolbarHeight_ = tbRc.bottom - tbRc.top;
}

static LRESULT CALLBACK StatusBarSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR /*dwRefData*/) {
    if (msg == WM_ERASEBKGND) {
        return 1;
    }
    if (msg == WM_PAINT) {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT rcClient;
        GetClientRect(hWnd, &rcClient);

        // Fill background with dark charcoal
        HBRUSH hBg = CreateSolidBrush(RGB(20, 22, 25));
        FillRect(hdc, &rcClient, hBg);
        DeleteObject(hBg);

        // Draw top 1px dividing border
        HPEN hBorderPen = CreatePen(PS_SOLID, 1, RGB(45, 50, 56));
        HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hBorderPen));
        MoveToEx(hdc, 0, 0, nullptr);
        LineTo(hdc, rcClient.right, 0);

        // Draw subtle vertical separators
        HPEN hSepPen = CreatePen(PS_SOLID, 1, RGB(48, 54, 62));
        SelectObject(hdc, hSepPen);

        int numParts = static_cast<int>(SendMessageW(hWnd, SB_GETPARTS, 0, 0));
        HFONT hFont = reinterpret_cast<HFONT>(SendMessageW(hWnd, WM_GETFONT, 0, 0));
        if (!hFont) hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

        SetBkMode(hdc, TRANSPARENT);

        for (int i = 0; i < numParts; ++i) {
            RECT rcPart;
            SendMessageW(hWnd, SB_GETRECT, i, reinterpret_cast<LPARAM>(&rcPart));

            // Vertical divider between parts
            if (i < numParts - 1 && rcPart.right < rcClient.right - 10) {
                MoveToEx(hdc, rcPart.right, 3, nullptr);
                LineTo(hdc, rcPart.right, rcClient.bottom - 3);
            }

            wchar_t text[256] = {};
            SendMessageW(hWnd, SB_GETTEXTW, i, reinterpret_cast<LPARAM>(text));

            // Distinct Syndicate colors for different parts
            if (i == 0) {
                // Coordinates: Syndicate Cyan
                SetTextColor(hdc, RGB(0, 220, 245));
            } else if (i == 2) {
                // Status / Ready: Radar Green
                SetTextColor(hdc, RGB(50, 220, 120));
            } else if (i == 3) {
                // Layer / Tool: Amber
                SetTextColor(hdc, RGB(255, 185, 40));
            } else {
                // Normal text: Crisp silver
                SetTextColor(hdc, RGB(210, 215, 222));
            }

            RECT rcText = rcPart;
            rcText.left += 6;
            rcText.right -= 4;
            DrawTextW(hdc, text, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
        }

        SelectObject(hdc, hOldPen);
        DeleteObject(hSepPen);
        DeleteObject(hBorderPen);
        SelectObject(hdc, hOldFont);

        EndPaint(hWnd, &ps);
        return 0;
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hWnd, StatusBarSubclass, uIdSubclass);
    }
    return DefSubclassProc(hWnd, msg, wParam, lParam);
}

void MainWindow::updateStatusBarParts() {
    if (!hStatusBar_) return;

    RECT rc;
    GetClientRect(hWnd_, &rc);
    int clientW = rc.right;

    // Right-side fixed widths
    const int wZoom = 145;       // Part 5: Zoom & Playtest shortcut + size grip
    const int wUndo = 75;        // Part 4: Undo count
    const int wTool = 280;       // Part 3: Layer & Tool mode
    const int totalRight = wZoom + wUndo + wTool; // 500px

    // Left-side fixed widths
    const int wCoords = 200;     // Part 0: Coordinates & hover flags
    const int wTile = 270;       // Part 1: Tile ID & surface info (increased to prevent truncation)

    int p0 = wCoords;
    int p1 = p0 + wTile;

    // Part 2 (Status Message) dynamically takes all remaining width between p1 and the right-side parts
    const int minPart2Width = 450;
    int p2 = std::max(p1 + minPart2Width, clientW - totalRight);
    int p3 = p2 + wTool;
    int p4 = p3 + wUndo;
    int p5 = -1; // Extends to the right edge

    int parts[] = { p0, p1, p2, p3, p4, p5 };
    SendMessage(hStatusBar_, SB_SETPARTS, 6, reinterpret_cast<LPARAM>(parts));
}

void MainWindow::createStatusBar() {
    HINSTANCE hInst = GetModuleHandle(nullptr);

    hStatusBar_ = CreateWindowExW(
        0, STATUSCLASSNAMEW, nullptr,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, statusBarHeight_,
        hWnd_, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_STATUSBAR)),
        hInst, nullptr
    );

    SetWindowTheme(hStatusBar_, L"", L"");
    SetWindowSubclass(hStatusBar_, StatusBarSubclass, 1001, 0);

    updateStatusBarParts();

    SendMessage(hStatusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"TX:00 TY:00 TZ:00"));
    SendMessage(hStatusBar_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(L"Tile: #000 [Empty]"));
    SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Ready"));
    SendMessage(hStatusBar_, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(L"Layer: 0 (Slice: OFF) | Tool: Pencil"));
    SendMessage(hStatusBar_, SB_SETTEXTW, 4, reinterpret_cast<LPARAM>(L"Undo: 0"));
    SendMessage(hStatusBar_, SB_SETTEXTW, 5, reinterpret_cast<LPARAM>(L"Zoom: 100% | F5: Playtest"));
}

int MainWindow::getEffectiveArtPanelWidth() const {
    if (!hWnd_) return artPanelWidth_;
    RECT rc = {};
    GetClientRect(hWnd_, &rc);
    int clientW = rc.right;
    if (clientW <= 0) return artPanelWidth_;
    int minArtW = 240;
    int maxArtW = std::max(minArtW, clientW - 300 - splitterWidth_);
    return std::clamp(artPanelWidth_, minArtW, maxArtW);
}

void MainWindow::layoutChildren() {
    if (!hWnd_ || IsIconic(hWnd_)) {
        return;
    }
    RECT rc;
    GetClientRect(hWnd_, &rc);
    int clientW = rc.right;
    int clientH = rc.bottom;
    if (clientW <= 0 || clientH <= 0) {
        return;
    }

    SetWindowPos(hToolbar_, nullptr, 0, 0, clientW, toolbarHeight_, SWP_NOZORDER);
    SetWindowPos(hStatusBar_, nullptr, 0, clientH - statusBarHeight_, clientW, statusBarHeight_, SWP_NOZORDER);
    updateStatusBarParts();

    int contentY = toolbarHeight_;
    int contentH = clientH - toolbarHeight_ - statusBarHeight_;

    int effectiveArtW = getEffectiveArtPanelWidth();
    int viewW = clientW - effectiveArtW - splitterWidth_;
    if (viewW < 100) viewW = 100;
    if (contentH < 100) contentH = 100;

    SetWindowPos(mapViewport_.hwnd(), nullptr, 0, contentY, viewW, contentH, SWP_NOZORDER);
    artPanel_.onResize(viewW + splitterWidth_, contentY, effectiveArtW, contentH);
}

void MainWindow::updateWindowTitle() {
    if (!hasActiveMission()) {
        SetWindowTextW(hWnd_, L"ReSyndicated Editor - " RS_VERSION_WIDE);
        return;
    }
    std::wstring modMarker = isModified() ? L" *" : L"";
    std::wstring title;
    if (!activeGamePath_.empty()) {
        title = std::format(L"ReSyndicated Editor - {} [{} - Mission {:02}{}]",
                            RS_VERSION_WIDE, activeGamePath_.filename().wstring(), currentMissionId_, modMarker);
    } else {
        title = std::format(L"ReSyndicated Editor - {} [Mission {:02}{}]", RS_VERSION_WIDE, currentMissionId_, modMarker);
    }
    SetWindowTextW(hWnd_, title.c_str());
}

void MainWindow::updateUIState() {
    bool hasMission = hasActiveMission();
    HMENU hMenu = GetMenu(hWnd_);

    if (hMenu) {
        UINT flag = hasMission ? MF_ENABLED : (MF_DISABLED | MF_GRAYED);

        // File Menu items
        EnableMenuItem(hMenu, IDM_FILE_CLOSE, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_FILE_SAVE, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_FILE_SAVEAS, MF_BYCOMMAND | flag);

        // Edit Menu items
        EnableMenuItem(hMenu, IDM_EDIT_UNDO, MF_BYCOMMAND | (hasMission && commandStack_.canUndo() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
        EnableMenuItem(hMenu, IDM_EDIT_REDO, MF_BYCOMMAND | (hasMission && commandStack_.canRedo() ? MF_ENABLED : (MF_DISABLED | MF_GRAYED)));
        EnableMenuItem(hMenu, IDM_EDIT_DELETE, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_EDIT_DESELECT, MF_BYCOMMAND | flag);

        // View Menu items
        EnableMenuItem(hMenu, IDM_VIEW_SLICE, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_SLICE, MF_BYCOMMAND | (mapViewport_.isSliceView() ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, IDM_VIEW_SHOW_STATS, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_SHOW_STATS, MF_BYCOMMAND | (mapViewport_.isShowStats() ? MF_CHECKED : MF_UNCHECKED));

        EnableMenuItem(hMenu, IDM_VIEW_FILTER_BUILDINGS, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_FILTER_BUILDINGS, MF_BYCOMMAND | (mapViewport_.isFilterBuildings() ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, IDM_VIEW_FILTER_VEHICLES, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_FILTER_VEHICLES, MF_BYCOMMAND | (mapViewport_.isFilterVehicles() ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, IDM_VIEW_FILTER_CIVILIANS, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_FILTER_CIVILIANS, MF_BYCOMMAND | (mapViewport_.isFilterCivilians() ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, IDM_VIEW_FILTER_POLICE, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_FILTER_POLICE, MF_BYCOMMAND | (mapViewport_.isFilterPolice() ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, IDM_VIEW_FILTER_GUARDS, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_FILTER_GUARDS, MF_BYCOMMAND | (mapViewport_.isFilterGuards() ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, IDM_VIEW_FILTER_CRIMINALS, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_FILTER_CRIMINALS, MF_BYCOMMAND | (mapViewport_.isFilterCriminals() ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, IDM_VIEW_FILTER_AGENTS, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_FILTER_AGENTS, MF_BYCOMMAND | (mapViewport_.isFilterAgents() ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, IDM_VIEW_FILTER_WEAPONS, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_VIEW_FILTER_WEAPONS, MF_BYCOMMAND | (mapViewport_.isFilterWeapons() ? MF_CHECKED : MF_UNCHECKED));
        EnableMenuItem(hMenu, IDM_VIEW_FILTER_SELECT_ALL, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_VIEW_FILTER_CLEAR_ALL, MF_BYCOMMAND | flag);

        EnableMenuItem(hMenu, IDM_VIEW_ZOOM_IN, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_VIEW_ZOOM_OUT, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_VIEW_ZOOM_100, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_VIEW_ZOOM_150, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_VIEW_ZOOM_200, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_VIEW_CENTER_SQUAD, MF_BYCOMMAND | flag);

        // Tools Menu items
        EnableMenuItem(hMenu, IDM_TOOLS_PENCIL, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_TOOLS_BOXFILL, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_TOOLS_SELECT, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_TOOLS_SEARCH, MF_BYCOMMAND | flag);

        ViewportTool curTool = mapViewport_.tool();
        UINT checkedMenuId = 0;
        if (mapViewport_.isObjectivePicking()) {
            checkedMenuId = 0;
        } else if (artPanel_.isTileSearchActive() || artPanel_.isEntitySearchActive()) {
            checkedMenuId = IDM_TOOLS_SEARCH;
        } else if (curTool == ViewportTool::Pencil ||
            curTool == ViewportTool::PlaceActor ||
            curTool == ViewportTool::PlaceVehicle ||
            curTool == ViewportTool::PlaceStatic ||
            curTool == ViewportTool::PlaceWeapon) checkedMenuId = IDM_TOOLS_PENCIL;
        else if (curTool == ViewportTool::BoxFill) checkedMenuId = IDM_TOOLS_BOXFILL;
        else if (curTool == ViewportTool::SelectTile ||
                 curTool == ViewportTool::SelectEntity) checkedMenuId = IDM_TOOLS_SELECT;

        if (checkedMenuId != 0) {
            CheckMenuRadioItem(hMenu, IDM_TOOLS_PENCIL, IDM_TOOLS_SEARCH, checkedMenuId, MF_BYCOMMAND);
        } else {
            CheckMenuItem(hMenu, IDM_TOOLS_PENCIL, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hMenu, IDM_TOOLS_BOXFILL, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hMenu, IDM_TOOLS_SELECT, MF_BYCOMMAND | MF_UNCHECKED);
            CheckMenuItem(hMenu, IDM_TOOLS_SEARCH, MF_BYCOMMAND | MF_UNCHECKED);
        }

        // Layer Menu items
        EnableMenuItem(hMenu, IDM_LAYER_UP, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_LAYER_DOWN, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_LAYER_AUTO, MF_BYCOMMAND | flag);
        CheckMenuItem(hMenu, IDM_LAYER_AUTO, MF_BYCOMMAND | (mapViewport_.isAutoLayerSwitchingEnabled() ? MF_CHECKED : MF_UNCHECKED));
        int maxTz = (hasMission && pMission_->map()) ? pMission_->map()->maxTz() : 13;
        for (int l = 0; l <= 12; ++l) {
            UINT layerFlag = (hasMission && l < maxTz) ? MF_ENABLED : MF_GRAYED;
            EnableMenuItem(hMenu, IDM_LAYER_0 + l, MF_BYCOMMAND | layerFlag);
        }
        if (hasMission) {
            CheckMenuRadioItem(hMenu, IDM_LAYER_0, IDM_LAYER_12, IDM_LAYER_0 + mapViewport_.activeZ(), MF_BYCOMMAND);
        } else {
            for (int l = 0; l <= 12; ++l) {
                CheckMenuItem(hMenu, IDM_LAYER_0 + l, MF_BYCOMMAND | MF_UNCHECKED);
            }
        }

        // Mission Menu items
        EnableMenuItem(hMenu, IDM_MISSION_PLAYTEST, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_MISSION_LOADOUT, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_MISSION_PROPERTIES, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_MISSION_VALIDATE, MF_BYCOMMAND | flag);
        EnableMenuItem(hMenu, IDM_MISSION_AI_TACTICS, MF_BYCOMMAND | flag);

        DrawMenuBar(hWnd_);
    }

    if (hToolbar_) {
        // Toolbar buttons
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_FILE_SAVE, MAKELPARAM(hasMission ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_EDIT_UNDO, MAKELPARAM((hasMission && commandStack_.canUndo()) ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_EDIT_REDO, MAKELPARAM((hasMission && commandStack_.canRedo()) ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_VIEW_SLICE, MAKELPARAM(hasMission ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_LAYER_DOWN, MAKELPARAM(hasMission ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_LAYER_UP, MAKELPARAM(hasMission ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_VIEW_CENTER_SQUAD, MAKELPARAM(hasMission ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_MISSION_PLAYTEST, MAKELPARAM(hasMission ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_VIEW_ZOOM_OUT, MAKELPARAM(hasMission ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_VIEW_ZOOM_100, MAKELPARAM(hasMission ? TRUE : FALSE, 0));
        SendMessage(hToolbar_, TB_ENABLEBUTTON, IDM_VIEW_ZOOM_IN, MAKELPARAM(hasMission ? TRUE : FALSE, 0));

        // Sync active tool buttons in side panel
        artPanel_.updateToolSelection();
    }

    if (hStatusBar_) {
        const wchar_t *toolName = L"Unknown";
        if (mapViewport_.isObjectivePicking()) {
            if (mapViewport_.objectivePickMode() == ObjectivePickMode::Ped) {
                toolName = L"Pick Target Ped";
            } else if (mapViewport_.objectivePickMode() == ObjectivePickMode::Vehicle) {
                toolName = L"Pick Target Vehicle";
            } else if (mapViewport_.objectivePickMode() == ObjectivePickMode::Coordinate) {
                toolName = L"Pick Evacuation Target";
            } else {
                toolName = L"Pick Target";
            }
        } else {
            switch (mapViewport_.tool()) {
                case ViewportTool::Pencil:           toolName = L"Pencil"; break;
                case ViewportTool::BoxFill:          toolName = L"Box Fill"; break;
                case ViewportTool::SelectTile:       toolName = L"Select Tile"; break;
                case ViewportTool::SelectEntity:     toolName = L"Select Entity"; break;
                case ViewportTool::PlaceActor:       toolName = L"Place Actor"; break;
                case ViewportTool::PlaceVehicle:     toolName = L"Place Vehicle"; break;
                case ViewportTool::PlaceStatic:      toolName = L"Place Static"; break;
                case ViewportTool::PlaceWeapon:      toolName = L"Place Weapon"; break;
                case ViewportTool::RouteTool:        toolName = L"Route Editor"; break;
            }
        }
        std::wstring layerMode = L"Manual";
        if (mapViewport_.isAutoLayerSwitchingEnabled()) {
            layerMode = mapViewport_.isManualLayerOverride() ? L"Auto (Manual)" : L"Auto";
        }
        std::wstring toolStr = std::format(L"Layer: {} [{}] (Slice: {}) | Tool: {}",
                                           mapViewport_.activeZ(),
                                           layerMode,
                                           mapViewport_.isSliceView() ? L"ON" : L"OFF",
                                           toolName);
        SendMessage(hStatusBar_, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(toolStr.c_str()));
    }
}

void MainWindow::closeMission() {
    if (pMission_ && isModified()) {
        int res = showEditorMessageBox(hWnd_,
            L"You have unsaved changes in the current mission.\n\n"
            L"Do you want to save them before closing?",
            L"Close Mission", MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1);
        if (res == IDYES) {
            saveMission();
            if (isModified()) {
                return;
            }
        } else if (res == IDCANCEL) {
            return;
        }
    }

    if (pMissionManager_) {
        pMissionManager_->destroyMission();
        pMissionManager_->mapManager().clearCache();
    }
    pMission_ = nullptr;
    currentMissionId_ = 0;
    activeGamePath_.clear();
    activeMapPath_.clear();
    activeMissPath_.clear();
    commandStack_.clear();
    savedUndoCount_ = 0;

    mapViewport_.setMission(nullptr, 0);
    mapViewport_.selectAllFilters(true);
    artPanel_.setMission(nullptr);

    updateUIState();
    updateWindowTitle();

    SendMessage(hStatusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(L"TX:-- TY:-- TZ:--"));
    SendMessage(hStatusBar_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(L"Tile: None"));
    SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"No mission loaded. Use File -> New Mission (Ctrl+N) or Open Mission (Ctrl+O)."));
    SendMessage(hStatusBar_, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(L"Ready"));
    SendMessage(hStatusBar_, SB_SETTEXTW, 4, reinterpret_cast<LPARAM>(L"Undo: 0"));
    SendMessage(hStatusBar_, SB_SETTEXTW, 5, reinterpret_cast<LPARAM>(L"Zoom: 100%"));

    InvalidateRect(mapViewport_.hwnd(), nullptr, TRUE);
    UpdateWindow(mapViewport_.hwnd());
    InvalidateRect(artPanel_.hwnd(), nullptr, TRUE);
    UpdateWindow(artPanel_.hwnd());
    InvalidateRect(hWnd_, nullptr, TRUE);
}

bool MainWindow::isModified() const {
    return commandStack_.undoCount() != savedUndoCount_;
}

void MainWindow::updateStatusBar(const std::string &coords, const std::string &tileInfo, const std::string &hoverInfo) {
    std::wstring wCoords = utf8ToWide(coords);
    std::wstring wTile = utf8ToWide(tileInfo);
    std::wstring wHover = utf8ToWide(hoverInfo);

    SendMessage(hStatusBar_, SB_SETTEXTW, 0, reinterpret_cast<LPARAM>(wCoords.c_str()));
    SendMessage(hStatusBar_, SB_SETTEXTW, 1, reinterpret_cast<LPARAM>(wTile.c_str()));
    SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(wHover.c_str()));

    const wchar_t *toolName = L"Pencil";
    if (mapViewport_.isObjectivePicking()) {
        if (mapViewport_.objectivePickMode() == ObjectivePickMode::Ped) {
            toolName = L"Pick Target Ped";
        } else if (mapViewport_.objectivePickMode() == ObjectivePickMode::Vehicle) {
            toolName = L"Pick Target Vehicle";
        } else if (mapViewport_.objectivePickMode() == ObjectivePickMode::Coordinate) {
            toolName = L"Pick Evacuation Target";
        } else {
            toolName = L"Pick Target";
        }
    } else {
        switch (mapViewport_.tool()) {
            case ViewportTool::Pencil:           toolName = L"Pencil"; break;
            case ViewportTool::BoxFill:          toolName = L"Box Fill"; break;
            case ViewportTool::SelectTile:       toolName = L"Select Tile"; break;
            case ViewportTool::SelectEntity:     toolName = L"Select Entity"; break;
            case ViewportTool::PlaceActor:       toolName = L"Place Actor"; break;
            case ViewportTool::PlaceVehicle:     toolName = L"Place Vehicle"; break;
            case ViewportTool::PlaceStatic:      toolName = L"Place Static"; break;
            case ViewportTool::PlaceWeapon:      toolName = L"Place Weapon"; break;
            case ViewportTool::RouteTool:        toolName = L"Route Editor"; break;
        }
    }
    std::wstring toolStr = std::format(L"Layer: {} (Slice: {}) | Tool: {}",
                                       mapViewport_.activeZ(),
                                       mapViewport_.isSliceView() ? L"ON" : L"OFF",
                                       toolName);
    SendMessage(hStatusBar_, SB_SETTEXTW, 3, reinterpret_cast<LPARAM>(toolStr.c_str()));

    std::wstring undoStr = std::format(L"Undo: {}", commandStack_.undoCount());
    SendMessage(hStatusBar_, SB_SETTEXTW, 4, reinterpret_cast<LPARAM>(undoStr.c_str()));

    std::wstring zoomStr = std::format(L"Zoom: {}%", static_cast<int>(mapViewport_.zoom() * 100));
    SendMessage(hStatusBar_, SB_SETTEXTW, 5, reinterpret_cast<LPARAM>(zoomStr.c_str()));
}

void MainWindow::loadMission(int missionId, const std::string &customPath) {
    rs_knl::MissionTriadPaths paths;
    if (!customPath.empty()) {
        if (!rs_knl::MissionTriad::resolveFromAnyFile(customPath, paths)) {
            showEditorMessageBox(hWnd_, L"Could not resolve mission triad files from path.", L"Error Loading Mission", MB_ICONERROR);
            return;
        }
    } else {
        paths = rs_knl::MissionTriad::resolveFromId(missionId);
    }

    rs_knl::TriadValidationResult val = rs_knl::MissionTriad::validate(paths);
    if (!val.isValid) {
        showEditorMessageBox(hWnd_, val.summaryMessage.c_str(), L"Mission Triad Validation Error", MB_ICONERROR);
        return;
    }

    activeGamePath_ = paths.gameFile;
    activeMapPath_ = paths.mapFile;
    activeMissPath_ = paths.missFile;
    currentMissionId_ = paths.missionId;

    if (pMissionManager_) {
        pMissionManager_->destroyMission();
        pMissionManager_->mapManager().clearCache();
        auto pLevelData = std::make_unique<LevelData::LevelDataAll>();
        bool ok = pMissionManager_->load_level_data(currentMissionId_, *pLevelData, activeGamePath_.string());

        if (ok) {
            if (rs_knl::MissionManager::missionRequiresAmericanRevolt(*pLevelData) && !rs_utl::File::hasAmericanRevoltData()) {
                showEditorMessageBox(hWnd_,
                    L"This mission contains American Revolt supplemental content (Air Strike / Clone Shield).\n\n"
                    L"Your active data directory only contains original Syndicate assets without American Revolt. "
                    L"To preview and use all American Revolt graphics and icons correctly, please configure your data directory "
                    L"to point to an American Revolt directory (such as Syndicate Plus DATADISK\\DATA) via File -> Settings.",
                    L"American Revolt Assets Detected", MB_ICONWARNING);
            }
            pMission_ = pMissionManager_->create_mission(*pLevelData, activeMapPath_.string());
            if (pMission_) {
                pMission_->buildNavigationGraph();
            }
        }
    }

    if (!pMission_) {
        std::wstring err = std::format(L"Failed to instantiate mission from:\n{}", activeGamePath_.wstring());
        showEditorMessageBox(hWnd_, err.c_str(), L"Error Loading Mission", MB_ICONERROR);
        return;
    }

    int paletteId = rs_knl::MissionManager::getMissionPaletteId(currentMissionId_);
    TileRasterizer::instance().setPalette(paletteId);
    mapViewport_.setMission(pMission_, currentMissionId_);
    mapViewport_.selectAllFilters(true);
    artPanel_.setMission(pMission_, currentMissionId_);
    artPanel_.refreshPalette();
    commandStack_.clear();
    savedUndoCount_ = 0;

    currentProps_.missionId = currentMissionId_;
    currentProps_.mapId = pMission_->map() ? pMission_->map()->id() : currentMissionId_;
    currentProps_.techLevel = pMission_->techLevel();
    currentProps_.initialTechLevel = pMission_->techLevel();
    currentProps_.language = 0;
    if (!activeMissPath_.empty()) {
        std::string fname = activeMissPath_.filename().string();
        if (fname.size() >= 7 && fname.rfind("miss", 0) == 0) {
            char prefix = fname[4];
            if (prefix == '1') currentProps_.language = 1;
            else if (prefix == '2') currentProps_.language = 2;
            else if (prefix == '3') currentProps_.language = 3;
        }
    }
    std::filesystem::path loadDir = !activeMissPath_.empty() ? activeMissPath_.parent_path()
                                  : (!activeGamePath_.empty() ? activeGamePath_.parent_path() : std::filesystem::path());
    currentProps_.sourceDir = loadDir;
    currentProps_.gameFilePath = activeGamePath_;
    MissionPropertiesDialog::loadAllBriefingLanguages(currentMissionId_, loadDir, currentProps_);
    std::string curMissFilename = MissionPropertiesDialog::getBriefingFilename(currentMissionId_, currentProps_.language);
    if (!loadDir.empty() && std::filesystem::exists(loadDir / curMissFilename)) {
        activeMissPath_ = loadDir / curMissFilename;
    }

    updateUIState();
    updateWindowTitle();
    std::wstring statusMsg = std::format(L"Mission {} loaded successfully ({}).", currentMissionId_, activeGamePath_.filename().wstring());
    SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(statusMsg.c_str()));
}

void MainWindow::newMission() {
    if (pMission_ && isModified()) {
        int res = showEditorMessageBox(hWnd_,
            L"You have unsaved changes in the current mission.\n\n"
            L"Do you want to discard them and create a new mission?",
            L"New Mission", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        if (res != IDYES) {
            return;
        }
    }

    NewMissionDialogData data;
    data.missionId = (currentMissionId_ > 0 && currentMissionId_ < 99) ? currentMissionId_ : 99;
    if (!NewMissionDialog::show(hWnd_, data)) {
        return;
    }

    // Load template mission to initialize engines, palettes, and structures
    int templateMission = data.isBlankMap ? 1 : data.baseMapId;
    loadMission(templateMission);

    if (data.isBlankMap) {
        TileRasterizer::instance().setPalette(1);
        artPanel_.refreshPalette();
    }

    currentMissionId_ = data.missionId;
    activeGamePath_.clear();
    activeMapPath_.clear();
    activeMissPath_.clear();

    currentProps_.missionId = data.missionId;
    currentProps_.mapId = data.isBlankMap ? data.missionId : data.baseMapId;
    currentProps_.language = 0;
    currentProps_.techLevel = 0;
    currentProps_.initialTechLevel = 0;
    currentProps_.sourceDir.clear();
    currentProps_.gameFilePath.clear();
    currentProps_.infoCost1 = 100;
    currentProps_.infoCost2 = 250;
    currentProps_.infoCost3 = 400;
    currentProps_.enhCost1 = 100;
    currentProps_.enhCost2 = 250;
    currentProps_.enhCost3 = 400;
    currentProps_.baseBriefing = data.briefingTitle + ":\nInfiltrate enemy territory and complete all objectives.";
    currentProps_.info1Briefing = "LEVEL 1 INTEL:\nEnemy security forces on alert.";
    currentProps_.info2Briefing = "LEVEL 2 INTEL:\nArmored vehicle reported in target sector.";
    currentProps_.info3Briefing = "TACTICS:\nProceed with caution.";
    currentProps_.infoCostLines = { "100", "250", "400" };
    currentProps_.enhCostLines = { "100", "250", "400" };
    currentProps_.briefingSections = { currentProps_.baseBriefing, currentProps_.info1Briefing, currentProps_.info2Briefing, currentProps_.info3Briefing };
    currentProps_.syncActiveToLang(0);
    currentProps_.langs[0].modified = true;
    currentProps_.langs[0].sessionEdited = true;
    for (int i = 1; i < 4; ++i) {
        currentProps_.langs[i].available = false;
        currentProps_.langs[i].modified = false;
        currentProps_.langs[i].sessionEdited = false;
        currentProps_.langs[i].baseBriefing.clear();
        currentProps_.langs[i].info1Briefing.clear();
        currentProps_.langs[i].info2Briefing.clear();
        currentProps_.langs[i].info3Briefing.clear();
        currentProps_.langs[i].infoCostLines.clear();
        currentProps_.langs[i].enhCostLines.clear();
        currentProps_.langs[i].briefingSections.clear();
    }

    if (pMission_) {
        int spawnTx = data.spawnTx;
        int spawnTy = data.spawnTy;
        int spawnTz = data.spawnTz;

        // If copying an existing map template, preserve the template's squad spawn location
        if (!data.isBlankMap && pMission_->numPeds() > 0 && pMission_->ped(0)) {
            spawnTx = pMission_->ped(0)->tileX();
            spawnTy = pMission_->ped(0)->tileY();
            spawnTz = pMission_->ped(0)->tileZ();
        }

        // Clear non-player entities to provide a clean slate
        while (pMission_->numPeds() > 0) {
            pMission_->removePed(pMission_->ped(0));
        }
        while (pMission_->numVehicles() > 0) {
            pMission_->removeVehicle(pMission_->vehicle(0));
        }
        while (pMission_->numWeaponsOnGround() > 0) {
            pMission_->removeGroundWeapon(pMission_->weaponOnGround(0));
        }
        while (pMission_->numStatics() > 0) {
            pMission_->removeStatic(pMission_->statics(0));
        }

        // Clear existing objectives
        pMission_->clearObjectives();

        // Reset raw level data so no ghost entities or old mission data leak
        pMission_->setRawLevelData(LevelData::LevelDataAll{});
        if (pMission_->map()) {
            int maxTx = pMission_->map()->maxTx();
            int maxTy = pMission_->map()->maxTy();
            LevelData::LevelDataAll &rawLd = pMission_->getRawLevelData();
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.map, static_cast<uint16_t>(currentMissionId_));
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.min_x, 0);
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.min_y, 0);
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.max_x, static_cast<uint16_t>((maxTx - 1) * 2));
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.max_y, static_cast<uint16_t>((maxTy - 1) * 2));
        }

        if (data.isBlankMap && pMission_->map()) {
            rs_knl::Map *pMap = pMission_->map();
            for (int y = 0; y < pMap->maxTy(); ++y) {
                for (int x = 0; x < pMap->maxTx(); ++x) {
                    for (int z = 0; z < pMap->maxTz(); ++z) {
                        pMap->setTileAt(x, y, z, 0);
                    }
                }
            }

            // Create starting concrete platform (Tile ID 12) at ground layer Z = spawnTz - 1
            int cx = spawnTx;
            int cy = spawnTy;
            int groundZ = std::max(0, spawnTz - 1);
            for (int y = cy - 2; y <= cy + 3; ++y) {
                for (int x = cx - 2; x <= cx + 3; ++x) {
                    pMap->setTileAt(x, y, groundZ, 12);
                }
            }
            mapViewport_.setActiveZ(groundZ);
            mapViewport_.setManualLayerOverride(false);
        }

        // Spawn 4 squad player agents at the initial squad spawn points
        pMission_->spawnPlayerAgent(0, rs_knl::TilePoint(spawnTx, spawnTy, spawnTz), 0);
        pMission_->spawnPlayerAgent(1, rs_knl::TilePoint(spawnTx + 1, spawnTy, spawnTz), 0);
        pMission_->spawnPlayerAgent(2, rs_knl::TilePoint(spawnTx, spawnTy + 1, spawnTz), 0);
        pMission_->spawnPlayerAgent(3, rs_knl::TilePoint(spawnTx + 1, spawnTy + 1, spawnTz), 0);

        pMission_->buildNavigationGraph();
        artPanel_.setMission(pMission_, currentMissionId_);
    }

    commandStack_.clear();
    savedUndoCount_ = 0;
    updateUIState();
    mapViewport_.centerOnSquadSpawn();
    InvalidateRect(mapViewport_.hwnd(), nullptr, FALSE);
    updateWindowTitle();
    std::wstring statusMsg = std::format(L"New mission initialized (Mission {}).", currentMissionId_);
    SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(statusMsg.c_str()));
}

void MainWindow::saveMission() {
    if (!pMission_) return;

    if (!validateMissionBeforeAction(L"Save")) {
        return;
    }

    if (activeGamePath_.empty() || activeMapPath_.empty() || activeMissPath_.empty()) {
        saveMissionAs();
        return;
    }

    std::vector<std::pair<int, std::filesystem::path>> savedLangs;
    for (int i = 0; i < 4; ++i) {
        if (currentProps_.langs[i].available) {
            std::filesystem::path lPath = activeMissPath_.parent_path() / MissionPropertiesDialog::getBriefingFilename(currentMissionId_, i);
            savedLangs.emplace_back(i, lPath);
        }
    }

    std::wstring briefingsDesc;
    for (const auto &[langIdx, lPath] : savedLangs) {
        if (!briefingsDesc.empty()) briefingsDesc += L"\n";
        briefingsDesc += std::format(L"  \u2022 {} ({})", lPath.filename().wstring(), MissionPropertiesDialog::getLanguageName(langIdx));
    }

    std::wstring confirmMsg = std::format(
        L"Save Mission {} files?\n\n"
        L"\u2022 Mission Binary: {}\n"
        L"\u2022 Map Binary: {}\n"
        L"\u2022 Briefing Files:\n{}\n\n"
        L"Do you want to proceed and overwrite these files?",
        currentMissionId_,
        activeGamePath_.filename().wstring(),
        activeMapPath_.filename().wstring(),
        briefingsDesc);

    if (showEditorMessageBox(hWnd_, confirmMsg.c_str(), L"Save Mission", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }

    int mapId = pMission_->map() ? pMission_->map()->id() : currentMissionId_;
    rs_utl::WRITE_LE_UINT16(pMission_->getRawLevelData().mapinfos.map, static_cast<uint16_t>(mapId));
    pMission_->setTechLevel(currentProps_.techLevel);
    pMission_->getRawLevelData().mapinfos.unkn1[0] = static_cast<uint8_t>(currentProps_.techLevel);
    if (pMission_->map()) {
        int maxTx = pMission_->map()->maxTx();
        int maxTy = pMission_->map()->maxTy();
        LevelData::LevelDataAll &rawLd = pMission_->getRawLevelData();
        uint16_t curMinX = rs_utl::READ_LE_UINT16(rawLd.mapinfos.min_x);
        uint16_t curMaxX = rs_utl::READ_LE_UINT16(rawLd.mapinfos.max_x);
        uint16_t curMinY = rs_utl::READ_LE_UINT16(rawLd.mapinfos.min_y);
        uint16_t curMaxY = rs_utl::READ_LE_UINT16(rawLd.mapinfos.max_y);
        if (curMaxX <= curMinX || curMaxY <= curMinY) {
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.min_x, 0);
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.min_y, 0);
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.max_x, static_cast<uint16_t>((maxTx - 1) * 2));
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.max_y, static_cast<uint16_t>((maxTy - 1) * 2));
        }
    }

    bool gOk = rs_knl::MissionManager::exportMissionToBinary(pMission_, currentMissionId_, activeGamePath_.string());
    bool mOk = (pMission_->map() != nullptr) && pMission_->map()->exportBinary(activeMapPath_.string());
    bool allBriefingsOk = true;
    for (const auto &[langIdx, lPath] : savedLangs) {
        if (!MissionPropertiesDialog::saveBriefingLangToPath(lPath.string(), currentProps_, langIdx)) {
            allBriefingsOk = false;
        } else {
            currentProps_.langs[langIdx].modified = false;
        }
    }

    if (gOk && mOk && allBriefingsOk) {
        savedUndoCount_ = commandStack_.undoCount();
        updateWindowTitle();

        std::wstring fullBriefingList;
        for (const auto &[langIdx, lPath] : savedLangs) {
            if (!fullBriefingList.empty()) fullBriefingList += L"\n";
            fullBriefingList += std::format(L"  [{}] {}", MissionPropertiesDialog::getLanguageName(langIdx), lPath.wstring());
        }

        std::wstring msg = std::format(
            L"Successfully saved Mission {}, Map, and Briefing files!\n\n"
            L"\u2022 Mission: {}\n"
            L"\u2022 Map: {}\n"
            L"\u2022 Briefing Files:\n{}",
            currentMissionId_,
            activeGamePath_.wstring(),
            activeMapPath_.wstring(),
            fullBriefingList);
        showEditorMessageBox(hWnd_, msg.c_str(), L"Save Complete", MB_ICONINFORMATION);
        SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Mission, map, and briefings saved successfully."));
    } else {
        std::wstring errMsg = std::format(
            L"Error saving mission files!\n\n"
            L"Mission status: {}\n"
            L"Map status: {}\n"
            L"Briefings status: {}",
            gOk ? L"OK" : L"FAILED",
            mOk ? L"OK" : L"FAILED",
            allBriefingsOk ? L"OK" : L"FAILED");
        showEditorMessageBox(hWnd_, errMsg.c_str(), L"Save Error", MB_ICONERROR);
    }
}

void MainWindow::saveMissionAs() {
    if (!pMission_) return;

    if (!validateMissionBeforeAction(L"Save As")) {
        return;
    }

    std::filesystem::path initialDir;
    if (!activeGamePath_.empty()) {
        initialDir = activeGamePath_.parent_path();
    } else {
        initialDir = std::filesystem::current_path() / "data";
        if (!std::filesystem::exists(initialDir)) {
            initialDir = std::filesystem::current_path();
        }
    }

    SaveMissionAsDialogData data;
    if (!SaveMissionAsDialog::show(hWnd_, currentMissionId_, currentProps_.mapId, currentProps_, initialDir, data)) {
        return;
    }

    int newId = data.targetMissionId;
    int targetMapId = data.targetMapId;
    rs_knl::MissionTriadPaths paths = data.resolvedPaths;

    currentMissionId_ = newId;
    currentProps_.missionId = newId;
    currentProps_.mapId = targetMapId;

    if (pMission_->map()) {
        pMission_->map()->setId(static_cast<uint16_t>(targetMapId));
        int maxTx = pMission_->map()->maxTx();
        int maxTy = pMission_->map()->maxTy();
        LevelData::LevelDataAll &rawLd = pMission_->getRawLevelData();
        uint16_t curMinX = rs_utl::READ_LE_UINT16(rawLd.mapinfos.min_x);
        uint16_t curMaxX = rs_utl::READ_LE_UINT16(rawLd.mapinfos.max_x);
        uint16_t curMinY = rs_utl::READ_LE_UINT16(rawLd.mapinfos.min_y);
        uint16_t curMaxY = rs_utl::READ_LE_UINT16(rawLd.mapinfos.max_y);
        if (curMaxX <= curMinX || curMaxY <= curMinY) {
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.min_x, 0);
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.min_y, 0);
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.max_x, static_cast<uint16_t>((maxTx - 1) * 2));
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.max_y, static_cast<uint16_t>((maxTy - 1) * 2));
        }
    }
    rs_utl::WRITE_LE_UINT16(pMission_->getRawLevelData().mapinfos.map, static_cast<uint16_t>(targetMapId));

    bool gOk = rs_knl::MissionManager::exportMissionToBinary(pMission_, currentMissionId_, paths.gameFile.string());
    bool mOk = (pMission_->map() != nullptr) && pMission_->map()->exportBinary(paths.mapFile.string());

    bool allBriefingsOk = true;
    std::vector<std::pair<int, std::filesystem::path>> savedBriefings;
    for (int i = 0; i < 4; ++i) {
        if (currentProps_.langs[i].available) {
            std::filesystem::path langFile = data.targetDir / MissionPropertiesDialog::getBriefingFilename(newId, i);
            if (!MissionPropertiesDialog::saveBriefingLangToPath(langFile.string(), currentProps_, i)) {
                allBriefingsOk = false;
            } else {
                currentProps_.langs[i].modified = false;
                savedBriefings.emplace_back(i, langFile);
            }
        }
    }

    if (gOk && mOk && allBriefingsOk) {
        activeGamePath_ = paths.gameFile;
        activeMapPath_ = paths.mapFile;
        activeMissPath_ = data.targetDir / MissionPropertiesDialog::getBriefingFilename(newId, currentProps_.language);
        currentProps_.sourceDir = data.targetDir;
        currentProps_.gameFilePath = activeGamePath_;
        currentProps_.initialTechLevel = currentProps_.techLevel;

        savedUndoCount_ = commandStack_.undoCount();
        updateWindowTitle();

        std::wstring briefingListStr;
        for (const auto &[langIdx, lPath] : savedBriefings) {
            if (!briefingListStr.empty()) briefingListStr += L"\n";
            briefingListStr += std::format(L"  [{}] {}", MissionPropertiesDialog::getLanguageName(langIdx), lPath.wstring());
        }

        std::wstring msg = std::format(
            L"Mission, map, and briefing files successfully saved!\n\n"
            L"\u2022 Mission: {}\n"
            L"\u2022 Map: {}\n"
            L"\u2022 Briefing Files:\n{}",
            paths.gameFile.wstring(),
            paths.mapFile.wstring(),
            briefingListStr);
        showEditorMessageBox(hWnd_, msg.c_str(), L"Save As Complete", MB_ICONINFORMATION);
        std::wstring statusMsg = std::format(L"Saved as {}", paths.gameFile.filename().wstring());
        SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(statusMsg.c_str()));
    } else {
        std::wstring errMsg = std::format(
            L"Error writing mission, map, or briefing binary files!\n\n"
            L"Mission: {}\n"
            L"Map: {}\n"
            L"Briefings: {}",
            gOk ? L"OK" : L"FAILED",
            mOk ? L"OK" : L"FAILED",
            allBriefingsOk ? L"OK" : L"FAILED");
        showEditorMessageBox(hWnd_, errMsg.c_str(), L"Save Error", MB_ICONERROR);
    }
}



void MainWindow::showMissionProperties() {
    MissionPropertiesData data = currentProps_;
    data.missionId = currentMissionId_;
    if (pMission_ && pMission_->map()) {
        data.mapId = pMission_->map()->id();
    }
    if (pMission_) {
        data.techLevel = pMission_->techLevel();
    }
    data.initialTechLevel = data.techLevel;

    std::filesystem::path dir = !activeMissPath_.empty() ? activeMissPath_.parent_path()
                              : (!activeGamePath_.empty() ? activeGamePath_.parent_path() : std::filesystem::path());
    data.sourceDir = dir;
    data.gameFilePath = activeGamePath_;
    int oldTechLevel = data.techLevel;

    if (MissionPropertiesDialog::show(hWnd_, data)) {
        currentProps_ = data;
        currentProps_.sourceDir = dir;
        currentProps_.gameFilePath = activeGamePath_;
        currentProps_.initialTechLevel = data.techLevel;
        if (pMission_) {
            pMission_->setTechLevel(data.techLevel);
            pMission_->getRawLevelData().mapinfos.unkn1[0] = static_cast<uint8_t>(data.techLevel);

            if (data.techLevel != oldTechLevel) {
                // Update all enemy agents that are inheriting mission cyberware
                for (size_t i = 0; i < pMission_->numPeds(); ++i) {
                    auto *pPed = pMission_->ped(i);
                    if (pPed && pPed->type() == rs_knl::PedInstance::kPedTypeAgent &&
                        !pPed->isOurAgent() && !pPed->isCyberwareOverridden()) {
                        pPed->applyCyberwareTier(data.techLevel);
                    }
                }
                artPanel_.refreshInspector();
            }
        }

        dir = !activeMissPath_.empty() ? activeMissPath_.parent_path()
            : (!activeGamePath_.empty() ? activeGamePath_.parent_path() : std::filesystem::path());

        for (int i = 0; i < 4; ++i) {
            if (currentProps_.langs[i].available && currentProps_.langs[i].modified) {
                std::string lName = MissionPropertiesDialog::getBriefingFilename(currentMissionId_, i);
                std::string outPath = !dir.empty() ? (dir / lName).string() : rs_utl::File::getOriginalDataFullPath(lName);
                MissionPropertiesDialog::saveBriefingLangToPath(outPath, currentProps_, i);
                currentProps_.langs[i].modified = false;
            }
        }

        std::string curMissFilename = MissionPropertiesDialog::getBriefingFilename(currentMissionId_, data.language);
        if (!dir.empty()) {
            activeMissPath_ = dir / curMissFilename;
        } else {
            activeMissPath_ = rs_utl::File::getOriginalDataFullPath(curMissFilename);
        }
        SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Mission properties and briefings updated."));
    }
}

void MainWindow::showAiTacticsDialog() {
    if (!pMission_) return;
    AiTacticsDialog::show(hWnd_, &mapViewport_, pMission_, &commandStack_);
}

void MainWindow::openMissionDialog() {
    if (isModified()) {
        int res = showEditorMessageBox(hWnd_,
            L"You have unsaved changes in the current mission.\n\n"
            L"Do you want to discard them and open another mission?",
            L"Open Mission", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
        if (res != IDYES) {
            return;
        }
    }

    int chosenId = currentMissionId_;
    std::wstring customPath;
    if (MissionChooserDialog::show(hWnd_, currentMissionId_, chosenId, customPath)) {
        std::string pathNarrow = std::filesystem::path(customPath).string();
        loadMission(chosenId, pathNarrow);
    }
}

void MainWindow::playtestMission() {
    if (!pMission_) return;

    if (!validateMissionBeforeAction(L"Playtest")) {
        return;
    }

    if (PlaytestManager::isPlaytesting()) {
        showEditorMessageBox(hWnd_,
                    L"A playtest session is already active.\nPlease close ReSyndicated before starting another playtest.",
                    L"Playtest Active",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Launching Playtest session..."));
    bool success = PlaytestManager::launchPlaytest(hWnd_, pMission_, currentMissionId_, currentProps_);

    if (success) {
        SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Playtest active: ReSyndicated is running..."));
    } else {
        SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Playtest launch cancelled or failed."));
    }
}

void MainWindow::configurePlaytestLoadout() {
    PlaytestLoadoutData data = PlaytestLoadoutDialog::getDefaultLoadout();
    EditorConfig::loadLoadoutConfig(data);

    if (PlaytestLoadoutDialog::show(hWnd_, data)) {
        if (EditorConfig::saveLoadoutConfig(data)) {
            SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Playtest configuration saved to resynd.conf."));
        } else {
            SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Failed to save playtest configuration to resynd.conf."));
        }
        if (mapViewport_.hwnd()) {
            InvalidateRect(mapViewport_.hwnd(), nullptr, FALSE);
        }
        artPanel_.refreshPalette();
    }
}

bool MainWindow::validateMissionBeforeAction(const std::wstring &actionName) {
    if (!pMission_) return true;

    pMission_->buildNavigationGraph();
    auto report = MissionValidator::validate(pMission_);
    if (report.passed()) {
        return true;
    }

    std::wstring issueType = report.hasErrors() ? L"Validation issues" : L"Validation warnings";
    std::wstring msg = std::format(
        L"{} detected before {}:\n\n"
        L"\u2022 Total Issues: {} ({} Errors, {} Warnings)\n\n",
        issueType,
        actionName,
        report.issues.size(),
        report.errorCount,
        report.warningCount
    );

    size_t limit = std::min<size_t>(report.issues.size(), 8);
    for (size_t i = 0; i < limit; ++i) {
        const auto &issue = report.issues[i];
        std::wstring prefix = (issue.severity == ValidationSeverity::Error) ? L"[ERROR] " : L"[WARN] ";
        std::wstring entity = utf8ToWide(issue.entityDesc);
        std::wstring desc = utf8ToWide(issue.message);
        if (!entity.empty() && (issue.position.tx != 0 || issue.position.ty != 0 || issue.position.tz != 0)) {
            msg += std::format(L"{} {} at ({}, {}, {}): {}\n", prefix, entity, issue.position.tx, issue.position.ty, issue.position.tz, desc);
        } else if (!entity.empty()) {
            msg += std::format(L"{} {}: {}\n", prefix, entity, desc);
        } else {
            msg += std::format(L"{} {}\n", prefix, desc);
        }
    }

    if (report.issues.size() > limit) {
        msg += std::format(L"... and {} more issue(s).\n", report.issues.size() - limit);
    }

    msg += std::format(L"\nDo you want to proceed with {} anyway?", actionName);

    int result = showEditorMessageBox(
        hWnd_,
        msg.c_str(),
        L"Mission Validation Warning",
        MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2
    );

    return (result == IDYES);
}

void MainWindow::validateMission() {
    if (!pMission_) return;

    pMission_->buildNavigationGraph();
    auto report = MissionValidator::validate(pMission_);
    if (report.passed()) {
        showEditorMessageBox(
            hWnd_,
            L"All mission entities, player agents, walkability tiles, and objective targets are properly placed!\n\nNo issues found.",
            L"Mission Validation",
            MB_OK | MB_ICONINFORMATION
        );
        SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Mission validation passed: All entities properly placed."));
    } else {
        std::wstring msg = std::format(
            L"Mission validation found {} issue(s) ({} Errors, {} Warnings):\n\n",
            report.issues.size(),
            report.errorCount,
            report.warningCount
        );

        size_t limit = std::min<size_t>(report.issues.size(), 12);
        for (size_t i = 0; i < limit; ++i) {
            const auto &issue = report.issues[i];
            std::wstring prefix = (issue.severity == ValidationSeverity::Error) ? L"[ERROR] " : L"[WARN] ";
            std::wstring entity = utf8ToWide(issue.entityDesc);
            std::wstring desc = utf8ToWide(issue.message);
            if (!entity.empty() && (issue.position.tx != 0 || issue.position.ty != 0 || issue.position.tz != 0)) {
                msg += std::format(L"{} {} at ({}, {}, {}): {}\n", prefix, entity, issue.position.tx, issue.position.ty, issue.position.tz, desc);
            } else if (!entity.empty()) {
                msg += std::format(L"{} {}: {}\n", prefix, entity, desc);
            } else {
                msg += std::format(L"{} {}\n", prefix, desc);
            }
        }

        if (report.issues.size() > limit) {
            msg += std::format(L"... and {} more issue(s).\n", report.issues.size() - limit);
        }

        showEditorMessageBox(
            hWnd_,
            msg.c_str(),
            L"Mission Validation Results",
            MB_OK | (report.hasErrors() ? MB_ICONERROR : MB_ICONWARNING)
        );
        std::wstring stat = std::format(L"Mission validation: {} issues found.", report.issues.size());
        SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(stat.c_str()));
    }
}

void MainWindow::saveCurrentWindowPlacement() {
    if (!hWnd_ || IsIconic(hWnd_)) return;
    WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
    if (GetWindowPlacement(hWnd_, &wp)) {
        if (wp.showCmd == SW_SHOWMINIMIZED) return;
        EditorWindowConfig cfg;
        cfg.x = wp.rcNormalPosition.left;
        cfg.y = wp.rcNormalPosition.top;
        cfg.width = wp.rcNormalPosition.right - wp.rcNormalPosition.left;
        cfg.height = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        cfg.maximized = (wp.showCmd == SW_SHOWMAXIMIZED);

        RECT rc = {};
        GetClientRect(hWnd_, &rc);
        if (rc.right > 0) {
            int clientW = rc.right;
            int effectiveArtW = getEffectiveArtPanelWidth();
            int viewW = clientW - effectiveArtW - splitterWidth_;
            cfg.rightPaneWidth = artPanelWidth_;
            cfg.leftPaneWidth = std::max(0, viewW);
        } else {
            cfg.rightPaneWidth = artPanelWidth_;
            cfg.leftPaneWidth = std::max(0, cfg.width - artPanelWidth_ - splitterWidth_);
        }

        cfg.entityGridHeight = artPanel_.entityGridHeight();
        cfg.tileGridHeight = artPanel_.tileGridHeight();
        EditorConfig::saveWindowConfig(cfg);
    }
}

LRESULT CALLBACK MainWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    MainWindow *pThis = nullptr;
    if (msg == WM_NCCREATE) {
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        pThis = reinterpret_cast<MainWindow*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    } else {
        pThis = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (pThis) {
        return pThis->handleMessage(hWnd, msg, wParam, lParam);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT MainWindow::handleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_EXITSIZEMOVE: {
            saveCurrentWindowPlacement();
            return 0;
        }

        case WM_SIZE: {
            if (wParam == SIZE_MINIMIZED) {
                return 0;
            }
            layoutChildren();
            if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED) {
                saveCurrentWindowPlacement();
            }
            return 0;
        }

        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWnd_, &pt);
                RECT rc;
                GetClientRect(hWnd_, &rc);
                int effectiveArtW = getEffectiveArtPanelWidth();
                int viewW = rc.right - effectiveArtW - splitterWidth_;
                if (pt.x >= viewW && pt.x <= viewW + splitterWidth_ &&
                    pt.y >= toolbarHeight_ && pt.y <= rc.bottom - statusBarHeight_) {
                    SetCursor(LoadCursor(nullptr, IDC_SIZEWE));
                    return TRUE;
                }
            }
            break;
        }

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            RECT rc;
            GetClientRect(hWnd_, &rc);
            int effectiveArtW = getEffectiveArtPanelWidth();
            int viewW = rc.right - effectiveArtW - splitterWidth_;
            if (x >= viewW && x <= viewW + splitterWidth_ &&
                y >= toolbarHeight_ && y <= rc.bottom - statusBarHeight_) {
                isDraggingSplitter_ = true;
                dragStartMouseX_ = x;
                dragStartArtPanelWidth_ = effectiveArtW;
                SetCapture(hWnd_);
                return 0;
            }
            break;
        }

        case WM_MOUSEMOVE: {
            if (isDraggingSplitter_) {
                int x = GET_X_LPARAM(lParam);
                int delta = x - dragStartMouseX_;
                int newWidth = dragStartArtPanelWidth_ - delta;
                RECT rc;
                GetClientRect(hWnd_, &rc);
                int minWidth = 240;
                int maxWidth = rc.right - 300;
                if (maxWidth < minWidth) maxWidth = minWidth;
                newWidth = std::clamp(newWidth, minWidth, maxWidth);
                if (newWidth != artPanelWidth_) {
                    artPanelWidth_ = newWidth;
                    layoutChildren();
                    InvalidateRect(hWnd_, nullptr, FALSE);
                }
                return 0;
            }
            break;
        }

        case WM_LBUTTONUP: {
            if (isDraggingSplitter_) {
                isDraggingSplitter_ = false;
                ReleaseCapture();
                saveCurrentWindowPlacement();
                return 0;
            }
            break;
        }

        case WM_CAPTURECHANGED: {
            if (isDraggingSplitter_) {
                isDraggingSplitter_ = false;
                saveCurrentWindowPlacement();
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            RECT rc;
            GetClientRect(hWnd, &rc);
            int effectiveArtW = getEffectiveArtPanelWidth();
            int viewW = rc.right - effectiveArtW - splitterWidth_;
            RECT splitRc = { viewW, toolbarHeight_, viewW + splitterWidth_, rc.bottom - statusBarHeight_ };
            HBRUSH hBr = CreateSolidBrush(RGB(30, 36, 44));
            FillRect(hdc, &splitRc, hBr);
            DeleteObject(hBr);
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(65, 75, 90));
            HPEN hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, hPen));
            MoveToEx(hdc, viewW + splitterWidth_ / 2, toolbarHeight_, nullptr);
            LineTo(hdc, viewW + splitterWidth_ / 2, rc.bottom - statusBarHeight_);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            EndPaint(hWnd, &ps);
            return 0;
        }

        case WM_THEMECHANGED: {
            HMODULE hUx = GetModuleHandleW(L"uxtheme.dll");
            if (hUx) {
                auto flush = reinterpret_cast<void(WINAPI*)()>(GetProcAddress(hUx, MAKEINTRESOURCEA(136)));
                if (flush) flush();
            }
            DrawMenuBar(hWnd);
            break;
        }

        case WM_NCPAINT:
        case WM_NCACTIVATE: {
            LRESULT res = DefWindowProc(hWnd, msg, wParam, lParam);
            HDC hdc = GetWindowDC(hWnd);
            if (hdc) {
                RECT rcWin;
                GetWindowRect(hWnd, &rcWin);
                POINT ptClient = { 0, 0 };
                ClientToScreen(hWnd, &ptClient);
                int clientTop = ptClient.y - rcWin.top;
                int clientLeft = ptClient.x - rcWin.left;
                RECT rcClient;
                GetClientRect(hWnd, &rcClient);

                // Overwrite the 1px/2px white separator line right between the menubar and toolbar
                RECT rcSep = { clientLeft, clientTop - 2, clientLeft + rcClient.right, clientTop };
                HBRUSH hBr = CreateSolidBrush(RGB(28, 30, 34));
                FillRect(hdc, &rcSep, hBr);
                DeleteObject(hBr);

                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(44, 48, 56));
                HPEN hOld = static_cast<HPEN>(SelectObject(hdc, hPen));
                MoveToEx(hdc, clientLeft, clientTop - 1, nullptr);
                LineTo(hdc, clientLeft + rcClient.right, clientTop - 1);
                SelectObject(hdc, hOld);
                DeleteObject(hPen);

                ReleaseDC(hWnd, hdc);
            }
            return res;
        }

        case WM_UAHDRAWMENU: {
            UAHMENU *pUahMenu = reinterpret_cast<UAHMENU*>(lParam);
            if (pUahMenu && pUahMenu->hdc) {
                MENUBARINFO mbi = { sizeof(mbi) };
                GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);
                RECT rcWindow;
                GetWindowRect(hWnd, &rcWindow);
                RECT rcBar = mbi.rcBar;
                OffsetRect(&rcBar, -rcWindow.left, -rcWindow.top);

                // Fill entire menubar rectangle
                HBRUSH hBr = CreateSolidBrush(RGB(28, 30, 34));
                FillRect(pUahMenu->hdc, &rcBar, hBr);
                DeleteObject(hBr);

                // Draw bottom border line
                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(44, 48, 56));
                HPEN hOldPen = static_cast<HPEN>(SelectObject(pUahMenu->hdc, hPen));
                MoveToEx(pUahMenu->hdc, rcBar.left, rcBar.bottom - 1, nullptr);
                LineTo(pUahMenu->hdc, rcBar.right, rcBar.bottom - 1);
                SelectObject(pUahMenu->hdc, hOldPen);
                DeleteObject(hPen);
            }
            return 0;
        }

        case WM_UAHDRAWMENUITEM: {
            UAHDRAWMENUITEM *pUahItem = reinterpret_cast<UAHDRAWMENUITEM*>(lParam);
            if (pUahItem && pUahItem->dis.hDC) {
                HDC hdc = pUahItem->dis.hDC;
                RECT rc = pUahItem->dis.rcItem;

                bool isHot = (pUahItem->dis.itemState & ODS_HOTLIGHT) != 0;
                bool isSelected = (pUahItem->dis.itemState & ODS_SELECTED) != 0;
                bool isDisabled = (pUahItem->dis.itemState & (ODS_DISABLED | ODS_GRAYED)) != 0;

                COLORREF bgCol = RGB(28, 30, 34);
                if (isSelected) {
                    bgCol = RGB(42, 48, 56);
                } else if (isHot) {
                    bgCol = RGB(46, 52, 60);
                }

                HBRUSH hBr = CreateSolidBrush(bgCol);
                FillRect(hdc, &rc, hBr);
                DeleteObject(hBr);

                if (isSelected || isHot) {
                    HPEN hBorderPen = CreatePen(PS_SOLID, 1, isSelected ? RGB(0, 160, 220) : RGB(72, 82, 94));
                    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hBorderPen));
                    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
                    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
                    SelectObject(hdc, hOldBrush);
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hBorderPen);
                }

                wchar_t text[128] = {};
                GetMenuStringW(pUahItem->uahMenu.hmenu, pUahItem->iPosition, text, 128, MF_BYPOSITION);

                COLORREF textCol = RGB(220, 225, 230);
                if (isDisabled) {
                    textCol = RGB(110, 118, 126);
                } else if (isSelected || isHot) {
                    textCol = RGB(255, 255, 255);
                }

                int oldBkMode = SetBkMode(hdc, TRANSPARENT);
                COLORREF oldTextCol = SetTextColor(hdc, textCol);
                HFONT hFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
                HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

                DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                SelectObject(hdc, hOldFont);
                SetTextColor(hdc, oldTextCol);
                SetBkMode(hdc, oldBkMode);
            }
            return 0;
        }

        case WM_INITMENUPOPUP: {
            updateUIState();
            return 0;
        }

        case WM_CLOSE: {
            if (isModified()) {
                int res = showEditorMessageBox(hWnd_,
                    L"You have unsaved changes in the current mission.\n\n"
                    L"Are you sure you want to exit without saving?",
                    L"Exit ReSyndicated Editor", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
                if (res != IDYES) {
                    return 0;
                }
            }
            saveCurrentWindowPlacement();
            DestroyWindow(hWnd);
            return 0;
        }

        case WM_NOTIFY: {
            NMHDR *pNmhdr = reinterpret_cast<NMHDR*>(lParam);
            if (pNmhdr && pNmhdr->hwndFrom == hToolbar_ && pNmhdr->code == NM_CUSTOMDRAW) {
                LPNMTBCUSTOMDRAW pCust = reinterpret_cast<LPNMTBCUSTOMDRAW>(lParam);
                if (pCust->nmcd.dwDrawStage == CDDS_PREPAINT) {
                    HBRUSH hBr = CreateSolidBrush(RGB(28, 30, 34));
                    FillRect(pCust->nmcd.hdc, &pCust->nmcd.rc, hBr);
                    DeleteObject(hBr);
                    HPEN hLinePen = CreatePen(PS_SOLID, 1, RGB(44, 48, 56));
                    HGDIOBJ oldPen = SelectObject(pCust->nmcd.hdc, hLinePen);
                    MoveToEx(pCust->nmcd.hdc, pCust->nmcd.rc.left, pCust->nmcd.rc.bottom - 1, nullptr);
                    LineTo(pCust->nmcd.hdc, pCust->nmcd.rc.right, pCust->nmcd.rc.bottom - 1);
                    SelectObject(pCust->nmcd.hdc, oldPen);
                    DeleteObject(hLinePen);
                    return CDRF_NOTIFYITEMDRAW;
                }
                if (pCust->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                    RECT rc = pCust->nmcd.rc;
                    UINT state = pCust->nmcd.uItemState;
                    if ((state & CDIS_CHECKED) || (state & CDIS_SELECTED)) {
                        HBRUSH hBr = CreateSolidBrush(RGB(42, 48, 56));
                        FillRect(pCust->nmcd.hdc, &rc, hBr);
                        DeleteObject(hBr);
                        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 160, 220));
                        HGDIOBJ oldPen = SelectObject(pCust->nmcd.hdc, hPen);
                        HGDIOBJ oldBrush = SelectObject(pCust->nmcd.hdc, GetStockObject(NULL_BRUSH));
                        Rectangle(pCust->nmcd.hdc, rc.left, rc.top, rc.right, rc.bottom);
                        SelectObject(pCust->nmcd.hdc, oldPen);
                        SelectObject(pCust->nmcd.hdc, oldBrush);
                        DeleteObject(hPen);
                    } else if (state & CDIS_HOT) {
                        HBRUSH hBr = CreateSolidBrush(RGB(46, 52, 60));
                        FillRect(pCust->nmcd.hdc, &rc, hBr);
                        DeleteObject(hBr);
                        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(72, 82, 94));
                        HGDIOBJ oldPen = SelectObject(pCust->nmcd.hdc, hPen);
                        HGDIOBJ oldBrush = SelectObject(pCust->nmcd.hdc, GetStockObject(NULL_BRUSH));
                        Rectangle(pCust->nmcd.hdc, rc.left, rc.top, rc.right, rc.bottom);
                        SelectObject(pCust->nmcd.hdc, oldPen);
                        SelectObject(pCust->nmcd.hdc, oldBrush);
                        DeleteObject(hPen);
                    }
                    return CDRF_DODEFAULT;
                }
            }
            if (pNmhdr && (pNmhdr->code == TTN_GETDISPINFOW || pNmhdr->code == TTN_GETDISPINFOA)) {
                NMTTDISPINFOW *pTt = reinterpret_cast<NMTTDISPINFOW*>(lParam);
                switch (pTt->hdr.idFrom) {
                    case IDM_FILE_NEW: pTt->lpszText = const_cast<LPWSTR>(L"New Mission (Ctrl+N)"); break;
                    case IDM_FILE_OPEN: pTt->lpszText = const_cast<LPWSTR>(L"Open Mission (Ctrl+O)"); break;
                    case IDM_FILE_CLOSE: pTt->lpszText = const_cast<LPWSTR>(L"Close Mission (Ctrl+W)"); break;
                    case IDM_FILE_SAVE: pTt->lpszText = const_cast<LPWSTR>(L"Save Mission (Ctrl+S)"); break;
                    case IDM_EDIT_UNDO: pTt->lpszText = const_cast<LPWSTR>(L"Undo (Ctrl+Z)"); break;
                    case IDM_EDIT_REDO: pTt->lpszText = const_cast<LPWSTR>(L"Redo (Ctrl+Y)"); break;
                    case IDM_VIEW_SLICE: pTt->lpszText = const_cast<LPWSTR>(L"Slice View / Z-Isolation (Z)"); break;
                    case IDM_LAYER_DOWN: pTt->lpszText = const_cast<LPWSTR>(L"Layer Down (PgDn / [) [Shift to Move]"); break;
                    case IDM_LAYER_UP: pTt->lpszText = const_cast<LPWSTR>(L"Layer Up (PgUp / ]) [Shift to Move]"); break;
                    case IDM_VIEW_CENTER_SQUAD: pTt->lpszText = const_cast<LPWSTR>(L"Center on Player's Agents (Home)"); break;
                    case IDM_MISSION_PLAYTEST: pTt->lpszText = const_cast<LPWSTR>(L"Playtest Mission in ReSyndicated (F5)"); break;
                    case IDM_VIEW_ZOOM_OUT: pTt->lpszText = const_cast<LPWSTR>(L"Zoom Out (-)"); break;
                    case IDM_VIEW_ZOOM_100: pTt->lpszText = const_cast<LPWSTR>(L"Zoom 100% (1)"); break;
                    case IDM_VIEW_ZOOM_IN: pTt->lpszText = const_cast<LPWSTR>(L"Zoom In (+)"); break;
                }
                return 0;
            }
            break;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (mapViewport_.isObjectivePicking()) {
                return 0;
            }
            if (!hasActiveMission() &&
                id != IDM_FILE_NEW &&
                id != IDM_FILE_OPEN &&
                id != IDM_FILE_EXIT &&
                id != IDM_FILE_SETTINGS &&
                id != IDM_HELP_SHORTCUTS &&
                id != IDM_HELP_TOOLS &&
                id != IDM_HELP_ABOUT) {
                return 0;
            }

            switch (id) {
                case IDM_FILE_NEW:
                    newMission();
                    return 0;
                case IDM_FILE_OPEN:
                    openMissionDialog();
                    return 0;
                case IDM_FILE_CLOSE:
                    closeMission();
                    return 0;
                case IDM_FILE_SAVE:
                    saveMission();
                    return 0;
                case IDM_FILE_SAVEAS:
                    saveMissionAs();
                    return 0;
                case IDM_FILE_SETTINGS:
                    showSettingsDialog();
                    return 0;
                case IDM_FILE_EXIT: {
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    return 0;
                }

                case IDM_EDIT_UNDO:
                    if (commandStack_.canUndo()) {
                        commandStack_.undo();
                        void *rawSel = static_cast<void*>(artPanel_.selectedEntity());
                        bool valid = false;
                        if (pMission_ && rawSel) {
                            for (size_t i = 0; i < pMission_->numPeds(); ++i) {
                                if (static_cast<void*>(pMission_->ped(i)) == rawSel) { valid = true; break; }
                            }
                            if (!valid) {
                                for (size_t i = 0; i < pMission_->numVehicles(); ++i) {
                                    if (static_cast<void*>(pMission_->vehicle(i)) == rawSel) { valid = true; break; }
                                }
                            }
                            if (!valid) {
                                for (size_t i = 0; i < pMission_->numStatics(); ++i) {
                                    if (static_cast<void*>(pMission_->statics(i)) == rawSel) { valid = true; break; }
                                }
                            }
                            if (!valid) {
                                for (size_t i = 0; i < pMission_->numWeaponsOnGround(); ++i) {
                                    if (static_cast<void*>(pMission_->weaponOnGround(i)) == rawSel) { valid = true; break; }
                                }
                            }
                        }
                        if (valid) {
                            artPanel_.refreshInspector();
                        } else {
                            artPanel_.clearSelectedEntity();
                            mapViewport_.clearSelection();
                        }
                        artPanel_.refreshObjectivesList();
                        InvalidateRect(artPanel_.hwnd(), nullptr, FALSE);
                        InvalidateRect(mapViewport_.hwnd(), nullptr, FALSE);
                        updateUIState();
                        updateWindowTitle();
                    }
                    return 0;

                case IDM_EDIT_REDO:
                    if (commandStack_.canRedo()) {
                        commandStack_.redo();
                        void *rawSel = static_cast<void*>(artPanel_.selectedEntity());
                        bool valid = false;
                        if (pMission_ && rawSel) {
                            for (size_t i = 0; i < pMission_->numPeds(); ++i) {
                                if (static_cast<void*>(pMission_->ped(i)) == rawSel) { valid = true; break; }
                            }
                            if (!valid) {
                                for (size_t i = 0; i < pMission_->numVehicles(); ++i) {
                                    if (static_cast<void*>(pMission_->vehicle(i)) == rawSel) { valid = true; break; }
                                }
                            }
                            if (!valid) {
                                for (size_t i = 0; i < pMission_->numStatics(); ++i) {
                                    if (static_cast<void*>(pMission_->statics(i)) == rawSel) { valid = true; break; }
                                }
                            }
                            if (!valid) {
                                for (size_t i = 0; i < pMission_->numWeaponsOnGround(); ++i) {
                                    if (static_cast<void*>(pMission_->weaponOnGround(i)) == rawSel) { valid = true; break; }
                                }
                            }
                        }
                        if (valid) {
                            artPanel_.refreshInspector();
                        } else {
                            artPanel_.clearSelectedEntity();
                            mapViewport_.clearSelection();
                        }
                        artPanel_.refreshObjectivesList();
                        InvalidateRect(artPanel_.hwnd(), nullptr, FALSE);
                        InvalidateRect(mapViewport_.hwnd(), nullptr, FALSE);
                        updateUIState();
                        updateWindowTitle();
                    }
                    return 0;

                case IDM_EDIT_DELETE:
                    mapViewport_.deleteSelection();
                    artPanel_.clearSelectedEntity();
                    updateUIState();
                    updateWindowTitle();
                    return 0;

                case IDM_EDIT_DESELECT:
                    if (mapViewport_.tool() == ViewportTool::RouteTool) {
                        mapViewport_.setTool(ViewportTool::SelectEntity);
                        updateUIState();
                        return 0;
                    }
                    artPanel_.clearSelectedEntity();
                    mapViewport_.clearSelection();
                    return 0;

                case IDM_TOOLS_PENCIL:
                    artPanel_.activatePencilTool();
                    updateUIState();
                    return 0;
                case IDM_TOOLS_BOXFILL:
                    artPanel_.activateBoxFillTool();
                    updateUIState();
                    return 0;
                case IDM_TOOLS_SELECT:
                    artPanel_.activateSelectTool();
                    updateUIState();
                    return 0;
                case IDM_TOOLS_SEARCH:
                    if (!artPanel_.confirmDiscardEntityEdits()) return 0;
                    if (mapViewport_.isObjectivePicking()) return 0;
                    artPanel_.activateSearchTool();
                    updateUIState();
                    return 0;

                case IDM_VIEW_SLICE:
                    mapViewport_.setSliceView(!mapViewport_.isSliceView());
                    updateUIState();
                    return 0;

                case IDM_VIEW_SHOW_STATS:
                    mapViewport_.setShowStats(!mapViewport_.isShowStats());
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_BUILDINGS:
                    mapViewport_.setFilterBuildings(!mapViewport_.isFilterBuildings());
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_VEHICLES:
                    mapViewport_.setFilterVehicles(!mapViewport_.isFilterVehicles());
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_CIVILIANS:
                    mapViewport_.setFilterCivilians(!mapViewport_.isFilterCivilians());
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_POLICE:
                    mapViewport_.setFilterPolice(!mapViewport_.isFilterPolice());
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_GUARDS:
                    mapViewport_.setFilterGuards(!mapViewport_.isFilterGuards());
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_CRIMINALS:
                    mapViewport_.setFilterCriminals(!mapViewport_.isFilterCriminals());
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_AGENTS:
                    mapViewport_.setFilterAgents(!mapViewport_.isFilterAgents());
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_WEAPONS:
                    mapViewport_.setFilterWeapons(!mapViewport_.isFilterWeapons());
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_SELECT_ALL:
                    mapViewport_.selectAllFilters(true);
                    updateUIState();
                    return 0;

                case IDM_VIEW_FILTER_CLEAR_ALL:
                    mapViewport_.selectAllFilters(false);
                    updateUIState();
                    return 0;

                case IDM_LAYER_UP:
                    if (GetKeyState(VK_SHIFT) < 0) {
                        if (mapViewport_.hasSelectedEntity() || mapViewport_.hasSelectedTile()) {
                            mapViewport_.moveSelectionZ(1);
                            updateWindowTitle();
                        }
                    } else {
                        mapViewport_.stepSelectionZ(1);
                    }
                    updateUIState();
                    return 0;
                case IDM_LAYER_DOWN:
                    if (GetKeyState(VK_SHIFT) < 0) {
                        if (mapViewport_.hasSelectedEntity() || mapViewport_.hasSelectedTile()) {
                            mapViewport_.moveSelectionZ(-1);
                            updateWindowTitle();
                        }
                    } else {
                        mapViewport_.stepSelectionZ(-1);
                    }
                    updateUIState();
                    return 0;

                case IDM_LAYER_0:
                case IDM_LAYER_1:
                case IDM_LAYER_2:
                case IDM_LAYER_3:
                case IDM_LAYER_4:
                case IDM_LAYER_5:
                case IDM_LAYER_6:
                case IDM_LAYER_7:
                case IDM_LAYER_8:
                case IDM_LAYER_9:
                case IDM_LAYER_10:
                case IDM_LAYER_11:
                case IDM_LAYER_12:
                    mapViewport_.setActiveZ(id - IDM_LAYER_0);
                    updateUIState();
                    return 0;

                case IDM_LAYER_AUTO:
                    mapViewport_.toggleAutoLayerSwitching();
                    updateUIState();
                    return 0;

                case IDM_VIEW_CENTER_SQUAD:
                    mapViewport_.centerOnSquadSpawn();
                    updateUIState();
                    return 0;

                case IDM_VIEW_ZOOM_IN:
                    mapViewport_.zoomIn();
                    return 0;
                case IDM_VIEW_ZOOM_OUT:
                    mapViewport_.zoomOut();
                    return 0;
                case IDM_VIEW_ZOOM_100:
                    mapViewport_.setZoom(1.0f);
                    return 0;
                case IDM_VIEW_ZOOM_150:
                    mapViewport_.setZoom(1.5f);
                    return 0;
                case IDM_VIEW_ZOOM_200:
                    mapViewport_.setZoom(2.0f);
                    return 0;

                case IDM_MISSION_PLAYTEST:
                    playtestMission();
                    return 0;

                case IDM_MISSION_LOADOUT:
                    configurePlaytestLoadout();
                    return 0;

                case IDM_MISSION_PROPERTIES:
                    showMissionProperties();
                    return 0;

                case IDM_MISSION_VALIDATE:
                    validateMission();
                    return 0;

                case IDM_MISSION_AI_TACTICS:
                    showAiTacticsDialog();
                    return 0;

                case IDM_HELP_TOOLS:
                    showToolsGuideDialog(hWnd);
                    return 0;

                case IDM_HELP_SHORTCUTS: {
                    showShortcutsDialog(hWnd);
                    return 0;
                }

                case IDM_HELP_ABOUT:
                    showAboutDialog(hWnd);
                    return 0;
            }
            break;
        }

        case WM_PLAYTEST_FINISHED: {
            DWORD exitCode = static_cast<DWORD>(wParam);
            if (exitCode == 0) {
                SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Playtest session completed successfully. Returned to editor."));
            } else {
                SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(L"Playtest session finished."));
            }
            return 0;
        }


        case WM_DESTROY: {
            saveCurrentWindowPlacement();
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void MainWindow::showSettingsDialog() {
    EditorPathsConfig currentPaths;
    if (!EditorConfig::loadPathsConfig(currentPaths) || currentPaths.dataDir.empty()) {
        currentPaths.dataDir = rs_utl::File::getOriginalDataFolder().string();
    }
    EditorPathsConfig newPaths = currentPaths;
    if (showEditorSettingsDialog(hWnd_, newPaths)) {
        if (!newPaths.dataDir.empty() && rs_eng::pathsAreDifferent(currentPaths.dataDir, newPaths.dataDir)) {
            newPaths.dataSignature = rs_utl::File::computeDataSignature(newPaths.dataDir);
            EditorConfig::savePathsConfig(newPaths);
            rs_eng::saveOriginalDataDirToResyndConf(newPaths.dataDir, newPaths.dataSignature);
            reloadAssets(newPaths.dataDir);
        }
    }
}

bool MainWindow::reloadAssets(const std::string &newResyndDir, const std::string &newOriginalDir) {
    rs_utl::File::setResyndDataFolder("");
    rs_utl::File::setOriginalDataFolder(newOriginalDir.empty() ? newResyndDir : newOriginalDir);

    // If switching to original Syndicate assets (non-AR), sanitize any AR-only weapons from playtest loadout
    if (!rs_utl::File::hasAmericanRevoltData()) {
        PlaytestLoadoutData playtestData;
        if (EditorConfig::loadLoadoutConfig(playtestData)) {
            if (EditorConfig::sanitizeLoadout(playtestData)) {
                EditorConfig::saveLoadoutConfig(playtestData);
            }
        }
    }

    int savedMissionId = currentMissionId_;
    bool hadMission = (pMission_ != nullptr && savedMissionId > 0);

    // 1. Detach old mission from UI and clean up mission pointers
    artPanel_.clearSelectedEntity();
    mapViewport_.clearSelection();
    commandStack_.clear();
    savedUndoCount_ = 0;
    mapViewport_.setMission(nullptr, 0);
    mapViewport_.selectAllFilters(true);
    artPanel_.setMission(nullptr);
    if (pMissionManager_) {
        pMissionManager_->destroyMission();
    }
    pMission_ = nullptr;

    // 2. Destroy managers before constructing new ones
    // CRITICAL: pAnimManager_ must be destroyed before creating a new AnimationManager
    // because AnimationManager is a Singleton<AnimationManager> which asserts(!singleton_) in its constructor!
    pMissionManager_.reset();
    pTileManager_.reset();
    pAnimManager_.reset();

    // 3. Reload rasterizers
    int reloadPal = (savedMissionId > 0) ? rs_knl::MissionManager::getMissionPaletteId(savedMissionId) : 1;
    TileRasterizer::instance().load(reloadPal);
    SpriteRasterizer::instance().load(true);
    SpriteRasterizer::instance().loadMenuSprites(true);

    // 4. Reconstruct animation manager and load animations
    pAnimManager_ = std::make_unique<rs_eng::AnimationManager>();
    rs_eng::OriginalFilesAnimationLoader animLoader;
    try {
        animLoader.loadAnimations(*pAnimManager_);
    } catch (...) {
        // Continue even if animations fail
    }

    // 5. Reconstruct tile and mission managers
    pTileManager_ = std::make_unique<rs_eng::TileManager>();
    pTileManager_->loadTiles();
    pMissionManager_ = std::make_unique<rs_knl::MissionManager>(pTileManager_.get());

    artPanel_.refreshPalette();

    if (hadMission) {
        loadMission(savedMissionId);
    } else {
        InvalidateRect(mapViewport_.hwnd(), nullptr, TRUE);
    }

    updateUIState();
    updateWindowTitle();

    std::wstring msg = std::format(L"Asset directory updated: {}",
        utf8ToWide(newOriginalDir.empty() ? newResyndDir : newOriginalDir));
    SendMessage(hStatusBar_, SB_SETTEXTW, 2, reinterpret_cast<LPARAM>(msg.c_str()));

    return true;
}

} // namespace rs_win_editor
