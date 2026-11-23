/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net>
 *   Copyright (C) 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-kernel/model/map.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <vector>
#include <map>
#include <cstring>

#include "rs-utils/log/log.h"
#include "rs-utils/io/dernc.h"
#include "rs-engine/gfx/tilemanager.h"

namespace rs_knl {

const uint8_t Map::kConnexionMaskEntryNorth = 0x80;
const uint8_t Map::kConnexionMaskEntrySouth = 0x40;
const uint8_t Map::kConnexionMaskEntryEast = 0x20;
const uint8_t Map::kConnexionMaskEntryWest = 0x10;

const uint8_t Map::kConnexionMaskExitNorth = 0x08;
const uint8_t Map::kConnexionMaskExitSouth = 0x04;
const uint8_t Map::kConnexionMaskExitEast = 0x02;
const uint8_t Map::kConnexionMaskExitWest = 0x01;

Map::Map(rs_eng::TileManager * tileManager, uint16_t anId) : tileManager_(tileManager)
{
    id_ = anId;
    a_tiles_ = nullptr;
    assert(tileManager != nullptr);
    minScrollTile_ = {0, 0};
    maxScrollTile_ = {0, 0};
}

Map::~Map()
{
    delete[] a_tiles_;
}

/*!
 * Set the list of tiles used for this map.
 * Loading tiles from game files is done in MapManager.
 * @param maxX Number of tile on X axis
 * @param maxY Number of tile on Y axis
 * @param maxZ Number of tile on Z axis
 * @param tiles The array of tiles. The array has one level more than
 * in game file for range checking
 */
void Map::setTiles(int maxX, int maxY, int maxZ, rs_eng::Tile **tiles) {
    maxTx_ = maxX;
    maxTy_ = maxY;
    maxTz_ = maxZ;

    int size = maxTx_ * maxTy_ * maxTz_;
    a_tiles_ = new rs_eng::Tile*[size];
    
    if (tiles != nullptr) {
        for (int i=0; i<size; i++) {
            a_tiles_[i] = tiles[i];
        }
    } else {
        rs_eng::Tile *emptyTile = tileManager_ ? tileManager_->getTile(0) : nullptr;
        for (int i=0; i<size; i++) {
            a_tiles_[i] = emptyTile;
        }
    }
}

void Map::setScrollLimits(Point2D minScrollTile, Point2D  maxScrollTile) {
    LOG(Log::k_FLG_GAME, "Map", "setScrollLimits", ("Min Tx Ty(%d, %d), max Tx Ty(%d, %d)\n", minScrollTile.x, minScrollTile.y, maxScrollTile.x, maxScrollTile.y));
    minScrollTile_ = minScrollTile;
    maxScrollTile_ = maxScrollTile;
}

Point2D Map::getEffectiveMinScrollTile(double zoom) const {
    if (zoom < 0.0) {
        zoom = rs_eng::g_mapZoom;
    }
    if (minScrollTile_.x == 0 && minScrollTile_.y == 0 &&
        maxScrollTile_.x == 0 && maxScrollTile_.y == 0) {
        return {0, 0};
    }
    if (std::abs(zoom - 1.0) < 0.01) {
        return minScrollTile_;
    }

    int effMinX = minScrollTile_.x;
    int effMinY = minScrollTile_.y;

    if (zoom < 1.0) {
        double delta = 1.0 - zoom;
        effMinX -= static_cast<int>(std::round(18.0 * delta));
        effMinY -= static_cast<int>(std::round(6.0 * delta));
    } else if (zoom > 1.0) {
        // When zoomed out, provide a calibrated margin on minX / minY
        // so edge landmarks (such as the base exit gate and perimeter road) remain fully reachable.
        double delta = zoom - 1.0;
        effMinX -= static_cast<int>(std::round(6.0 * std::min(1.0, delta)));
        effMinY -= static_cast<int>(std::round(3.0 * std::min(1.0, delta)));
    }

    effMinX = std::max(0, effMinX);
    effMinY = std::max(0, effMinY);

    return {effMinX, effMinY};
}

Point2D Map::getEffectiveMaxScrollTile(double zoom) const {
    if (zoom < 0.0) {
        zoom = rs_eng::g_mapZoom;
    }
    if (minScrollTile_.x == 0 && minScrollTile_.y == 0 &&
        maxScrollTile_.x == 0 && maxScrollTile_.y == 0) {
        return {maxTx_ > 0 ? maxTx_ - 1 : 0, maxTy_ > 0 ? maxTy_ - 1 : 0};
    }
    if (std::abs(zoom - 1.0) < 0.01) {
        return maxScrollTile_;
    }

    int effMaxX = maxScrollTile_.x;
    int effMaxY = maxScrollTile_.y;

    if (zoom < 1.0) {
        double delta = 1.0 - zoom;
        effMaxX += static_cast<int>(std::round(24.0 * delta + 6.0));
        effMaxY += static_cast<int>(std::round(8.0 * delta + 3.0));
    } else if (zoom > 1.0) {
        // For maxX (eastern perimeter), contract by the additional viewport span
        // so the camera does not pan beyond edge entities (like squad spawn) into empty space.
        // For maxY (southwestern perimeter), allow a modest margin to view full exit paths and gates.
        double delta = zoom - 1.0;
        effMaxX -= static_cast<int>(std::round(20.5 * delta));
        effMaxY += static_cast<int>(std::round(4.0 * std::min(1.0, delta)));
    }

    Point2D effMin = getEffectiveMinScrollTile(zoom);
    effMaxX = std::max(effMin.x, effMaxX);
    effMaxY = std::max(effMin.y, effMaxY);

    if (maxTx_ > 0) effMaxX = std::min(maxTx_ - 1, effMaxX);
    if (maxTy_ > 0) effMaxY = std::min(maxTy_ - 1, effMaxY);

    return {effMaxX, effMaxY};
}

/*!
 * @brief 
 * @param point 
 */
void Map::clipToScrollLimits(rs_knl::TilePoint &point) {
    Point2D effMin = getEffectiveMinScrollTile();
    Point2D effMax = getEffectiveMaxScrollTile();

    if (point.tx < effMin.x)
        point.tx = effMin.x;
    else if (point.tx > effMax.x) {
        point.tx = effMax.x;
    }

    if (point.ty < effMin.y)
        point.ty = effMin.y;
    else if (point.ty > effMax.y) {
        point.ty = effMax.y;
    }
}


void Map::mapDimensions(int *x, int *y, int *z)
{
    *x = maxTx();
    *y = maxTy();
    *z = maxTz();
}

/*!
 * Clip x,y,z to map dimensions.
 */
void Map::adjXYZ(int &x, int &y, int &z) {
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (z < 0 || z >= maxTz())
        z = 0;
    if (x >= maxTx())
        x = maxTx() - 1;
    if (y >= maxTy())
        y = maxTy() - 1;
}

void Map::clip(Point2D *pPoint) {
    if (pPoint->x < 0) {
        pPoint->x = 0;
    } else if (pPoint->x >= maxTx()) {
        pPoint->x = maxTx();
    }

    if (pPoint->y < 0) {
        pPoint->y = 0;
    } else if (pPoint->y >= maxTy()) {
        pPoint->y = maxTy();
    }
}

void Map::clip(TilePoint *pPoint) {
    if (pPoint->tx < 0) {
        pPoint->tx = 0;
    } else if (pPoint->tx >= maxTx()) {
        pPoint->tx = maxTx() - 1;
    }

    if (pPoint->ty < 0) {
        pPoint->ty = 0;
    } else if (pPoint->ty >= maxTy()) {
        pPoint->ty = maxTy() - 1;
    }

    if (pPoint->tz < 0 || pPoint->tz >= maxTz()) {
        pPoint->tz = 0;
    }
}

/**
 * Checks if a tile point is within the map boundaries.
 * @return true if point is within boundaries
 */
bool Map::isWithinMapBounds(const TilePoint &point) const {
    return point.tz >= 0 && point.tz < maxTz() &&
           point.tx >= 0 && point.tx < maxTx() &&
           point.ty >= 0 && point.ty < maxTy();
}

/**
 * Projects a tile point to a specific Z level using isometric transformation.
 * Each Z level shifts position by half a tile (128 units) in both X and Y.
 */
TilePoint Map::projectToZLevel(const TilePoint &point, int targetZ) {
    TilePoint result;
    
    // Convert to continuous coordinates with isometric Z offset
    int worldX = point.tx * 256 + point.ox + 128 * (targetZ - 1);
    int worldY = point.ty * 256 + point.oy + 128 * (targetZ - 1);
    
    // Convert back to tile + offset coordinates
    result.tx = worldX / 256;
    result.ox = worldX % 256;
    result.ty = worldY / 256;
    result.oy = worldY % 256;
    result.tz = targetZ;
    
    return result;
}

float scalexPx = 256.0f;
float scalexPy = 256.0f;
float scaleyPx = 256.0f;
float scaleyPy = 256.0f;


void Map::tileToScreenPoint(const TilePoint &tPt, Point2D *pScp) {
    float fx = static_cast<float> (tPt.tx) + tPt.ox / scalexPx;
    float fy = tPt.ty + tPt.oy / scalexPy;

    pScp->x = (int) ((maxTx_ * rs_eng::Tile::kTileWidth / 2) + (fx - fy) * rs_eng::Tile::kTileWidth / 2
                  + rs_eng::Tile::kTileWidth / 2);

    pScp->y = (int) ((maxTz_ + 1) * rs_eng::Tile::kTileHeight / 3 + (fx + fy) * rs_eng::Tile::kTileHeight / 3);
}

/*!
 * Returns the tile position corresponding to the screen position.
 * Usually after the user clicked on the map, this method is used
 * to find what tile has been clicked.
 * \param x The x position on the screen (in pixel).
 * \param y The y position on the screen (in pixel).
 */
TilePoint Map::screenToTilePoint(int x, int y)
{
    TilePoint mtp;

    x -= (maxTx_ + 1) * (rs_eng::Tile::kTileWidth / 2);
    // x now equals fx * Tile::kTileWidth / 2 - fy * Tile::kTileWidth / 2
    // which equals Tile::kTileWidth/2 * (fx - fy)
    y -= (maxTz_ + 1) * (rs_eng::Tile::kTileHeight / 3);
    // y now equals (fx + fy) * Tile::kTileHeight / 3
    float dx = (float) x / (rs_eng::Tile::kTileWidth / 2);
    float dy = (float) y / (rs_eng::Tile::kTileHeight / 3);

    // dx equals fx - fy
    // dy equals fx + fy
    float f_tx = (dx + dy) / 2;
    mtp.tx = (int) f_tx;
    mtp.ox = (int) ((f_tx - mtp.tx) * 256.0f);

    float f_ty = (dy - dx) / 2;
    mtp.ty = (int) f_ty;
    mtp.oy = (int) ((f_ty - mtp.ty) * 256.0f);

    return mtp;
}

int Map::maxZAt(int x, int y)
{
    assert(x < maxTx_);
    assert(y < maxTy_);
    /*
    int idx = y * maxTx_ + x;
    int mz = 0;
    for (int z = 0; z < maxTz_; z++) {
        int tile = map_data_[idx * maxTz_ + z];
        if (tile > 5) {
            mz = z;
        }
    }
    */
    // TODO: disabling this thing, causes a lot of speed drain
    // find a better for such optimization, not all objects are drawn
    // that is why I disabled it
    return maxTz_ - 1;
}

rs_eng::Tile * Map::getTileAt(int x, int y, int z)
{
    if (x < 0 || x >= maxTx_ || y < 0 || y >= maxTy_) {
        return tileManager_->getTile(z < 2 ? 6 : 0);
    }

    if (z < 0 || z >= maxTz_) {
        return tileManager_->getTile(0);
    }

    return a_tiles_[(y * maxTx_ + x) * maxTz_ + z];
}

/*!
 * Return the tile at given position.
 * @param tilePt Coord of the tile. Only uses tx, ty and tz
 * @return If coord are out of map limit, return a default tile
 */
rs_eng::Tile * Map::getTileAt(const TilePoint &tilePt) {
    return getTileAt(tilePt.tx, tilePt.ty, tilePt.tz);
}

int Map::getTileIdAt(int tx, int ty, int tz)
{
    if (tx < 0 || tx >= maxTx_)
        return tz < 2 ? 6 : 0;
    if (ty < 0 || ty >= maxTy_)
        return tz < 2 ? 6 : 0;
    if (tz < 0 || tz >= maxTz_)
        return 0;

    rs_eng::Tile *tile = a_tiles_[(ty * maxTx_ + tx) * maxTz_ + tz];
    return tile ? tile->id() : 0;
}

void Map::patchMap(int x, int y, int z, uint8_t tileNum)
{
    assert((x >= 0 && x < maxTx_)
        && (y >= 0 && y < maxTy_)
        && (z >= 0 && z < maxTz_));
    a_tiles_[(y * maxTx_ + x) * maxTz_ + z] = tileManager_->getTile(tileNum);
}

bool Map::setTileAt(int tx, int ty, int tz, uint8_t tileNum) {
    if (tx < 0 || tx >= maxTx_ || ty < 0 || ty >= maxTy_ || tz < 0 || tz >= maxTz_) {
        return false;
    }
    a_tiles_[(ty * maxTx_ + tx) * maxTz_ + tz] = tileManager_->getTile(tileNum);
    return true;
}

bool Map::exportBinary(const std::string &filepath) {
    if (!a_tiles_ || maxTx_ <= 0 || maxTy_ <= 0 || maxTz_ <= 0) {
        return false;
    }

    uint32_t maxX = static_cast<uint32_t>(maxTx_);
    uint32_t maxY = static_cast<uint32_t>(maxTy_);
    uint32_t maxZ = static_cast<uint32_t>(maxTz_ > 1 ? maxTz_ - 1 : 1);

    uint32_t numColumns = maxX * maxY;
    uint32_t lookupTableSizeBytes = numColumns * sizeof(uint32_t);

    std::map<std::vector<uint8_t>, uint32_t> uniqueColumns;
    std::vector<uint32_t> lookupTable(numColumns);
    std::vector<uint8_t> tilePayload;
    tilePayload.reserve(numColumns * maxZ / 4);

    std::vector<uint8_t> colBuffer(maxZ);

    for (uint32_t y = 0; y < maxY; ++y) {
        for (uint32_t x = 0; x < maxX; ++x) {
            uint32_t colIdx = y * maxX + x;

            for (uint32_t z = 0; z < maxZ; ++z) {
                rs_eng::Tile *tile = a_tiles_[(y * maxTx_ + x) * maxTz_ + z];
                colBuffer[z] = tile ? static_cast<uint8_t>(tile->id()) : 0;
            }

            auto it = uniqueColumns.find(colBuffer);
            if (it == uniqueColumns.end()) {
                uint32_t offset = lookupTableSizeBytes + static_cast<uint32_t>(tilePayload.size());
                uniqueColumns[colBuffer] = offset;
                lookupTable[colIdx] = offset;
                tilePayload.insert(tilePayload.end(), colBuffer.begin(), colBuffer.end());
            } else {
                lookupTable[colIdx] = it->second;
            }
        }
    }

    uint32_t totalSizeBytes = 12 + lookupTableSizeBytes + static_cast<uint32_t>(tilePayload.size());
    std::vector<uint8_t> buffer(totalSizeBytes, 0);

    rs_utl::WRITE_LE_UINT32(buffer.data() + 0, maxX);
    rs_utl::WRITE_LE_UINT32(buffer.data() + 4, maxY);
    rs_utl::WRITE_LE_UINT32(buffer.data() + 8, maxZ);

    for (uint32_t i = 0; i < numColumns; ++i) {
        rs_utl::WRITE_LE_UINT32(buffer.data() + 12 + i * 4, lookupTable[i]);
    }

    if (!tilePayload.empty()) {
        std::memcpy(buffer.data() + 12 + lookupTableSizeBytes, tilePayload.data(), tilePayload.size());
    }

    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }

    std::vector<uint8_t> packed;
    rnc::RncRetCode rc = rnc::pack(buffer.data(), buffer.size(), packed);
    if (rc == rnc::kOk && !packed.empty()) {
        out.write(reinterpret_cast<const char*>(packed.data()), packed.size());
    } else {
        out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    }
    out.close();
    return true;
}

/*!
 *
 * @return SurfaceType
 *
 */
SurfaceType Map::getSurfaceTypeForTile(int tx, int ty, int tz) {
    rs_eng::Tile *pTile = getTileAt(tx, ty, tz);
    
    // For the following tiles, we overload the type based on the tile id
    switch (pTile->id()) {
    case 0x80 :
        return SurfaceType::kTrainPlatformNS;
    case 0x81 :
        return SurfaceType::kTrainPlatformEW;
    case 0x8F :
        return SurfaceType::kEmpty;
    case 0x93 :
        return SurfaceType::kEmpty;
    }
    
    // else it's based on the type
    switch (pTile->type()) {
    case rs_eng::Tile::kNone:
    case rs_eng::Tile::kTrainStop:
        return SurfaceType::kEmpty;
    case rs_eng::Tile::kSlopeSN:
        return SurfaceType::kSlopeSN;
    case rs_eng::Tile::kSlopeNS:
        return SurfaceType::kSlopeNS;
    case rs_eng::Tile::kSlopeEW:
        return SurfaceType::kSlopeEW;
    case rs_eng::Tile::kSlopeWE:
        return SurfaceType::kSlopeWE;
    case rs_eng::Tile::kGround:
    case rs_eng::Tile::kRoof:
    case rs_eng::Tile::kRoadPedCross:
        return SurfaceType::kFloor;
    case rs_eng::Tile::kRoadSideEW:
    case rs_eng::Tile::kRoadSideWE:
    case rs_eng::Tile::kRoadSideSN:
    case rs_eng::Tile::kRoadSideNS:
    case rs_eng::Tile::kRoadCurve:
    case rs_eng::Tile::kRoadMark:
        return SurfaceType::kRoad;
    case rs_eng::Tile::kWall:
        return SurfaceType::kNonWalkable;
    case rs_eng::Tile::kHandrailLight:
        return SurfaceType::kHandrailLight;
    default:
        LOG(Log::k_FLG_GAME, "Map", "getSurfaceTypeForTile", ("Unknown tileType at %d, %d, %d", tx, ty, tz));
        return SurfaceType::kUnknown;
    }
}

/**
 * Return true if tile at given position is traversable by a car.
 * \param x int X coordinate
 * \param y int Y coordinate
 * \param z int Z coordinate
 * \return bool
 *
 */
bool Map::isTileWalkableByCar(int x, int y, int z)
{
    rs_eng::Tile *pTile = getTileAt(x, y, z);
    int tileId = pTile->id();

    if(tileId == rs_eng::Tile::kTileIdLargeDoorRailEW) {
        rs_eng::Tile::EType near_type = getTileAt(x, y - 1, z)->type();
        if((near_type < rs_eng::Tile::kRoadSideEW || near_type > rs_eng::Tile::kRoadSideNS)
            && near_type != rs_eng::Tile::kRoadPedCross) {
            return false;
        }
        near_type = getTileAt(x, y + 1, z)->type();
         if((near_type < rs_eng::Tile::kRoadSideEW || near_type > rs_eng::Tile::kRoadSideNS)
             && near_type != rs_eng::Tile::kRoadPedCross)
         {
            return false;
         }
        return true;
    }
    if(tileId == rs_eng::Tile::kTileIdLargeDoorRailNS) {
        rs_eng::Tile::EType near_type = getTileAt(x - 1, y, z)->type();
         if((near_type < rs_eng::Tile::kRoadSideEW || near_type > rs_eng::Tile::kRoadSideNS)
             && near_type != rs_eng::Tile::kRoadPedCross)
         {
            return false;
         }
        near_type = getTileAt(x + 1, y, z)->type();
         if((near_type < rs_eng::Tile::kRoadSideEW || near_type > rs_eng::Tile::kRoadSideNS)
             && near_type != rs_eng::Tile::kRoadPedCross)
         {
            return false;
         }
        return true;
    }

    if(tileId == 119) {
        return false;
    }
    return  pTile->isRoad();
}

/*!
 * Return a bifmask that tells which directions can be taken going out of this tile.
 * When a bit is set to 1, it means this direction is possible.
 * Bitmask is : 
 * - bit 4 : North
 * - bit 3 : South
 * - bit 2 : East
 * - bit 1 : West
 * @param tilePt 
 * @return A bitmask
 */
uint8_t Map::getPossibleConnexionsForRoadTile(const TilePoint &tilePt) {
    uint8_t possibleConnexions = 0;
    uint8_t fromConnexions = getTileAt(tilePt)->getEdgeConnexionsForRoadTile();
    
    if (rs_utl::isBitsOnWithMask(fromConnexions, kConnexionMaskExitNorth)) {
        uint8_t toConnexions = getTileAt(tilePt.tx, tilePt.ty - 1, tilePt.tz)->getEdgeConnexionsForRoadTile();
        if (rs_utl::isBitsOnWithMask(toConnexions, kConnexionMaskEntrySouth)) {
            possibleConnexions |= kConnexionMaskExitNorth;
        }
    }

    if (rs_utl::isBitsOnWithMask(fromConnexions, kConnexionMaskExitSouth)) {
        uint8_t toConnexions = getTileAt(tilePt.tx, tilePt.ty + 1, tilePt.tz)->getEdgeConnexionsForRoadTile();
        if (rs_utl::isBitsOnWithMask(toConnexions, kConnexionMaskEntryNorth)) {
            possibleConnexions |= kConnexionMaskExitSouth;
        }
    }

    if (rs_utl::isBitsOnWithMask(fromConnexions, kConnexionMaskExitEast)) {
        uint8_t toConnexions = getTileAt(tilePt.tx + 1, tilePt.ty, tilePt.tz)->getEdgeConnexionsForRoadTile();
        if (rs_utl::isBitsOnWithMask(toConnexions, kConnexionMaskEntryWest)) {
            possibleConnexions |= kConnexionMaskExitEast;
        }
    }

    if (rs_utl::isBitsOnWithMask(fromConnexions, Map::kConnexionMaskExitWest)) {
        uint8_t toConnexions = getTileAt(tilePt.tx - 1, tilePt.ty, tilePt.tz)->getEdgeConnexionsForRoadTile();
        if (rs_utl::isBitsOnWithMask(toConnexions, kConnexionMaskEntryEast)) {
            possibleConnexions |= kConnexionMaskExitWest;
        }
    }

    return possibleConnexions;
}

/*!
 * This method checks that the given point points to a road type of tile.
 * If it points to some kind of tiles (for example, road separator), depending
 * on where the point is, it moves the given point to a "real" road tile.
 * @param tilePt The point to check and change if necessary
 * @return True if the tile is a road tile
 */
bool Map::adjustClickOnRoad(TilePoint &tilePt) {
    // Road tiles sit one level below the vehicle's visual Z coordinate.
    // All pathfinding is done at (pos_.tz - 1);
    rs_eng::Tile *pTile = (tilePt.tz > 0) ? getTileAt(tilePt.tx, tilePt.ty, tilePt.tz - 1) : nullptr;
    if (!pTile || (!pTile->isRoad() && !pTile->isPedCrossing() && !pTile->isRoadMark())) {
        // Check if tilePt.tz itself was passed as the road tile level
        rs_eng::Tile *pDirectTile = getTileAt(tilePt.tx, tilePt.ty, tilePt.tz);
        if (pDirectTile && (pDirectTile->isRoad() || pDirectTile->isPedCrossing() || pDirectTile->isRoadMark())) {
            pTile = pDirectTile;
            tilePt.tz = tilePt.tz + 1;
        }
    }

    if (!pTile) {
        return false;
    }

    if (pTile->isRoad()) {
        return true;
    } else if (pTile->isPedCrossing()) {
        return pTile->id() != rs_eng::Tile::kTileIdPedCrossManhole;
    } else if (pTile->isRoadMark()) {
        if (pTile->id() == rs_eng::Tile::kTileIdRoadMarkSeparatorNS ||
            pTile->id() == rs_eng::Tile::kTileIdRoadMarkSeparatorEndNS1 ||
            pTile->id() == rs_eng::Tile::kTileIdRoadMarkSeparatorEndNS2) {
            if (tilePt.ox < 60) { // We hit the North to south portion of the tile
                // So offset destination from one tile
                tilePt.tx -= 1;
                return true;
            } else if (tilePt.ox > 180) { // We hit the South to North portion of the tile
                tilePt.tx += 1;
                return true;
            }
        } else if (pTile->id() == rs_eng::Tile::kTileIdRoadMarkSeparatorEW ||
                    pTile->id() == rs_eng::Tile::kTileIdRoadMarkSeparatorEndEW1 ||
                    pTile->id() == rs_eng::Tile::kTileIdRoadMarkSeparatorEndEW2) {
            if (tilePt.oy < 60) { // We hit the East to West portion of the tile
                tilePt.ty -= 1;
                return true;
            } else if (tilePt.oy > 180) { // We hit the West to East portion of the tile
                tilePt.ty += 1;
                return true;
            }
        }
    }

    return false;
}

const uint8_t MiniMap::kOverlayNone = 0;
const uint8_t MiniMap::kOverlayOurAgent = 1;
const uint8_t MiniMap::kOverlayEnemyAgent = 2;

/*!
 * Construct the minimap.
 */
MiniMap::MiniMap() :
    a_minimap_(nullptr),
    mmax_x_(0), mmax_y_(0),
    p_target_(nullptr) {}

MiniMap::~MiniMap() {
    if (a_minimap_) {
        free(a_minimap_);
        a_minimap_ = nullptr;
    }
}

bool MiniMap::init(Map *p_map) {
    // walkdata based colours
    uint8_t minimap_colours[] = {
        8,  7,  7,  7,
        7,  7, 10, 10,
       10, 10,  0, 10,
       15, 15, 10, 10,
       0,   0,  0,  0,
    };

    mmax_x_ = p_map->maxTx();
    mmax_y_ = p_map->maxTy();

    a_minimap_ = (uint8_t *)( malloc(mmax_x_ * mmax_y_) );
    if(a_minimap_ == NULL) {
        RSERR(Log::k_FLG_MEM, "MiniMap", "MiniMap", ("memory allocation failed"));
        return false;
    }
    for (unsigned short y = 0; y < mmax_y_; y++) {
        unsigned short yadd = y * mmax_x_;
        for (unsigned short x = 0; x < mmax_x_; x++) {
            rs_eng::Tile::EType type = p_map->getTileAt(x, y, 0)->type();
            a_minimap_[x + yadd] = minimap_colours[type];
        }
    }

    return true;
}

/*!
 * Return the color at the given point.
 * \param x
 * \param y
 */
uint8_t MiniMap::getColourAt(int x, int y) {

    if (x < 0 || y < 0 || x >= mmax_x_ || y >= mmax_y_)
        return 0;

    if (a_minimap_ != 0)
        return a_minimap_[x + y * mmax_x_];
    return 0;
}

//! Defines a source on the minimap for the signal
void MiniMap::setTarget(MapObject *pTarget) {
    p_target_ = pTarget;
}
//! Clear the target source
void MiniMap::clearTarget() {
    p_target_ = NULL;
}

}