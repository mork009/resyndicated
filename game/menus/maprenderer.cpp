/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net>
 *   Copyright (C) 2013, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "menus/maprenderer.h"

#include <cmath>
#include <algorithm>

#include "rs-engine/gfx/tilemanager.h"
#include "rs-engine/gfx/rstexture.h"
#include "rs-engine/system/system.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/squad.h"
#include "rs-kernel/model/sfxobject.h"
#include "rs-kernel/mgr/agentmanager.h"

#include "menus/squadselection.h"
#include "rs-engine/config.h"

const int MapRenderer::kGameplayPanelWidth = 129;

MapRenderer::~MapRenderer() {
    pZoomTexture_.reset();
}

/*!
 * Initialize the renderer and centers on the leader of the selection.
 * @param pMission Pointer to current mission
 * @param pSelection Pointer to selection
 */
void MapRenderer::init(rs_knl::Mission *pMission, SquadSelection *pSelection) {
    pMission_ = pMission;
    pMap_ = pMission->map();
    pSelection_ = pSelection;
    lastZoom_ = rs_eng::g_mapZoom;

    // get the leader position on the map
    rs_knl::PedInstance *pLeader = pSelection_->leader();
    centerMapOnPed(pLeader);
}

/*!
 * Centers the map on the given ped
 * @param pPed 
 */
void MapRenderer::centerMapOnPed(rs_knl::PedInstance *pPed) {
    rs_knl::TilePoint center(pPed->tileX(),
                                pPed->tileY(),
                                pMission_->mmax_z_ + 1,
                                0, 0);

    Point2D start;
    pMap_->tileToScreenPoint(center, &start);
    int viewW = static_cast<int>((rs_eng::kScreenWidth - kGameplayPanelWidth) * rs_eng::g_mapZoom);
    int viewH = static_cast<int>(rs_eng::kScreenHeight * rs_eng::g_mapZoom);
    start.x -= viewW / 2;
    start.y -= viewH / 2;

    if (start.x < 0)
        start.x = 0;

    if (start.y < 0)
        start.y = 0;

    // Check if the position is within map borders
    rs_knl::TilePoint mpt = pMap_->screenToTilePoint(start.x, start.y);

    pMap_->clipToScrollLimits(mpt);

    // recalculating new screen coords
    rs_knl::TilePoint newPoint(mpt.tx,
                                mpt.ty,
                                pMission_->mmax_z_ + 1, 
                                0, 0);
    
    pMap_->tileToScreenPoint(newPoint, &viewportOriginPt_);
}

/*!
 * Centers the map on the given tile point
 * @param tilePt 
 */
void MapRenderer::centerMapOnTile(const rs_knl::TilePoint &tilePt) {
    rs_knl::TilePoint center(tilePt.tx,
                                tilePt.ty,
                                pMission_->mmax_z_ + 1,
                                0, 0);

    Point2D start;
    pMap_->tileToScreenPoint(center, &start);
    int viewW = static_cast<int>((rs_eng::kScreenWidth - kGameplayPanelWidth) * rs_eng::g_mapZoom);
    int viewH = static_cast<int>(rs_eng::kScreenHeight * rs_eng::g_mapZoom);
    start.x -= viewW / 2;
    start.y -= viewH / 2;

    if (start.x < 0)
        start.x = 0;

    if (start.y < 0)
        start.y = 0;

    // Check if the position is within map borders
    rs_knl::TilePoint mpt = pMap_->screenToTilePoint(start.x, start.y);

    pMap_->clipToScrollLimits(mpt);

    // recalculating new screen coords
    rs_knl::TilePoint newPoint(mpt.tx,
                                mpt.ty,
                                pMission_->mmax_z_ + 1, 
                                0, 0);
    
    pMap_->tileToScreenPoint(newPoint, &viewportOriginPt_);

    if (pMission_) {
        pMission_->setViewport(viewportOriginPt_, viewW, viewH,
                               std::max(256, static_cast<int>(256 * rs_eng::g_mapZoom)),
                               std::max(384, static_cast<int>(384 * rs_eng::g_mapZoom)));
    }
}

/**
 * Draw tiles and map objects.
 */
void MapRenderer::render() {
    double scale = rs_eng::g_mapZoom;
    int renderW = rs_eng::kScreenWidth - kGameplayPanelWidth;
    int renderH = rs_eng::kScreenHeight;
    int viewW = static_cast<int>(renderW * scale);
    int viewH = static_cast<int>(renderH * scale);

    if (std::abs(lastZoom_ - scale) > 0.001) {
        if (lastZoom_ > 0.0 && pMap_) {
            int oldViewW = static_cast<int>(renderW * lastZoom_);
            int oldViewH = static_cast<int>(renderH * lastZoom_);

            Point2D centerPt = {
                viewportOriginPt_.x + oldViewW / 2,
                viewportOriginPt_.y + oldViewH / 2
            };

            Point2D start = {
                centerPt.x - viewW / 2,
                centerPt.y - viewH / 2
            };
            if (start.x < 0) start.x = 0;
            if (start.y < 0) start.y = 0;

            rs_knl::TilePoint mpt = pMap_->screenToTilePoint(start.x, start.y);
            pMap_->clipToScrollLimits(mpt);
            rs_knl::TilePoint newPoint(mpt.tx, mpt.ty, pMission_ ? pMission_->mmax_z_ + 1 : 0, 0, 0);
            pMap_->tileToScreenPoint(newPoint, &viewportOriginPt_);
        }
        lastZoom_ = scale;
    }

    if (pMission_) {
        pMission_->setViewport(viewportOriginPt_, viewW, viewH,
                               std::max(256, static_cast<int>(256 * scale)),
                               std::max(384, static_cast<int>(384 * scale)));
    }

    if (std::abs(scale - 1.0) > 0.01) {
        int virtualWidth = static_cast<int>(renderW * scale);
        int virtualHeight = static_cast<int>(renderH * scale);

        if (!pZoomTexture_ || zoomTextureWidth_ != virtualWidth || zoomTextureHeight_ != virtualHeight) {
            pZoomTexture_ = g_System.createTexture();
            pZoomTexture_->createRenderTargetTexture(virtualWidth, virtualHeight);
            zoomTextureWidth_ = virtualWidth;
            zoomTextureHeight_ = virtualHeight;
        }

        pZoomTexture_->setAsRenderTarget();
        g_System.clearScreen();

        int buffer = std::max(256, static_cast<int>(256 * scale));
        rs_knl::TilePoint mtp = pMap_->screenToTilePoint(viewportOriginPt_.x, viewportOriginPt_.y);
        int offset = std::max(42, static_cast<int>(32 * scale) + 10);
        int sw = mtp.tx - offset;
        int chk = (virtualWidth + 2 * buffer) / (rs_eng::Tile::kTileWidth / 2)
            + (virtualHeight + 2 * buffer) / (rs_eng::Tile::kTileHeight / 3) + pMap_->maxTz() * 4 + offset * 2;
        int sh = mtp.ty - offset;

        int shm = sh + chk;

        listObjectsToDraw(viewportOriginPt_);

        int cmw = viewportOriginPt_.x + virtualWidth + buffer;
        int cmh = viewportOriginPt_.y + virtualHeight + buffer;
        int cmx = viewportOriginPt_.x;
        int chky = sh < 0 ? 0 : sh;
        int zr = shm + pMap_->maxTz() + 1;
        for (int inc = 0; inc < zr; ++inc) {
            int ye = sh + inc;
            int ys = ye - pMap_->maxTz() - 2;
            int tile_z = pMap_->maxTz() + 1;
            for (int yb = ys; yb < ye; ++yb) {
                if (yb < 0 || yb < sh || yb >= shm) {
                    --tile_z;
                    continue;
                }
                int tile_y = yb;
                for (int tile_x = sw; tile_y >= chky && tile_x < pMap_->maxTx(); ++tile_x) {
                    if (tile_x < 0 || tile_y >= pMap_->maxTy()) {
                        --tile_y;
                        continue;
                    }
                    int screen_w = (pMap_->maxTx() + (tile_x - tile_y)) * (rs_eng::Tile::kTileWidth / 2);
                    int coord_h = ((pMap_->maxTz() + tile_x + tile_y) - (tile_z - 1)) * (rs_eng::Tile::kTileHeight / 3);
                    if (screen_w >= viewportOriginPt_.x - buffer
                        && screen_w < cmw
                        && coord_h >= viewportOriginPt_.y - buffer
                        && coord_h < cmh) {

                        // Draw dynamic objects and wall statics on tile_z - 1 first so tile_z can clip them
                        if (tile_z - 1 >= 0) {
                            rs_knl::TilePoint currentTile(tile_x, tile_y, tile_z - 1);
                            Point2D screenPos = {screen_w - cmx + rs_eng::Tile::kTileWidth / 2,
                                coord_h - viewportOriginPt_.y + rs_eng::Tile::kTileHeight / 3 * 2};

                            drawWallStaticsForTile(currentTile, screenPos);
                            drawObjectsOnTile(currentTile, screenPos);
                        }

                        // Draw static map tile at tile_z
                        if (tile_z < pMap_->maxTz()) {
                            rs_eng::Tile *pTile = pMap_->getTileAt(tile_x, tile_y, tile_z);
                            if (pTile->notTransparent()) {
                                int dx = 0, dy = 0;
                                if (screen_w - viewportOriginPt_.x < 0)
                                    dx = -(screen_w - viewportOriginPt_.x);
                                if (coord_h - viewportOriginPt_.y < 0)
                                    dy = -(coord_h - viewportOriginPt_.y);
                                if (dx < rs_eng::Tile::kTileWidth && dy < rs_eng::Tile::kTileHeight) {
                                    pMap_->getTileManager()->drawTile(pTile, screen_w - cmx, coord_h - viewportOriginPt_.y);
                                }
                            }
                        }
                    }
                    --tile_y;
                }
                --tile_z;
            }
        }

        freeUnreleasedResources();

        if (g_System.getShowAgentPaths() || (g_System.getKeyModState() & rs_eng::KMD_LALT)) {
            rs_eng::RSColor yellow {227, 219, 40, 0xFF};
            for (SquadSelection::Iterator it = pSelection_->begin();
                it != pSelection_->end(); ++it) {
                (*it)->showPath(cmx, viewportOriginPt_.y, yellow);
            }
        }

        drawAgentMarkersOverlay(cmx);

        g_System.resetRenderTarget();
        pZoomTexture_->renderStretchedDest({kGameplayPanelWidth, 0}, renderW, renderH);
    } else {
        rs_knl::TilePoint mtp = pMap_->screenToTilePoint(viewportOriginPt_.x, viewportOriginPt_.y);
        int sw = mtp.tx;
        int chk = rs_eng::kScreenWidth / (rs_eng::Tile::kTileWidth / 2) + 2
            + rs_eng::kScreenHeight / (rs_eng::Tile::kTileHeight / 3) + pMap_->maxTz() * 2;
        int sh = mtp.ty - 8;

        int shm = sh + chk;

        listObjectsToDraw(viewportOriginPt_);

        int cmw = viewportOriginPt_.x + rs_eng::kScreenWidth -
                    kGameplayPanelWidth + 128;
        int cmh = viewportOriginPt_.y + rs_eng::kScreenHeight + 128;
        int cmx = viewportOriginPt_.x - kGameplayPanelWidth;
         //  z = 0 - is minimap data and mapdata
        int chky = sh < 0 ? 0 : sh;
        int zr = shm + pMap_->maxTz() + 1;
        for (int inc = 0; inc < zr; ++inc) {
            int ye = sh + inc;
            int ys = ye - pMap_->maxTz() - 2;
            int tile_z = pMap_->maxTz() + 1;  // the Z coord of the next tile to draw
            for (int yb = ys; yb < ye; ++yb) {
                if (yb < 0 || yb < sh || yb >= shm) {
                    --tile_z;
                    continue;
                }
                int tile_y = yb;  // The Y coord of the tile to draw
                for (int tile_x = sw; tile_y >= chky && tile_x < pMap_->maxTx(); ++tile_x) {
                    if (tile_x < 0 || tile_y >= pMap_->maxTy()) {
                        --tile_y;
                        continue;
                    }
                    int screen_w = (pMap_->maxTx() + (tile_x - tile_y)) * (rs_eng::Tile::kTileWidth / 2);
                    int coord_h = ((pMap_->maxTz() + tile_x + tile_y) - (tile_z - 1)) * (rs_eng::Tile::kTileHeight / 3);
                    if (screen_w >= viewportOriginPt_.x - rs_eng::Tile::kTileWidth * 2
                        && screen_w + rs_eng::Tile::kTileWidth * 2 < cmw
                        && coord_h >= viewportOriginPt_.y - rs_eng::Tile::kTileHeight * 2
                        && coord_h + rs_eng::Tile::kTileHeight * 2 < cmh) {

                        // Draw dynamic objects and wall statics on tile_z - 1 first so tile_z can clip them
                        if (tile_z - 1 >= 0) {
                            rs_knl::TilePoint currentTile(tile_x, tile_y, tile_z - 1);
                            Point2D screenPos = {screen_w - cmx + rs_eng::Tile::kTileWidth / 2,
                                coord_h - viewportOriginPt_.y + rs_eng::Tile::kTileHeight / 3 * 2};

                            drawWallStaticsForTile(currentTile, screenPos);
                            drawObjectsOnTile(currentTile, screenPos);
                        }

                        // Draw static map tile at tile_z
                        if (tile_z < pMap_->maxTz()) {
                            rs_eng::Tile *pTile = pMap_->getTileAt(tile_x, tile_y, tile_z);
                            if (pTile->notTransparent()) {
                                int dx = 0, dy = 0;
                                if (screen_w - viewportOriginPt_.x < 0)
                                    dx = -(screen_w - viewportOriginPt_.x);
                                if (coord_h - viewportOriginPt_.y < 0)
                                    dy = -(coord_h - viewportOriginPt_.y);
                                if (dx < rs_eng::Tile::kTileWidth && dy < rs_eng::Tile::kTileHeight) {
                                    pMap_->getTileManager()->drawTile(pTile, screen_w - cmx, coord_h - viewportOriginPt_.y);
                                }
                            }
                        }
                    }
                    --tile_y;
                }
                --tile_z;
            }
        }

        freeUnreleasedResources();

        if (g_System.getShowAgentPaths() || (g_System.getKeyModState() & rs_eng::KMD_LALT)) {
            rs_eng::RSColor yellow {227, 219, 40, 0xFF};
            for (SquadSelection::Iterator it = pSelection_->begin();
                it != pSelection_->end(); ++it) {
                (*it)->showPath(cmx, viewportOriginPt_.y, yellow);
            }
        }

        drawAgentMarkersOverlay(cmx);
    }
    DEBUG_SPEED_LOG("MapRenderer::render")
}

void MapRenderer::drawAgentMarkersOverlay(int cmx) {
    for (size_t i = 0; i < pMission_->numSfxObjects(); i++) {
        rs_knl::SFXObject *pSfx = pMission_->sfxObjects(i);
        if (pSfx->isDrawable() &&
            ((pSfx->type() >= rs_knl::SFXObject::sfxt_AgentFirst &&
              pSfx->type() <= rs_knl::SFXObject::sfxt_AgentFourth) ||
             pSfx->type() == rs_knl::SFXObject::sfxt_SelArrow)) {
            if (isObjectInsideDrawingArea(pSfx, viewportOriginPt_)) {
                rs_knl::TilePoint pos = pSfx->position();
                int screen_w = (pMap_->maxTx() + (pos.tx - pos.ty)) * (rs_eng::Tile::kTileWidth / 2);
                int coord_h = ((pMap_->maxTz() + pos.tx + pos.ty) - pos.tz) * (rs_eng::Tile::kTileHeight / 3);
                Point2D screenPos = {
                    screen_w - cmx + rs_eng::Tile::kTileWidth / 2,
                    coord_h - viewportOriginPt_.y + rs_eng::Tile::kTileHeight / 3 * 2
                };
                pSfx->draw(screenPos);
            }
        }
    }
}

int MapRenderer::tileHashKey(rs_knl::MapObject * m) {
    return tileHashKey(m->position());
}

static bool isGroundLyingObject(rs_knl::MapObject *obj) {
    if (!obj) return false;
    if (obj->is(rs_knl::MapObject::kNatureWeapon)) return true;
    if (obj->is(rs_knl::MapObject::kNaturePed)) {
        auto *ped = static_cast<rs_knl::PedInstance *>(obj);
        if (!ped->isAlive() || ped->health() <= 0) return true;
    }
    return false;
}

bool MapRenderer::shouldDrawBefore(rs_knl::MapObject *a, rs_knl::MapObject *b) {
    if (!a || !b) return false;

    bool aIsGround = isGroundLyingObject(a);
    bool bIsGround = isGroundLyingObject(b);
    bool aIsVeh = a->is(rs_knl::MapObject::kNatureVehicle);
    bool bIsVeh = b->is(rs_knl::MapObject::kNatureVehicle);

    // Ground objects (dead peds, ground weapons) always draw BEFORE vehicles on the same tile
    // so vehicles roll over them smoothly and occlude them progressively.
    if (aIsGround && bIsVeh) return true;
    if (aIsVeh && bIsGround) return false;

    // Ground weapons draw before dead peds
    bool aIsWpn = a->is(rs_knl::MapObject::kNatureWeapon);
    bool bIsWpn = b->is(rs_knl::MapObject::kNatureWeapon);
    if (aIsWpn != bIsWpn) {
        return aIsWpn;
    }

    // Default depth sorting on same tile
    return a->isBehindObjectOnSameTile(b);
}

void MapRenderer::listObjectsToDraw(const Point2D &viewport) {
    /*if (tilex < 0)
        tilex = 0;
    if (tiley < 0)
        tiley = 0;
    if (maxtilex >= pMap_->maxX())
        maxtilex = pMap_->maxX();
    if (maxtiley >= pMap_->maxTy())
        maxtiley = pMap_->maxTy();*/


    // Include peds
    for (size_t i = 0; i < pMission_->numPeds(); i++) {
        rs_knl::PedInstance *pPed = pMission_->ped(i);
        if (pPed->isDrawable() && isObjectInsideDrawingArea(pPed, viewport)) {
            addObjectToDraw(pPed);
        }
    }

    // vehicles
    for (size_t i = 0; i < pMission_->numVehicles(); i++) {
        rs_knl::Vehicle *pVehicle = pMission_->vehicle(i);
        if (isObjectInsideDrawingArea(pVehicle, viewport)) {
            addObjectToDraw(pVehicle);
        }
    }

    // weapons
    for (size_t i = 0; i < pMission_->numWeaponsOnGround(); i++) {
        rs_knl::WeaponInstance *pWeapon = pMission_->weaponOnGround(i);
        if (pWeapon->isDrawable() && isObjectInsideDrawingArea(pWeapon, viewport)) {
            addObjectToDraw(pWeapon);
        }
    }

    // statics
    for (size_t i = 0; i < pMission_->numStatics(); i++) {
        rs_knl::Static *pStatic = pMission_->statics(i);
        if (isObjectInsideDrawingArea(pStatic, viewport)) {
            addObjectToDraw(pStatic);
        }
    }

    // sfx objects
    for (size_t i = 0; i < pMission_->numSfxObjects(); i++) {
        rs_knl::SFXObject *pSfx = pMission_->sfxObjects(i);
        if ((pSfx->type() >= rs_knl::SFXObject::sfxt_AgentFirst &&
             pSfx->type() <= rs_knl::SFXObject::sfxt_AgentFourth) ||
            pSfx->type() == rs_knl::SFXObject::sfxt_SelArrow) {
            // Agent overhead markers and arrows are rendered in an overlay pass so they never clip
            continue;
        }
        if (pSfx->isDrawable() && isObjectInsideDrawingArea(pSfx, viewport)) {
            addObjectToDraw(pSfx);
        }
    }
}

/**
 * Return true if the object appears on the screen and so should be drawn.
 * \param pObject MapObject*
 * \param viewport const Point2D&
 * \return bool
 *
 */
bool MapRenderer::isObjectInsideDrawingArea(rs_knl::MapObject *pObject, const Point2D &viewport) {
    Point2D objectViewport;
    pMap_->tileToScreenPoint(pObject->position(), &objectViewport);

    double scale = rs_eng::g_mapZoom;
    int viewW = static_cast<int>((rs_eng::kScreenWidth - kGameplayPanelWidth) * scale);
    int viewH = static_cast<int>(rs_eng::kScreenHeight * scale);

    int marginX = std::max(256, static_cast<int>(256 * scale));
    int marginY = std::max(384, static_cast<int>(384 * scale));

    return objectViewport.x >= (viewport.x - marginX) &&
           objectViewport.x <= (viewport.x + viewW + marginX) &&
           objectViewport.y >= (viewport.y - marginY) &&
           objectViewport.y <= (viewport.y + viewH + marginY);
}

/**
 * Draw all objects on the given tile.
 * \param tilePos const TilePoint& tile coordinates
 * \param screenPos const Point2D& position of tile on the screen
 * \return int number of objects for debug
 *
 */
int MapRenderer::drawObjectsOnTile(const rs_knl::TilePoint & tilePos, const Point2D &screenPos) {
    int tileKey = tileHashKey(tilePos);
    int nbDrawnObjects = 0;

    std::map<int, ObjectToDraw *>::iterator it = objectsByTile_.find(tileKey);
    if(it != objectsByTile_.end()) {
        ObjectToDraw *pObj = it->second;
        objectsByTile_.erase(it);
        while(pObj != NULL) {
            rs_knl::MapObject *mObj = pObj->getObject();
            int dtx = tilePos.tx - mObj->position().tx;
            int dty = tilePos.ty - mObj->position().ty;
            int dz = tilePos.tz - mObj->position().tz;
            Point2D drawPos = screenPos;
            if (dtx != 0 || dty != 0) {
                drawPos.x -= (dtx - dty) * (rs_eng::Tile::kTileWidth / 2);
                drawPos.y -= (dtx + dty) * (rs_eng::Tile::kTileHeight / 3);
            }
            if (dz != 0) {
                drawPos.y += dz * (rs_eng::Tile::kTileHeight / 3);
            }
            mObj->draw(drawPos);
            ObjectToDraw *pNext = pObj->getNext();
            pool_.releaseResource(pObj);
            pObj = pNext;
            nbDrawnObjects++;
        }
    }

    return nbDrawnObjects;
}

int MapRenderer::drawWallStaticsOnTile(const rs_knl::TilePoint & tilePos, const Point2D &screenPos) {
    int tileKey = tileHashKey(tilePos);
    int nbDrawnObjects = 0;

    std::map<int, ObjectToDraw *>::iterator it = wallStaticsByTile_.find(tileKey);
    if(it != wallStaticsByTile_.end()) {
        ObjectToDraw *pObj = it->second;
        wallStaticsByTile_.erase(it);
        while(pObj != NULL) {
            pObj->getObject()->draw(screenPos);
            ObjectToDraw *pNext = pObj->getNext();
            pool_.releaseResource(pObj);
            pObj = pNext;
            nbDrawnObjects++;
        }
    }

    return nbDrawnObjects;
}


int MapRenderer::drawWallStaticsForTile(const rs_knl::TilePoint & tilePos, const Point2D &screenPos) {
    int nbDrawn = drawWallStaticsOnTile(tilePos, screenPos);
    if (objectsByTile_.find(tileHashKey(tilePos)) != objectsByTile_.end()) {
        for (int dz = 1; dz <= 3; ++dz) {
            rs_knl::TilePoint upperTile(tilePos.tx, tilePos.ty, tilePos.tz + dz);
            Point2D upperScreenPos = screenPos;
            upperScreenPos.y -= dz * (rs_eng::Tile::kTileHeight / 3);
            nbDrawn += drawWallStaticsOnTile(upperTile, upperScreenPos);
        }
    }
    return nbDrawn;
}

/**
 * Adds an object to the list of objects to draw for the tile it's on.
 * For a given tile object are sorted from back to front so that
 * objects in the back are drawn first.
 * \param pObjectToAdd MapObject* Object to add
 * \return void
 *
 */
void MapRenderer::addObjectToDraw(rs_knl::MapObject *pObjectToAdd) {
    int tileKey;
    ObjectToDraw *pNewEntry = pool_.getResource();
    pNewEntry->setObject(pObjectToAdd);

    bool isWallStatic = false;
    if (pObjectToAdd->is(rs_knl::MapObject::kNatureStatic)) {
        auto *pStatic = static_cast<rs_knl::Static *>(pObjectToAdd);
        if (pStatic->isWallMounted()) {
            isWallStatic = true;
        }
    }

    rs_knl::TilePoint drawTile = pObjectToAdd->position();
    if (pObjectToAdd->is(rs_knl::MapObject::kNatureVehicle)) {
        auto *pV = static_cast<rs_knl::Vehicle *>(pObjectToAdd);
        if (pV->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead ||
            pV->getType() == rs_knl::Vehicle::kVehicleTypeTrainBody) {
            // Trains in Syndicate level data are stored at track_z (e.g. tz=2),
            // while pedestrians, bodies, and weapons on the tracks are stored at track_z + 1 (e.g. tz=3).
            // Elevate train draw bucket to tz + 1 so it shares the exact layer bucket with track entities,
            // matching the behavior of road vehicles (which are stored at road_z + 1).
            drawTile.tz += 1;
            // Use the foremost tile in isometric depth (max X+Y) so the train's nose
            // renders after ground entities on the tile it is entering, allowing smooth progressive occlusion
            int wx = pV->position().tx * 256 + pV->position().ox;
            int wy = pV->position().ty * 256 + pV->position().oy;
            drawTile.tx = (wx + 128) / 256;
            drawTile.ty = (wy + 128) / 256;
        }
    }

    tileKey = tileHashKey(drawTile);

    std::map<int, ObjectToDraw *> &targetMap = isWallStatic ? wallStaticsByTile_ : objectsByTile_;

    std::map<int, ObjectToDraw *>::iterator element = targetMap.find(tileKey);
    if(element == targetMap.end()) {
        // no element has been set with the tile so add the first element
        targetMap[tileKey] = pNewEntry;
    } else {
        // there is at least one element already set with the tile
        ObjectToDraw *pObjectInList = element->second;
        if (shouldDrawBefore(pObjectToAdd, pObjectInList->getObject())) {
            // first case is when the new object should be first in the list
            pNewEntry->setNext(pObjectInList);
            targetMap[tileKey] = pNewEntry;
        } else {
            // second case is when new object is somewhere in the list
            while (pObjectInList != NULL) {
                if (pObjectInList->getNext() != NULL) {
                    if (shouldDrawBefore(pObjectToAdd, pObjectInList->getNext()->getObject())) {
                        pObjectInList->insertNext(pNewEntry);
                        break;
                    } else {
                        pObjectInList = pObjectInList->getNext();
                    }
                } else {
                    pObjectInList->setNext(pNewEntry);
                    break;
                }
            }
        }
    }
}

/**
 * Objects that were listed for drawing may be bigger than objects really drawn.
 * So remove those objects from the list.
 * \return void
 *
 */
void MapRenderer::freeUnreleasedResources() {
    int nbFreed = 0;
    std::map<int, ObjectToDraw *>::iterator itr = objectsByTile_.begin();
    while (itr != objectsByTile_.end()) {
        std::map<int, ObjectToDraw *>::iterator toErase = itr;
        ++itr;
        ObjectToDraw *pObj = toErase->second;
        while(pObj != NULL) {
            ObjectToDraw *pNext = pObj->getNext();
            pool_.releaseResource(pObj);
            nbFreed++;
            pObj = pNext;
        }
        objectsByTile_.erase(toErase);
    }
    std::map<int, ObjectToDraw *>::iterator itrWall = wallStaticsByTile_.begin();
    while (itrWall != wallStaticsByTile_.end()) {
        std::map<int, ObjectToDraw *>::iterator toErase = itrWall;
        ++itrWall;
        ObjectToDraw *pObj = toErase->second;
        while(pObj != NULL) {
            ObjectToDraw *pNext = pObj->getNext();
            pool_.releaseResource(pObj);
            nbFreed++;
            pObj = pNext;
        }
        wallStaticsByTile_.erase(toErase);
    }
}

/*!
 * Scroll the map horizontally.
 * Each map has a min and max value for the world origin coords and this
 * method moves that point between those limits. If scrolling hits the
 * map border, the scrolling is made along that border.
 */
void MapRenderer::scrollOnX(int scrollAmount) {
    int newOriginX = viewportOriginPt_.x + scrollAmount;

    rs_knl::TilePoint mpt = pMap_->screenToTilePoint(newOriginX, viewportOriginPt_.y);

    int stepX = scrollAmount;
    int stepY = scrollAmount / 2;

    // Scroll to the right
    if (scrollAmount > 0) {
        if (pMap_->isScrollMinLimitHitOnTy(mpt)) {
            // we hit the upper right border of the map
            // so we scroll down until the far right corner
            int newWorldY = viewportOriginPt_.y + stepY;
            newOriginX = viewportOriginPt_.x + stepX;
            mpt = pMap_->screenToTilePoint(newOriginX, newWorldY);

            if (pMap_->isScrollMinLimitHitOnTy(mpt) || pMap_->isScrollMaxLimitHitOnTx(mpt)) {
                // We hit the corner so don't scroll
                return;
            } else {
                viewportOriginPt_.x = newOriginX;
                viewportOriginPt_.y = newWorldY;
            }
        } else if (pMap_->isScrollMaxLimitHitOnTx(mpt)) {
            // we hit the lower right border of the map
            // so we scroll up until the far right corner
            int newWorldY = viewportOriginPt_.y - stepY;
            newOriginX = viewportOriginPt_.x + stepX;
            mpt = pMap_->screenToTilePoint(newOriginX, newWorldY);

            if (pMap_->isScrollMinLimitHitOnTy(mpt) || pMap_->isScrollMaxLimitHitOnTx(mpt)) {
                return;
            } else {
                viewportOriginPt_.x = newOriginX;
                viewportOriginPt_.y = newWorldY;
            }
        } else {
            // This is a regular right scroll
            viewportOriginPt_.x = newOriginX;
        }

    } else { // Scroll to the left
        if (pMap_->isScrollMinLimitHitOnTx(mpt)) {
            // we hit the west border of the map
            // so we scroll toward south border
            int newWorldY = viewportOriginPt_.y - stepY;
            newOriginX = viewportOriginPt_.x + stepX;
            mpt = pMap_->screenToTilePoint(newOriginX, newWorldY);

            if (pMap_->isScrollMinLimitHitOnTx(mpt) || pMap_->isScrollMaxLimitHitOnTy(mpt)) {
                return;
            } else {
                viewportOriginPt_.x = newOriginX;
                viewportOriginPt_.y = newWorldY;
            }
        } else if (pMap_->isScrollMaxLimitHitOnTy(mpt)) {
            // we hit the south border of the map
            // so we scroll towards the west border
            int newWorldY = viewportOriginPt_.y + stepY;
            newOriginX = viewportOriginPt_.x + stepX;
            mpt = pMap_->screenToTilePoint(newOriginX, newWorldY);

            if (pMap_->isScrollMinLimitHitOnTx(mpt) || pMap_->isScrollMaxLimitHitOnTy(mpt)) {
                return;
            } else {
                viewportOriginPt_.x = newOriginX;
                viewportOriginPt_.y = newWorldY;
            }
        } else {
            viewportOriginPt_.x = newOriginX;
        }
    }
}

/*!
 * Scroll the map vertically.
 * Each map has a min and max value for the world origin coords and this
 * method moves that point between those limits. If scrolling hits the
 * map border, the scrolling is made along that border.
 */
void MapRenderer::scrollOnY(int scrollAmount) {
    int newWorldY = viewportOriginPt_.y + scrollAmount;

    rs_knl::TilePoint mpt = pMap_->screenToTilePoint(viewportOriginPt_.x, newWorldY);

    int stepY = (scrollAmount > 8 || scrollAmount < -8) ? (scrollAmount / 2) : scrollAmount;
    int stepX = 2 * std::abs(stepY);

    // Scroll down
    if (scrollAmount > 0) {
        if (pMap_->isScrollMaxLimitHitOnTx(mpt)) {
            // we hit the lower right border of the map
            // so we scroll down until the lower corner
            int newOriginX = viewportOriginPt_.x - stepX;
            int newSlideY = viewportOriginPt_.y + stepY;
            mpt = pMap_->screenToTilePoint(newOriginX, newSlideY);

            if (pMap_->isScrollMaxLimitHitOnTy(mpt) || pMap_->isScrollMaxLimitHitOnTx(mpt)) {
                return;
            } else {
                viewportOriginPt_.x = newOriginX;
                viewportOriginPt_.y = newSlideY;
            }
        } else if (pMap_->isScrollMaxLimitHitOnTy(mpt)) {
            // we hit the lower left border of the map
            // so we scroll down until the lower corner
            int newOriginX = viewportOriginPt_.x + stepX;
            int newSlideY = viewportOriginPt_.y + stepY;
            mpt = pMap_->screenToTilePoint(newOriginX, newSlideY);

            if (pMap_->isScrollMaxLimitHitOnTy(mpt) || pMap_->isScrollMaxLimitHitOnTx(mpt)) {
                return;
            } else {
                viewportOriginPt_.x = newOriginX;
                viewportOriginPt_.y = newSlideY;
            }
        } else {
            viewportOriginPt_.y = newWorldY;
        }

    } else { // Scroll up
        if (pMap_->isScrollMinLimitHitOnTx(mpt)) {
            // we hit the west border of the map
            // so we scroll towards the south border
            int newOriginX = viewportOriginPt_.x + stepX;
            int newSlideY = viewportOriginPt_.y + stepY;
            mpt = pMap_->screenToTilePoint(newOriginX, newSlideY);

            if (pMap_->isScrollMinLimitHitOnTy(mpt) || pMap_->isScrollMinLimitHitOnTx(mpt)) {
                return;
            } else {
                viewportOriginPt_.x = newOriginX;
                viewportOriginPt_.y = newSlideY;
            }
        } else if (pMap_->isScrollMinLimitHitOnTy(mpt)) {
            // we hit the upper left border of the map
            // so we scroll up until the upper corner
            int newOriginX = viewportOriginPt_.x - stepX;
            int newSlideY = viewportOriginPt_.y + stepY;
            mpt = pMap_->screenToTilePoint(newOriginX, newSlideY);

            if (pMap_->isScrollMinLimitHitOnTy(mpt) || pMap_->isScrollMinLimitHitOnTx(mpt)) {
                return;
            } else {
                viewportOriginPt_.x = newOriginX;
                viewportOriginPt_.y = newSlideY;
            }
        } else {
            viewportOriginPt_.y = newWorldY;
        }
    }
}

rs_knl::TilePoint MapRenderer::getTilePointFromMouse(const Point2D &mousePt) {
    if (mousePt.x <= kGameplayPanelWidth) {
        return pMap_->screenToTilePoint(viewportOriginPt_.x, viewportOriginPt_.y);
    }
    double scale = rs_eng::g_mapZoom;
    int virtualX = static_cast<int>((mousePt.x - kGameplayPanelWidth) * scale) + viewportOriginPt_.x;
    int virtualY = static_cast<int>(mousePt.y * scale) + viewportOriginPt_.y;
    return pMap_->screenToTilePoint(virtualX, virtualY);
}

/*!
 * @brief 
 * @param mousePt 
 * @param mapObject 
 * @param padTopLeft 
 * @param padSize 
 * @return 
 */
bool MapRenderer::isMouseHovering(const Point2D &mousePt, const rs_knl::MapObject &mapObject, const Point2D &padTopLeft, const Point2D &padSize) {
    if (mousePt.x <= kGameplayPanelWidth) return false;

    double scale = rs_eng::g_mapZoom;
    int virtualX = static_cast<int>((mousePt.x - kGameplayPanelWidth) * scale) + viewportOriginPt_.x;
    int virtualY = static_cast<int>(mousePt.y * scale) + viewportOriginPt_.y;

    Point2D scPt;
    pMap_->tileToScreenPoint(mapObject.position(), &scPt);

    int px = scPt.x - padTopLeft.x;
    int py = scPt.y - padTopLeft.y;

    return (virtualX >= px && virtualX < px + padSize.x &&
            virtualY >= py && virtualY < py + padSize.y);
}
