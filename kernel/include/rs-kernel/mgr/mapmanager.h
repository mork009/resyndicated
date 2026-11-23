/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net>
 *   Copyright (C) 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef MAPMANAGER_H
#define MAPMANAGER_H

#include <map>
#include "rs-utils/common.h"
#include "rs-kernel/model/map.h"
#include "rs-kernel/model/leveldata.h"

namespace rs_knl {
/*!
 * Map manager class.
 */
class MapManager {
public:
    MapManager(rs_eng::TileManager *pTileManager);
    ~MapManager();

    //! Load the map with the given id if not in cache
    Map * getMap(const LevelData::MapInfos &mapInfo);
    //! Load a map directly from a specific file path
    Map * getMapFromPath(const std::string &mapFilePath, const LevelData::MapInfos &mapInfo);
    //! Clears the map cache so maps are reloaded from disk
    void clearCache();

    void setCustomDataDir(const std::string &dir) { customDataDir_ = dir; }
    const std::string& customDataDir() const { return customDataDir_; }

    //! Sets the palette for the given mission
    bool loadPalette(int paletteId);

protected:
    //! Create a new Map using the game data
    Map * createMap(uint16_t mapId, uint8_t * mapData, const LevelData::MapInfos &mapInfo);
    //! Use to correct some data after loading the map
    void patchMap(uint16_t mapId);

protected:
    //! a cache of the latest maps
    std::map<int, Map *> maps_;
    rs_eng::TileManager *pTileManager_;
    std::string customDataDir_;
};

}
#endif
