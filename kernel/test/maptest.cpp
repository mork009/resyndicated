/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2013, 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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
#include <catch2/catch_test_macros.hpp>

#include "testcase.h"
#include "rs-utils/io/dernc.h"

#include "rs-kernel/model/map.h"
#include "rs-kernel/mgr/mapmanager.h"
#include "rs-engine/gfx/tilemanager.h"
#include "rs-utils/io/file.h"


TEST_CASE( "Map", "[kernel][map]" ) {
    rs_eng::TileManager tileMgr;
    rs_knl::Map map(&tileMgr, 0);

    map.setTiles(4, 5, 4, nullptr);

    SECTION( "mapDimensions") {
        SECTION( "should return map dimensions") {
            int maxTx, maxTy, maxTz;

            map.mapDimensions(&maxTx, &maxTy, &maxTz);

            REQUIRE( maxTx == 4);
            REQUIRE( maxTy == 5);
            REQUIRE( maxTz == 4);
        }
    }

    SECTION("tile diamond center and projection roundtrip") {
        int tx = 60, ty = 53, tz = 1;
        int maxTx = 128, maxTy = 96, maxTz = 13;
        rs_eng::TileManager tm;
        rs_knl::Map m(&tm, 0);
        m.setTiles(maxTx, maxTy, maxTz, nullptr);

        int screen_w = (maxTx + (tx - ty)) * 32;
        int coord_h = ((maxTz + tx + ty) - (tz - 1)) * 16;

        Point2D p;
        m.tileToScreenPoint(rs_knl::TilePoint(tx, ty, tz), &p);
        int worldX = p.x;
        int worldY = p.y - tz * 16;

        // worldX, worldY must be the exact 2D center of the tile's top diamond
        REQUIRE(worldX == screen_w + 32);
        REQUIRE(worldY == coord_h + 16);

        // Inverting screen coordinate back to map tile point at tz
        rs_knl::TilePoint tp = m.screenToTilePoint(worldX, worldY + tz * 16);
        REQUIRE(tp.tx == tx);
        REQUIRE(tp.ty == ty);
    }

    SECTION("entity screen anchor vs tile diamond center projection") {
        int tx = 60, ty = 53, tz = 1;
        int maxTx = 128, maxTy = 96, maxTz = 13;
        rs_eng::TileManager tm;
        rs_knl::Map m(&tm, 0);
        m.setTiles(maxTx, maxTy, maxTz, nullptr);

        int coord_h = ((maxTz + tx + ty) - (tz - 1)) * 16;

        Point2D p;
        m.tileToScreenPoint(rs_knl::TilePoint(tx, ty, tz), &p);

        // Tile top diamond center: worldY = p.y - tz * 16
        int tileTopDiamondY = p.y - tz * 16;
        REQUIRE(tileTopDiamondY == coord_h + 16);

        // Entity anchor base: worldY = p.y - tz * 16 + 16 (matching MapRenderer coord_h + 32)
        int entityAnchorY = p.y - tz * 16 + 16;
        REQUIRE(entityAnchorY == coord_h + 32);
        REQUIRE(entityAnchorY - tileTopDiamondY == 16);
    }
}

TEST_CASE( "Map::adjustClickOnRoad", "[kernel][map]" ) {
    MemoryTileManager tileMgr;
    rs_knl::Map map(&tileMgr, 1);
    // 5x5 map, 4 Z levels. All tiles default to kNone (index 0).
    REQUIRE( loadMapFromCsv(TEST_DATA_DIR "/map-5x5x4.csv", tileMgr, map) );

    // The method inspects the tile at (tx, ty, tz - 1).
    // We use tz=1 so the inspected Z is 0.
    const int tz = 1;

    SECTION( "returns false for a non-road tile" ) {
        // Default tile is kNone — not a road, not a ped crossing, not a road mark.
        rs_knl::TilePoint tilePt(1, 3, 3);
        REQUIRE_FALSE( map.adjustClickOnRoad(tilePt) );
        REQUIRE( tilePt.tx == 1 );
        REQUIRE( tilePt.ty == 3 );
    }

    SECTION( "returns true for a road tile without modifying position" ) {
        rs_knl::TilePoint tilePt(2, 2, tz);
        REQUIRE( map.adjustClickOnRoad(tilePt) );
        REQUIRE( tilePt.tx == 2 );
        REQUIRE( tilePt.ty == 2 );
    }

    SECTION( "pedestrian crossing" ) {
        SECTION( "returns true for kTileIdPedCrossNS" ) {
            rs_knl::TilePoint tilePt(2, 1, tz);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
        }

        SECTION( "returns true for kTileIdPedCrossEW" ) {
            rs_knl::TilePoint tilePt(1, 3, tz);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
        }
    }

    SECTION( "road mark separator NS (id 100)" ) {
        
        SECTION( "ox < 60: tx is decremented and returns true" ) {
            rs_knl::TilePoint tilePt(3, 2, tz, 30, 128, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.tx == 2 );
        }

        SECTION( "ox > 180: tx is incremented and returns true" ) {
            rs_knl::TilePoint tilePt(3, 2, tz, 200, 128, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.tx == 4 );
        }

        SECTION( "ox in [60, 180]: position unchanged and returns false" ) {
            rs_knl::TilePoint tilePt(3, 2, tz, 120, 128, 0);
            REQUIRE_FALSE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.tx == 3 );
        }
    }

    SECTION( "road mark separator end NS1 (id 114)" ) {
        
        SECTION( "ox < 60: tx is decremented and returns true" ) {
            rs_knl::TilePoint tilePt(3, 4, tz, 30, 128, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.tx == 2 );
        }

        SECTION( "ox > 180: tx is incremented and returns true" ) {
            rs_knl::TilePoint tilePt(3, 4, tz, 200, 128, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.tx == 4 );
        }
    }

    SECTION( "road mark separator end NS2 (id 116)" ) {
        
        SECTION( "ox < 60: tx is decremented and returns true" ) {
            rs_knl::TilePoint tilePt(1, 4, tz, 30, 128, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.tx == 0 );
        }

        SECTION( "ox > 180: tx is incremented and returns true" ) {
            rs_knl::TilePoint tilePt(1, 4, tz, 200, 128, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.tx == 2 );
        }
    }

    SECTION( "road mark separator EW (id 101)" ) {
        
        SECTION( "oy < 60: ty is decremented and returns true" ) {
            rs_knl::TilePoint tilePt(1, 2, tz, 128, 30, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.ty == 1 );
        }

        SECTION( "oy > 180: ty is incremented and returns true" ) {
            rs_knl::TilePoint tilePt(1, 2, tz, 128, 200, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.ty == 3 );
        }

        SECTION( "oy in [60, 180]: position unchanged and returns false" ) {
            rs_knl::TilePoint tilePt(1, 2, tz, 128, 120, 0);
            REQUIRE_FALSE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.ty == 2 );
        }
    }

    SECTION( "road mark separator end EW1 (id 115)" ) {
        
        SECTION( "oy < 60: ty is decremented and returns true" ) {
            rs_knl::TilePoint tilePt(3, 1, tz, 128, 30, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.ty == 0 );
        }

        SECTION( "oy > 180: ty is incremented and returns true" ) {
            rs_knl::TilePoint tilePt(3, 1, tz, 128, 200, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.ty == 2 );
        }
    }

    SECTION( "road mark separator end EW2 (id 117)" ) {
        //map.patchMap(tx, ty, tz - 1, rs_eng::Tile::kTileIdRoadMarkSeparatorEndEW2);

        SECTION( "oy < 60: ty is decremented and returns true" ) {
            rs_knl::TilePoint tilePt(3, 3, tz, 128, 30, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.ty == 2 );
        }

        SECTION( "oy > 180: ty is incremented and returns true" ) {
            rs_knl::TilePoint tilePt(3, 3, tz, 128, 200, 0);
            REQUIRE( map.adjustClickOnRoad(tilePt) );
            REQUIRE( tilePt.ty == 4 );
        }
    }
}

TEST_CASE( "Map::setTileAt and exportBinary", "[kernel][map]" ) {
    MemoryTileManager tileMgr;
    rs_knl::Map map(&tileMgr, 1);
    REQUIRE( loadMapFromCsv(TEST_DATA_DIR "/map-5x5x4.csv", tileMgr, map) );

    SECTION( "setTileAt updates tile within bounds and rejects out of bounds" ) {
        REQUIRE( map.setTileAt(2, 2, 1, 42) );
        REQUIRE( map.getTileIdAt(2, 2, 1) == 42 );

        REQUIRE_FALSE( map.setTileAt(-1, 0, 0, 1) );
        REQUIRE_FALSE( map.setTileAt(10, 0, 0, 1) );
        REQUIRE_FALSE( map.setTileAt(0, 10, 0, 1) );
        REQUIRE_FALSE( map.setTileAt(0, 0, 10, 1) );
    }

    SECTION( "exportBinary writes valid map binary with deduplicated columns" ) {
        std::string tempFile = "temp_test_map.dat";
        REQUIRE( map.setTileAt(1, 1, 0, 10) );
        REQUIRE( map.exportBinary(tempFile) );

        std::ifstream in(tempFile, std::ios::binary);
        REQUIRE( in.is_open() );
        in.seekg(0, std::ios::end);
        size_t sz = in.tellg();
        in.seekg(0, std::ios::beg);
        std::vector<uint8_t> fileBytes(sz);
        in.read(reinterpret_cast<char*>(fileBytes.data()), sz);
        in.close();

        REQUIRE( sz > 18 );
        REQUIRE( rnc::isRncCompressed(fileBytes.data()) );

        size_t unpLen = 0;
        REQUIRE( rnc::unpackedLength(fileBytes.data(), unpLen) == rnc::kOk );
        // 5x5 map with maxZ=3 (from maxTz_=4) has 25 columns. Deduplication reduces 25 columns to 11 unique columns:
        // 12B header + 25*4 lookup table + 11*3 unique tile bytes = 145 bytes (instead of 187 bytes un-deduplicated).
        REQUIRE( unpLen == 145 );

        std::remove(tempFile.c_str());
    }

    SECTION( "exportBinary deduplication matches Bullfrog map01.dat size" ) {
        std::string mapPath = rs_utl::File::getOriginalDataFullPath("map01.dat");
        std::ifstream mapIn(mapPath, std::ios::binary);
        if (mapIn.is_open()) {
            mapIn.close();
            LevelData::MapInfos mapInfo{};
            rs_utl::WRITE_LE_UINT16(mapInfo.map, 1);
            rs_knl::MapManager mapMgr(&tileMgr);
            rs_knl::Map *pMap = mapMgr.getMap(mapInfo);
            if (pMap != nullptr) {
                std::string tempExport = "temp_exported_map01.dat";
                REQUIRE( pMap->exportBinary(tempExport) );

                std::ifstream expIn(tempExport, std::ios::binary);
                REQUIRE( expIn.is_open() );
                expIn.seekg(0, std::ios::end);
                size_t expSz = expIn.tellg();
                expIn.seekg(0, std::ios::beg);
                std::vector<uint8_t> expBytes(expSz);
                expIn.read(reinterpret_cast<char*>(expBytes.data()), expSz);
                expIn.close();

                REQUIRE( rnc::isRncCompressed(expBytes.data()) );
                size_t unpExpLen = 0;
                REQUIRE( rnc::unpackedLength(expBytes.data(), unpExpLen) == rnc::kOk );
                // Bullfrog's original uncompressed size for map01.dat is 52632 bytes!
                REQUIRE( unpExpLen == 52632 );
                // Compressed size should be around 5400-5600 bytes, NOT ~24600 bytes!
                UNSCOPED_INFO("Exported map01 compressed size: " << expSz << " bytes vs original 5406 bytes");
                REQUIRE( expSz < 6000 );
                REQUIRE( expSz >= 5000 );

                std::remove(tempExport.c_str());
            }
        }
    }
}

TEST_CASE( "Map::setScrollLimits Applies Scroll Bounds", "[kernel][map]" ) {
    MemoryTileManager tileMgr;
    rs_knl::Map map(&tileMgr, 1);
    REQUIRE( loadMapFromCsv(TEST_DATA_DIR "/map-5x5x4.csv", tileMgr, map) );
    REQUIRE( map.maxTx() == 5 );
    REQUIRE( map.maxTy() == 5 );

    SECTION( "Scroll limits clamp out-of-bounds coordinates" ) {
        map.setScrollLimits({1, 1}, {3, 3});
        rs_knl::TilePoint tp(4, 4, 0);
        map.clipToScrollLimits(tp);
        CHECK( tp.tx == 3 );
        CHECK( tp.ty == 3 );

        rs_knl::TilePoint tpLow(0, 0, 0);
        map.clipToScrollLimits(tpLow);
        CHECK( tpLow.tx == 1 );
        CHECK( tpLow.ty == 1 );
    }

    SECTION( "Effective scroll limits on 5x5 map are clamped to map dimensions" ) {
        map.setScrollLimits({1, 1}, {3, 3});
        CHECK( map.getEffectiveMaxScrollTile(0.5).x <= 4 );
        CHECK( map.getEffectiveMaxScrollTile(0.5).y <= 4 );
        CHECK( map.getEffectiveMinScrollTile(0.5).x >= 0 );
        CHECK( map.getEffectiveMinScrollTile(0.5).y >= 0 );
    }
}

TEST_CASE( "Map::getEffectiveScrollLimits Scales With Zoom on Full Map", "[kernel][map]" ) {
    MemoryTileManager tileMgr;
    rs_knl::Map map(&tileMgr, 1);
    map.setTiles(128, 96, 12, nullptr);
    map.setScrollLimits({39, 21}, {75, 49});

    // At zoom 1.0, effective limits equal base limits
    CHECK( map.getEffectiveMinScrollTile(1.0).x == 39 );
    CHECK( map.getEffectiveMinScrollTile(1.0).y == 21 );
    CHECK( map.getEffectiveMaxScrollTile(1.0).x == 75 );
    CHECK( map.getEffectiveMaxScrollTile(1.0).y == 49 );

    // At zoom 0.5 (zoomed in), limits expand to allow panning to edge entities (e.g. Mission 1 agents at tx=90)
    CHECK( map.getEffectiveMaxScrollTile(0.5).x >= 90 );
    CHECK( map.getEffectiveMaxScrollTile(0.5).y > 49 );
    CHECK( map.getEffectiveMinScrollTile(0.5).x <= 39 );
    CHECK( map.getEffectiveMinScrollTile(0.5).y <= 21 );

    // At zoom 2.0 (zoomed out), effective maxX contracts proportionally to prevent panning
    // beyond edge entities into empty space, while minX/maxY allow reaching perimeter exits/gates
    CHECK( map.getEffectiveMaxScrollTile(2.0).x <= 55 );
    CHECK( map.getEffectiveMaxScrollTile(2.0).x >= 54 );
    CHECK( map.getEffectiveMaxScrollTile(2.0).y >= 49 );
    CHECK( map.getEffectiveMinScrollTile(2.0).x <= 35 );
    CHECK( map.getEffectiveMinScrollTile(2.0).y <= 21 );
}