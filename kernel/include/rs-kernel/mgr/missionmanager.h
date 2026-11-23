/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
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

#ifndef MISSIONMANAGER_H
#define MISSIONMANAGER_H

#include <map>

#include "rs-utils/common.h"
#include "rs-utils/misc/singleton.h"
#include "rs-kernel/model/leveldata.h"
#include "rs-kernel/ia/actions.h"
#include "rs-kernel/mgr/mapmanager.h"
#include "rs-kernel/model/missionbriefing.h"
#include "rs-kernel/model/mission.h"

namespace rs_knl {
/*!
 * Mission manager class.
 * Stores information about all missions.
 */
class MissionManager : public Singleton < MissionManager > {
public:
    MissionManager(rs_eng::TileManager *pTileManager);
    //! Loads mission for the given mission id (paletteId <= 0 defaults to getMissionPaletteId(missionId))
    Mission *loadMission(int missionId, int paletteId = -1);
    //! Returns the original campaign palette ID (1..5) for a given mission ID (1..50), or 1 as default/fallback.
    static int getMissionPaletteId(int missionId);
    //! Loads briefing for the given mission id
    MissionBriefing *loadBriefing(int n);

    Mission * mission() {
        return pMission_;
    }

    void setMission(Mission *pMission) {
        pMission_ = pMission;
    }

    void destroyMission();

    //! Reads the mission file and return a representation of that file
    bool load_level_data(int n, LevelData::LevelDataAll &level_data, const std::string &customPath = "");
    //! Saves level data to binary file
    static bool save_level_data(int n, const LevelData::LevelDataAll &level_data, const std::string &customPath = "");
    //! Exports a mission state into binary gameXX.dat format
    static bool exportMissionToBinary(Mission *pMission, int missionId, const std::string &customPath = "");
    //! Inspects level data to detect if any entity, weapon, or asset depends on American Revolt
    static bool missionRequiresAmericanRevolt(const LevelData::LevelDataAll &levelData);
    //! Rebuilds a ped's default scripted actions and current action from its scriptSteps()
    static void buildScriptedActionsForPed(Mission *pMission, PedInstance *pPed);
    // Instanciate a mission from the data file
    Mission * create_mission(LevelData::LevelDataAll &level_data, const std::string &customMapPath = "");
    //! Loads a mission briefing from a specific file path
    MissionBriefing * loadBriefingFromPath(const std::string &briefingPath);

    MapManager & mapManager() { return mapManager_; }

private:
    /*!
     * NOTE: Original objects data is based on offsets, but our objects are different
     * in size and are not in a single memory block.
     * Because of this indexes in our data, "arrays" are mirrored for object's
     * position within original array.
     */
    struct DataIndex {
        // indexes within vehicle array
        uint16_t vindx[64];
        // indexes within peds array
        uint16_t pindx[256];
        // contains indexes for driver's vehicle
        uint16_t driverindx[256];
        //uint16 windx[512];
        WeaponInstance * weapons[512];

        DataIndex() : weapons() {}
    };

    /**
     * List of attributes of the scenario structure.
     */
    enum ScenarioAttribute {
        kScenarioAttTileX = 4,
        kScenarioAttTileY = 5,
        kScenarioAttTileZ = 6
    };

    //! When loading missions, possibly adds some info to the data
    void hackMissions(int n, uint8_t *data);
    //! Convenient method to modify an specific attribute of a scenario
    void hackScenarioLocation(uint8_t *data, int offset, ScenarioAttribute coord, uint8_t newCoord);
    //! Creates all weapons
    void createWeapons(const LevelData::LevelDataAll &level_data, DataIndex &di, Mission *pMission);
    //! Creates a weapon from the game data
    WeaponInstance * create_weapon_instance(const LevelData::Weapons &gamdata, Map *pMap);
    //! Creates all vehicles
    void createVehicles(const LevelData::LevelDataAll &level_data,
                            DataIndex &di, Mission *pMission);
    //! Creates a vehicle from the game data
    Vehicle * createVehicleInstance(const LevelData::Cars &gamdata, uint16_t id, Map *pMap);
    //! Creates all peds
    void createPeds(const LevelData::LevelDataAll &level_data,
                            DataIndex &di, Mission *pMission);
    void createScriptedActionsForPed(Mission *pMission,
                                        DataIndex &di,
                                        const LevelData::LevelDataAll &level_data,
                                        PedInstance *pPed);
    //! Creates objectives
    void createObjectives(const LevelData::LevelDataAll &level_data,
                            DataIndex &di, Mission *pMission);

private:
    MapManager mapManager_;
    /*!
     * Currently played mission.
     */
    Mission *pMission_;
};

}
#define g_missionCtrl    rs_knl::MissionManager::singleton()

#endif
