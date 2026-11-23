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
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static std::filesystem::path getExecutableDirectory() {
    char buf[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, buf, MAX_PATH);
    if (len > 0) {
        return std::filesystem::path(buf).parent_path();
    }
    return std::filesystem::current_path();
}

#include "testcase.h"
#include "rs-utils/io/file.h"

std::string getTestDataFolder() {
    // 1. Check environment variable override
    const char *envDir = std::getenv("RESYND_DATA_DIR");
    if (envDir && std::filesystem::exists(envDir)) {
        return std::filesystem::path(envDir).lexically_normal().string();
    }

    // 2. Search upwards from current working directory
    std::filesystem::path cur = std::filesystem::current_path();
    for (int i = 0; i < 6; ++i) {
        if (std::filesystem::exists(cur / "synd" / "SYNDICAT" / "DATA" / "GAME01.DAT") ||
            std::filesystem::exists(cur / "synd" / "SYNDICAT" / "DATA" / "game01.dat")) {
            return (cur / "synd" / "SYNDICAT" / "DATA").lexically_normal().string();
        }
        if (std::filesystem::exists(cur / "synd" / "DATADISK" / "DATA" / "GAME01.DAT") ||
            std::filesystem::exists(cur / "synd" / "DATADISK" / "DATA" / "game01.dat")) {
            return (cur / "synd" / "DATADISK" / "DATA").lexically_normal().string();
        }
        if (std::filesystem::exists(cur / "synd" / "data" / "GAME01.DAT") ||
            std::filesystem::exists(cur / "synd" / "data" / "game01.dat")) {
            return (cur / "synd" / "data").lexically_normal().string();
        }
        if (std::filesystem::exists(cur / "data" / "GAME01.DAT") ||
            std::filesystem::exists(cur / "data" / "game01.dat")) {
            return (cur / "data").lexically_normal().string();
        }
        if (!cur.has_parent_path() || cur == cur.parent_path()) break;
        cur = cur.parent_path();
    }

    // 3. Search upwards from executable directory
    std::filesystem::path exeP = getExecutableDirectory();
    for (int i = 0; i < 6; ++i) {
        if (std::filesystem::exists(exeP / "synd" / "SYNDICAT" / "DATA" / "GAME01.DAT") ||
            std::filesystem::exists(exeP / "synd" / "SYNDICAT" / "DATA" / "game01.dat")) {
            return (exeP / "synd" / "SYNDICAT" / "DATA").lexically_normal().string();
        }
        if (std::filesystem::exists(exeP / "synd" / "DATADISK" / "DATA" / "GAME01.DAT") ||
            std::filesystem::exists(exeP / "synd" / "DATADISK" / "DATA" / "game01.dat")) {
            return (exeP / "synd" / "DATADISK" / "DATA").lexically_normal().string();
        }
        if (std::filesystem::exists(exeP / "synd" / "data" / "GAME01.DAT") ||
            std::filesystem::exists(exeP / "synd" / "data" / "game01.dat")) {
            return (exeP / "synd" / "data").lexically_normal().string();
        }
        if (std::filesystem::exists(exeP / "data" / "GAME01.DAT") ||
            std::filesystem::exists(exeP / "data" / "game01.dat")) {
            return (exeP / "data").lexically_normal().string();
        }
        if (!exeP.has_parent_path() || exeP == exeP.parent_path()) break;
        exeP = exeP.parent_path();
    }

    // 4. Check CMake compile definition
#ifdef GAME_DATA_DIR
    if (std::filesystem::exists(GAME_DATA_DIR)) {
        return std::filesystem::path(GAME_DATA_DIR).lexically_normal().string();
    }
#endif

    return "data";
}

void initTestDataFolders() {
    std::string dataDir = getTestDataFolder();
    rs_utl::File::setOriginalDataFolder(dataDir);
    rs_utl::File::setResyndDataFolder("");
}

void initWeaponConfigFile( ConfigFile &config ) {
    config.add("weapon.1.name", "WEAPON_PISTOL");
    config.add("weapon.1.icon.small", 15);
    config.add("weapon.1.icon.big", 65);
    config.add("weapon.1.cost", 0);
    config.add("weapon.1.ammo.nb", 13);
    config.add("weapon.1.ammo.price", 0);
    config.add("weapon.1.range", 1280);
    config.add("weapon.1.rank", 1);
    config.add("weapon.1.anim", 368);
    config.add("weapon.1.ammopershot", 1);
    config.add("weapon.1.reloadtime", 600);
    config.add("weapon.1.damagerange", 0);
    config.add("weapon.1.shotanglemin", 1.0);
    config.add("weapon.1.shotanglemax", 16.0);
    config.add("weapon.1.shotdepthmin", 0.0);
    config.add("weapon.1.shotdepthmax", 16.0);
    config.add("weapon.1.shotspeed", 0);
    config.add("weapon.1.dmg_per_shot", 1);
    config.add("weapon.1.ammo.impactNb", 1);
    config.add("weapon.1.weight", 20);

    config.add("weapon.2.name", "GAUSS_GUN");
    config.add("weapon.2.icon.small", 15);
    config.add("weapon.2.icon.big", 65);
    config.add("weapon.2.cost", 0);
    config.add("weapon.2.ammo.nb", 13);
    config.add("weapon.2.ammo.price", 0);
    config.add("weapon.2.range", 1280);
    config.add("weapon.2.rank", 0);
    config.add("weapon.2.anim", 368);
    config.add("weapon.2.ammopershot", 1);
    config.add("weapon.2.reloadtime", 600);
    config.add("weapon.2.damagerange", 0);
    config.add("weapon.2.shotanglemin", 0.5);
    config.add("weapon.2.shotanglemax", 1.0);
    config.add("weapon.2.shotdepthmin", 0.0);
    config.add("weapon.2.shotdepthmax", 0.0);
    config.add("weapon.2.shotspeed", 0);
    config.add("weapon.2.dmg_per_shot", 2);
    config.add("weapon.2.ammo.impactNb", 1);
    config.add("weapon.2.weight", 3000);

    config.add("weapon.12.name", "scanner");
    config.add("weapon.12.icon.small", 26);
    config.add("weapon.12.icon.big", 76);
    config.add("weapon.12.cost", 1000);
    config.add("weapon.12.range", 256);
    config.add("weapon.12.anim", 379);
    config.add("weapon.12.reloadtime", 1);
    config.add("weapon.12.weight", 20);

    config.add("weapon.3.name", "WEAPON_SHOTGUN");
    config.add("weapon.3.icon.small", 17);
    config.add("weapon.3.icon.big", 67);
    config.add("weapon.3.cost", 250);
    config.add("weapon.3.ammo.nb", 12);
    config.add("weapon.3.ammo.price", 2);
    config.add("weapon.3.range", 1024);
    config.add("weapon.3.rank", 2);
    config.add("weapon.3.anim", 370);
    config.add("weapon.3.ammopershot", 1);
    config.add("weapon.3.reloadtime", 200);
    config.add("weapon.3.damagerange", 0);
    config.add("weapon.3.shotanglemin", 15.0);
    config.add("weapon.3.shotanglemax", 40.0);
    config.add("weapon.3.shotdepthmin", 0.0);
    config.add("weapon.3.shotdepthmax", 32.0);
    config.add("weapon.3.shotspeed", 0);
    config.add("weapon.3.dmg_per_shot", 1);
    config.add("weapon.3.ammo.impactNb", 7);
    config.add("weapon.3.weight", 90);

    config.add("weapon.4.name", "WEAPON_UZI");
    config.add("weapon.4.icon.small", 18);
    config.add("weapon.4.icon.big", 68);
    config.add("weapon.4.cost", 750);
    config.add("weapon.4.ammo.nb", 50);
    config.add("weapon.4.ammo.price", 5);
    config.add("weapon.4.range", 1536);
    config.add("weapon.4.rank", 5);
    config.add("weapon.4.anim", 371);
    config.add("weapon.4.ammopershot", 1);
    config.add("weapon.4.auto.fire_rate", 100);
    config.add("weapon.4.damagerange", 0);
    config.add("weapon.4.shotanglemin", 1.0);
    config.add("weapon.4.shotanglemax", 14.0);
    config.add("weapon.4.shotdepthmin", 0.0);
    config.add("weapon.4.shotdepthmax", 16.0);
    config.add("weapon.4.shotspeed", 0);
    config.add("weapon.4.dmg_per_shot", 1);
    config.add("weapon.4.ammo.impactNb", 1);
    config.add("weapon.4.weight", 150);

    config.add("weapon.5.name", "WEAPON_MINIGUN");
    config.add("weapon.5.icon.small", 19);
    config.add("weapon.5.icon.big", 69);
    config.add("weapon.5.cost", 10000);
    config.add("weapon.5.ammo.nb", 500);
    config.add("weapon.5.ammo.price", 25);
    config.add("weapon.5.range", 2816);
    config.add("weapon.5.rank", 6);
    config.add("weapon.5.anim", 372);
    config.add("weapon.5.ammopershot", 1);
    config.add("weapon.5.auto.fire_rate", 40);
    config.add("weapon.5.damagerange", 0);
    config.add("weapon.5.shotanglemin", 1.0);
    config.add("weapon.5.shotanglemax", 20.0);
    config.add("weapon.5.shotdepthmin", 0.0);
    config.add("weapon.5.shotdepthmax", 16.0);
    config.add("weapon.5.shotspeed", 0);
    config.add("weapon.5.dmg_per_shot", 1);
    config.add("weapon.5.ammo.impactNb", 5);
    config.add("weapon.5.weight", 3000);

    config.add("weapon.6.name", "WEAPON_LASER");
    config.add("weapon.6.icon.small", 20);
    config.add("weapon.6.icon.big", 70);
    config.add("weapon.6.cost", 35000);
    config.add("weapon.6.ammo.nb", 5);
    config.add("weapon.6.ammo.price", 1000);
    config.add("weapon.6.range", 4096);
    config.add("weapon.6.rank", 7);
    config.add("weapon.6.anim", 373);
    config.add("weapon.6.ammopershot", 1);
    config.add("weapon.6.reloadtime", 800);
    config.add("weapon.6.damagerange", 0);
    config.add("weapon.6.shotanglemin", 0.5);
    config.add("weapon.6.shotanglemax", 1.0);
    config.add("weapon.6.shotdepthmin", 0.0);
    config.add("weapon.6.shotdepthmax", 0.0);
    config.add("weapon.6.shotspeed", 0);
    config.add("weapon.6.dmg_per_shot", 500);
    config.add("weapon.6.ammo.impactNb", 1);
    config.add("weapon.6.weight", 2500);

    config.add("weapon.7.name", "WEAPON_FLAMER");
    config.add("weapon.7.icon.small", 21);
    config.add("weapon.7.icon.big", 71);
    config.add("weapon.7.cost", 1500);
    config.add("weapon.7.ammo.nb", 1000);
    config.add("weapon.7.ammo.price", 10);
    config.add("weapon.7.range", 512);
    config.add("weapon.7.rank", 4);
    config.add("weapon.7.anim", 374);
    config.add("weapon.7.ammopershot", 1);
    config.add("weapon.7.auto.fire_rate", 50);
    config.add("weapon.7.damagerange", 0);
    config.add("weapon.7.shotanglemin", 10.0);
    config.add("weapon.7.shotanglemax", 20.0);
    config.add("weapon.7.shotdepthmin", 0.0);
    config.add("weapon.7.shotdepthmax", 0.0);
    config.add("weapon.7.shotspeed", 2400);
    config.add("weapon.7.dmg_per_shot", 2);
    config.add("weapon.7.ammo.impactNb", 1);
    config.add("weapon.7.weight", 3500);

    config.add("weapon.8.name", "WEAPON_LONG_RANGE");
    config.add("weapon.8.icon.small", 22);
    config.add("weapon.8.icon.big", 72);
    config.add("weapon.8.cost", 1000);
    config.add("weapon.8.ammo.nb", 30);
    config.add("weapon.8.ammo.price", 2);
    config.add("weapon.8.range", 6144);
    config.add("weapon.8.rank", 3);
    config.add("weapon.8.anim", 375);
    config.add("weapon.8.ammopershot", 1);
    config.add("weapon.8.reloadtime", 1000);
    config.add("weapon.8.damagerange", 0);
    config.add("weapon.8.shotanglemin", 0.5);
    config.add("weapon.8.shotanglemax", 14.0);
    config.add("weapon.8.shotdepthmin", 0.0);
    config.add("weapon.8.shotdepthmax", 16.0);
    config.add("weapon.8.shotspeed", 0);
    config.add("weapon.8.dmg_per_shot", 1);
    config.add("weapon.8.ammo.impactNb", 1);
    config.add("weapon.8.weight", 100);

    config.add("weapon.10.name", "WEAPON_MEDIKIT");
    config.add("weapon.10.icon.small", 24);
    config.add("weapon.10.icon.big", 74);
    config.add("weapon.10.cost", 2000);
    config.add("weapon.10.ammo.nb", 1);
    config.add("weapon.10.ammo.price", 0);
    config.add("weapon.10.range", 0);
    config.add("weapon.10.anim", 377);
    config.add("weapon.10.reloadtime", 1);
    config.add("weapon.10.weight", 50);

    config.add("weapon.11.name", "WEAPON_TIME_BOMB");
    config.add("weapon.11.icon.small", 25);
    config.add("weapon.11.icon.big", 75);
    config.add("weapon.11.cost", 25000);
    config.add("weapon.11.ammo.nb", 1);
    config.add("weapon.11.ammo.price", 1);
    config.add("weapon.11.range", 0);
    config.add("weapon.11.anim", 378);
    config.add("weapon.11.bomb.explosiondelay", 20000);
    config.add("weapon.11.damagerange", 512);
    config.add("weapon.11.dmg_per_shot", 64);
    config.add("weapon.11.reloadtime", 1);
    config.add("weapon.11.weight", 500);

    config.add("weapon.14.name", "WEAPON_PERSUADATRON");
    config.add("weapon.14.icon.small", 29);
    config.add("weapon.14.icon.big", 79);
    config.add("weapon.14.cost", 5000);
    config.add("weapon.14.range", 1024);
    config.add("weapon.14.anim", 382);
    config.add("weapon.14.reloadtime", 1);
    config.add("weapon.14.weight", 100);

    config.add("weapon.9.name", "WEAPON_SCANNER");
    config.add("weapon.9.icon.small", 23);
    config.add("weapon.9.icon.big", 73);
    config.add("weapon.9.cost", 500);
    config.add("weapon.9.range", 4096);
    config.add("weapon.9.anim", 376);
    config.add("weapon.9.weight", 30);

    config.add("weapon.13.name", "WEAPON_ENERGY_SHIELD");
    config.add("weapon.13.icon.small", 28);
    config.add("weapon.13.icon.big", 78);
    config.add("weapon.13.cost", 8000);
    config.add("weapon.13.ammo.nb", 200);
    config.add("weapon.13.ammo.price", 15);
    config.add("weapon.13.range", 768);
    config.add("weapon.13.anim", 381);
    config.add("weapon.13.ammopershot", 1);
    config.add("weapon.13.auto.fire_rate", 75);
    config.add("weapon.13.ammo.impactNb", 1);
    config.add("weapon.13.weight", 180);

    config.add("weapon.15.name", "WEAPON_AIR_STRIKE");
    config.add("weapon.15.icon.small", 27);
    config.add("weapon.15.icon.big", 1180);
    config.add("weapon.15.cost", 5000);
    config.add("weapon.15.ammo.nb", 1);
    config.add("weapon.15.ammo.price", 50000);
    config.add("weapon.15.range", 5120);
    config.add("weapon.15.rank", 8);
    config.add("weapon.15.anim", 378);
    config.add("weapon.15.ammopershot", 1);
    config.add("weapon.15.timeforshot", 250);
    config.add("weapon.15.reloadtime", 5000);
    config.add("weapon.15.damagerange", 1024);
    config.add("weapon.15.shotanglemin", 0.0);
    config.add("weapon.15.shotanglemax", 0.0);
    config.add("weapon.15.shotdepthmin", 0.0);
    config.add("weapon.15.shotdepthmax", 0.0);
    config.add("weapon.15.shotspeed", 0);
    config.add("weapon.15.dmg_per_shot", 128);
    config.add("weapon.15.ammo.impactNb", 1);
    config.add("weapon.15.weight", 100);

    config.add("weapon.16.name", "WEAPON_CLONE_SHIELD");
    config.add("weapon.16.icon.small", 29);
    config.add("weapon.16.icon.big", 83);
    config.add("weapon.16.cost", 9000);
    config.add("weapon.16.ammo.nb", 200);
    config.add("weapon.16.ammo.price", 15);
    config.add("weapon.16.range", 768);
    config.add("weapon.16.rank", 0);
    config.add("weapon.16.anim", 381);
    config.add("weapon.16.ammopershot", 1);
    config.add("weapon.16.timeforshot", 1);
    config.add("weapon.16.reloadtime", 100);
    config.add("weapon.16.damagerange", 0);
    config.add("weapon.16.shotanglemin", 0.0);
    config.add("weapon.16.shotanglemax", 0.0);
    config.add("weapon.16.shotdepthmin", 0.0);
    config.add("weapon.16.shotdepthmax", 0.0);
    config.add("weapon.16.shotspeed", 0);
    config.add("weapon.16.dmg_per_shot", 0);
    config.add("weapon.16.ammo.impactNb", 1);
    config.add("weapon.16.weight", 180);
    config.add("weapon.16.auto.fire_rate", 180);
}

bool loadMapFromCsv(const std::string &filepath,
                    rs_eng::TileManager &tileMgr,
                    rs_knl::Map &map) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    int maxX = 0, maxY = 0, maxZ = 0;
    std::string line;

    // Read dimensions from the first non-comment line
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        char sep;
        ss >> maxX >> sep >> maxY >> sep >> maxZ;
        break;
    }
    if (maxX <= 0 || maxY <= 0 || maxZ <= 0) return false;

    int total = maxX * maxY * maxZ;
    rs_eng::Tile **tiles = new rs_eng::Tile*[total];
    // Default every slot to the transparent tile
    for (int i = 0; i < total; i++) {
        tiles[i] = tileMgr.getTile(
            static_cast<uint8_t>(rs_eng::TileManager::kIndexTransparentTile));
    }

    int previousZ = -1;
    int y = 0;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        char sep;
        int z;
        ss >> z >> sep;
        y++;
        if (y < maxY) {
            if (z != previousZ) {
                previousZ = z;
                y = 0;
            }
            for (int x = 0; x < maxX; x++) {
                int tileId;
                ss >> tileId;
                if (x + 1 < maxX || y + 1 < maxY) ss >> sep; // consume comma
                tiles[(y * maxX + x) * maxZ + z] =
                    tileMgr.getTile(static_cast<uint8_t>(tileId));
            }
        }
    }

    map.setTiles(maxX, maxY, maxZ, tiles);
    delete[] tiles;
    return true;
}

MemoryTileManager::MemoryTileManager() : rs_eng::TileManager() {
    for (int i = 0; i < 256; ++i) {
        tiles_[i] = new rs_eng::Tile(i, false, rs_eng::Tile::kNone, {0, 0});
    }
    // Road tile (kRoadNtoS id, kRoadSideEW type — isRoad() == true)
    delete tiles_[106];
    tiles_[106] = new rs_eng::Tile(106, false, rs_eng::Tile::kRoadSideEW,  {0, 0});
    // Pedestrian crossing tiles (kRoadPedCross type — isRoad() and isPedCrossing() both true)
    delete tiles_[225];
    tiles_[225] = new rs_eng::Tile(225, false, rs_eng::Tile::kRoadPedCross, {0, 0});
    delete tiles_[226];
    tiles_[226] = new rs_eng::Tile(226, false, rs_eng::Tile::kRoadPedCross, {0, 0});
    // Road mark separator tiles (kRoadMark type — isRoadMark() == true, isRoad() == false)
    delete tiles_[100];
    tiles_[100] = new rs_eng::Tile(100, false, rs_eng::Tile::kRoadMark,    {0, 0});
    delete tiles_[101];
    tiles_[101] = new rs_eng::Tile(101, false, rs_eng::Tile::kRoadMark,    {0, 0});
    delete tiles_[114];
    tiles_[114] = new rs_eng::Tile(114, false, rs_eng::Tile::kRoadMark,    {0, 0});
    delete tiles_[115];
    tiles_[115] = new rs_eng::Tile(115, false, rs_eng::Tile::kRoadMark,    {0, 0});
    delete tiles_[116];
    tiles_[116] = new rs_eng::Tile(116, false, rs_eng::Tile::kRoadMark,    {0, 0});
    delete tiles_[117];
    tiles_[117] = new rs_eng::Tile(117, false, rs_eng::Tile::kRoadMark,    {0, 0});
}

TEST_CASE( "1: All test cases reside in other .cpp files (empty)", "[multi-file:1]" ) {
}