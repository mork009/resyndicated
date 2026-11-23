/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
 *   Copyright (C) 2007  Davor Ocelic <docelic@mail.inet.hr>
 *   Copyright (C) 2010, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net>
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

#include "gameplaymenu.h"

#include <cassert>
#include <format>

#include "tracy/Tracy.hpp"

#include "rs-engine/appcontext.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/logomanager.h"
#include "rs-engine/sound/musicmanager.h"
#include "rs-engine/sound/soundmanager.h"
#include "menus/gamemenuid.h"
#include "rs-engine/menus/fliplayer.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/model/shot.h"
#include "rs-kernel/model/leveldata.h"
#include "core/gamecontroller.h"
#include "core/gamesession.h"

#include <windows.h>

// The number of pixel of a scroll
const int kScrollStep = 16;

const Point2D GameplayMenu::kMiniMapScreenPos = {0, 46 + 44 + 10 + 46 + 44 + 15 + 2 * 32 + 2};
const uint32_t GameplayMenu::kDoublePressMs = 400;

GameplayMenu::GameplayMenu(rs_eng::MenuManager *m) :
Menu(m, rs_game_menus::kMenuIdGameplay, rs_game_menus::kMenuIdDebrief),
tick_count_(0), last_animate_tick_(0),
drawHintYellowRect_(false),
hintTimer_(1200, false), hintColorTimer_(200, false), hintSpaceTimer_(500, false),
mission_(nullptr), selection_(), target_(nullptr),
mm_renderer_(kMiniMapScreenPos), warningTimer_(20000)
{
    cursorOnShow_ = kGameplayCursor;
    scroll_ = {0, 0};
    ipa_chng_.ipa_chng = -1;
    canPlayPoliceWarnSound_ = true;
    lastSelectionTick_ = 0;
}

/*!
 * Initialize the screen position centered on the squad leader.
 */
void GameplayMenu::initRenderers() {
    // Init renderers
    map_renderer_.init(mission_, &selection_);
    mm_renderer_.init(mission_, mission_->getSquad()->hasScanner(), missionPalette_);
    centerMinimapOnLeader();
}

/*!
 * Called before the menu is first shown.
 * Place to do some initializing.
 */
bool GameplayMenu::handleBeforeShow() {
    mission_ = g_missionCtrl.mission();
    mission_->start(g_gameCtrl.weaponManager());
    // init selection to the first selectable agent
    selection_.setSquad(mission_->getSquad());

    // init menu internal state
    isButtonSelectAllPressed_ = false;

    // set graphic palette
    missionPalette_ = mission_->map()->getTileManager()->getPalette();
    g_SpriteMgr.setPlayerFactionColor(g_LogoMgr.getColorAtIndex(g_Session.getLogoColour()));
    g_SpriteMgr.setPalette(missionPalette_);

    highlightLeaderMarker();
    updateMarkersPosition();

    initRenderers();

    isPlayerShooting_ = false;

    // Register event handlers
    handleAgentDied_ = EventManager::listen<rs_knl::AgentDiedEvent>(this, &GameplayMenu::onAgentDiedEvent);
    handleWeaponSelected_ = EventManager::listen<rs_knl::ShootingWeaponSelectedEvent>(this, &GameplayMenu::onShootingWeaponSelectedEvent);
    handleAgentWarned_ = EventManager::listen<rs_knl::PoliceWarningEmittedEvent>(this, &GameplayMenu::onPoliceWarningEmittedEvent);
    handleMissionEnded_ = EventManager::listen<rs_knl::MissionEndedEvent>(this, &GameplayMenu::onMissionEndedEvent);

    // play game track
    g_MusicMgr.playSong(rs_eng::MusicManager::kMusicSongAssassinate, true);
    
    menu_manager_->resetSinceMouseDown();

    return true;
}

int GameplayMenu::isMousePositionScrollonX(Point2D point) {
    const int edge_margin = 15;
    const int corner_length = 60;

    bool near_left = (point.x < corner_length);
    bool near_right = (point.x > rs_eng::kScreenWidth - corner_length);
    bool near_top = (point.y < corner_length);
    bool near_bottom = (point.y > rs_eng::kScreenHeight - corner_length);

    bool edge_left = (point.x < edge_margin);
    bool edge_right = (point.x > rs_eng::kScreenWidth - edge_margin);
    bool edge_top = (point.y < edge_margin);
    bool edge_bottom = (point.y > rs_eng::kScreenHeight - edge_margin);

    // Top-Left corner intersection
    if ((edge_top && near_left) || (edge_left && near_top)) {
        return -1;
    }
    // Top-Right corner intersection
    if ((edge_top && near_right) || (edge_right && near_top)) {
        return 1;
    }
    // Bottom-Left corner intersection
    if ((edge_bottom && near_left) || (edge_left && near_bottom)) {
        return -1;
    }
    // Bottom-Right corner intersection
    if ((edge_bottom && near_right) || (edge_right && near_bottom)) {
        return 1;
    }

    // Straight cardinal edges
    if (edge_left) {
        return -1;
    }
    if (edge_right) {
        return 1;
    }

    return 0;
}

int GameplayMenu::isMousePositionScrollonY(Point2D point) {
    const int edge_margin = 15;
    const int corner_length = 60;

    bool near_left = (point.x < corner_length);
    bool near_right = (point.x > rs_eng::kScreenWidth - corner_length);
    bool near_top = (point.y < corner_length);
    bool near_bottom = (point.y > rs_eng::kScreenHeight - corner_length);

    bool edge_left = (point.x < edge_margin);
    bool edge_right = (point.x > rs_eng::kScreenWidth - edge_margin);
    bool edge_top = (point.y < edge_margin);
    bool edge_bottom = (point.y > rs_eng::kScreenHeight - edge_margin);

    // Top-Left corner intersection
    if ((edge_top && near_left) || (edge_left && near_top)) {
        return -1;
    }
    // Top-Right corner intersection
    if ((edge_top && near_right) || (edge_right && near_top)) {
        return -1;
    }
    // Bottom-Left corner intersection
    if ((edge_bottom && near_left) || (edge_left && near_bottom)) {
        return 1;
    }
    // Bottom-Right corner intersection
    if ((edge_bottom && near_right) || (edge_right && near_bottom)) {
        return 1;
    }

    // Straight cardinal edges
    if (edge_top) {
        return -1;
    }
    if (edge_bottom) {
        return 1;
    }

    return 0;
}

void GameplayMenu::handleMapPanning(const Point2D &mousePos) {
    int mouseDirX = isMousePositionScrollonX(mousePos);
    int mouseDirY = isMousePositionScrollonY(mousePos);

    int keyDirX = 0;
    bool keyLeft = g_System.isKeyPressed(rs_eng::kKeyCode_Left);
    bool keyRight = g_System.isKeyPressed(rs_eng::kKeyCode_Right);
    if (keyLeft && !keyRight) {
        keyDirX = -1;
    } else if (keyRight && !keyLeft) {
        keyDirX = 1;
    }

    int keyDirY = 0;
    bool keyUp = g_System.isKeyPressed(rs_eng::kKeyCode_Up);
    bool keyDown = g_System.isKeyPressed(rs_eng::kKeyCode_Down);
    if (keyUp && !keyDown) {
        keyDirY = -1;
    } else if (keyDown && !keyUp) {
        keyDirY = 1;
    }

    int dirX = (mouseDirX != 0) ? mouseDirX : keyDirX;
    int dirY = (mouseDirY != 0) ? mouseDirY : keyDirY;

    if (dirX != 0 || dirY != 0) {
        int stepX = kScrollStep;
        int stepY = (dirX != 0 && dirY != 0) ? (kScrollStep / 2) : kScrollStep;

        if (dirX != 0) {
            map_renderer_.scrollOnX(dirX * stepX);
        }
        if (dirY != 0) {
            map_renderer_.scrollOnY(dirY * stepY);
        }

        scroll_.x = dirX * stepX;
        scroll_.y = dirY * stepY;
    } else {
        scroll_ = {0, 0};
    }
}

/*!
 * @brief 
 * @param point 
 */
void GameplayMenu::updateCursorFromTarget(const Point2D &point) {
    last_motion_x_ = point.x;
    last_motion_y_ = point.y;

    bool inrange = false;
    target_ = nullptr;

    if (point.x > 128) {
        for (size_t i = 0; mission_ && i < mission_->numPeds(); ++i) {
            rs_knl::PedInstance *p = mission_->ped(i);
            if (p->isAlive() && p->isDrawable()) {
                Point2D padTopLeft = { 10,
                    (1 + p->tileZ()) * rs_eng::Tile::kTileHeight / 3 - 
                    (p->offZ() * rs_eng::Tile::kTileHeight / 3) / 128};

                if (map_renderer_.isMouseHovering(point, *p, padTopLeft, {21, 34})) {
                    target_ = p;
                    if (!p->isOurAgent() && !p->isPersuaded()) {
                        inrange = selection_.isTargetInRange(mission_, target_);
                    }
                    break;
                }
            }
        }

        if (target_ == nullptr) {
            for (size_t i = 0; mission_ && i < mission_->numVehicles(); ++i) {
                rs_knl::Vehicle *v = mission_->vehicle(i);
                if (v->isAlive() && v->getType() != rs_knl::Vehicle::kVehicleTypeTrainHead) {
                    Point2D padTopLeft = { 20, 10 + v->tileZ() * rs_eng::Tile::kTileHeight / 3};

                    if (map_renderer_.isMouseHovering(point, *v, padTopLeft, {40, 32})) {
                        target_ = v;
                        inrange = selection_.isTargetInRange(mission_, target_);
                        break;
                    }
                }
            }
        }

        if (target_ == nullptr) {
            for (size_t i = 0; mission_ && i < mission_->numWeaponsOnGround(); ++i) {
                rs_knl::WeaponInstance *w = mission_->weaponOnGround(i);

                if (w->isDrawable()) {
                    Point2D padTopLeft = { 10, -4 + w->tileZ() * rs_eng::Tile::kTileHeight / 3
                        + (w->offZ() * rs_eng::Tile::kTileHeight / 3) / 128};

                    if (map_renderer_.isMouseHovering(point, *w, padTopLeft, {20, 15})) {
                        target_ = w;
                        break;
                    }
                }
            }
        }

        if (target_ == nullptr) {
            for (size_t i = 0; mission_ && i < mission_->numStatics(); ++i) {
                rs_knl::Static *s = mission_->statics(i);
                if (s && s->isDrawable() && s->startHealth() > 0) {
                    Point2D padTopLeft = { 16, 16 + s->tileZ() * rs_eng::Tile::kTileHeight / 3};

                    if (map_renderer_.isMouseHovering(point, *s, padTopLeft, {32, 32})) {
                        target_ = s;
                        break;
                    }
                }
            }
        }
    }

    if (target_) {
        if (target_->is(rs_knl::MapObject::kNaturePed)) {
            rs_knl::PedInstance *pPed = static_cast<rs_knl::PedInstance *>(target_);
            if (pPed->isOurAgent()) {
                g_System.usePointerCursor();
            } else if (pPed->isPersuaded()) {
                g_System.useTargetCursor();
            } else {
                if (inrange)
                    g_System.useTargetRedCursor();
                else
                    g_System.useTargetCursor();
            }
        } else if (target_->is(rs_knl::MapObject::kNatureVehicle)) {
            if (inrange)
                g_System.useTargetRedCursor();
            else
                g_System.useTargetCursor();
        } else if (target_->is(rs_knl::MapObject::kNatureWeapon)) {
            g_System.usePickupCursor();
        }
    } else if (point.x > 128) {
        g_System.usePointerCursor();
    } else {
        g_System.usePointerYellowCursor();
    }
}

bool GameplayMenu::handleTick(uint32_t elapsed)
{
    ZoneScoped;
    if (paused_) {
        if (g_Ctx.isPausePanningAllowed()) {
            Point2D mousePos;
            g_System.getMousePos(mousePos);

            handleMapPanning(mousePos);

            updateCursorFromTarget(mousePos);
        }
        return true;
    }

    tick_count_ += elapsed;

    if (mission_->isRunning()) {
        // Update stats
        mission_->stats()->incrMissionDuration(elapsed);
        mission_->checkObjectives();

        bool hostileEngaged = false;
        for (size_t i = 0; i < mission_->numPeds(); ++i) {
            rs_knl::PedInstance *pPed = mission_->ped(i);
            // In original DOS Syndicate, combat danger music (kMusicSongDanger) triggers exclusively
            // when a hostile Enemy Agent (kPedTypeAgent) is actively engaged in combat proximity
            // (~20 tiles = 5120 world units) of the player squad.
            // Police officers and platform guards never trigger danger music when engaging our agents;
            // ambient mission tension music continues uninterrupted while police sirens and gunshots play as SFX.
            if (pPed && pPed->isAlive() && pPed->type() == rs_knl::PedInstance::kPedTypeAgent &&
                !pPed->isOurAgent() && pPed->behaviour().hasHostileEngaged()) {
                constexpr int kAgentDangerProximity = 5120; // 20 tiles
                bool inCombatProximity = false;
                if (mission_->getSquad()) {
                    for (size_t a = 0; a < mission_->getSquad()->size(); ++a) {
                        rs_knl::PedInstance *pAgent = mission_->getSquad()->member(a);
                        if (pAgent && pAgent->isAlive() && pPed->isCloseTo(pAgent, kAgentDangerProximity)) {
                            inCombatProximity = true;
                            break;
                        }
                    }
                }
                if (inCombatProximity) {
                    hostileEngaged = true;
                    break;
                }
            }
        }

        if (hostileEngaged) {
            g_MusicMgr.playSong(rs_eng::MusicManager::kMusicSongDanger, true);
        } else {
            g_MusicMgr.playSong(rs_eng::MusicManager::kMusicSongAssassinate, true);
        }
    }

    if (!canPlayPoliceWarnSound_ && warningTimer_.update(elapsed)) {
        // wait an amount of time before allowing another warning
        canPlayPoliceWarnSound_ = true;
    }

    Point2D mousePos;
    g_System.getMousePos(mousePos);

    // Scroll the map
    handleMapPanning(mousePos);

    if (tick_count_ - last_animate_tick_ > 33) {
        uint32_t diff = tick_count_ - last_animate_tick_;
        last_animate_tick_ = tick_count_;

        mission_->handleTick(elapsed, diff);

        updateMarkersPosition();
    }

    mm_renderer_.setScannerEnabled(mission_->getSquad()->hasScanner());
    mm_renderer_.handleTick(elapsed);

    updateIPALevelMeters(elapsed);

    // If scrolling has happen, then update cursor
    updateCursorFromTarget(mousePos);

    updateMissionHint(elapsed);

    return true;
}

void GameplayMenu::handleRender() {
    map_renderer_.render();
    g_System.drawRect({0,0}, 129, rs_eng::kScreenHeight, menu_manager_->kMenuColorBlack);
    agt_sel_renderer_.render(selection_, mission_->getSquad(), missionPalette_);
    drawSelectAllButton();
    drawMissionHint();
    drawWeaponSelectors();
    mm_renderer_.render(missionPalette_);

    if (paused_) {
        drawPausePanel();
    }

    if (g_System.getShowCoordinates()) {
        int mouse_x = last_motion_x_;
        int mouse_y = last_motion_y_;
        int tile_x = 0;
        int tile_y = 0;
        int tile_z = 0;
        if (mouse_x > 128 && mission_ && mission_->map()) {
            Point2D origin = map_renderer_.getViewportOriginPt();
            int virtualX = static_cast<int>((mouse_x - 129) * rs_eng::g_mapZoom) + origin.x;
            int virtualY = static_cast<int>(mouse_y * rs_eng::g_mapZoom) + origin.y;
            rs_knl::Map *pMap = mission_->map();

            rs_knl::TilePoint groundPt = pMap->screenToTilePoint(virtualX, virtualY);
            rs_knl::TilePoint walkPt = groundPt;
            if (mission_->findReachableTileFromBase(walkPt)) {
                tile_x = walkPt.tx;
                tile_y = walkPt.ty;
                tile_z = walkPt.tz;
            } else if (mission_->getWalkableClosestByZ(walkPt)) {
                tile_x = walkPt.tx;
                tile_y = walkPt.ty;
                tile_z = walkPt.tz;
            } else {
                tile_x = groundPt.tx;
                tile_y = groundPt.ty;
                tile_z = 0;
            }

            if (pMap && tile_x >= 0 && tile_x < pMap->maxTx() && tile_y >= 0 && tile_y < pMap->maxTy()) {
                Point2D tileSc;
                pMap->tileToScreenPoint({(uint16_t)tile_x, (uint16_t)tile_y, 0}, &tileSc);

                int topVX = tileSc.x;
                int topVY = tileSc.y - (tile_z > 0 ? (tile_z - 1) : 0) * (rs_eng::Tile::kTileHeight / 3);

                auto mapToScreen = [&](int vx, int vy) -> Point2D {
                    int sx = static_cast<int>((vx - origin.x) / rs_eng::g_mapZoom) + 129;
                    int sy = static_cast<int>((vy - origin.y) / rs_eng::g_mapZoom);
                    return {sx, sy};
                };

                int tw2 = rs_eng::Tile::kTileWidth / 2;
                int th3 = rs_eng::Tile::kTileHeight / 3;

                Point2D t = mapToScreen(topVX, topVY);
                Point2D r = mapToScreen(topVX + tw2, topVY + th3);
                Point2D b = mapToScreen(topVX, topVY + 2 * th3);
                Point2D l = mapToScreen(topVX - tw2, topVY + th3);

                rs_eng::RSColor color = menu_manager_->kMenuColorYellow;
                g_System.drawLine(t, r, color);
                g_System.drawLine(r, b, color);
                g_System.drawLine(b, l, color);
                g_System.drawLine(l, t, color);

                g_System.drawLine(t.add(0, 1), r.add(-1, 0), color);
                g_System.drawLine(r.add(-1, 0), b.add(0, -1), color);
                g_System.drawLine(b.add(0, -1), l.add(1, 0), color);
                g_System.drawLine(l.add(1, 0), t.add(0, 1), color);
            }
        }

        std::stringstream coordText;
        coordText << "SCREEN: (X: " << mouse_x << ", Y: " << mouse_y << ") \nTILE: (X: " << tile_x << ", Y: " << tile_y << ", Z: " << tile_z << ")";
        int draw_x = rs_eng::kScreenWidth - 200;
        gameFont()->drawText(draw_x, 10, coordText.str().c_str(), menu_manager_->kMenuColorWhite);
    }

    if (g_System.getShowStats() && target_) {
        renderStatsOverlay(target_);
    }
}

void GameplayMenu::handleLeave()
{
    g_MusicMgr.stopCurrentSong();

    // Remove handlers to prevent events coming after the end of mission
    EventManager::remove_listener(handleAgentDied_);
    EventManager::remove_listener(handleWeaponSelected_);
    EventManager::remove_listener(handleAgentWarned_);
    EventManager::remove_listener(handleMissionEnded_);

    selection_.clear();

    tick_count_ = 0;
    last_animate_tick_ = 0;
    target_ = NULL;
    mission_ = NULL;
    scroll_ = {0, 0};
    paused_ = false;
    ipa_chng_.ipa_chng = -1;
}

void GameplayMenu::handleMouseMotion(Point2D point, [[maybe_unused]] uint32_t state) {
    // locking mouse motion on ipa change until mouseup is recieved
    if (ipa_chng_.ipa_chng != -1 && menu_manager_->isMouseDragged()) {
        rs_knl::PedInstance *p = mission_->ped(ipa_chng_.agent_used);
        if (p->isAlive()) {
            uint8_t percent = agt_sel_renderer_.getPercentageAnyX(
                ipa_chng_.agent_used, point.x);

            // if agent is in selected group we will update all groups IPA
            if (selection_.isAgentSelected(ipa_chng_.agent_used)) {
                for (uint8_t i = 0; i < rs_knl::Squad::kMaxSlot; ++i) {
                    if (selection_.isAgentSelected(i)) {
                        setIPAForAgent(i, (IPAStim::IPAType)ipa_chng_.ipa_chng,
                            percent);
                    }
                }
            } else {
                setIPAForAgent(ipa_chng_.agent_used,
                    (IPAStim::IPAType)ipa_chng_.ipa_chng, percent);
            }
            return;
        } else
            ipa_chng_.ipa_chng = -1;
    }

    updateCursorFromTarget(point);

    if (point.x < 129 && isPlayerShooting_) {
        stopShootingEvent();
    }

    if (isPlayerShooting_) {
        // update direction for each shooting player
        rs_knl::TilePoint mapPt = map_renderer_.getTilePointFromMouse(point);
        if (getAimedAt(mapPt)) {
            rs_knl::WorldPoint aimedAtLocW(mapPt);
            for (SquadSelection::Iterator it = selection_.begin(); it != selection_.end(); ++it) {
                rs_knl::PedInstance *pAgent = *it;
                if (pAgent->isUsingWeapon()) {
                    // If ped is currently shooting
                    // then update the action with new shooting target
                    pAgent->updateShootingTarget(aimedAtLocW);
                }
            }
        }
    }
}

bool GameplayMenu::handleMouseDown(Point2D point, int button)
{
    if (paused_) {
        if (point.x < 129 && point.y > kMiniMapScreenPos.y) {
            if (button == kMouseLeftButton) {
                handleClickOnMinimap(point);
            } else if (button == kMouseRightButton) {
                rs_knl::TilePoint pt = mm_renderer_.minimapToMapPoint(point.add(-kMiniMapScreenPos.x, -kMiniMapScreenPos.y));
                map_renderer_.centerMapOnTile(pt);
            }
        }
        return true;
    }

    Point2D mPos;
    uint32_t mouseState = g_System.getMousePos(mPos);
    bool isLeftDown = (mouseState & (1 << (kMouseLeftButton - 1))) != 0;
    bool isRightDown = (mouseState & (1 << (kMouseRightButton - 1))) != 0;

    if (isLeftDown && isRightDown) {
        // Dual mouse click panic mode: maximize IPA for selected agents
        for (auto it = selection_.begin(); it != selection_.end(); ++it) {
            rs_knl::PedInstance *pAgent = *it;
            if (pAgent && pAgent->isAlive()) {
                pAgent->setIPAAmount(IPAStim::Adrenaline, 100);
                pAgent->setIPAAmount(IPAStim::Perception, 100);
                pAgent->setIPAAmount(IPAStim::Intelligence, 100);
            }
        }
        return true;
    }

    if (point.x < 129) {
        // Is control button pressed
        bool ctrl = g_System.isKeyModStatePressed(rs_eng::KMD_CTRL);

        // First check if player has clicked on agent selectors
        SelectorEvent selEvt;
        if (agt_sel_renderer_.hasClickedOnAgentSelector(point.x, point.y, selEvt)) {
            switch (selEvt.eventType) {
            case SelectorEvent::kSelectAgent:
                // Handle agent selection. Click on an agent changes selection
                // to it. If control key is pressed, add or removes agent from
                // current selection.
                selectAgent(selEvt.agentSlot, ctrl);
                break;
            case SelectorEvent::kSelectIpa:
                ipa_chng_.ipa_chng = static_cast<int8_t>(selEvt.IpaType);
                ipa_chng_.agent_used = selEvt.agentSlot;
                if (selection_.isAgentSelected(selEvt.agentSlot)) {
                    for (uint8_t i = 0; i < rs_knl::Squad::kMaxSlot; ++i) {
                        if (selection_.isAgentSelected(i)) {
                            setIPAForAgent(i, (IPAStim::IPAType)selEvt.IpaType, selEvt.percentage);
                        }
                    }
                } else {
                    setIPAForAgent(selEvt.agentSlot, (IPAStim::IPAType)selEvt.IpaType, selEvt.percentage);
                }
                break;
            case SelectorEvent::kNone:
                break;
            }
        } else if (point.y >= 42 + 48 && point.y < 42 + 48 + 10) {
            // User clicked on the select all button
            selectAllAgents();
        }
        else if (point.y >= 2 + 46 + 44 + 10 + 46 + 44 + 15
                 && point.y < 2 + 46 + 44 + 10 + 46 + 44 + 15 + 64)
        {
            // user clicked on the weapon selector
            handleClickOnWeaponSelector(point, button);
        } else if (point.y > kMiniMapScreenPos.y) {
            if (button == kMouseLeftButton) {
                handleClickOnMinimap(point);
            } else if (button == kMouseRightButton) {
                rs_knl::TilePoint pt = mm_renderer_.minimapToMapPoint(point.add(-kMiniMapScreenPos.x, -kMiniMapScreenPos.y));
                map_renderer_.centerMapOnTile(pt);
            }
        }
    } else {
        // User clicked on the map
        handleClickOnMap(point, button);
    }

    return true;
}

/*!
 * The user has clicked on the weapon selector.
 * \param point Mouse coord
 * \param button Mouse button that was clicked
 */
void GameplayMenu::handleClickOnWeaponSelector(Point2D point, int button) {
    uint8_t w_num = static_cast<uint8_t>(((point.y - (2 + 46 + 44 + 10 + 46 + 44 + 15)) / 32) * 4
            + point.x / 32);
    rs_knl::PedInstance *pLeader = selection_.leader();
    if (pLeader->isAlive()) {
        bool is_ctrl = g_System.isKeyModStatePressed(rs_eng::KMD_CTRL);
        if (w_num < pLeader->numWeapons()) {
            if (button == kMouseLeftButton) {
                // Button 1 : selection/deselection of weapon for all selection
                handleWeaponSelection(w_num, is_ctrl);
            } else {
                // Button 3 : drop weapon from selected agent inventory
                pLeader->addActionPutdown(w_num, is_ctrl);
            }
        }
    }
}

void GameplayMenu::setIPAForAgent(size_t slot, IPAStim::IPAType ipa_type, uint8_t percentage) {
    rs_knl::PedInstance *ped = mission_->ped(slot);
    if (ped->isDead())
        return;

    ped->setIPAAmount(ipa_type, percentage);
}

void GameplayMenu::updateIPALevelMeters(uint32_t elapsed) {
    for (auto it = mission_->getSquad()->alive_begin(); it != mission_->getSquad()->alive_end(); ++it) {
        (*it)->updateAllIPA(elapsed);
    }
}

/*!
 * React to a click on the map
 * @param point Where the player clicked in screen coordinates
 * @param button The mouse button he clicked
 */
void GameplayMenu::handleClickOnMap(Point2D point, int button) {
    rs_knl::TilePoint mapPt = map_renderer_.getTilePointFromMouse(point);

#ifdef _DEBUG
    if (g_System.isKeyModStatePressed(rs_eng::KMD_ALT)) {
        printf("Tile x:%d, y:%d, z:%d, ox:%d, oy:%d\n",
            mapPt.tx, mapPt.ty, mapPt.tz, mapPt.ox, mapPt.oy);

        if (target_) {
            printf("   > target(%i) : %s\n",
                target_->id(), target_->natureName());
            printf("   >   Tile : %d, %d, %d\n", target_->tileX(), target_->tileY(), target_->tileZ());
        }

        int tileid = mission_->map()->getTileIdAt(mapPt.tx, mapPt.ty, mapPt.tz);
        printf("Tile id %d\n", tileid);
        return;
    }
#endif //_DEBUG

    bool ctrl = g_System.isKeyModStatePressed(rs_eng::KMD_CTRL);
    if (button == kMouseLeftButton) {
        if (target_) {
            switch (target_->nature()) {
            case rs_knl::MapObject::kNatureWeapon:
                selection_.pickupWeapon(dynamic_cast<rs_knl::WeaponInstance *>(target_), ctrl);
                break;
            case rs_knl::MapObject::kNaturePed:
                {
                    rs_knl::PedInstance *pPed = dynamic_cast<rs_knl::PedInstance *>(target_);
                    if (pPed && pPed->isOurAgent()) {
                        if (mission_->findReachableTileFromBase(mapPt)) {
                            selection_.moveTo(mapPt, ctrl);
                        }
                    } else if (pPed) {
                        selection_.followPed(pPed);
                    }
                }
                break;
            case rs_knl::MapObject::kNatureVehicle:
                selection_.enterOrLeaveVehicle(dynamic_cast<rs_knl::Vehicle *>(target_), ctrl);
                break;
            default:
                break;
            }
        } else if (mission_->findReachableTileFromBase(mapPt)) {
            selection_.moveTo(mapPt, ctrl);
        }
    } else if (button == kMouseRightButton) {
        bool hasAirStrike = false;
        rs_knl::PedInstance *pAirStrikeAgent = nullptr;
        for (SquadSelection::Iterator it = selection_.begin(); it != selection_.end(); ++it) {
            rs_knl::WeaponInstance *w = (*it)->selectedWeapon();
            if (w && w->type() == rs_knl::Weapon::AirStrike) {
                hasAirStrike = true;
                if (!pAirStrikeAgent) {
                    pAirStrikeAgent = *it;
                }
            }
        }
        if (hasAirStrike) {
            if (g_Session.getMoney() < 50000) {
                needMoneyTimer_ = 3000;
                g_SoundMgr.play(rs_eng::SPEECH_SELECTED);
                return;
            } else {
                g_Session.decreaseMoney(50000);
                airRaidCountdownRemaining_ = 9880;
                getAimedAt(mapPt);
                rs_knl::WorldPoint aimedAtLocW(mapPt);
                rs_knl::DamageToInflict dmg;
                dmg.dtype = rs_knl::kDmgTypeExplosion;
                dmg.dvalue = 800;
                dmg.range = 450.0;
                dmg.nonIncendiary = true;
                dmg.aimedLocW = aimedAtLocW;
                dmg.d_owner = pAirStrikeAgent;
                dmg.pWeapon = pAirStrikeAgent ? pAirStrikeAgent->selectedWeapon() : nullptr;
                mission_->addProjectileShot(std::make_unique<rs_knl::AirStrikeShot>(dmg));
                g_SoundMgr.play(rs_eng::SPEECH_SELECTED);
                return;
            }
        }

        if (getAimedAt(mapPt)) {
            isPlayerShooting_ = true;
            rs_knl::WorldPoint aimedAtLocW(mapPt);
            selection_.shootAt(aimedAtLocW);
        }
    }
}

/*!
 * User has clicked on the minimap. All selected agent go to destination.
 * Clicking on the minimap does not allow to shoot or to use objects.
 * \param point minimap coordinate
 */
void GameplayMenu::handleClickOnMinimap(Point2D point) {
    // convert minimap coordinate in map coordinate
    rs_knl::TilePoint pt = mm_renderer_.minimapToMapPoint(point.add(-kMiniMapScreenPos.x, -kMiniMapScreenPos.y));
    // As minimap is flat, we can't see the height. So take the Z coordinate
    // of the leader as a reference
    pt.tz = selection_.leader()->tileZ();
    if (mission_->getWalkableClosestByZ(pt))
    {
        // Destination is walkable so go
        selection_.moveTo(pt, false);
     }
}

/*!
 * Set the point on the map the player is aiming at.
 * It depends on whether the player has clicked on a shootable target
 * or a point on the ground.
 * @param basePt base tilepoint corresponding to the click on the map.
 *  basePt will be updated with the final position.
 * @return True if location has been set.
 */
bool GameplayMenu::getAimedAt(rs_knl::TilePoint &basePt) {
    bool locationSet = false;

    if (target_ && !(target_->is(rs_knl::MapObject::kNaturePed) && static_cast<rs_knl::PedInstance *>(target_)->isOurAgent())) {
        //  Player has aimed an object
        basePt = target_->position();
        // z is set to half the size of the object
        // TODO : check that the line below is correct as size may not be expressed as Tilepoint coords
        basePt.oz += target_->sizeZ() >> 1;
        locationSet = true;
    } else {
        // Player is shooting on the ground
        if (mission_->findShootableTileFromBase(basePt)) {
            locationSet = true;
        }
    }

    return locationSet;
}

/*!
 *
 */
void GameplayMenu::stopShootingEvent()
{
    isPlayerShooting_ = false;
    for (SquadSelection::Iterator it = selection_.begin(); it != selection_.end(); ++it) {
        rs_knl::PedInstance *pAgent = *it;

        pAgent->stopShooting();
    }
}


void GameplayMenu::handleMouseUp([[maybe_unused]] Point2D point, int button)
{
    ipa_chng_.ipa_chng = -1;

    if (button == kMouseRightButton) {
        stopShootingEvent();
    }

}

bool GameplayMenu::handleUnMappedKey(const rs_eng::RS_Key key) {
    bool consumed = true;

    // Pause/unpause game
    if (key.keyCode == rs_eng::kKeyCode_P) {
        paused_ = !paused_;
        if (paused_) {
            stopShootingEvent();
            g_MusicMgr.pause();
        } else {
            g_MusicMgr.resume();
        }
        return true;
    }

    if (paused_)
        return true;

    bool ctrl = g_System.isKeyModStatePressed(rs_eng::KMD_CTRL);

    // SPACE is pressed when the mission failed or succeeded to return
    // to debrief menu
    if (key.keyCode == rs_eng::kKeyCode_Space) {
        if (mission_->completed() || mission_->failed()) {
            if (mission_->completed()) {
                // Display success animation
                menu_manager_->gotoMenu(rs_game_menus::kMenuIdFliSuccess);
            }
            else if (mission_->failed()) {
                menu_manager_->gotoMenu(rs_game_menus::kMenuIdFliFailedMission);
            }

            return true;
        }
    } else if (key.keyCode == rs_eng::kKeyCode_Escape) {
        if (mission_->isRunning()) {
            // Abort mission
            mission_->endWithStatus(rs_knl::Mission::kMissionStatusAborted);
        }
        // Display only the menu up animation
        menu_manager_->gotoMenu(rs_game_menus::kMenuIdFliMissionAborted);
        return true;
    }

    // Handle agent selection by numeric keys. Key 0 cycles between one agent
    // selection and all 4 agents.
    if (key.keyCode == rs_eng::kKeyCode_0) {
        // This code is exactly the same as for clicking on "group-button"
        // as you can see above.
        selectAllAgents();
    }
    // Individual keys select the specified agent unless ctrl is pressed -
    // then they add/remove agent from current selection.
    else if (key.keyCode >= rs_eng::kKeyCode_1 && key.keyCode <= rs_eng::kKeyCode_4) {
        size_t agentIdx = static_cast<size_t>(key.keyCode - rs_eng::kKeyCode_1);
        // Double press on the same key make the screen centers on the selection
        if ((tick_count_ - lastSelectionTick_) < kDoublePressMs) {
            if (selection_.getLeaderSlot() == agentIdx) {
                map_renderer_.centerMapOnPed(selection_.leader());
            }
        } else {
            lastSelectionTick_ = tick_count_;
        }
        
        if (selection_.getLeaderSlot() != agentIdx) {
            selectAgent(agentIdx, ctrl);
        }
        
    } else if (key.keyCode == rs_eng::kKeyCode_Left ||
               key.keyCode == rs_eng::kKeyCode_Right ||
               key.keyCode == rs_eng::kKeyCode_Up ||
               key.keyCode == rs_eng::kKeyCode_Down) {
        // Arrow keys panning is handled continuously in handleTick via real-time key state
        consumed = true;
    }

#ifdef _DEBUG
    else if (key.keyCode == rs_eng::kKeyCode_F11) {
        mission_->endWithStatus(rs_knl::Mission::kMissionStatusCompleted);
        return true;
    } else if (key.keyCode == rs_eng::kKeyCode_F12) {
        mission_->endWithStatus(rs_knl::Mission::kMissionStatusFailed);
        return true;
    }
#endif
    else if (key.keyCode >= rs_eng::kKeyCode_F1 && key.keyCode <= rs_eng::kKeyCode_F8) {
        // Those keys are direct access to inventory
        uint8_t weapon_idx = (uint8_t) key.keyCode - (uint8_t) rs_eng::kKeyCode_F1;
        handleWeaponSelection(weapon_idx, ctrl);
        return true;
    } else if ((key.keyCode == rs_eng::kKeyCode_D) && ctrl && mission_->isRunning()) { // selected agents are killed with 'd'
        // save current selection as it will be modified when agents die
        std::vector<rs_knl::PedInstance *> agents_suicide;
        for (SquadSelection::Iterator it = selection_.begin();
                        it != selection_.end(); ++it) {
                agents_suicide.push_back(*it);
        }

        for (size_t i=0; i < agents_suicide.size(); i++) {
            agents_suicide[i]->commitSuicide();
        }
    } else {
        consumed = false;
    }


    return consumed;
}

void GameplayMenu::drawSelectAllButton() {
    // 64x10
    if(isButtonSelectAllPressed_) {
        g_SpriteMgr.drawSprite(1792, {0, 46 + 44});
        g_SpriteMgr.drawSprite(1793, {64, 46 + 44});
    } else {
        g_SpriteMgr.drawSprite(1796, {0, 46 + 44});
        g_SpriteMgr.drawSprite(1797, {64, 46 + 44});
    }
}

/*!
 * Manage the animation of the hint display.
 * We either display the current action of the squad leader or
 * the mission's objective if mission is ongoing or a status
 * if mission has failed.
 * For objective/status, the font is blinking and there is 
 * a yellow box to accentuate the display
 * @param elapsed 
 */
void GameplayMenu::updateMissionHint(uint32_t elapsed) {
    drawHintYellowRect_ = false;

    if (needMoneyTimer_ > 0) {
        if (elapsed >= needMoneyTimer_) {
            needMoneyTimer_ = 0;
        } else {
            needMoneyTimer_ -= elapsed;
        }
        hint_ = "NEED 50,000";
        hintColor_ = 11;
        drawHintYellowRect_ = false;
        return;
    }

    if (airRaidCountdownRemaining_ > 0) {
        if (elapsed >= airRaidCountdownRemaining_) {
            airRaidCountdownRemaining_ = 0;
        } else {
            airRaidCountdownRemaining_ -= elapsed;
        }
        int sec = static_cast<int>(std::ceil(airRaidCountdownRemaining_ / 1000.0));
        hint_ = std::format("AIR RAID T-{}", sec);
        hintColor_ = 11;
        drawHintYellowRect_ = false;
        return;
    }

    hintTimer_.update(elapsed);

    if (hintTimer_.state()) {
        rs_knl::PedInstance *pLeader = selection_.leader();
        if (pLeader->hasDestination()) {
            hint_ = getMessage("HINT_GOING");
        } else {
            hint_ = getMessage("HINT_OBSERVING");
        }
        if (pLeader->isState(rs_knl::kPedActionStatePickUp)) {
            hint_ = getMessage("HINT_PICKUP_WEAPON");
        }
        if (pLeader->isHitByBullet()) {
            hint_ = getMessage("HINT_HIT_BY_BULLET");
        }
        hintColor_ = 14;
    } else {
        hintColorTimer_.update(elapsed);
        hintColor_ = hintColorTimer_.state() ? 0 : 11;

        if (mission_->isRunning()) {
            if (target_ && 
                    target_->nature() == rs_knl::MapObject::kNatureWeapon && 
                    target_->isDrawable()) {
                // The player is pointing to a weapon on ground -> display the name of weapon
                hint_ = static_cast<rs_knl::WeaponInstance *>(target_)->name();
                hintColor_ = hintColorTimer_.state() ? 14 : 11;
            } else {
                mission_->objectiveMsg(hint_);
                drawHintYellowRect_ = hintColorTimer_.state();
            }
        } else {
            hintSpaceTimer_.update(elapsed);
            if (hintSpaceTimer_.state()) { // When true, we display the press space message
                hint_ = getMessage("HINT_PRESS_SPACE");
            } else {
                if (mission_->failed()) {
                    hint_ = getMessage("HINT_MISSION_FAILED");
                } else {
                    hint_ = getMessage("HINT_MISSION_COMPLETE");
                }
            }
        }
    }
}

/*!
 * @brief 
 */
void GameplayMenu::drawMissionHint() {
    g_SpriteMgr.drawSprite(1798, {0, 46 + 44 + 10 + 46 + 44 - 1});
    g_SpriteMgr.drawSprite(1799, {64, 46 + 44 + 10 + 46 + 44 - 1});

    if (drawHintYellowRect_) {
        g_System.drawFillRect({0, 46 + 44 + 10 + 46 + 44}, 128, 12, menu_manager_->kMenuColorYellow);
    }

    int width = gameFont()->textWidth(hint_, false);
    int x = 64 - width / 2;
    rs_eng::RSColor aColor;
    menu_manager_->getColorFromMenuPalette(hintColor_, aColor);
    gameFont()->drawText(x, 46 + 44 + 10 + 46 + 44 + 2 - 1, hint_, aColor);
}

void GameplayMenu::drawPausePanel() {
    std::string str_paused = getMessage("GAME_PAUSED");
    rs_eng::MenuFont *font_used = getMenuFont(rs_eng::FontManager::SIZE_1);
    int txt_width = font_used->textWidth(str_paused);
    int txt_posx = rs_eng::kScreenWidth / 2 - txt_width / 2;
    int txt_height = font_used->textHeight(false);
    int txt_posy = rs_eng::kScreenHeight / 2 - txt_height / 2;

    g_System.drawFillRect({txt_posx - 10, txt_posy - 5},
        txt_width + 20, txt_height + 10, menu_manager_->kMenuColorBlack);
    rs_eng::RSColor aColor;
    menu_manager_->getColorFromMenuPalette(11, aColor);
    gameFont()->drawText(txt_posx, txt_posy, str_paused, aColor);
}

namespace {

struct OverlayPixel {
    int8_t dx;
    int8_t dy;
    uint8_t palIdx;
};

// 32x32 HUD Air Strike stealth jet overlay
static const OverlayPixel kAirStrikePixels[122] = {
    {16, 10, 1}, {15, 11, 1}, {16, 11, 6}, {17, 11, 1}, {15, 12, 1}, {16, 12, 6},
    {17, 12, 3}, {14, 13, 1}, {15, 13, 6}, {16, 13, 6}, {17, 13, 6}, {18, 13, 1},
    {14, 14, 1}, {15, 14, 6}, {16, 14, 6}, {17, 14, 6}, {18, 14, 3}, {13, 15, 1},
    {14, 15, 6}, {15, 15, 6}, {16, 15, 6}, {17, 15, 6}, {18, 15, 6}, {19, 15, 1},
    {12, 16, 1}, {13, 16, 6}, {14, 16, 6}, {15, 16, 6}, {16, 16, 6}, {17, 16, 6},
    {18, 16, 6}, {19, 16, 6}, {20, 16, 1}, {11, 17, 1}, {12, 17, 6}, {13, 17, 6},
    {14, 17, 6}, {15, 17, 6}, {16, 17, 6}, {17, 17, 6}, {18, 17, 6}, {19, 17, 6},
    {20, 17, 6}, {21, 17, 1}, {10, 18, 1}, {11, 18, 6}, {12, 18, 6}, {13, 18, 6},
    {14, 18, 6}, {15, 18, 6}, {16, 18, 6}, {17, 18, 6}, {18, 18, 6}, {19, 18, 6},
    {20, 18, 6}, {21, 18, 6}, {22, 18, 1}, {9, 19, 1}, {10, 19, 6}, {11, 19, 6},
    {12, 19, 6}, {13, 19, 6}, {14, 19, 6}, {15, 19, 6}, {16, 19, 6}, {17, 19, 6},
    {18, 19, 6}, {19, 19, 6}, {20, 19, 6}, {21, 19, 6}, {22, 19, 6}, {23, 19, 1},
    {8, 20, 1}, {9, 20, 6}, {10, 20, 6}, {11, 20, 6}, {12, 20, 6}, {13, 20, 6},
    {14, 20, 3}, {15, 20, 3}, {16, 20, 3}, {17, 20, 3}, {18, 20, 3}, {19, 20, 6},
    {20, 20, 6}, {21, 20, 6}, {22, 20, 6}, {23, 20, 6}, {24, 20, 1}, {7, 21, 1},
    {8, 21, 6}, {9, 21, 6}, {10, 21, 6}, {11, 21, 3}, {12, 21, 3}, {13, 21, 3},
    {19, 21, 1}, {20, 21, 3}, {21, 21, 3}, {22, 21, 6}, {23, 21, 6}, {24, 21, 6},
    {25, 21, 1}, {6, 22, 1}, {7, 22, 3}, {8, 22, 3}, {9, 22, 3}, {10, 22, 3},
    {22, 22, 1}, {23, 22, 3}, {24, 22, 3}, {25, 22, 3}, {26, 22, 1}, {15, 22, 1},
    {16, 22, 1}, {17, 22, 1}, {16, 23, 1}, {16, 25, 1}, {15, 26, 1}, {16, 26, 6},
    {17, 26, 1}, {16, 27, 1}
};

// 32x32 HUD Clone Shield humanoid clone silhouette overlay
static const OverlayPixel kCloneShieldPixels[46] = {
    {15, 14, 1}, {16, 14, 1}, {15, 15, 1}, {16, 15, 3}, {15, 16, 1}, {16, 16, 3},
    {13, 17, 1}, {14, 17, 1}, {15, 17, 6}, {16, 17, 6}, {17, 17, 1}, {18, 17, 1},
    {13, 18, 1}, {14, 18, 6}, {15, 18, 6}, {16, 18, 6}, {17, 18, 6}, {18, 18, 3},
    {14, 19, 1}, {15, 19, 6}, {16, 19, 6}, {17, 19, 3}, {14, 20, 1}, {15, 20, 6},
    {16, 20, 6}, {17, 20, 3}, {14, 21, 1}, {15, 21, 6}, {16, 21, 6}, {17, 21, 3},
    {14, 22, 1}, {15, 22, 6}, {16, 22, 6}, {17, 22, 3}, {14, 23, 1}, {15, 23, 6},
    {16, 23, 6}, {17, 23, 3}, {14, 24, 1}, {15, 24, 3}, {16, 24, 3}, {17, 24, 3},
    {13, 25, 1}, {14, 25, 3}, {17, 25, 1}, {18, 25, 1}
};

} // namespace

void GameplayMenu::drawAirStrikeOverlay(int slotX, int slotY) {
    for (const auto &p : kAirStrikePixels) {
        g_System.drawPoint({slotX + p.dx, slotY + p.dy}, missionPalette_[p.palIdx]);
    }
}

void GameplayMenu::drawCloneShieldOverlay(int slotX, int slotY) {
    for (const auto &p : kCloneShieldPixels) {
        g_System.drawPoint({slotX + p.dx, slotY + p.dy}, missionPalette_[p.palIdx]);
    }
}

void GameplayMenu::drawWeaponSelectors() {
    rs_knl::PedInstance *p = NULL;

    p = selection_.leader();

    if (p) {
        bool draw_pw = true;
        for (uint8_t j = 0; j < 2; j++) {
            for (uint8_t i = 0; i < 4; i++) {
                rs_knl::WeaponInstance *wi = NULL;
                int s = 1601;
                bool is_selected = false;

                if (i + j * 4u < p->numWeapons()) {
                    wi = p->weapon(i + j * 4u);
                    s = wi->getClass()->selector();
                    if (p->selectedWeapon() && p->selectedWeapon() == wi) {
                        is_selected = true;
                        s += 40;
                    }
                } else if (draw_pw) {
                    if (target_ && target_->nature() == rs_knl::MapObject::kNatureWeapon
                        && target_->isDrawable())
                    {
                        // player is pointing a weapon on the ground and there's space
                        // in the inventory to display its icon
                        wi = (rs_knl::WeaponInstance *)target_;
                        draw_pw = false;
                        is_selected = true;
                        s = wi->getClass()->selector() + 40;
                    }
                }

                int slotX = 32 * i;
                int slotY = 2 + 46 + 44 + 10 + 46 + 44 + 15 + j * 32;

                if (false && wi && wi->type() == rs_knl::Weapon::AirStrike) {
                    // Air Strike: base slot is 1601
                    g_SpriteMgr.drawSprite(1601, {slotX, slotY});
                    if (is_selected) {
                        g_System.drawRect({slotX + 1, slotY + 1}, 30, 30, missionPalette_[11]);
                        g_System.drawRect({slotX + 2, slotY + 2}, 28, 28, missionPalette_[14]);
                    }
                    drawAirStrikeOverlay(slotX, slotY);
                } else if (false && wi && wi->type() == rs_knl::Weapon::CloneShield) {
                    // Clone Shield: base slot is 1618 (or 1658 when selected)
                    int shieldSprite = is_selected ? 1658 : 1618;
                    g_SpriteMgr.drawSprite(shieldSprite, {slotX, slotY});
                    drawCloneShieldOverlay(slotX, slotY);
                } else {
                    g_SpriteMgr.drawSprite(s, {slotX, slotY});
                }

                // draw ammo bars
                if (wi) {
                    if (!wi->usesAmmo() || wi->ammoCapacity() <= 0)
                        continue;

                    int n = 25 * wi->ammoRemaining() / wi->ammoCapacity();

                    g_System.drawFillRect({slotX + 3, slotY + 23},
                        n, 5, menu_manager_->kMenuColorWhiteBlue);
                }
            }
        }
    } else {
        for (int j = 0; j < 2; j++)
            for (int i = 0; i < 4; i++) {
                int s = 1601;

                g_SpriteMgr.drawSprite(s, {32 * i, 2 + 46 + 44 + 10 + 46 + 44 + 15 + j * 32});
            }

    }
}

/*!
 * Select the given agent.
 * \param agentNo Agent id
 * \param addToGroup If true, agent is added to the current selection.
 * If not, the selection is emptied and filled with the new agent.
 */
void GameplayMenu::selectAgent(size_t agentNo, bool addToGroup) {
    // TODO: when agent deselected and he was shooting, he should stop
    if (selection_.selectAgent(agentNo, addToGroup)) {
        updateSelectAll();
        centerMinimapOnLeader();
        highlightLeaderMarker();
        g_SoundMgr.play(rs_eng::SPEECH_SELECTED);
    }
}

/*!
 * Selects all agents.
 * \param invert If true, selects only the agents that were not already
 * selected.
 */
void GameplayMenu::selectAllAgents() {
    bool prv_state = isButtonSelectAllPressed_;
    isButtonSelectAllPressed_ = !isButtonSelectAllPressed_;
    selection_.selectAllAgents(isButtonSelectAllPressed_);
    updateSelectAll();
    if (isButtonSelectAllPressed_ != prv_state) {
        g_SoundMgr.play(rs_eng::SPEECH_SELECTED);
    }
}

/*!
 * Make the current leader marker blinks.
 * All other agents not.
 */
void GameplayMenu::highlightLeaderMarker()
{
    for (size_t i = rs_knl::Squad::kSlot1; i < rs_knl::Squad::kMaxSlot; i++) {
        // draw animation only for leader
        //mission_->sfxObjects(4 + i)->setDrawAllFrames(selection_.getLeaderSlot() == i);
        //mission_->sfxObjects(4 + i)->playMainAnimation();
        if (selection_.getLeaderSlot() == i) {
            mission_->sfxObjects(4 + i)->playMainAnimation();
        } else {
            mission_->sfxObjects(4 + i)->resetAnimation();
        }
    }
}

/**
 * Updating position for visual markers for all agents.
 * \return void
 *
 */
void GameplayMenu::updateMarkersPosition() {
    for (size_t i = 0; i < rs_knl::Squad::kMaxSlot; i++) {
        if (mission_->sfxObjects(i + 4)->isDrawable()) {
            rs_knl::PedInstance *pAgent = mission_->getSquad()->member(i);
            if (pAgent != NULL && pAgent->isAlive()) {
                rs_knl::TilePoint agentPos = pAgent->position();
                agentPos.ox -= 16;
                agentPos.oz += 256;

                mission_->sfxObjects(i + 4)->setPosition(agentPos);
            }
        }
    }
}

/*!
 * This method checks among the squad to see if an agent died and deselects him.
 */
void GameplayMenu::updateSelectionForDeadAgent(rs_knl::PedInstance *pPed) {
    // Deselects dead agent
    selection_.deselectAgent(pPed);
    // hide dead agent's marker
    mission_->sfxObjects(pPed->id() + 4)->setDrawable(false);

    // if selection is empty after agent's death
    // selects the first selectable agent
    if (selection_.size() == 0) {
        for (size_t i = rs_knl::Squad::kSlot1; i < rs_knl::Squad::kMaxSlot; i++) {
            if (selection_.selectAgent(i, false)) {
                // Agent has been selected -> quit
                break;
            }
        }
    }

    // anyway updates markers
    highlightLeaderMarker();
    centerMinimapOnLeader();
    updateSelectAll();
}

/*!
 * Updates the minimap.
 */
void GameplayMenu::centerMinimapOnLeader() {
    // Centers the minimap on the selection leader
    if (selection_.size() > 0) {
        rs_knl::PedInstance *pAgent = selection_.leader();
        if (pAgent != nullptr) {
            mm_renderer_.centerOn(pAgent);
        }
    }
}

/*!
 * Update the select all button's state
 */
void GameplayMenu::updateSelectAll() {
    uint8_t nbAgentAlive = 0;
    // count the number of remaining agents
    for (size_t indx = 0; indx < rs_knl::Squad::kMaxSlot; indx++) {
        rs_knl::PedInstance *pAgent = mission_->getSquad()->member(indx);
        if (pAgent && pAgent->isAlive()) {
            nbAgentAlive++;
        }
    }

    // if number of agents alive is the same as number of selected agents
    // then button is pressed.
    isButtonSelectAllPressed_ = ((nbAgentAlive == selection_.size()
        && nbAgentAlive != 0) || nbAgentAlive == 1);
}

/*!
 *
 */
void GameplayMenu::handleWeaponSelection(uint8_t selectorIndex, bool ctrl) {
    rs_knl::PedInstance *pLeader = selection_.leader();

    if (selectorIndex < pLeader->numWeapons()) {
        rs_knl::WeaponInstance *wi = pLeader->weapon(selectorIndex);
        if (pLeader->selectedWeapon() == wi) {
            // Player clicked on an already selected weapon -> deselect
            selection_.deselectWeaponOfSameCategory(wi->getClass());
        } else {
            selection_.selectWeaponFromLeader(selectorIndex, ctrl);
        }
    }
    g_SoundMgr.play(rs_eng::SPEECH_SELECTED);
}

/**
 * Method to intercept game events.
 */
void GameplayMenu::onPoliceWarningEmittedEvent([[maybe_unused]] rs_knl::PoliceWarningEmittedEvent *pEvt) {
    if (canPlayPoliceWarnSound_) {
        // warn
        g_SoundMgr.play(rs_eng::PUTDOWN_WEAPON);
        canPlayPoliceWarnSound_ = false;
    }
}

void GameplayMenu::onAgentDiedEvent(rs_knl::AgentDiedEvent *pEvt) {
    LOG(Log::k_FLG_GAME, "GameplayMenu", "onAgentDiedEvent", ("AgentDiedEvent received"))
    // checking agents, if all are dead -> mission failed
    if (mission_->getSquad()->isAllDead()) {
        mission_->endWithStatus(rs_knl::Mission::kMissionStatusFailed);
    }

    // Anyway update selection
    updateSelectionForDeadAgent(pEvt->pPed);
}

void GameplayMenu::onShootingWeaponSelectedEvent(rs_knl::ShootingWeaponSelectedEvent *pEvt) {
    if (pEvt->isSelected) {
        LOG(Log::k_FLG_GAME, "GameplayMenu", "onShootingWeaponSelectedEvent", ("shooting weapon was selected"))
        rs_knl::PedInstance *pPedSource = pEvt->pPed;
        mission_->addArmedPed(pPedSource);
        for (size_t i = 0; i < mission_->numPeds(); i++) {
            rs_knl::PedInstance *pPed = mission_->ped(i);
            if (pPed != pPedSource) {
                pPed->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponOut, pPedSource);
            }
        }
    } else {
        LOG(Log::k_FLG_GAME, "GameplayMenu", "onShootingWeaponSelectedEvent", ("shooting weapon was deselected"))
        rs_knl::PedInstance *pPedSource = pEvt->pPed;
        mission_->removeArmedPed(pPedSource);
        for (size_t i = 0; i < mission_->numPeds(); i++) {
            rs_knl::PedInstance *pPed = mission_->ped(i);
            if (pPed != pPedSource) {
                pPed->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponCleared, pPedSource);
            }
        }
    }
}

/*!
 * Call when receiving the event that tells the mission has ended.
 * @param pEvt MissionEndedEvent
 */
void GameplayMenu::onMissionEndedEvent(rs_knl::MissionEndedEvent *pEvt) {
    if (mission_ && mission_->getSquad()) {
        for (size_t i = 0; i < mission_->getSquad()->size(); ++i) {
            rs_knl::PedInstance *pAgent = mission_->getSquad()->member(i);
            if (pAgent) {
                pAgent->stopShooting();
            }
        }
    }
    isPlayerShooting_ = false;

    if (pEvt->status == rs_knl::Mission::kMissionStatusCompleted) {
        LOG(Log::k_FLG_GAME, "Mission", "endWithStatus()", ("Mission succeeded"));
        g_SoundMgr.play(rs_eng::SPEECH_MISSION_COMPLETED);
        g_MusicMgr.playSong(rs_eng::MusicManager::kMusicSongMissionCompleted);
    } else if (pEvt->status == rs_knl::Mission::kMissionStatusFailed) {
        LOG(Log::k_FLG_GAME, "Mission", "endWithStatus()", ("Mission failed"));
        g_SoundMgr.play(rs_eng::SPEECH_MISSION_FAILED);
        g_MusicMgr.playSong(rs_eng::MusicManager::kMusicSongMissionFailed);
    }
}

void GameplayMenu::renderStatsOverlay(rs_knl::MapObject *pObj) {
    if (!pObj) return;

    std::vector<std::string> lines;

    if (pObj->is(rs_knl::MapObject::kNaturePed)) {
        rs_knl::PedInstance *pPed = static_cast<rs_knl::PedInstance *>(pObj);

        std::string typeStr = "Civilian";
        switch (pPed->type()) {
        case rs_knl::PedInstance::kPedTypeAgent:
            if (pPed->isOurAgent()) {
                int agentNo = -1;
                if (mission_ && mission_->getSquad()) {
                    for (size_t sqIdx = 0; sqIdx < mission_->getSquad()->size(); ++sqIdx) {
                        if (mission_->getSquad()->member(sqIdx) == pPed) {
                            agentNo = static_cast<int>(sqIdx + 1);
                            break;
                        }
                    }
                }
                if (agentNo > 0) {
                    typeStr = "Agent " + std::to_string(agentNo);
                } else {
                    typeStr = "Player Agent";
                }
            } else {
                static const char* kFactionNames[] = {
                    "Player", "Blue", "Orange", "Yellow", "Yellow", "Purple", "Cyan", "Pink"
                };
                int fId = pPed->factionId();
                std::string fName = (fId >= 0 && fId < 8) ? kFactionNames[fId] : ("Faction " + std::to_string(fId));
                typeStr = "Enemy Agent " + std::to_string(pPed->id()) + " [" + fName + "]";
            }
            break;
        case rs_knl::PedInstance::kPedTypePolice:
            typeStr = "Police " + std::to_string(pPed->id());
            break;
        case rs_knl::PedInstance::kPedTypeGuard:
            typeStr = "Guard " + std::to_string(pPed->id());
            break;
        case rs_knl::PedInstance::kPedTypeCriminal:
            typeStr = "Criminal " + std::to_string(pPed->id());
            break;
        default:
            typeStr = "Civilian " + std::to_string(pPed->id());
            break;
        }
        lines.push_back(typeStr);

        std::stringstream ssHealth;
        ssHealth << "Health: " << pPed->health() << " / " << pPed->startHealth();
        lines.push_back(ssHealth.str());

        std::stringstream ssSpeed;
        int currentSpeed = pPed->isMoving() ? pPed->speed() : 0;
        int topSpeed = pPed->maxSpeed();
        ssSpeed << "Speed: " << currentSpeed << " (max: " << topSpeed << ")";
        lines.push_back(ssSpeed.str());

        std::stringstream ssAcc;
        double acc = 0.5;
        pPed->getAccuracy(acc);
        ssAcc << "Accuracy: " << static_cast<int>(acc * 100) << "%";
        lines.push_back(ssAcc.str());

        std::string patrolStr = (pPed->initialLevelState() & LevelData::kPeopleStateWalking) ? "Wander" : "Sentry";
        if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent && !pPed->isOurAgent()) {
            std::string aiMode = pPed->isHunter() ? "Hunter" : "Sentry";
            lines.push_back("Tactical: " + aiMode + " (" + patrolStr + ")");
        } else if (!pPed->isOurAgent()) {
            lines.push_back("Movement: " + patrolStr);
        }

        std::string stateStr = "Idle";
        if (!pPed->isAlive()) {
            stateStr = "Dead";
        } else if (pPed->isPersuaded()) {
            stateStr = "Persuaded";
        } else if (pPed->isInPanic()) {
            stateStr = "Panic";
        } else if (pPed->behaviour().hasHostileEngaged()) {
            stateStr = "Hostile";
        } else if (pPed->isInVehicle()) {
            stateStr = "In Vehicle";
        } else if (pPed->isState(rs_knl::kPedActionStateFiring)) {
            stateStr = "Firing";
        } else if (pPed->isState(rs_knl::kPedActionStateWalking)) {
            stateStr = "Walking";
        }
        lines.push_back("State: " + stateStr);

        if (!pPed->isOurAgent()) {
            if (pPed->numWeapons() > 0) {
                std::string wepsStr = "Weapons: ";
                for (size_t w = 0; w < pPed->numWeapons(); ++w) {
                    rs_knl::WeaponInstance *wi = pPed->weapon(w);
                    if (wi) {
                        if (w > 0) wepsStr += ", ";
                        wepsStr += wi->name();
                    }
                }
                lines.push_back(wepsStr);
            } else {
                lines.push_back("Weapons: Unarmed");
            }
        }

        if (pPed->isOurAgent()) {
            int group1Slots[] = { rs_knl::Mod::MOD_LEGS, rs_knl::Mod::MOD_ARMS, rs_knl::Mod::MOD_CHEST };
            const char* group1Names[] = { "Legs", "Arms", "Chest" };
            std::vector<std::string> group1Strs;
            for (int i = 0; i < 3; ++i) {
                rs_knl::Mod *pMod = pPed->slot(group1Slots[i]);
                if (pMod) {
                    std::string modVer = "V1";
                    if (pMod->getVersion() == rs_knl::Mod::MOD_V2) modVer = "V2";
                    else if (pMod->getVersion() == rs_knl::Mod::MOD_V3) modVer = "V3";
                    group1Strs.push_back(std::string(group1Names[i]) + " " + modVer);
                }
            }

            int group2Slots[] = { rs_knl::Mod::MOD_BRAIN, rs_knl::Mod::MOD_EYES, rs_knl::Mod::MOD_HEART };
            const char* group2Names[] = { "Brain", "Eyes", "Heart" };
            std::vector<std::string> group2Strs;
            for (int i = 0; i < 3; ++i) {
                rs_knl::Mod *pMod = pPed->slot(group2Slots[i]);
                if (pMod) {
                    std::string modVer = "V1";
                    if (pMod->getVersion() == rs_knl::Mod::MOD_V2) modVer = "V2";
                    else if (pMod->getVersion() == rs_knl::Mod::MOD_V3) modVer = "V3";
                    group2Strs.push_back(std::string(group2Names[i]) + " " + modVer);
                }
            }

            if (!group1Strs.empty() || !group2Strs.empty()) {
                std::string modsLine1 = "Mods: ";
                if (!group1Strs.empty()) {
                    for (size_t i = 0; i < group1Strs.size(); ++i) {
                        if (i > 0) modsLine1 += ", ";
                        modsLine1 += group1Strs[i];
                    }
                    lines.push_back(modsLine1);
                }

                if (!group2Strs.empty()) {
                    std::string modsLine2 = group1Strs.empty() ? "Mods: " : "      ";
                    for (size_t i = 0; i < group2Strs.size(); ++i) {
                        if (i > 0) modsLine2 += ", ";
                        modsLine2 += group2Strs[i];
                    }
                    lines.push_back(modsLine2);
                }
            } else {
                lines.push_back("Mods: None");
            }
        }

        if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent || pPed->intelligence().amount() > 0 || pPed->perception().amount() > 0 || pPed->adrenaline().amount() > 0) {
            std::stringstream ssIPA;
            ssIPA << "IPA: I:" << (int)pPed->intelligence().amount() << "% P:" << (int)pPed->perception().amount() << "% A:" << (int)pPed->adrenaline().amount() << "%";
            lines.push_back(ssIPA.str());
        }

    } else if (pObj->is(rs_knl::MapObject::kNatureVehicle)) {
        rs_knl::Vehicle *pVeh = static_cast<rs_knl::Vehicle *>(pObj);

        std::string vehName = "Car";
        uint8_t vType = pVeh->getType();
        if (vType == rs_knl::Vehicle::kVehicleTypePolice) {
            vehName = "Police Car";
        } else if (vType == rs_knl::Vehicle::kVehicleTypeFireFighter) {
            vehName = "Fire Truck";
        } else if (vType == rs_knl::Vehicle::kVehicleTypeMedics) {
            vehName = "Ambulance";
        } else if (vType == rs_knl::Vehicle::kVehicleTypeSmallArmored) {
            vehName = "Small Armored Car";
        } else if (vType == rs_knl::Vehicle::kVehicleTypeLargeArmored || vType == rs_knl::Vehicle::kVehicleTypeLargeArmoredDamaged) {
            vehName = "Large Armored Car";
        } else if (vType == rs_knl::Vehicle::kVehicleTypeTrainHead) {
            vehName = "Train Locomotive";
        } else if (vType == rs_knl::Vehicle::kVehicleTypeTrainBody) {
            vehName = "Train Carriage";
        }
        vehName += " " + std::to_string(pVeh->id());
        lines.push_back(vehName);

        std::stringstream ssHealth;
        ssHealth << "Health: " << pVeh->health() << " / " << pVeh->startHealth();
        lines.push_back(ssHealth.str());

        std::stringstream ssSpeed;
        int currentSpeed = pVeh->isMoving() ? pVeh->speed() : 0;
        ssSpeed << "Speed: " << currentSpeed << " (max: " << pVeh->maxSpeed() << ")";
        lines.push_back(ssSpeed.str());

        std::string stateStr = "Stationary";
        if (!pVeh->isAlive()) {
            stateStr = "Destroyed";
        } else if (pVeh->isMoving()) {
            stateStr = "Moving";
        }
        lines.push_back("State: " + stateStr);

        rs_knl::PedInstance *pDriver = pVeh->getDriver();
        if (pDriver && pDriver->isAlive()) {
            std::string driverStr = "Civilian";
                switch (pDriver->type()) {
                case rs_knl::PedInstance::kPedTypeAgent:
                    if (pDriver->isOurAgent()) {
                        int agentNo = -1;
                        if (mission_ && mission_->getSquad()) {
                            for (size_t sqIdx = 0; sqIdx < mission_->getSquad()->size(); ++sqIdx) {
                                if (mission_->getSquad()->member(sqIdx) == pDriver) {
                                    agentNo = static_cast<int>(sqIdx + 1);
                                    break;
                                }
                            }
                        }
                        if (agentNo > 0) {
                            driverStr = "Agent " + std::to_string(agentNo);
                        } else {
                            driverStr = "Player Agent";
                        }
                    } else {
                        driverStr = "Enemy Agent " + std::to_string(pDriver->id());
                    }
                    break;
                case rs_knl::PedInstance::kPedTypePolice:
                    driverStr = "Police " + std::to_string(pDriver->id());
                    break;
                case rs_knl::PedInstance::kPedTypeGuard:
                    driverStr = "Guard " + std::to_string(pDriver->id());
                    break;
                case rs_knl::PedInstance::kPedTypeCriminal:
                    driverStr = "Criminal " + std::to_string(pDriver->id());
                    break;
                default:
                    driverStr = "Civilian " + std::to_string(pDriver->id());
                    break;
                }
                lines.push_back("Driver: " + driverStr);
        } else {
            lines.push_back("Driver: None");
        }
    } else if (pObj->is(rs_knl::MapObject::kNatureStatic)) {
        rs_knl::Static *pStatic = static_cast<rs_knl::Static *>(pObj);

        std::string staticName = "Object";
        switch (pStatic->type()) {
        case rs_knl::Static::smt_Door:
            staticName = "Door";
            break;
        case rs_knl::Static::smt_LargeDoor:
            staticName = "Large Door";
            break;
        case rs_knl::Static::smt_Window:
        case rs_knl::Static::smt_AnimatedWindow:
            staticName = "Window";
            break;
        case rs_knl::Static::smt_Tree:
            staticName = "Tree";
            break;
        case rs_knl::Static::smt_Advertisement:
            staticName = "Advertisement";
            break;
        case rs_knl::Static::smt_Semaphore:
            staticName = "Traffic Light";
            break;
        case rs_knl::Static::smt_NeonSign:
            staticName = "Neon Sign";
            break;
        default:
            break;
        }
        lines.push_back(staticName);

        if (pStatic->startHealth() > 0) {
            std::stringstream ssHealth;
            ssHealth << "Health: " << pStatic->health() << " / " << pStatic->startHealth();
            lines.push_back(ssHealth.str());
        }

        std::string stateStr = pStatic->isAlive() ? "Intact" : "Destroyed";
        lines.push_back("State: " + stateStr);

    } else if (pObj->is(rs_knl::MapObject::kNatureWeapon)) {
        rs_knl::WeaponInstance *pWep = static_cast<rs_knl::WeaponInstance *>(pObj);
        lines.push_back(pWep->name());

        if (pWep->startHealth() > 0) {
            std::stringstream ssHealth;
            ssHealth << "Health: " << pWep->health() << " / " << pWep->startHealth();
            lines.push_back(ssHealth.str());
        }

        if (pWep->usesAmmo() && pWep->ammoCapacity() > 0) {
            std::stringstream ssAmmo;
            ssAmmo << "Ammo: " << pWep->ammoRemaining() << " / " << pWep->ammoCapacity();
            lines.push_back(ssAmmo.str());
        }
    } else {
        lines.push_back(pObj->natureName());

        rs_knl::ShootableMapObject *pShoot = dynamic_cast<rs_knl::ShootableMapObject *>(pObj);
        if (pShoot && pShoot->startHealth() > 0) {
            std::stringstream ssHealth;
            ssHealth << "Health: " << pShoot->health() << " / " << pShoot->startHealth();
            lines.push_back(ssHealth.str());
        }

        rs_knl::ShootableMovableMapObject *pMove = dynamic_cast<rs_knl::ShootableMovableMapObject *>(pObj);
        if (pMove) {
            std::stringstream ssSpeed;
            int currentSpeed = pMove->isMoving() ? pMove->speed() : 0;
            ssSpeed << "Speed: " << currentSpeed;
            lines.push_back(ssSpeed.str());
        }
    }

    int maxTextWidth = 0;
    for (const auto &l : lines) {
        int w = static_cast<int>(l.length() * 8.5f);
        if (w > maxTextWidth) maxTextWidth = w;
    }
    int boxWidth = maxTextWidth + 20;
    int lineHeight = 13;
    int boxHeight = static_cast<int>(lines.size()) * lineHeight + 10;

    int boxX = last_motion_x_ + 15;
    int boxY = last_motion_y_ + 15;
    if (boxX + boxWidth > rs_eng::kScreenWidth - 10) {
        boxX = last_motion_x_ - boxWidth - 10;
    }
    if (boxY + boxHeight > rs_eng::kScreenHeight - 10) {
        boxY = rs_eng::kScreenHeight - boxHeight - 10;
    }
    if (boxX < 135) boxX = 135;
    if (boxY < 10) boxY = 10;

    g_System.drawFillRect({boxX, boxY}, boxWidth, boxHeight, menu_manager_->kMenuColorBlack);
    g_System.drawRect({boxX, boxY}, boxWidth, boxHeight, menu_manager_->kMenuColorYellow);

    for (size_t i = 0; i < lines.size(); ++i) {
        gameFont()->drawText(boxX + 8, boxY + 5 + static_cast<int>(i) * lineHeight, lines[i].c_str(), i == 0 ? menu_manager_->kMenuColorYellow : menu_manager_->kMenuColorWhite);
    }
}
