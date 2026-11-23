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
#include <queue>

#include "testcase.h"
#include "rs-utils/io/dernc.h"

#include "rs-kernel/model/mission.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/train.h"
#include "rs-kernel/model/leveldata.h"
#include "rs-kernel/model/static.h"
#include "rs-kernel/model/shot.h"
#include "rs-kernel/model/objectivedesc.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-engine/appcontext.h"
#include "rs-engine/sound/soundmanager.h"

const int kTestMapMaxTx = 60;
const int kTestMapMaxTy = 50;
const int kTestMapMaxTz = 5;

void configureMission(const LevelData::MapInfos &mapInfos, rs_knl::Map *map, rs_knl::Mission &mission) {
    REQUIRE( mission.init(map) );
    // Define a squad of 2 agents
    rs_knl::PedInstance *agent = new rs_knl::PedInstance(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    mission.addPed(agent);
    mission.getSquad()->setMember(rs_knl::Squad::kSlot1, agent);
    agent->setStartHealth(10, true);
    agent->setPosition(10, 20, 3, 5, 56, 0);
    agent = new rs_knl::PedInstance(2, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    mission.addPed(agent);
    agent->setStartHealth(10, true);
    agent->setPosition(20, 45, 1, 10, 45, 0);
    mission.getSquad()->setMember(rs_knl::Squad::kSlot2, agent);

    // Enemies : 4 total : 3 killed, 1 persuaded
    rs_knl::PedInstance *ped = new rs_knl::PedInstance(3, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 128);
    ped->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
    mission.addPed(ped);
    ped->setStartHealth(10);
    ped = new rs_knl::PedInstance(4, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 128);
    mission.addPed(ped);
    ped->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
    ped->setStartHealth(10, true);
    ped->setPosition(10, 20, 3, 56, 12, 0); //same tile as agent 1
    ped->handlePersuadedBy(agent);
    ped = new rs_knl::PedInstance(5, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 128);
    mission.addPed(ped);
    ped->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
    ped->setStartHealth(10);
    ped = new rs_knl::PedInstance(6, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 128);
    mission.addPed(ped);
    ped->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
    ped->setStartHealth(10);
    ped->setPosition(10, 20, 3, 45, 20, 0); //same tile as ped 4 but dead

    // Civilian : 5 total : 2 killed, 2 persuaded
    ped = new rs_knl::PedInstance(7, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10, true);
    ped->handlePersuadedBy(agent);
    ped->setPosition(55, 20, 3, 45, 20, 0);
    ped = new rs_knl::PedInstance(8, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10);
    ped = new rs_knl::PedInstance(9, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10, true);
    ped->handlePersuadedBy(agent);
    ped = new rs_knl::PedInstance(10, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10);
    ped = new rs_knl::PedInstance(11, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10, true);

    // Criminal : 2 total : 1 killed
    ped = new rs_knl::PedInstance(12, nullptr, rs_knl::PedInstance::kPedTypeCriminal, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10);
    ped = new rs_knl::PedInstance(13, nullptr, rs_knl::PedInstance::kPedTypeCriminal, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10, true);

    // Guard : 3 total : 3 killed
    ped = new rs_knl::PedInstance(14, nullptr, rs_knl::PedInstance::kPedTypeGuard, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10);
    ped = new rs_knl::PedInstance(15, nullptr, rs_knl::PedInstance::kPedTypeGuard, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10);
    ped = new rs_knl::PedInstance(16, nullptr, rs_knl::PedInstance::kPedTypeGuard, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10);

    // Police : 2 total : 1 killed
    ped = new rs_knl::PedInstance(17, nullptr, rs_knl::PedInstance::kPedTypePolice, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10);
    ped = new rs_knl::PedInstance(18, nullptr, rs_knl::PedInstance::kPedTypeGuard, false, 128);
    mission.addPed(ped);
    ped->setStartHealth(10, true);

    // Vehicle : 1 alive, 1 destroyed
    rs_knl::Vehicle *pVehicle = 
        new rs_knl::GenericCar(11, rs_knl::Vehicle::kVehicleTypePolice, nullptr, 100);
    mission.addVehicle(pVehicle);
    pVehicle->setPosition(12, 45, 2, 0, 5, 0);
    pVehicle->setStartHealth(10, true);

    pVehicle = 
        new rs_knl::GenericCar(12, rs_knl::Vehicle::kVehicleTypeRegularCar, nullptr, 100);
    pVehicle->setPosition(12, 45, 2, 0, 5, 0);
    pVehicle->setStartHealth(10);
    mission.addVehicle(pVehicle);

    mission.buildNavigationGraph();
}

// Helper function to set a tile as walkable
void configureMapInfo(LevelData::MapInfos *mapInfos) {
    rs_utl::WRITE_LE_UINT16(mapInfos->min_x, 10);
    rs_utl::WRITE_LE_UINT16(mapInfos->min_y, 6);
    rs_utl::WRITE_LE_UINT16(mapInfos->max_x, 30);
    rs_utl::WRITE_LE_UINT16(mapInfos->max_y, 20);
}

// Helper function to set a tile as walkable
void configureMap(rs_eng::TileManager &tileMgr, rs_knl::Map &map) {
    int nbTiles = kTestMapMaxTx * kTestMapMaxTy * kTestMapMaxTz;
    rs_eng::Tile **tiles = new rs_eng::Tile*[nbTiles];
    // initialise with transparent tiles
    for (int i=0; i<nbTiles; i++) {
        tiles[i] = tileMgr.getTile(rs_eng::TileManager::kIndexTransparentTile);
    }
    map.setTiles(kTestMapMaxTx, kTestMapMaxTy, kTestMapMaxTz, tiles);

    // TODO : use a file to initialize the map
    /*
    REQUIRE( loadMapFromCsv(TEST_DATA_DIR "/map-10-10-2.csv", tileMgr, map) );
    REQUIRE( map.maxTx() == 10 );
    REQUIRE( map.maxTy() == 10 );
    REQUIRE( map.maxTz() == 2 );
    */

    delete[] tiles;
}

TEST_CASE( "Mission", "[kernel][mission]" ) {
    LevelData::MapInfos mapInfos;
    configureMapInfo(&mapInfos);
    MemoryTileManager tileMgr;
    rs_knl::Map map(&tileMgr, 1);
    configureMap(tileMgr, map);

    rs_knl::Mission cut;
    
    configureMission(mapInfos, &map, cut);

    SECTION( "Add/Remove Armed peds") {
        REQUIRE( cut.numArmedPeds() == 0 );

        cut.addArmedPed(cut.ped(0));
        cut.addArmedPed(cut.ped(1));
        REQUIRE( cut.numArmedPeds() == 2 );

        cut.removeArmedPed(cut.ped(1));
        REQUIRE( cut.numArmedPeds() == 1 );
        REQUIRE( cut.armedPedAtIndex(0) == cut.ped(0) );
    }

    SECTION( "Update stats") {
        cut.stats()->init(2);
        cut.endWithStatus(rs_knl::Mission::kMissionStatusCompleted);

        REQUIRE( cut.stats()->enemyKilled() == 3 );
        REQUIRE( cut.stats()->civilKilled() == 2 );
        REQUIRE( cut.stats()->criminalKilled() == 1 );
        REQUIRE( cut.stats()->guardKilled() == 3 );
        REQUIRE( cut.stats()->policeKilled() == 1 );

        REQUIRE( cut.stats()->nbAgentCaptured() == 1 );
        REQUIRE( cut.stats()->convinced() == 2 );
    }

    SECTION( "findObjectWithNatureAtPos") {
        SECTION( "should find nothing") {
            rs_knl::MapObject::ObjectNature nature = rs_knl::MapObject::kNaturePed;
            size_t searchIndex = 0;

            rs_knl::MapObject *pObject = 
                cut.findObjectWithNatureAtPos(20, 45, 3, nature, &searchIndex);

            REQUIRE( pObject == nullptr );
            REQUIRE( nature == rs_knl::MapObject::kNaturePed );
            REQUIRE( searchIndex == 0 );
        }

        SECTION( "should find agent #2") {
            rs_knl::MapObject::ObjectNature nature = rs_knl::MapObject::kNaturePed;
            size_t searchIndex = 0;

            rs_knl::MapObject *pObject = 
                cut.findObjectWithNatureAtPos(20, 45, 1, nature, &searchIndex);

            REQUIRE( pObject->id() == 2 );
            REQUIRE( nature == rs_knl::MapObject::kNaturePed );
            REQUIRE( searchIndex == 2 );
        }

        SECTION( "should find vehicle #2") {
            rs_knl::MapObject::ObjectNature nature = rs_knl::MapObject::kNatureVehicle;
            size_t searchIndex = 1;

            rs_knl::MapObject *pObject = 
                cut.findObjectWithNatureAtPos(12, 45, 2, nature, &searchIndex);

            REQUIRE( pObject->id() == 12 );
            REQUIRE( nature == rs_knl::MapObject::kNatureVehicle );
            REQUIRE( searchIndex == 2 );
        }
    }

    SECTION( "GetWalkable") {
        // Test 1: Basic flat walkable tile at z=0
        SECTION( "FlatWalkableTileAtGroundLevel") {
            //setWalkable(cut, 5, 5, 0, 0); // Flat surface (default case)
            
            rs_knl::TilePoint mtp;
            mtp.tx = 5;
            mtp.ty = 5;
            mtp.tz = 4; // Start from top
            mtp.ox = 128;
            mtp.oy = 128;
            
            //bool result = cut.getWalkable(mtp);
            
            REQUIRE(mtp.tx == 5);
            REQUIRE(mtp.ty == 5);
            //REQUIRE(mtp.tz == 0);
            REQUIRE(mtp.ox == 128);
            REQUIRE(mtp.oy == 128);
        }
    }
}

#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/animationloader.h"
#include "rs-engine/appcontext.h"
#include "rs-engine/system/system.h"
#include "rs-utils/io/file.h"
#include "rs-kernel/mgr/modmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/agentmanager.h"

TEST_CASE("Load original missions 1 to 50 level data and entities", "[.][mission][all_50]") {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    for (int id = 1; id <= 50; id++) {
        LevelData::LevelDataAll level_data;
        INFO("Mission " << id);
        REQUIRE(misMgr.load_level_data(id, level_data));

        rs_knl::Mission *pMission = misMgr.create_mission(level_data);
        REQUIRE(pMission != nullptr);
        REQUIRE(pMission->buildNavigationGraph());

        delete pMission;
    }
}

TEST_CASE("MissionGateVehiclePathing", "[kernel][vehicle][gate]") {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    // 1. Mission 17: East-West road with North-South door rails (tile 81, orient=2)
    {
        LevelData::LevelDataAll level_data;
        REQUIRE(misMgr.load_level_data(17, level_data));
        rs_knl::Mission *pMission = misMgr.create_mission(level_data);
        REQUIRE(pMission != nullptr);
        REQUIRE(pMission->buildNavigationGraph());
        misMgr.setMission(pMission);

        // Under gate at (74, 38, 1), ground tile must be RailNS (tile 81)
        auto *gateTile = pMission->map()->getTileAt(74, 38, 1);
        REQUIRE(gateTile != nullptr);
        REQUIRE(gateTile->id() == rs_eng::Tile::kTileIdLargeDoorRailNS);
        REQUIRE(gateTile->isRoad());
        REQUIRE(pMission->map()->isTileWalkableByCar(74, 38, 1));
        // RailNS runs North-South across an East-West road, so edge connexions must permit East-West (0x33)
        REQUIRE(gateTile->getEdgeConnexionsForRoadTile() == 0x33);

        uint8_t possWest = pMission->map()->getPossibleConnexionsForRoadTile(rs_knl::TilePoint(74, 38, 1));
        REQUIRE((possWest & rs_knl::Map::kConnexionMaskExitWest) != 0);

        auto *car = dynamic_cast<rs_knl::GenericCar *>(pMission->vehicle(1));
        REQUIRE(car != nullptr);

        // Pathfind into the city past the gate (tx=70, ty=38, tz=2)
        rs_knl::TilePoint ptInside(70, 38, 2);
        REQUIRE(pMission->map()->adjustClickOnRoad(ptInside));
        REQUIRE(car->initMovementToDestination(pMission, ptInside));
        REQUIRE(!car->destPath().empty());

        // Simulate driving into the city through the gate
        auto *agent = pMission->ped(0);
        car->addPassenger(agent);
        car->setDriver(agent, true);
        agent->addActionDriveVehicle(car, ptInside, false);

        bool reachedInside = false;
        for (int step = 0; step < 300; step++) {
            pMission->handleTick(step * 50, 50);
            if (car->tileX() == 70 && car->tileY() == 38) {
                reachedInside = true;
                break;
            }
        }
        REQUIRE(reachedInside);

        // Also test driving out through the gate from inside on lane 2 (ty = 40)
        car->setPosition(71, 40, 2);
        car->clearDestination();
        car->unblockPath();
        rs_knl::TilePoint ptOutsideEast(80, 40, 2);
        REQUIRE(car->initMovementToDestination(pMission, ptOutsideEast));
        REQUIRE(!car->destPath().empty());
        agent->addActionDriveVehicle(car, ptOutsideEast, false);

        bool reachedOutside = false;
        for (int step = 0; step < 300; step++) {
            pMission->handleTick(step * 50, 50);
            if (car->tileX() == 80 && car->tileY() == 40) {
                reachedOutside = true;
                break;
            }
        }
        REQUIRE(reachedOutside);

        delete pMission;
    }

    // 2. Mission 7: North-South road with East-West door rails (tile 80, orient=0)
    {
        LevelData::LevelDataAll m7Data;
        REQUIRE(misMgr.load_level_data(7, m7Data));
        rs_knl::Mission *pM7 = misMgr.create_mission(m7Data);
        REQUIRE(pM7 != nullptr);
        REQUIRE(pM7->buildNavigationGraph());

        // Lane 1 at (59, 26, 1): North-to-South lane across RailEW (tile 80)
        auto *t59 = pM7->map()->getTileAt(59, 26, 1);
        REQUIRE(t59 != nullptr);
        REQUIRE(t59->id() == rs_eng::Tile::kTileIdLargeDoorRailEW);
        REQUIRE(t59->isRoad());
        REQUIRE(pM7->map()->isTileWalkableByCar(59, 26, 1));
        // RailEW runs East-West across a North-South road, so edge connexions must permit North-South (0xCC)
        REQUIRE(t59->getEdgeConnexionsForRoadTile() == 0xCC);
        uint8_t poss59 = pM7->map()->getPossibleConnexionsForRoadTile(rs_knl::TilePoint(59, 26, 1));
        REQUIRE((poss59 & rs_knl::Map::kConnexionMaskExitSouth) != 0);

        // Lane 2 at (61, 26, 1): South-to-North lane across RailEW (tile 80)
        auto *t61 = pM7->map()->getTileAt(61, 26, 1);
        REQUIRE(t61 != nullptr);
        REQUIRE(t61->id() == rs_eng::Tile::kTileIdLargeDoorRailEW);
        REQUIRE(t61->isRoad());
        REQUIRE(pM7->map()->isTileWalkableByCar(61, 26, 1));
        REQUIRE(t61->getEdgeConnexionsForRoadTile() == 0xCC);
        uint8_t poss61 = pM7->map()->getPossibleConnexionsForRoadTile(rs_knl::TilePoint(61, 26, 1));
        REQUIRE((poss61 & rs_knl::Map::kConnexionMaskExitNorth) != 0);

        delete pM7;
    }
}

TEST_CASE("Mission5VehiclePathing", "[VehiclePathing]") {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    LevelData::LevelDataAll level_data;
    REQUIRE(misMgr.load_level_data(5, level_data));

    rs_knl::Mission *pMission = misMgr.create_mission(level_data);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());
    misMgr.setMission(pMission);

    for (int carId = 0; carId < (int)pMission->numVehicles(); carId++) {
        rs_knl::Vehicle *v = pMission->vehicle(carId);
        rs_knl::GenericCar *car = dynamic_cast<rs_knl::GenericCar *>(v);
        if (!car || !car->getDriver()) continue;
        rs_knl::PedInstance *driver = car->getDriver();
        rs_knl::MovementAction *act = driver->defaultAction();
        rs_knl::TilePoint curPos = car->position();
        std::set<rs_knl::MovementAction*> visited;
        int actionCount = 0;
        while (act && visited.find(act) == visited.end() && actionCount < 50) {
            visited.insert(act);
            actionCount++;
            rs_knl::DriveVehicleAction *dv = dynamic_cast<rs_knl::DriveVehicleAction *>(act);
            if (dv) {
                car->setPosition(curPos.tx, curPos.ty, curPos.tz, curPos.ox, curPos.oy, curPos.oz);
                bool ok = car->initMovementToDestination(pMission, dv->dest());
                REQUIRE(ok);
                REQUIRE(!car->destPath().empty());
                curPos = car->destPath().back();
            }
            act = act->next();
        }
    }

    // Simulate 10 seconds (100 ticks of 100ms)
    for (int t = 0; t < 100; t++) {
        pMission->handleTick(t * 100, 100);
        if (t % 25 == 0 || t == 99) {
            for (int carId = 0; carId < (int)pMission->numVehicles(); carId++) {
                rs_knl::Vehicle *v = pMission->vehicle(carId);
                rs_knl::GenericCar *car = dynamic_cast<rs_knl::GenericCar *>(v);
                if (!car || !car->getDriver()) continue;
                bool isRoad = pMission->map()->isTileWalkableByCar(v->tileX(), v->tileY(), 1);
                // Ensure every vehicle remains on drivable roadway
                REQUIRE(isRoad);
            }
        }
    }

    delete pMission;
}

TEST_CASE("Pedestrian Direct Path Smoothing", "[kernel][ped][mission]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    LevelData::LevelDataAll level_data;
    REQUIRE(misMgr.load_level_data(5, level_data));

    rs_knl::Mission *pMission = misMgr.create_mission(level_data);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());
    misMgr.setMission(pMission);

    REQUIRE(pMission->numPeds() > 0);
    rs_knl::PedInstance *agent = pMission->ped(0);
    REQUIRE(agent != nullptr);

    rs_knl::TilePoint startPos = agent->position();

    rs_knl::TilePoint directTarget = startPos;
    directTarget.tx += 3;
    directTarget.ty += 2;

    if (pMission->isTileWalkable(directTarget.tx, directTarget.ty, directTarget.tz)) {
        bool ok = agent->initMovementToDestination(pMission, directTarget);
        REQUIRE(ok);
        REQUIRE(agent->destPath().size() == 1);
        REQUIRE(agent->destPath().front().tx == directTarget.tx);
        REQUIRE(agent->destPath().front().ty == directTarget.ty);
    }

    delete pMission;
}

TEST_CASE("Mission 22 platform tiles", "[mission22]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    LevelData::LevelDataAll level_data;
    REQUIRE(misMgr.load_level_data(22, level_data));

    rs_knl::Mission *pMission = misMgr.create_mission(level_data);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());
    misMgr.setMission(pMission);

    rs_knl::Map *pMap = pMission->map();
    REQUIRE(pMap != nullptr);
    int targetIdx76_z2 = 51 + 76 * pMap->maxTx() + 2 * (pMap->maxTx() * pMap->maxTy());
    int targetIdx78_z2 = 51 + 78 * pMap->maxTx() + 2 * (pMap->maxTx() * pMap->maxTy());
    int targetIdx76_z3 = 51 + 76 * pMap->maxTx() + 3 * (pMap->maxTx() * pMap->maxTy());
    int targetIdx78_z3 = 51 + 78 * pMap->maxTx() + 3 * (pMap->maxTx() * pMap->maxTy());

    // Verify orphaned tiles at z=2 are not falsely marked walkable
    REQUIRE((pMission->mdpoints_[targetIdx76_z2].flags & rs_knl::kWalkable) == 0);
    REQUIRE((pMission->mdpoints_[targetIdx78_z2].flags & rs_knl::kWalkable) == 0);

    // Verify correct nodes at z=3 are marked walkable
    REQUIRE((pMission->mdpoints_[targetIdx76_z3].flags & rs_knl::kWalkable) == rs_knl::kWalkable);
    REQUIRE((pMission->mdpoints_[targetIdx78_z3].flags & rs_knl::kWalkable) == rs_knl::kWalkable);

    // Platform walkable surface nodes at (50, 75..81, 3) must be walkable
    for (int ty = 75; ty <= 81; ++ty) {
        int idx = 50 + ty * pMap->maxTx() + 3 * (pMap->maxTx() * pMap->maxTy());
        REQUIRE((pMission->mdpoints_[idx].flags & rs_knl::kWalkable) == rs_knl::kWalkable);
    }

    rs_knl::PedInstance *agent = pMission->ped(0);
    REQUIRE(agent != nullptr);

    // Clicking directly on visual platform tiles tx=50, ty=74..81
    for (int ty = 74; ty <= 81; ++ty) {
        int tz = 2; // platform tiles are at tz=2, ty=74 stairs at tz=2
        int screen_w = (pMap->maxTx() + (50 - ty)) * (rs_eng::Tile::kTileWidth / 2);
        int coord_h = ((pMap->maxTz() + 50 + ty) - (tz - 1)) * (rs_eng::Tile::kTileHeight / 3);
        int clickVX = screen_w + rs_eng::Tile::kTileWidth / 2;
        int clickVY = coord_h + rs_eng::Tile::kTileHeight / 3;

        rs_knl::TilePoint mapPt = pMap->screenToTilePoint(clickVX, clickVY);
        rs_knl::TilePoint walkTarget = mapPt;
        bool reachable = pMission->findReachableTileFromBase(walkTarget);
        REQUIRE(reachable);
        if (ty >= 75) {
            REQUIRE(walkTarget.tx == 50);
            REQUIRE(walkTarget.ty == ty);
            REQUIRE(walkTarget.tz == 3);
        }

        agent->setPosition(50, 74, 2, 128, 128, 0);
        bool canMove = agent->initMovementToDestination(pMission, walkTarget);
        REQUIRE(canMove);
        REQUIRE(!agent->destPath().empty());
    }

    // Pathfinding between platform tiles (50, 75), (50, 76), (50, 77)
    agent->setPosition(50, 76, 3, 128, 128, 0);
    rs_knl::TilePoint target75(50, 75, 3, 128, 128, 0);
    REQUIRE(agent->initMovementToDestination(pMission, target75));
    REQUIRE(agent->destPath().front().tx == 50);
    REQUIRE(agent->destPath().front().ty == 75);

    agent->setPosition(50, 76, 3, 128, 128, 0);
    rs_knl::TilePoint target77(50, 77, 3, 128, 128, 0);
    REQUIRE(agent->initMovementToDestination(pMission, target77));
    REQUIRE(agent->destPath().front().tx == 50);
    REQUIRE(agent->destPath().front().ty == 77);

    delete pMission;
}

TEST_CASE("SearchMissionCriteriaValidation", "[search]") {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::MissionManager misMgr(&tileMgr);

    std::vector<int> trainMissions;
    std::vector<int> gaussMissions;
    std::vector<int> protectMissions;
    std::vector<int> criminalMissions;
    std::vector<int> agentAndCriminalMissions;

    for (int misId = 1; misId <= 50; misId++) {
        LevelData::LevelDataAll data;
        if (!misMgr.load_level_data(misId, data)) continue;

        // Train check
        bool hasTrain = false;
        for (int i = 0; i < 64; i++) {
            if (data.cars[i].type != 0 && data.cars[i].type != 0xFF) {
                if (data.cars[i].sub_type == rs_knl::Vehicle::kVehicleTypeTrainHead) {
                    hasTrain = true;
                    break;
                }
            }
        }
        if (hasTrain) trainMissions.push_back(misId);

        // Gauss Gun check
        bool hasGauss = false;
        for (int i = 0; i < 512; i++) {
            if (data.weapons[i].desc != 0 && data.weapons[i].sub_type == 0x03) {
                hasGauss = true;
                break;
            }
        }
        if (hasGauss) gaussMissions.push_back(misId);

        // Protect objective check
        bool hasProtect = false;
        for (int i = 0; i < 6; i++) {
            if (rs_utl::READ_LE_UINT16(data.objectives[i].type) == LevelData::kObjectiveTypeProtect) {
                hasProtect = true;
                break;
            }
        }
        if (hasProtect) protectMissions.push_back(misId);

        // Criminal and Enemy Agent check
        bool hasCriminal = false;
        bool hasEnemyAgent = false;
        for (int i = 0; i < 256; i++) {
            if (data.people[i].type_ped != 0 && data.people[i].location != LevelData::kPeopleLocNotVisible) {
                if (data.people[i].type_ped & rs_knl::PedInstance::kPedTypeCriminal) {
                    hasCriminal = true;
                }
                if (i >= 8 && (data.people[i].type_ped & rs_knl::PedInstance::kPedTypeAgent)) {
                    hasEnemyAgent = true;
                }
            }
        }
        if (hasCriminal) criminalMissions.push_back(misId);
        if (hasCriminal && hasEnemyAgent) agentAndCriminalMissions.push_back(misId);
    }

    REQUIRE(trainMissions.size() == 5);
    REQUIRE(gaussMissions.size() == 5);
    REQUIRE(protectMissions.size() == 4);
    REQUIRE(criminalMissions.size() == 11);
    REQUIRE(!agentAndCriminalMissions.empty());
}

TEST_CASE( "Mission Dynamic Spawning & Binary Export", "[kernel][mission]" ) {
    rs_eng::AppContext appCtx;
    MemoryTileManager tileMgr;
    rs_knl::Map map(&tileMgr, 1);
    REQUIRE( loadMapFromCsv(TEST_DATA_DIR "/map-5x5x4.csv", tileMgr, map) );

    rs_knl::Mission mission;
    REQUIRE( mission.init(&map) );

    SECTION( "spawnPed and removePed" ) {
        size_t initialPeds = mission.numPeds();
        rs_knl::TilePoint spawnPos(2, 2, 1);
        rs_knl::PedInstance *pGuard = mission.spawnPed(rs_knl::PedInstance::kPedTypeGuard, spawnPos, 64);
        REQUIRE( pGuard != nullptr );
        REQUIRE( mission.numPeds() == initialPeds + 1 );
        REQUIRE( pGuard->tileX() == 2 );
        REQUIRE( pGuard->tileY() == 2 );
        REQUIRE( pGuard->tileZ() == 1 );
        REQUIRE( pGuard->type() == rs_knl::PedInstance::kPedTypeGuard );

        REQUIRE( mission.removePed(pGuard) );
        REQUIRE( mission.numPeds() == initialPeds );
    }

    SECTION( "spawnVehicle and removeVehicle" ) {
        size_t initialVehicles = mission.numVehicles();
        rs_knl::TilePoint spawnPos(1, 1, 0);
        rs_knl::Vehicle *pCar = mission.spawnVehicle(rs_knl::Vehicle::kVehicleTypeRegularCar, spawnPos, 128);
        REQUIRE( pCar != nullptr );
        REQUIRE( mission.numVehicles() == initialVehicles + 1 );
        REQUIRE( pCar->tileX() == 1 );
        REQUIRE( pCar->tileY() == 1 );

        REQUIRE( mission.removeVehicle(pCar) );
        REQUIRE( mission.numVehicles() == initialVehicles );
    }

    SECTION( "spawnGroundWeapon and removeGroundWeapon" ) {
        ConfigFile config;
        initWeaponConfigFile(config);
        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);

        size_t initialWeapons = mission.numWeaponsOnGround();
        rs_knl::TilePoint spawnPos(3, 3, 1);
        rs_knl::WeaponInstance *pW = mission.spawnGroundWeapon(&pistolClass, spawnPos);
        REQUIRE( pW != nullptr );
        REQUIRE( mission.numWeaponsOnGround() == initialWeapons + 1 );
        REQUIRE( pW->tileX() == 3 );
        REQUIRE( pW->tileY() == 3 );

        REQUIRE( mission.removeGroundWeapon(pW) );

        rs_knl::Weapon airStrikeClass(rs_knl::Weapon::AirStrike, config);
        rs_knl::WeaponInstance airStrike(&airStrikeClass, 100, &map);
        rs_knl::DamageToInflict dmg;
        dmg.dtype = rs_knl::kDmgTypeExplosion;
        dmg.dvalue = airStrikeClass.damagePerShot();
        dmg.range = airStrikeClass.range(); // 5120 targeting range as populated by ShootAction::fillDamageDesc
        dmg.d_owner = nullptr;
        dmg.pWeapon = &airStrike;
        dmg.originLocW = rs_knl::WorldPoint(1 * 256, 2 * 256, 128);
        dmg.aimedLocW = rs_knl::WorldPoint(3 * 256, 2 * 256, 128);

        airStrike.fire(&mission, dmg, 50);
        REQUIRE( mission.numProjectileShots() == 1 );
        size_t maxSfxObserved = 0;
        for (int step = 0; step < 320 && mission.numProjectileShots() > 0; ++step) {
            mission.handleTick(100, 100);
            maxSfxObserved = std::max(maxSfxObserved, mission.numSfxObjects());
        }
        REQUIRE( mission.numProjectileShots() == 0 );
        REQUIRE( maxSfxObserved > 0 );
        REQUIRE( maxSfxObserved < 1000 );
        REQUIRE( mission.numWeaponsOnGround() == initialWeapons );
    }

    SECTION( "Squad spawn points and exportMissionToBinary" ) {
        // Set up 4 agents with distinct sub-tile offsets across 2 tiles (like Mission 1)
        rs_knl::TilePoint sp0(4, 4, 1, 180, 172, 0);
        rs_knl::TilePoint sp1(4, 4, 1, 124, 20, 0);
        rs_knl::TilePoint sp2(4, 3, 1, 124, 148, 0);
        rs_knl::TilePoint sp3(4, 3, 1, 108, 20, 0);

        mission.setSquadSpawnPoint(0, sp0, 192);
        mission.setSquadSpawnPoint(1, sp1, 192);
        mission.setSquadSpawnPoint(2, sp2, 192);
        mission.setSquadSpawnPoint(3, sp3, 192);

        REQUIRE( mission.getSquadSpawnPoint(0).tx == 4 );
        REQUIRE( mission.getSquadSpawnPoint(0).ox == 180 );
        REQUIRE( mission.getSquadSpawnPoint(0).oy == 172 );
        REQUIRE( mission.getSquadSpawnPoint(1).tx == 4 );
        REQUIRE( mission.getSquadSpawnPoint(1).ox == 124 );
        REQUIRE( mission.getSquadSpawnPoint(1).oy == 20 );

        std::string tempGameFile = "temp_test_game.dat";
        REQUIRE( rs_knl::MissionManager::exportMissionToBinary(&mission, 99, tempGameFile) );

        std::ifstream in(tempGameFile, std::ios::binary);
        REQUIRE( in.is_open() );
        in.seekg(0, std::ios::end);
        size_t sz = in.tellg();
        in.seekg(0, std::ios::beg);
        std::vector<uint8_t> fileBytes(sz);
        in.read(reinterpret_cast<char*>(fileBytes.data()), sz);
        in.close();

        REQUIRE( sz > 18 );
        REQUIRE( sz < sizeof(LevelData::LevelDataAll) );
        REQUIRE( rnc::isRncCompressed(fileBytes.data()) );

        LevelData::LevelDataAll reloadedData;
        rs_knl::MissionManager mm(&tileMgr);
        REQUIRE( mm.load_level_data(99, reloadedData, tempGameFile) );
        REQUIRE( reloadedData.people[0].type_ped == rs_knl::PedInstance::kPedTypeAgent );
        REQUIRE( reloadedData.people[0].location == LevelData::kPeopleLocOnMap );
        REQUIRE( reloadedData.people[0].mapposx[1] == 4 );
        REQUIRE( reloadedData.people[0].mapposx[0] == 180 );
        REQUIRE( reloadedData.people[0].mapposy[1] == 4 );
        REQUIRE( reloadedData.people[0].mapposy[0] == 172 );

        REQUIRE( reloadedData.people[1].mapposx[1] == 4 );
        REQUIRE( reloadedData.people[1].mapposx[0] == 124 );
        REQUIRE( reloadedData.people[1].mapposy[1] == 4 );
        REQUIRE( reloadedData.people[1].mapposy[0] == 20 );

        REQUIRE( reloadedData.people[2].mapposx[1] == 4 );
        REQUIRE( reloadedData.people[2].mapposx[0] == 124 );
        REQUIRE( reloadedData.people[2].mapposy[1] == 3 );
        REQUIRE( reloadedData.people[2].mapposy[0] == 148 );

        REQUIRE( reloadedData.people[3].mapposx[1] == 4 );
        REQUIRE( reloadedData.people[3].mapposx[0] == 108 );
        REQUIRE( reloadedData.people[3].mapposy[1] == 3 );
        REQUIRE( reloadedData.people[3].mapposy[0] == 20 );

        // Test collision avoidance when two agents share a tile with identical offsets
        rs_knl::TilePoint coll0(2, 2, 1, 128, 128, 0);
        rs_knl::TilePoint coll1(2, 2, 1, 128, 128, 0);
        mission.setSquadSpawnPoint(0, coll0, 0);
        mission.setSquadSpawnPoint(1, coll1, 0);
        REQUIRE( rs_knl::MissionManager::exportMissionToBinary(&mission, 99, tempGameFile) );
        REQUIRE( mm.load_level_data(99, reloadedData, tempGameFile) );
        // The colliding agent should have been shifted to prevent overlap
        REQUIRE( !(reloadedData.people[0].mapposx[0] == reloadedData.people[1].mapposx[0] &&
                   reloadedData.people[0].mapposy[0] == reloadedData.people[1].mapposy[0]) );

        std::remove(tempGameFile.c_str());
    }

    SECTION( "Player agent count and free slot allocation" ) {
        rs_knl::Mission testMission;
        REQUIRE( testMission.init(&map) );
        REQUIRE( testMission.countPlayerAgents() == 0 );
        REQUIRE( testMission.findNextFreePlayerSlot() == 0 );

        auto *p0 = testMission.spawnPlayerAgent(0, rs_knl::TilePoint(2, 2, 1), 0);
        REQUIRE( p0 != nullptr );
        REQUIRE( testMission.countPlayerAgents() == 1 );
        REQUIRE( testMission.findNextFreePlayerSlot() == 1 );

        auto *p2 = testMission.spawnPlayerAgent(2, rs_knl::TilePoint(2, 3, 1), 0);
        REQUIRE( p2 != nullptr );
        REQUIRE( testMission.countPlayerAgents() == 2 );
        REQUIRE( testMission.findNextFreePlayerSlot() == 1 );

        auto *p1 = testMission.spawnPlayerAgent(1, rs_knl::TilePoint(3, 2, 1), 0);
        REQUIRE( p1 != nullptr );
        REQUIRE( testMission.countPlayerAgents() == 3 );
        REQUIRE( testMission.findNextFreePlayerSlot() == 3 );

        auto *p3 = testMission.spawnPlayerAgent(3, rs_knl::TilePoint(3, 3, 1), 0);
        REQUIRE( p3 != nullptr );
        REQUIRE( testMission.countPlayerAgents() == 4 );
        REQUIRE( testMission.findNextFreePlayerSlot() == -1 );

        testMission.removePed(p1);
        REQUIRE( testMission.countPlayerAgents() == 3 );
        REQUIRE( testMission.findNextFreePlayerSlot() == 1 );
    }

    SECTION( "Custom map path and cache clearing" ) {
        std::string tempMapFile = "temp_custom_map.dat";
        REQUIRE( map.exportBinary(tempMapFile) );

        rs_knl::MissionManager mm(&tileMgr);
        LevelData::MapInfos mapInfos = {};
        mapInfos.map[0] = 100;

        rs_knl::Map *pCustomMap = mm.mapManager().getMapFromPath(tempMapFile, mapInfos);
        REQUIRE( pCustomMap != nullptr );
        REQUIRE( pCustomMap->maxTx() == 5 );
        REQUIRE( pCustomMap->maxTy() == 5 );
        REQUIRE( pCustomMap->maxTz() == 4 );

        mm.mapManager().clearCache();

        LevelData::LevelDataAll lData = {};
        lData.mapinfos = mapInfos;
        rs_knl::Mission *pCustomMis = mm.create_mission(lData, tempMapFile);
        REQUIRE( pCustomMis != nullptr );
        REQUIRE( pCustomMis->map() != nullptr );
        REQUIRE( pCustomMis->map()->maxTx() == 5 );

        mm.destroyMission();
        std::remove(tempMapFile.c_str());
    }

    SECTION( "Custom data directory loading without modifying data/" ) {
        std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "resynd_test_dir";
        std::filesystem::create_directories(tempDir);

        std::filesystem::path tempGameFile = tempDir / "game99.dat";
        std::filesystem::path tempMapFile = tempDir / "map99.dat";

        REQUIRE( rs_knl::MissionManager::exportMissionToBinary(&mission, 99, tempGameFile.string()) );
        REQUIRE( map.exportBinary(tempMapFile.string()) );

        rs_knl::MissionManager mm(&tileMgr);
        mm.mapManager().setCustomDataDir(tempDir.string());

        LevelData::LevelDataAll loadedData;
        REQUIRE( mm.load_level_data(99, loadedData) );
        REQUIRE( loadedData.people[0].type_ped == rs_knl::PedInstance::kPedTypeAgent );

        std::error_code ec;
        std::filesystem::remove_all(tempDir, ec);
    }
}

TEST_CASE( "All 50 missions spawn points and playability validation", "[all50_playability]" ) {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    int unwalkableSpawnMissions = 0;
    for (int m = 1; m <= 50; ++m) {
        LevelData::LevelDataAll level_data;
        if (!misMgr.load_level_data(m, level_data)) continue;
        rs_knl::Mission *pMission = misMgr.create_mission(level_data);
        if (!pMission) continue;
        REQUIRE(pMission->buildNavigationGraph());

        int agentCount = 0;
        for (size_t i = 0; i < pMission->numPeds(); ++i) {
            rs_knl::PedInstance *p = pMission->ped(i);
            if (p && p->isOurAgent()) {
                agentCount++;
                if (!pMission->isTileWalkable(p->tileX(), p->tileY(), p->tileZ())) {
                    unwalkableSpawnMissions++;
                    printf("Mission %d: Agent #%d at (%d, %d, %d) is NOT walkable!\n",
                        m, p->id() + 1, p->tileX(), p->tileY(), p->tileZ());
                }
            }
        }
        int objCount = pMission->numObjectives();
        if (objCount == 0) {
            printf("Mission %d has 0 objectives!\n", m);
        }
        REQUIRE(agentCount > 0);
        REQUIRE(objCount > 0);
        misMgr.destroyMission();
    }
    REQUIRE(unwalkableSpawnMissions == 0);

    SECTION( "Export and reload all 50 missions via customDataDir" ) {
        std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "test_all50_export";
        std::filesystem::create_directories(tempDir);

        for (int m = 1; m <= 50; ++m) {
            LevelData::LevelDataAll origData;
            if (!misMgr.load_level_data(m, origData)) continue;
            rs_knl::Mission *pOrigMis = misMgr.create_mission(origData);
            if (!pOrigMis) continue;

            if (m == 8) {
                printf("\n--- Mission 8 LevelData objectives ---\n");
                for (int oi = 0; oi < 6; ++oi) {
                    uint16_t ot = rs_utl::READ_LE_UINT16(origData.objectives[oi].type);
                    uint16_t of = rs_utl::READ_LE_UINT16(origData.objectives[oi].offset);
                    printf("  obj[%d]: type=0x%04X, offset=0x%04X\n", oi, ot, of);
                }
                printf("--- Mission 8 pOrigMis objectives (%zu) ---\n", pOrigMis->numObjectives());
                for (size_t oi = 0; oi < pOrigMis->numObjectives(); ++oi) {
                    auto *o = pOrigMis->objective(oi);
                    printf("  pOrigMis obj[%zu]: type=%s\n", oi, typeid(*o).name());
                }
            }
            std::filesystem::path tempGame = tempDir / std::format("game{:02}.dat", m);
            REQUIRE( rs_knl::MissionManager::exportMissionToBinary(pOrigMis, m, tempGame.string()) );

            misMgr.destroyMission();

            // Now reload the exported file via customDataDir
            misMgr.mapManager().setCustomDataDir(tempDir.string());
            LevelData::LevelDataAll reloadedData;
            REQUIRE( misMgr.load_level_data(m, reloadedData) );
            rs_knl::Mission *pReloadedMis = misMgr.create_mission(reloadedData);
            REQUIRE( pReloadedMis != nullptr );
            REQUIRE( pReloadedMis->buildNavigationGraph() );

            for (size_t i = 0; i < pReloadedMis->numPeds(); ++i) {
                rs_knl::PedInstance *p = pReloadedMis->ped(i);
                if (p && p->isOurAgent()) {
                    REQUIRE( pReloadedMis->isTileWalkable(p->tileX(), p->tileY(), p->tileZ()) );
                }
            }
            printf("Checking exported mission %d (reloaded objs: %zu, orig objs: %zu)...\n",
                m, pReloadedMis->numObjectives(), pOrigMis ? pOrigMis->numObjectives() : 0);
            REQUIRE( pReloadedMis->numObjectives() > 0 );
            misMgr.destroyMission();
            misMgr.mapManager().setCustomDataDir("");
        }

        std::error_code ec;
        std::filesystem::remove_all(tempDir, ec);
    }

    SECTION( "New custom mission creation, walkability, movement and export/reload validation" ) {
        // 1. Simulate creating a new blank mission with concrete spawn platform at Z=1
        LevelData::LevelDataAll blankData{};
        blankData.mapinfos.map[0] = 1; // using map 1 tile blocks
        int cx = 64;
        int cy = 48;
        int groundZ = 1;
        int spawnZ = groundZ + 1; // Z = 2
        for (int i = 0; i < 4; ++i) {
            blankData.people[i].type_ped = rs_knl::PedInstance::kPedTypeAgent;
            blankData.people[i].location = LevelData::kPeopleLocOnMap;
            blankData.people[i].state = LevelData::kPeopleStateWalking;
            blankData.people[i].mapposx[0] = 128;
            blankData.people[i].mapposx[1] = static_cast<uint8_t>(cx + (i % 2));
            blankData.people[i].mapposy[0] = 128;
            blankData.people[i].mapposy[1] = static_cast<uint8_t>(cy + (i / 2));
            rs_utl::WRITE_LE_UINT16(blankData.people[i].mapposz, static_cast<uint16_t>(spawnZ << 7));
            rs_utl::WRITE_LE_UINT16(blankData.people[i].index_base_anim, 1);
        }

        rs_knl::Mission *pNewMis = misMgr.create_mission(blankData);
        REQUIRE( pNewMis != nullptr );

        rs_knl::Map *pMap = pNewMis->map();
        REQUIRE( pMap != nullptr );

        // Clear all map tiles
        for (int y = 0; y < pMap->maxTy(); ++y) {
            for (int x = 0; x < pMap->maxTx(); ++x) {
                for (int z = 0; z < pMap->maxTz(); ++z) {
                    pMap->setTileAt(x, y, z, 0);
                }
            }
        }

        // Create 6x6 concrete platform (Tile ID 12) at Z=1
        for (int y = cy - 2; y <= cy + 3; ++y) {
            for (int x = cx - 2; x <= cx + 3; ++x) {
                pMap->setTileAt(x, y, groundZ, 12);
            }
        }

        // Spawn a civilian actor standing on the platform at (cx - 1, cy, 2)
        rs_knl::PedInstance *pCiv = pNewMis->spawnPed(
            rs_knl::PedInstance::kPedTypeCivilian,
            rs_knl::TilePoint(cx - 1, cy, spawnZ),
            0);
        REQUIRE( pCiv != nullptr );
        REQUIRE( pCiv->baseAnim() == 1537 ); // Correct civilian suit animation, NOT 1744 (Guard) or 0

        // Add assassination and evacuation objectives
        pNewMis->addObjective(new rs_knl::ObjAssassinate(pCiv));
        std::vector<rs_knl::PedInstance*> evacTargets;
        pNewMis->addObjective(new rs_knl::ObjEvacuate(cx, cy, spawnZ, evacTargets));

        // Build navigation graph
        REQUIRE( pNewMis->buildNavigationGraph() );

        // Validate that all squad agents and civilian are on walkable tiles
        for (size_t i = 0; i < pNewMis->numPeds(); ++i) {
            rs_knl::PedInstance *p = pNewMis->ped(i);
            REQUIRE( pNewMis->isTileWalkable(p->tileX(), p->tileY(), p->tileZ()) );
        }

        // Validate agent movement towards civilian
        rs_knl::PedInstance *pAgent0 = pNewMis->ped(0);
        REQUIRE( pAgent0 != nullptr );
        REQUIRE( pAgent0->isOurAgent() );
        rs_knl::TilePoint civPos(pCiv->tileX(), pCiv->tileY(), pCiv->tileZ());
        pAgent0->addActionWalk(civPos, false);
        REQUIRE( pAgent0->currentAction() != nullptr );
        REQUIRE( pAgent0->currentAction()->type() == rs_knl::Action::kActTypeWalk );
        REQUIRE( !pAgent0->currentAction()->hasFailed() );

        // Export this mission to a temporary directory
        std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "rs_custom_mis_test";
        std::filesystem::create_directories(tempDir);
        std::filesystem::path gameFile = tempDir / "game99.dat";
        std::filesystem::path mapFile = tempDir / "map99.dat";

        REQUIRE( rs_knl::MissionManager::exportMissionToBinary(pNewMis, 99, gameFile.string()) );
        REQUIRE( pNewMis->map()->exportBinary(mapFile.string()) );

        // Reload mission from exported files
        misMgr.destroyMission();
        misMgr.mapManager().setCustomDataDir(tempDir.string());

        LevelData::LevelDataAll reloadedData;
        REQUIRE( misMgr.load_level_data(99, reloadedData) );
        rs_knl::Mission *pReloaded = misMgr.create_mission(reloadedData);
        REQUIRE( pReloaded != nullptr );
        REQUIRE( pReloaded->buildNavigationGraph() );

        // Verify reloaded civilian properties
        REQUIRE( pReloaded->numPeds() == 5 ); // 4 agents + 1 civilian
        rs_knl::PedInstance *pReloadedCiv = nullptr;
        for (size_t i = 0; i < pReloaded->numPeds(); ++i) {
            if (!pReloaded->ped(i)->isOurAgent()) {
                pReloadedCiv = pReloaded->ped(i);
                break;
            }
        }
        REQUIRE( pReloadedCiv != nullptr );
        REQUIRE( pReloadedCiv->type() == rs_knl::PedInstance::kPedTypeCivilian );
        REQUIRE( pReloadedCiv->baseAnim() == 1537 ); // NOT 1744 (Guard)!
        REQUIRE( pReloadedCiv->tileZ() == spawnZ );
        REQUIRE( pReloaded->isTileWalkable(pReloadedCiv->tileX(), pReloadedCiv->tileY(), pReloadedCiv->tileZ()) );

        // Verify reloaded objectives
        REQUIRE( pReloaded->numObjectives() == 2 );

        // Verify reloaded agent pathfinding
        rs_knl::PedInstance *pReloadedAgent = pReloaded->ped(0);
        pReloadedAgent->addActionWalk(civPos, false);
        REQUIRE( pReloadedAgent->currentAction() != nullptr );
        REQUIRE( pReloadedAgent->currentAction()->type() == rs_knl::Action::kActTypeWalk );
        REQUIRE( !pReloadedAgent->currentAction()->hasFailed() );


        misMgr.destroyMission();
        misMgr.mapManager().setCustomDataDir("");
        std::error_code ec;
        std::filesystem::remove_all(tempDir, ec);
    }
}

TEST_CASE("WindowOrientationAndAnim", "[window]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::Map map(&tileMgr, 0);
    configureMap(tileMgr, map);
    rs_knl::Mission mission;
    REQUIRE(mission.init(&map));

    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);

    rs_knl::TilePoint pos(2, 2, 0, 128, 128, 0);

    // 1. Closed Window: NS (0, facing down/right) uses baseAnim 550; EW (1/2, facing down/left) uses baseAnim 551
    rs_knl::Static *pWinClosedNS = mission.spawnStatic(0x13, pos, 550, rs_knl::Static::kStaticOrientationNS);
    REQUIRE(pWinClosedNS != nullptr);
    CHECK(pWinClosedNS->orientation() == rs_knl::Static::kStaticOrientationNS);
    CHECK(pWinClosedNS->baseAnim() == 550);

    rs_knl::Static *pWinClosedEW = mission.spawnStatic(0x13, pos, 551, rs_knl::Static::kStaticOrientationEW);
    REQUIRE(pWinClosedEW != nullptr);
    CHECK(pWinClosedEW->orientation() == rs_knl::Static::kStaticOrientationEW);
    CHECK(pWinClosedEW->baseAnim() == 551);

    // 2. Open Window: NS uses 552, EW uses 553
    rs_knl::Static *pWinOpenNS = mission.spawnStatic(0x12, pos, 552, rs_knl::Static::kStaticOrientationNS);
    REQUIRE(pWinOpenNS != nullptr);
    CHECK(pWinOpenNS->orientation() == rs_knl::Static::kStaticOrientationNS);
    CHECK(pWinOpenNS->baseAnim() == 552);

    rs_knl::Static *pWinOpenEW = mission.spawnStatic(0x12, pos, 553, rs_knl::Static::kStaticOrientationEW);
    REQUIRE(pWinOpenEW != nullptr);
    CHECK(pWinOpenEW->orientation() == rs_knl::Static::kStaticOrientationEW);
    CHECK(pWinOpenEW->baseAnim() == 553);

    // 3. Damaged Window: NS uses 556, EW uses 557
    rs_knl::Static *pWinDamagedNS = mission.spawnStatic(0x15, pos, 556, rs_knl::Static::kStaticOrientationNS);
    REQUIRE(pWinDamagedNS != nullptr);
    CHECK(pWinDamagedNS->orientation() == rs_knl::Static::kStaticOrientationNS);
    CHECK(pWinDamagedNS->baseAnim() == 556);

    rs_knl::Static *pWinDamagedEW = mission.spawnStatic(0x15, pos, 557, rs_knl::Static::kStaticOrientationEW);
    REQUIRE(pWinDamagedEW != nullptr);
    CHECK(pWinDamagedEW->orientation() == rs_knl::Static::kStaticOrientationEW);
    CHECK(pWinDamagedEW->baseAnim() == 557);

    // Test dynamic orientation switching on a WindowObj
    auto *pWinObj = dynamic_cast<rs_knl::WindowObj*>(pWinClosedNS);
    REQUIRE(pWinObj != nullptr);
    pWinObj->setOrientation(rs_knl::Static::kStaticOrientationEW);
    CHECK(pWinObj->orientation() == rs_knl::Static::kStaticOrientationEW);
    CHECK(pWinObj->baseAnim() == 551);
    pWinObj->setOrientation(rs_knl::Static::kStaticOrientationNS);
    CHECK(pWinObj->orientation() == rs_knl::Static::kStaticOrientationNS);
    CHECK(pWinObj->baseAnim() == 550);
}

TEST_CASE("TrainFullGroupPlacementAndDirections", "[train]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::Map map(&tileMgr, 0);
    configureMap(tileMgr, map);
    rs_knl::Mission mission;
    REQUIRE(mission.init(&map));

    rs_knl::TilePoint startPos(20, 20, 0, 128, 128, 0);

    // Test SW (0): rear carriage should face opposite direction NE (128)
    rs_knl::Vehicle *vHead = mission.spawnVehicle(0xFE, startPos, 0);
    REQUIRE(vHead != nullptr);
    auto *pHead = dynamic_cast<rs_knl::TrainHead*>(vHead);
    REQUIRE(pHead != nullptr);

    // Should have spawned 6 segments total matching authentic Bullfrog layout (e.g. Siberia)
    REQUIRE(mission.numVehicles() == 6);
    CHECK(mission.vehicle(0)->direction() == 0);   // Engine (SW)
    CHECK(mission.vehicle(1)->direction() == 0);   // Carriage 1 (Windows, anim 428)
    CHECK(mission.vehicle(2)->direction() == 0);   // Carriage 2 (Door, anim 434)
    CHECK(mission.vehicle(3)->direction() == 0);   // Carriage 3 (Windows, anim 428)
    CHECK(mission.vehicle(4)->direction() == 0);   // Carriage 4 (Raised Roof, anim 440)
    CHECK(mission.vehicle(5)->direction() == 128); // Rear Locomotive (NE - opposite direction!)

    // Check positions along Y axis (orientation 0 moves along Y)
    rs_knl::WorldPoint w0 = mission.vehicle(0)->position();
    rs_knl::WorldPoint w1 = mission.vehicle(1)->position();
    rs_knl::WorldPoint w2 = mission.vehicle(2)->position();
    rs_knl::WorldPoint w3 = mission.vehicle(3)->position();
    rs_knl::WorldPoint w4 = mission.vehicle(4)->position();
    rs_knl::WorldPoint w5 = mission.vehicle(5)->position();

    CHECK(w0.x == w1.x);
    CHECK(w0.y - w1.y == 232);
    CHECK(w1.y - w2.y == 256);
    CHECK(w2.y - w3.y == 256);
    CHECK(w3.y - w4.y == 256);
    CHECK(w4.y - w5.y == 184);

    // Test non-diagonal orientation quantization:
    // Spawning with orientation 32 (South - invalid for train) must quantize to 64 (SE)
    rs_knl::TilePoint pos2(25, 25, 0, 128, 128, 0);
    rs_knl::Vehicle *vHead2 = mission.spawnVehicle(0xFE, pos2, 32);
    REQUIRE(vHead2 != nullptr);
    CHECK(vHead2->direction() == 64);
    // Rear section of this second train should face (64 + 128) % 256 = 192 (NW)
    REQUIRE(mission.numVehicles() == 12);
    CHECK(mission.vehicle(11)->direction() == 192);
}

TEST_CASE("TrainHeadDirectionsAndCarriageExport", "[train]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::Map map(&tileMgr, 0);
    configureMap(tileMgr, map);
    rs_knl::Mission mission;
    REQUIRE(mission.init(&map));

    // 1. TrainHead in all 4 directions
    rs_knl::Vehicle *h0 = mission.spawnVehicle(rs_knl::Vehicle::kVehicleTypeTrainHead, rs_knl::TilePoint(10, 10, 0), 0);
    rs_knl::Vehicle *h64 = mission.spawnVehicle(rs_knl::Vehicle::kVehicleTypeTrainHead, rs_knl::TilePoint(11, 10, 0), 64);
    rs_knl::Vehicle *h128 = mission.spawnVehicle(rs_knl::Vehicle::kVehicleTypeTrainHead, rs_knl::TilePoint(12, 10, 0), 128);
    rs_knl::Vehicle *h192 = mission.spawnVehicle(rs_knl::Vehicle::kVehicleTypeTrainHead, rs_knl::TilePoint(13, 10, 0), 192);

    REQUIRE(h0 != nullptr);
    REQUIRE(h64 != nullptr);
    REQUIRE(h128 != nullptr);
    REQUIRE(h192 != nullptr);

    CHECK(h0->direction() == 0);     // SW
    CHECK(h64->direction() == 64);   // SE
    CHECK(h128->direction() == 128); // NE
    CHECK(h192->direction() == 192); // NW

    // 2. Train Carriage Raised Roof (anim 440) export to variant 2
    rs_knl::Vehicle *cRaised = mission.spawnVehicle(rs_knl::Vehicle::kVehicleTypeTrainBody, rs_knl::TilePoint(14, 10, 0), 0, 440);
    REQUIRE(cRaised != nullptr);
    CHECK(cRaised->baseSpriteAnim() == 440);

    std::string tempGameFile = "test_train_export.dat";
    REQUIRE(rs_knl::MissionManager::exportMissionToBinary(&mission, 99, tempGameFile));

    LevelData::LevelDataAll exportedData;
    rs_knl::MissionManager mm(&tileMgr);
    REQUIRE(mm.load_level_data(99, exportedData, tempGameFile));

    // Find cRaised in exported cars
    bool foundVariant2 = false;
    for (int i = 0; i < 64; ++i) {
        if (exportedData.cars[i].sub_type == rs_knl::Vehicle::kVehicleTypeTrainBody) {
            uint16_t variant = rs_utl::READ_LE_UINT16(exportedData.cars[i].index_base_anim);
            if (variant == 2) {
                foundVariant2 = true;
                break;
            }
        }
    }
    CHECK(foundVariant2);
    std::filesystem::remove(tempGameFile);
}

TEST_CASE("TrainDriverBoardCarriageAndDrive", "[train]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::Map map(&tileMgr, 0);
    configureMap(tileMgr, map);
    rs_knl::Mission mission;
    REQUIRE(mission.init(&map));

    // Spawn Train Head and attach a carriage
    rs_knl::TrainHead *pHead = dynamic_cast<rs_knl::TrainHead*>(
        mission.spawnVehicle(rs_knl::Vehicle::kVehicleTypeTrainHead, rs_knl::TilePoint(20, 20, 0), 0));
    REQUIRE(pHead != nullptr);

    rs_knl::TrainBody *pCarriage = dynamic_cast<rs_knl::TrainBody*>(
        mission.spawnVehicle(rs_knl::Vehicle::kVehicleTypeTrainBody, rs_knl::TilePoint(20, 19, 0), 0, 434));
    REQUIRE(pCarriage != nullptr);
    pHead->appendTrainBody(pCarriage);

    // Spawn pedestrian outside the train
    rs_knl::PedInstance *pDriver = mission.spawnPed(rs_knl::PedInstance::kPedTypePolice, rs_knl::TilePoint(20, 19, 0), 0);
    REQUIRE(pDriver != nullptr);

    // Board carriage (vehicle index 1)
    rs_knl::ScriptStep step0;
    step0.type = LevelData::kScenarioTypeUseVehicle;
    step0.targetEntityId = 1; // Carriage
    pDriver->addScriptStep(step0);

    // Drive train to (20, 30, 0)
    rs_knl::ScriptStep step1;
    step1.type = LevelData::kScenarioTypeGoto;
    step1.targetPos = rs_knl::TilePoint(20, 30, 0);
    pDriver->addScriptStep(step1);

    // Train station dwell wait (10s)
    rs_knl::ScriptStep step2;
    step2.type = LevelData::kScenarioTypeTrainWait;
    step2.param = 10000;
    pDriver->addScriptStep(step2);

    rs_knl::MissionManager::buildScriptedActionsForPed(&mission, pDriver);

    // Check actions created
    rs_knl::MovementAction *act0 = pDriver->defaultAction();
    REQUIRE(act0 != nullptr); // WalkAction to carriage
    rs_knl::MovementAction *act1 = act0->next();
    REQUIRE(act1 != nullptr);
    CHECK(dynamic_cast<rs_knl::EnterVehicleAction*>(act1) != nullptr);

    rs_knl::MovementAction *act2 = act1->next();
    REQUIRE(act2 != nullptr);
    CHECK(dynamic_cast<rs_knl::DriveTrainAction*>(act2) != nullptr);

    rs_knl::MovementAction *act3 = act2->next();
    REQUIRE(act3 != nullptr);
    CHECK(dynamic_cast<rs_knl::WaitAction*>(act3) != nullptr);

    // Simulate entering carriage
    pCarriage->addPassenger(pDriver);
    CHECK(pHead->containsPed(pDriver)); // Virtual check across linked carriages!
}

TEST_CASE("CarDriverAndRoutePreservation", "[car_driver]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;

    // Load Mission 2
    REQUIRE(misMgr.load_level_data(2, levelData));

    // In Mission 2, Car 0 has sub_type 13, driver offset 0x48 (Civilian 1), and civilian 1 has scripted DriveVehicleAction
    uint16_t driverOff = rs_utl::READ_LE_UINT16(levelData.cars[0].offset_of_driver);
    int driverSlot = (driverOff - 2) / 92;
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);

    REQUIRE(driverSlot >= 0);
    REQUIRE(driverSlot < 256);
    REQUIRE(rs_utl::READ_LE_UINT16(levelData.people[driverSlot].offset_scenario_start) != 0);

    // Verify Window statics in Mission 2 preserve their correct orientations:
    // Static 45 is on the down/left facing wall (NS / along X): anim 550
    rs_knl::Static *s45 = pMission->statics(45);
    REQUIRE(s45 != nullptr);
    CHECK(s45->orientation() == rs_knl::Static::kStaticOrientationNS);
    CHECK(s45->baseAnim() == 550);
    CHECK(s45->getDrawAnimation() == 550);

    // Static 69 is on the down/right facing wall (EW / along Y): anim 551
    rs_knl::Static *s69 = pMission->statics(69);
    REQUIRE(s69 != nullptr);
    CHECK(s69->orientation() == rs_knl::Static::kStaticOrientationEW);
    CHECK(s69->baseAnim() == 551);
    CHECK(s69->getDrawAnimation() == 551);

    // Verify Car 0 in simulation has driver and driver is Civilian
    REQUIRE(pMission->numVehicles() > 0);
    auto *pCar = dynamic_cast<rs_knl::GenericCar*>(pMission->vehicle(0));
    REQUIRE(pCar != nullptr);
    rs_knl::PedInstance *pDriver = pCar->getDriver();
    REQUIRE(pDriver != nullptr);
    CHECK(pDriver->type() == rs_knl::PedInstance::kPedTypeCivilian);

    // Export mission to temporary file and verify exported binary data
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "synd_driver_test";
    std::filesystem::create_directories(tempDir);
    std::filesystem::path tempGame = tempDir / "game02.dat";

    REQUIRE(rs_knl::MissionManager::exportMissionToBinary(pMission, 2, tempGame.string()));

    // Reload exported level data via MissionManager
    misMgr.destroyMission();
    misMgr.mapManager().setCustomDataDir(tempDir.string());
    LevelData::LevelDataAll exportedData;
    REQUIRE(misMgr.load_level_data(2, exportedData));

    // Verify Car 0 still points to driver and driver scenario start is NOT wiped!
    uint16_t expDriverOff = rs_utl::READ_LE_UINT16(exportedData.cars[0].offset_of_driver);
    CHECK(expDriverOff != 0);
    int expDriverSlot = (expDriverOff - 2) / 92;
    CHECK(expDriverSlot == driverSlot);
    CHECK(exportedData.people[expDriverSlot].offset_scenario_start != 0);
    CHECK(rs_utl::READ_LE_UINT16(exportedData.people[expDriverSlot].offset_of_vehicle) == 0x5C02 + 0 * 42);
    CHECK(exportedData.people[expDriverSlot].location == LevelData::kPeopleLocInVehicle);

    misMgr.mapManager().setCustomDataDir("");
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);
}

TEST_CASE( "Scripted Actions, Waypoint Routes, Loops and Binary Serialization", "[mission][scripting]" ) {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;

    // Load Mission 1
    REQUIRE(misMgr.load_level_data(1, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);

    printf("\n=== MISSION 1 SQUAD SPAWN ===\n");
    for (int i = 0; i < 4; ++i) {
        auto pt = pMission->getSquadSpawnPoint(i);
        printf("Squad %d spawn: (%d, %d, %d)\n", i, pt.tx, pt.ty, pt.tz);
    }
    printf("=== RAW LEVELDATA PEOPLE 0..15 ===\n");
    for (int i = 0; i < 16; ++i) {
        const auto &p = levelData.people[i];
        int z = rs_utl::READ_LE_UINT16(p.mapposz) >> 7;
        printf("Person %d: type=%d, loc=%d, state=%d, pos=(%d, %d, %d), scStart=%u, scCurr=%u\n",
            i, p.type_ped, p.location, p.state, p.mapposx[1], p.mapposy[1], z,
            rs_utl::READ_LE_UINT16(p.offset_scenario_start), rs_utl::READ_LE_UINT16(p.offset_scenario_curr));
    }
    printf("=== MISSION 1 STATICS (total %zu) ===\n", pMission->numStatics());
    for (size_t i = 0; i < pMission->numStatics(); ++i) {
        auto *st = pMission->statics(i);
        const auto &rawSt = levelData.statics[i];
        printf("Static %zu: desc=0x%02X, type=0x%02X, subType=0x%02X, pos=(%d, %d, %d), bAnim=%d, cAnim=%d, drawAnim=%d\n",
            i, rawSt.desc, rawSt.type, st->subType(), st->position().tx, st->position().ty, st->position().tz,
            rs_utl::READ_LE_UINT16(rawSt.index_base_anim), rs_utl::READ_LE_UINT16(rawSt.index_current_anim), st->getDrawAnimation());
    }
    printf("=== MISSION 1 PEDS (total %zu) ===\n", pMission->numPeds());
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        auto *pd = pMission->ped(i);
        printf("Ped %zu (id %d): type=%d, pos=(%d, %d, %d), isOur=%d, bAnim=%d, drawAnim=%d\n",
            i, pd->id(), pd->type(), pd->position().tx, pd->position().ty, pd->position().tz,
            pd->isOurAgent(), pd->baseAnim(), pd->getDrawAnimation());
        if (pd->id() == 11) {
            uint16_t scStart = rs_utl::READ_LE_UINT16(levelData.people[11].offset_scenario_start);
            uint16_t scCurr = rs_utl::READ_LE_UINT16(levelData.people[11].offset_scenario_curr);
            printf("  Person 11 in levelData: scStart=%u (idx %u), scCurr=%u\n", scStart, scStart / 8, scCurr);
            for (int idx = 20; idx <= 32; ++idx) {
                const auto &sc = levelData.scenarios[idx];
                printf("    Scen idx %d (byte %d): type=%u, tilex=%u, tiley=%u, tilez=%u, objOff=%u, nextOff=%u\n",
                    idx, idx * 8, sc.type, sc.tilex, sc.tiley, sc.tilez,
                    rs_utl::READ_LE_UINT16(sc.offset_object), rs_utl::READ_LE_UINT16(sc.next));
            }
        }
    }

    // 1. Test scripted pedestrian on foot
    REQUIRE(pMission->numPeds() > 0);
    rs_knl::PedInstance *pPed = nullptr;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        if (!pMission->ped(i)->isOurAgent()) {
            pPed = pMission->ped(i);
            break;
        }
    }
    REQUIRE(pPed != nullptr);
    int pedSlot = pPed->id();

    // Initial state: no script
    pPed->clearScriptSteps();
    CHECK_FALSE(pPed->hasScript());
    CHECK_FALSE(pPed->isScriptLooping());

    // Add waypoints: Goto -> Wait 5s -> Proximity Ambush Trigger -> Goto
    rs_knl::ScriptStep step1;
    step1.type = LevelData::kScenarioTypeGoto;
    step1.targetPos = rs_knl::TilePoint(40, 50, 2);

    rs_knl::ScriptStep step2;
    step2.type = LevelData::kScenarioTypeWait;
    step2.targetPos = rs_knl::TilePoint(45, 50, 2);
    step2.param = 5000;

    rs_knl::ScriptStep step3;
    step3.type = LevelData::kScenarioTypeTrigger;
    step3.targetPos = rs_knl::TilePoint(50, 50, 2);
    step3.param = 6 * 256;

    rs_knl::ScriptStep step4;
    step4.type = LevelData::kScenarioTypeGoto;
    step4.targetPos = rs_knl::TilePoint(55, 55, 2);

    pPed->addScriptStep(step1);
    pPed->addScriptStep(step2);
    pPed->addScriptStep(step3);
    pPed->addScriptStep(step4);

    CHECK(pPed->hasScript());
    CHECK(pPed->scriptSteps().size() == 4);
    CHECK_FALSE(pPed->isScriptLooping());

    // Enable loop
    pPed->setScriptLooping(true);
    CHECK(pPed->isScriptLooping());
    CHECK(pPed->scriptSteps().size() == 5);
    CHECK(pPed->scriptSteps().back().type == LevelData::kScenarioTypeReset);

    // Build gameplay actions for ped
    rs_knl::MissionManager::buildScriptedActionsForPed(pMission, pPed);
    rs_knl::MovementAction *act1 = pPed->defaultAction();
    REQUIRE(act1 != nullptr);
    CHECK(dynamic_cast<rs_knl::WalkAction*>(act1) != nullptr);

    // 2. Test vehicle driver scripting
    REQUIRE(pMission->numVehicles() > 0);
    auto *pCar = dynamic_cast<rs_knl::GenericCar*>(pMission->vehicle(0));
    REQUIRE(pCar != nullptr);

    // Spawn a driver if car does not have one
    rs_knl::PedInstance *pDriver = pCar->getDriver();
    if (!pDriver) {
        pDriver = pMission->spawnPed(rs_knl::PedInstance::kPedTypeCivilian, pCar->position(), 0);
        pCar->setDriver(pDriver);
    }
    REQUIRE(pDriver != nullptr);

    // Configure 2 destination waypoints for driver
    pDriver->clearScriptSteps();
    rs_knl::ScriptStep carWp1;
    carWp1.type = LevelData::kScenarioTypeGoto;
    carWp1.targetPos = rs_knl::TilePoint(30, 40, 2);
    rs_knl::ScriptStep carWp2;
    carWp2.type = LevelData::kScenarioTypeGoto;
    carWp2.targetPos = rs_knl::TilePoint(30, 60, 2);
    pDriver->addScriptStep(carWp1);
    pDriver->addScriptStep(carWp2);
    pDriver->setScriptLooping(true);

    rs_knl::MissionManager::buildScriptedActionsForPed(pMission, pDriver);
    rs_knl::MovementAction *drvAct = pDriver->defaultAction();
    REQUIRE(drvAct != nullptr);
    CHECK(dynamic_cast<rs_knl::DriveVehicleAction*>(drvAct) != nullptr);

    // 3. Export to binary GAMEXX.DAT and verify binary format compliance
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "synd_route_test";
    std::filesystem::create_directories(tempDir);
    std::filesystem::path tempGame = tempDir / "game01.dat";

    REQUIRE(rs_knl::MissionManager::exportMissionToBinary(pMission, 1, tempGame.string()));

    // Reload exported level data and verify scenario serialization
    misMgr.destroyMission();
    misMgr.mapManager().setCustomDataDir(tempDir.string());
    LevelData::LevelDataAll exportedData;
    REQUIRE(misMgr.load_level_data(1, exportedData));

    // Pedestrian should have a valid offset_scenario_start pointing into scenarios table
    uint16_t scOff = rs_utl::READ_LE_UINT16(exportedData.people[pedSlot].offset_scenario_start);
    REQUIRE(scOff != 0);
    int scIdx = scOff / 8;
    REQUIRE(scIdx > 0);
    REQUIRE(scIdx < 2048);

    // Step 1: Goto (0x01) at (40, 50, 2)
    CHECK(exportedData.scenarios[scIdx].type == LevelData::kScenarioTypeGoto);
    CHECK((exportedData.scenarios[scIdx].tilex >> 1) == 40);
    CHECK((exportedData.scenarios[scIdx].tiley >> 1) == 50);
    CHECK(exportedData.scenarios[scIdx].tilez == 2);

    // Step 2: Wait (0x05)
    uint16_t nextOff2 = rs_utl::READ_LE_UINT16(exportedData.scenarios[scIdx].next);
    int scIdx2 = nextOff2 / 8;
    CHECK(exportedData.scenarios[scIdx2].type == LevelData::kScenarioTypeWait);

    // Step 3: Trigger (0x08)
    uint16_t nextOff3 = rs_utl::READ_LE_UINT16(exportedData.scenarios[scIdx2].next);
    int scIdx3 = nextOff3 / 8;
    CHECK(exportedData.scenarios[scIdx3].type == LevelData::kScenarioTypeTrigger);

    // Step 4: Goto (0x01)
    uint16_t nextOff4 = rs_utl::READ_LE_UINT16(exportedData.scenarios[scIdx3].next);
    int scIdx4 = nextOff4 / 8;
    CHECK(exportedData.scenarios[scIdx4].type == LevelData::kScenarioTypeGoto);

    // Step 5: Loop / Reset (0x09)
    uint16_t nextOff5 = rs_utl::READ_LE_UINT16(exportedData.scenarios[scIdx4].next);
    int scIdx5 = nextOff5 / 8;
    CHECK(exportedData.scenarios[scIdx5].type == LevelData::kScenarioTypeReset);

    // Re-create mission from reloaded binary and verify scriptSteps parsed back accurately
    rs_knl::Mission *pReloadedMis = misMgr.create_mission(exportedData);
    REQUIRE(pReloadedMis != nullptr);
    REQUIRE(pReloadedMis->buildNavigationGraph());
    rs_knl::PedInstance *pReloadedPed = nullptr;
    for (size_t i = 0; i < pReloadedMis->numPeds(); ++i) {
        if (pReloadedMis->ped(i)->id() == pedSlot) {
            pReloadedPed = pReloadedMis->ped(i);
            break;
        }
    }
    REQUIRE(pReloadedPed != nullptr);
    CHECK(pReloadedPed->scriptSteps().size() == 5);
    CHECK(pReloadedPed->isScriptLooping());
    CHECK(pReloadedPed->scriptSteps()[0].targetPos.tx == 40);
    CHECK(pReloadedPed->scriptSteps()[0].targetPos.ty == 50);
    REQUIRE(pReloadedPed->currentAction() != nullptr);
    printf("pReloadedPed currentAction type: %d, isFinished: %d\n",
        pReloadedPed->currentAction()->type(), pReloadedPed->currentAction()->isFinished());
    auto startPos = pReloadedPed->position();
    for (int t = 0; t < 100; ++t) {
        pReloadedPed->executeAction(100, pReloadedMis);
    }
    auto endPos = pReloadedPed->position();
    printf("startPos: (%d, %d), endPos: (%d, %d)\n", startPos.tx, startPos.ty, endPos.tx, endPos.ty);

    misMgr.mapManager().setCustomDataDir("");
    std::error_code ecClean;
    std::filesystem::remove_all(tempDir, ecClean);
}

TEST_CASE("AmbulancePathTest", "[AmbulancePathTest]") {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(2, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());

    rs_knl::TilePoint pt1(36, 53, 2);
    rs_knl::TilePoint pt2(39, 56, 2);
    rs_knl::TilePoint dest(55, 48, 2);
    rs_knl::TilePoint pt2_lane(39, 55, 1);
    auto path2_lane = rs_knl::GenericCar::calculateRoadPath(pMission->map(), pt2_lane, dest);
    printf("PATH2_LANE size: %zu\n", path2_lane.size());

    for (int y = 52; y <= 57; ++y) {
        for (int x = 35; x <= 40; ++x) {
            for (int z = 0; z <= 2; ++z) {
                if (pMission->map()->isTileWalkableByCar(x, y, z)) {
                    printf("TILE (%d,%d,z=%d): conn=0x%02X\n", x, y, z,
                        pMission->map()->getPossibleConnexionsForRoadTile(rs_knl::TilePoint(x, y, z)));
                }
            }
        }
    }

    auto path1 = rs_knl::GenericCar::calculateRoadPath(pMission->map(), pt1, dest);
    printf("PATH1 size: %zu\n", path1.size());
    for (size_t i = 0; i < path1.size(); ++i) {
        printf("  p1[%zu]: (%d,%d,%d, ox=%d, oy=%d)\n", i, path1[i].tx, path1[i].ty, path1[i].tz, path1[i].ox, path1[i].oy);
    }

    auto path2 = rs_knl::GenericCar::calculateRoadPath(pMission->map(), pt2, dest);
    printf("PATH2 size: %zu\n", path2.size());

    // Test Mission 2 driver movement:
    // Car 6 driver is Ped 46
    auto *car6 = dynamic_cast<rs_knl::GenericCar*>(pMission->vehicle(6));
    REQUIRE(car6 != nullptr);
    auto *driver46 = car6->getDriver();
    printf("Car 6 initial pos: (%d,%d,%d), driver=%p (id=%d)\n",
        car6->tileX(), car6->tileY(), car6->tileZ(), driver46, driver46 ? driver46->id() : -1);
    if (driver46) {
        printf("Driver 46 inVehicle: %p, currentAction: %p\n",
            driver46->inVehicle(), driver46->currentAction());
        if (driver46->currentAction()) {
            printf("  action type: %d, isFinished: %d, hasFailed: %d\n",
                driver46->currentAction()->type(),
                driver46->currentAction()->isFinished(),
                driver46->currentAction()->hasFailed());
        }
    }

    // Tick driver46 and car6 10 times
    for (int t = 0; t < 10; ++t) {
        driver46->executeAction(100, pMission);
        car6->doMove(100);
    }
    printf("After 10 ticks, Car 6 pos: (%d,%d,%d), speed: %d\n",
        car6->tileX(), car6->tileY(), car6->tileZ(), car6->speed());
    if (driver46 && driver46->currentAction()) {
        printf("Driver 46 currentAction type: %d, isFinished: %d, hasFailed: %d\n",
            driver46->currentAction()->type(),
            driver46->currentAction()->isFinished(),
            driver46->currentAction()->hasFailed());
    } else {
        printf("Driver 46 currentAction is NULL!\n");
    }

    // Test Ped Boarding:
    // Spawn civilian at (37, 52, 2)
    rs_knl::PedInstance *pBoarder = pMission->spawnPed(
        rs_knl::PedInstance::kPedTypeCivilian, rs_knl::TilePoint(37, 52, 2), 0);
    REQUIRE(pBoarder != nullptr);
    std::vector<rs_knl::ScriptStep> boarderSteps;
    rs_knl::ScriptStep step0;
    step0.type = LevelData::kScenarioTypeGoto;
    step0.targetPos = rs_knl::TilePoint(36, 52, 2);
    boarderSteps.push_back(step0);
    rs_knl::ScriptStep step1;
    step1.type = LevelData::kScenarioTypeUseVehicle;
    step1.targetEntityId = 6; // Car 6
    boarderSteps.push_back(step1);
    pBoarder->setScriptSteps(boarderSteps);
    rs_knl::MissionManager::buildScriptedActionsForPed(pMission, pBoarder);

    printf("pBoarder currentAction: %p\n", pBoarder->currentAction());
    for (int t = 0; t < 100; ++t) {
        pBoarder->executeAction(100, pMission);
        if (pBoarder->inVehicle() != nullptr) {
            printf("pBoarder boarded at tick %d!\n", t);
            break;
        }
    }
    printf("pBoarder final inVehicle: %p, currentAction: %p\n",
        pBoarder->inVehicle(), pBoarder->currentAction());
    if (pBoarder->currentAction()) {
        printf("  currentAction type: %d, hasFailed: %d, isFinished: %d\n",
            pBoarder->currentAction()->type(),
            pBoarder->currentAction()->hasFailed(),
            pBoarder->currentAction()->isFinished());
    }

    delete pMission;
}

TEST_CASE("Mission1AmbushAndOpponentBehaviorTest", "[Mission1AmbushAndOpponentBehaviorTest]") {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(1, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());

    // Find Ped 11 in Mission 1
    rs_knl::PedInstance *pPed11 = nullptr;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        if (pMission->ped(i)->id() == 11) {
            pPed11 = pMission->ped(i);
            break;
        }
    }
    REQUIRE(pPed11 != nullptr);
    CHECK(pPed11->type() == rs_knl::PedInstance::kPedTypeGuard);

    // Initial position of Ped 11 is (71, 40, 2)
    CHECK(pPed11->tileX() == 71);
    CHECK(pPed11->tileY() == 40);

    // Get player squad member 0 (starts unarmed)
    rs_knl::PedInstance *pAgent = pMission->getSquad()->member(0);
    REQUIRE(pAgent != nullptr);
    pAgent->deselectWeapon();
    CHECK(!pAgent->isArmed());

    // Position agent at (88, 40, 2) - behind the wall, out of line-of-sight
    pAgent->setPosition(88, 40, 2, 128, 128, 0);

    // Tick mission: Ped 11 executes Step 0 (Goto 71,40) and enters TriggerAction (Step 1)
    for (int t = 0; t < 10; ++t) {
        pPed11->executeAction(100, pMission);
    }
    // Should NOT have engaged combat yet because agent is behind the wall
    CHECK(!pPed11->behaviour().hasHostileEngaged());

    // Now move agent into the open line-of-sight corridor at (84, 40, 2), still unarmed!
    pAgent->setPosition(84, 40, 2, 128, 128, 0);

    // Tick Ped 11: TriggerAction should evaluate line-of-sight and trigger combat engagement
    for (int t = 0; t < 10; ++t) {
        pPed11->executeAction(100, pMission);
    }
    // Ambush triggered! Ped 11 should have drawn a weapon and engaged in hostile combat even though agent is unarmed
    CHECK(pPed11->behaviour().hasHostileEngaged());
    CHECK(pPed11->isArmed());

    delete pMission;
}

TEST_CASE("Mission1TargetPed8AmbushBehaviorTest", "[Mission1TargetPed8AmbushBehaviorTest]") {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(1, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());
    misMgr.setMission(pMission);

    rs_knl::PedInstance *pPed8 = nullptr;
    rs_knl::PedInstance *pPed12 = nullptr;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        if (pMission->ped(i)->id() == 8) pPed8 = pMission->ped(i);
        if (pMission->ped(i)->id() == 12) pPed12 = pMission->ped(i);
    }
    REQUIRE(pPed8 != nullptr);
    REQUIRE(pPed12 != nullptr);

    // Guard 12 (unscripted guard) must remain stationary at start position, no movement actions assigned
    CHECK(pPed12->type() == rs_knl::PedInstance::kPedTypeGuard);
    CHECK(pPed12->tileX() == 50);
    CHECK(pPed12->tileY() == 54);
    CHECK(pPed12->currentAction() == nullptr);

    // Target Ped 8 starts with script steps and initial Goto (Step 0)
    CHECK(pPed8->type() == rs_knl::PedInstance::kPedTypeCivilian);
    CHECK(pPed8->tileX() == 64);
    CHECK(pPed8->tileY() == 30);
    REQUIRE(pPed8->currentAction() != nullptr);

    // Advance Ped 8 through initial position arrival into TriggerAction (Step 1)
    for (int t = 0; t < 20; ++t) {
        pPed8->executeAction(100, pMission);
    }
    REQUIRE(pPed8->currentAction() != nullptr);
    CHECK(pPed8->tileX() == 64);
    CHECK(pPed8->tileY() == 30);

    // Move player agent into the ambush trigger radius at (63, 33)
    rs_knl::PedInstance *pAgent = pMission->getSquad()->member(0);
    REQUIRE(pAgent != nullptr);
    pAgent->setPosition(63, 33, 2, 128, 128, 0);

    // Ticking Ped 8 now triggers the ambush! Ped 8 must flee towards the getaway car (Car 0)
    for (int t = 0; t < 20; ++t) {
        pPed8->executeAction(100, pMission);
    }
    // Ped 8 has left start position and is moving towards Car 0
    CHECK(pPed8->tileY() > 30);
    CHECK(pPed8->currentAction() != nullptr);

    // Ticking mission simulation allows Ped 8 to enter the car and drive away
    for (int t = 0; t < 200; ++t) {
        pMission->handleTick(100, 100);
    }
    // Ped 8 has boarded the getaway car and driven away down the street
    CHECK(pPed8->inVehicle() != nullptr);
    CHECK(pPed8->tileX() < 60);

    delete pMission;
}

TEST_CASE("Mission2PedBoardingTest", "[Mission2PedBoardingTest]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);

    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(2, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());

    rs_knl::PedInstance *pPed216 = nullptr;
    for (size_t pi = 0; pi < pMission->numPeds(); ++pi) {
        if (pMission->ped(pi)->id() == 216) {
            pPed216 = pMission->ped(pi);
            break;
        }
    }
    REQUIRE(pPed216 != nullptr);

    // Spawn vehicle #007 at (38, 53, 2)
    rs_knl::TilePoint carPos(38, 53, 2, 128, 128, 0);
    rs_knl::Vehicle *pCar = pMission->spawnVehicle(rs_knl::Vehicle::kVehicleTypeRegularCar, carPos, 64);
    REQUIRE(pCar != nullptr);

    // Find car index
    int carIdx = -1;
    for (size_t vi = 0; vi < pMission->numVehicles(); ++vi) {
        if (pMission->vehicle(vi) == pCar) {
            carIdx = static_cast<int>(vi);
            break;
        }
    }

    std::vector<rs_knl::ScriptStep> steps;
    rs_knl::ScriptStep s0;
    s0.type = LevelData::kScenarioTypeGoto;
    s0.targetPos = rs_knl::TilePoint(41, 53, 2, 128, 128, 0);
    steps.push_back(s0);

    rs_knl::ScriptStep s1;
    s1.type = LevelData::kScenarioTypeUseVehicle;
    s1.targetEntityId = carIdx;
    s1.targetPos = rs_knl::TilePoint(0, 0, 0);
    steps.push_back(s1);

    pPed216->setScriptSteps(steps);
    rs_knl::MissionManager::buildScriptedActionsForPed(pMission, pPed216);

    // Now test with export and reload (as Playtest does)!
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "synd_playtest_board_test";
    std::filesystem::create_directories(tempDir);
    std::filesystem::path tempGame = tempDir / "game02.dat";

    REQUIRE(rs_knl::MissionManager::exportMissionToBinary(pMission, 2, tempGame.string()));

    misMgr.destroyMission();
    misMgr.mapManager().setCustomDataDir(tempDir.string());
    LevelData::LevelDataAll exportedData;
    REQUIRE(misMgr.load_level_data(2, exportedData));

    rs_knl::Mission *pReloaded = misMgr.create_mission(exportedData);
    REQUIRE(pReloaded != nullptr);
    REQUIRE(pReloaded->buildNavigationGraph());

    rs_knl::PedInstance *pReloadedPed = nullptr;
    for (size_t pi = 0; pi < pReloaded->numPeds(); ++pi) {
        if (pReloaded->ped(pi)->id() == 216) {
            pReloadedPed = pReloaded->ped(pi);
            break;
        }
    }
    REQUIRE(pReloadedPed != nullptr);
    REQUIRE(pReloadedPed->scriptSteps().size() >= 2);
    CHECK(pReloadedPed->scriptSteps()[1].type == LevelData::kScenarioTypeUseVehicle);
    CHECK(pReloadedPed->scriptSteps()[1].targetEntityId == carIdx);

    for (int t = 0; t < 200; ++t) {
        pReloadedPed->executeAction(100, pReloaded);
        if (pReloadedPed->inVehicle() != nullptr) {
            break;
        }
    }
    CHECK(pReloadedPed->inVehicle() != nullptr);

    misMgr.mapManager().setCustomDataDir("");
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    delete pReloaded;
    delete pMission;

    // Test Scenario 2: Goto waypoint on vehicle AND boarding action in the same spot
    {
        LevelData::LevelDataAll levelData2;
        REQUIRE(misMgr.load_level_data(2, levelData2));
        rs_knl::Mission *pMission2 = misMgr.create_mission(levelData2);
        REQUIRE(pMission2 != nullptr);
        REQUIRE(pMission2->buildNavigationGraph());

        rs_knl::TilePoint carPos(38, 53, 2, 128, 128, 0);
        rs_knl::Vehicle *pCar2 = pMission2->spawnVehicle(rs_knl::Vehicle::kVehicleTypeRegularCar, carPos, 64);
        REQUIRE(pCar2 != nullptr);

        int carIdx2 = -1;
        for (size_t vi = 0; vi < pMission2->numVehicles(); ++vi) {
            if (pMission2->vehicle(vi) == pCar2) {
                carIdx2 = static_cast<int>(vi);
                break;
            }
        }

        rs_knl::PedInstance *pPed2 = nullptr;
        for (size_t pi = 0; pi < pMission2->numPeds(); ++pi) {
            if (pMission2->ped(pi)->id() == 216) {
                pPed2 = pMission2->ped(pi);
                break;
            }
        }
        REQUIRE(pPed2 != nullptr);

        std::vector<rs_knl::ScriptStep> steps2;
        rs_knl::ScriptStep s0;
        s0.type = LevelData::kScenarioTypeGoto;
        s0.targetPos = rs_knl::TilePoint(41, 53, 2, 128, 128, 0);
        steps2.push_back(s0);

        rs_knl::ScriptStep s1;
        s1.type = LevelData::kScenarioTypeGoto;
        s1.targetPos = rs_knl::TilePoint(38, 53, 2, 128, 128, 0);
        steps2.push_back(s1);

        rs_knl::ScriptStep s2;
        s2.type = LevelData::kScenarioTypeUseVehicle;
        s2.targetEntityId = carIdx2;
        s2.targetPos = rs_knl::TilePoint(38, 53, 2, 128, 128, 0); // Waypoint position kept on map
        steps2.push_back(s2);

        pPed2->setScriptSteps(steps2);
        rs_knl::MissionManager::buildScriptedActionsForPed(pMission2, pPed2);

        std::filesystem::path tempDir2 = std::filesystem::temp_directory_path() / "synd_playtest_board_test2";
        std::filesystem::create_directories(tempDir2);
        std::filesystem::path tempGame2 = tempDir2 / "game02.dat";

        REQUIRE(rs_knl::MissionManager::exportMissionToBinary(pMission2, 2, tempGame2.string()));

        misMgr.destroyMission();
        misMgr.mapManager().setCustomDataDir(tempDir2.string());
        LevelData::LevelDataAll exportedData2;
        REQUIRE(misMgr.load_level_data(2, exportedData2));
        rs_knl::Mission *pReloaded2 = misMgr.create_mission(exportedData2);
        REQUIRE(pReloaded2 != nullptr);
        REQUIRE(pReloaded2->buildNavigationGraph());

        rs_knl::PedInstance *pReloadedPed2 = nullptr;
        for (size_t pi = 0; pi < pReloaded2->numPeds(); ++pi) {
            if (pReloaded2->ped(pi)->id() == 216) {
                pReloadedPed2 = pReloaded2->ped(pi);
                break;
            }
        }
        REQUIRE(pReloadedPed2 != nullptr);
        for (int t = 0; t < 200; ++t) {
            pReloadedPed2->executeAction(100, pReloaded2);
            if (pReloadedPed2->inVehicle() != nullptr) {
                break;
            }
        }
        CHECK(pReloadedPed2->inVehicle() != nullptr);

        misMgr.mapManager().setCustomDataDir("");
        std::filesystem::remove_all(tempDir2, ec);

        delete pReloaded2;
        delete pMission2;
    }
}

TEST_CASE("MultiModalPedRouteTransitionsTest", "[mission]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();

    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(2, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());

    // 1. Spawn Car 1 and Car 2
    rs_knl::TilePoint car1Pos(38, 53, 2, 128, 128, 0);
    rs_knl::Vehicle *pCar1 = pMission->spawnVehicle(rs_knl::Vehicle::kVehicleTypeRegularCar, car1Pos, 64);
    REQUIRE(pCar1 != nullptr);
    int car1Idx = -1;
    for (size_t vi = 0; vi < pMission->numVehicles(); ++vi) {
        if (pMission->vehicle(vi) == pCar1) { car1Idx = static_cast<int>(vi); break; }
    }
    REQUIRE(car1Idx >= 0);

    rs_knl::TilePoint car2Pos(46, 53, 2, 128, 128, 0);
    rs_knl::Vehicle *pCar2 = pMission->spawnVehicle(rs_knl::Vehicle::kVehicleTypeRegularCar, car2Pos, 64);
    REQUIRE(pCar2 != nullptr);
    int car2Idx = -1;
    for (size_t vi = 0; vi < pMission->numVehicles(); ++vi) {
        if (pMission->vehicle(vi) == pCar2) { car2Idx = static_cast<int>(vi); break; }
    }
    REQUIRE(car2Idx >= 0);

    // 2. Locate Pedestrian 216
    rs_knl::PedInstance *pPed = nullptr;
    for (size_t pi = 0; pi < pMission->numPeds(); ++pi) {
        if (pMission->ped(pi)->id() == 216) {
            pPed = pMission->ped(pi);
            break;
        }
    }
    REQUIRE(pPed != nullptr);

    // 3. Construct multi-modal route:
    // Step 0: Goto (Walk on foot)
    // Step 1: UseVehicle (Board Car 1)
    // Step 2: Goto (Drive Car 1)
    // Step 3: LeaveVehicle (Unboard Car 1)
    // Step 4: Goto (Walk on foot)
    // Step 5: UseVehicle (Board Car 2)
    // Step 6: Goto (Drive Car 2)
    // Step 7: Reset (Loop route)
    std::vector<rs_knl::ScriptStep> steps;

    rs_knl::ScriptStep s0;
    s0.type = LevelData::kScenarioTypeGoto;
    s0.targetPos = rs_knl::TilePoint(41, 53, 2, 128, 128, 0);
    steps.push_back(s0);

    rs_knl::ScriptStep s1;
    s1.type = LevelData::kScenarioTypeUseVehicle;
    s1.targetEntityId = car1Idx;
    s1.targetPos = rs_knl::TilePoint(0, 0, 0);
    steps.push_back(s1);

    rs_knl::ScriptStep s2;
    s2.type = LevelData::kScenarioTypeGoto;
    s2.targetPos = rs_knl::TilePoint(43, 53, 2, 128, 128, 0);
    steps.push_back(s2);

    rs_knl::ScriptStep s3;
    s3.type = LevelData::kScenarioTypeLeaveVehicle;
    s3.targetPos = rs_knl::TilePoint(0, 0, 0);
    steps.push_back(s3);

    rs_knl::ScriptStep s4;
    s4.type = LevelData::kScenarioTypeGoto;
    s4.targetPos = rs_knl::TilePoint(45, 53, 2, 128, 128, 0);
    steps.push_back(s4);

    rs_knl::ScriptStep s5;
    s5.type = LevelData::kScenarioTypeUseVehicle;
    s5.targetEntityId = car2Idx;
    s5.targetPos = rs_knl::TilePoint(0, 0, 0);
    steps.push_back(s5);

    rs_knl::ScriptStep s6;
    s6.type = LevelData::kScenarioTypeGoto;
    s6.targetPos = rs_knl::TilePoint(48, 53, 2, 128, 128, 0);
    steps.push_back(s6);

    rs_knl::ScriptStep s7;
    s7.type = LevelData::kScenarioTypeReset;
    steps.push_back(s7);

    pPed->setScriptSteps(steps);
    rs_knl::MissionManager::buildScriptedActionsForPed(pMission, pPed);

    // Verify constructed actions match the multi-modal sequence
    rs_knl::MovementAction *act = pPed->defaultAction();
    REQUIRE(act != nullptr);
    // Step 0: WalkAction
    CHECK(dynamic_cast<rs_knl::WalkAction*>(act) != nullptr);

    // Step 1: WalkAction to car1 + EnterVehicleAction
    act = act->next();
    REQUIRE(act != nullptr);
    CHECK(dynamic_cast<rs_knl::WalkAction*>(act) != nullptr);
    act = act->next();
    REQUIRE(act != nullptr);
    CHECK(dynamic_cast<rs_knl::EnterVehicleAction*>(act) != nullptr);

    // Step 2: DriveVehicleAction with car 1
    act = act->next();
    REQUIRE(act != nullptr);
    auto *drive1 = dynamic_cast<rs_knl::DriveVehicleAction*>(act);
    REQUIRE(drive1 != nullptr);

    // Step 3: LeaveVehicleAction
    act = act->next();
    REQUIRE(act != nullptr);
    CHECK(dynamic_cast<rs_knl::LeaveVehicleAction*>(act) != nullptr);

    // Step 4: WalkAction on foot
    act = act->next();
    REQUIRE(act != nullptr);
    CHECK(dynamic_cast<rs_knl::WalkAction*>(act) != nullptr);

    // Step 5: WalkAction to car2 + EnterVehicleAction
    act = act->next();
    REQUIRE(act != nullptr);
    CHECK(dynamic_cast<rs_knl::WalkAction*>(act) != nullptr);
    act = act->next();
    REQUIRE(act != nullptr);
    CHECK(dynamic_cast<rs_knl::EnterVehicleAction*>(act) != nullptr);

    // Step 6: DriveVehicleAction with car 2
    act = act->next();
    REQUIRE(act != nullptr);
    auto *drive2 = dynamic_cast<rs_knl::DriveVehicleAction*>(act);
    REQUIRE(drive2 != nullptr);

    // Step 7: ResetScriptedAction
    act = act->next();
    REQUIRE(act != nullptr);
    CHECK(dynamic_cast<rs_knl::ResetScriptedAction*>(act) != nullptr);

    // 4. Test export and reload round-trip
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "synd_multimodal_route_test";
    std::filesystem::create_directories(tempDir);
    std::filesystem::path tempGame = tempDir / "game02.dat";

    REQUIRE(rs_knl::MissionManager::exportMissionToBinary(pMission, 2, tempGame.string()));

    misMgr.destroyMission();
    misMgr.mapManager().setCustomDataDir(tempDir.string());
    LevelData::LevelDataAll exportedData;
    REQUIRE(misMgr.load_level_data(2, exportedData));

    rs_knl::Mission *pReloaded = misMgr.create_mission(exportedData);
    REQUIRE(pReloaded != nullptr);
    REQUIRE(pReloaded->buildNavigationGraph());

    rs_knl::PedInstance *pReloadedPed = nullptr;
    for (size_t pi = 0; pi < pReloaded->numPeds(); ++pi) {
        if (pReloaded->ped(pi)->id() == 216) {
            pReloadedPed = pReloaded->ped(pi);
            break;
        }
    }
    REQUIRE(pReloadedPed != nullptr);
    REQUIRE(pReloadedPed->scriptSteps().size() == 8);
    CHECK(pReloadedPed->scriptSteps()[0].type == LevelData::kScenarioTypeGoto);
    CHECK(pReloadedPed->scriptSteps()[1].type == LevelData::kScenarioTypeUseVehicle);
    CHECK(pReloadedPed->scriptSteps()[1].targetEntityId == car1Idx);
    CHECK(pReloadedPed->scriptSteps()[2].type == LevelData::kScenarioTypeGoto);
    CHECK(pReloadedPed->scriptSteps()[3].type == LevelData::kScenarioTypeLeaveVehicle);
    CHECK(pReloadedPed->scriptSteps()[4].type == LevelData::kScenarioTypeGoto);
    CHECK(pReloadedPed->scriptSteps()[5].type == LevelData::kScenarioTypeUseVehicle);
    CHECK(pReloadedPed->scriptSteps()[5].targetEntityId == car2Idx);
    CHECK(pReloadedPed->scriptSteps()[6].type == LevelData::kScenarioTypeGoto);
    CHECK(pReloadedPed->scriptSteps()[7].type == LevelData::kScenarioTypeReset);

    misMgr.mapManager().setCustomDataDir("");
    std::error_code ec;
    std::filesystem::remove_all(tempDir, ec);

    delete pReloaded;
    delete pMission;
}

TEST_CASE("Mission2PedDriveUnboardTest", "[mission]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();

    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(2, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());
    misMgr.setMission(pMission);

    // Spawn vehicle at (41, 58, 1)
    rs_knl::TilePoint carPos(41, 58, 1, 128, 128, 0);
    rs_knl::Vehicle *pCar = pMission->spawnVehicle(rs_knl::Vehicle::kVehicleTypeRegularCar, carPos, 64);
    REQUIRE(pCar != nullptr);
    auto *genericCar = dynamic_cast<rs_knl::GenericCar*>(pCar);
    REQUIRE(genericCar != nullptr);

    int carIdx = -1;
    for (size_t vi = 0; vi < pMission->numVehicles(); ++vi) {
        if (pMission->vehicle(vi) == pCar) { carIdx = static_cast<int>(vi); break; }
    }
    REQUIRE(carIdx >= 0);

    // Find Ped 216
    rs_knl::PedInstance *pPed = nullptr;
    for (size_t pi = 0; pi < pMission->numPeds(); ++pi) {
        if (pMission->ped(pi)->id() == 216) {
            pPed = pMission->ped(pi);
            break;
        }
    }
    REQUIRE(pPed != nullptr);

    // Give ped script steps:
    // 0: Goto (41, 59, 2)
    // 1: Board Car
    // 2: Drive Car to (51, 57, 1)
    // 3: Unboard
    // 4: Goto (48, 58, 2)
    std::vector<rs_knl::ScriptStep> steps;
    rs_knl::ScriptStep s0;
    s0.type = LevelData::kScenarioTypeGoto;
    s0.targetPos = rs_knl::TilePoint(41, 59, 2, 128, 128, 0);
    steps.push_back(s0);

    rs_knl::ScriptStep s1;
    s1.type = LevelData::kScenarioTypeUseVehicle;
    s1.targetEntityId = carIdx;
    steps.push_back(s1);

    rs_knl::ScriptStep s2;
    s2.type = LevelData::kScenarioTypeGoto;
    s2.targetPos = rs_knl::TilePoint(51, 57, 1, 128, 128, 0);
    steps.push_back(s2);

    rs_knl::ScriptStep s3;
    s3.type = LevelData::kScenarioTypeLeaveVehicle;
    steps.push_back(s3);

    rs_knl::ScriptStep s4;
    s4.type = LevelData::kScenarioTypeGoto;
    s4.targetPos = rs_knl::TilePoint(48, 58, 2, 128, 128, 0);
    steps.push_back(s4);

    pPed->setScriptSteps(steps);
    rs_knl::MissionManager::buildScriptedActionsForPed(pMission, pPed);

    bool boarded = false;
    bool unboarded = false;
    for (int tick = 0; tick < 200; ++tick) {
        pMission->handleTick(100, 100);

        if (pPed->inVehicle() == genericCar) {
            boarded = true;
        }
        if (boarded && pPed->inVehicle() == nullptr) {
            unboarded = true;
            // Verify unboarding placed ped on a valid walkable tile
            CHECK(pMission->isTileWalkable(pPed->tileX(), pPed->tileY(), pPed->tileZ()));
            break;
        }
    }

    CHECK(boarded);
    CHECK(unboarded);
    CHECK(pPed->inVehicle() == nullptr);
    CHECK(genericCar->tileX() == 51);
    CHECK(genericCar->tileY() == 57);

    delete pMission;
}

TEST_CASE("Blank Mission Export Preserves Valid Map Scroll Bounds", "[kernel][mission][export]") {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    // Create a mission with an allocated map (128x96x12) but zeroed raw level data
    LevelData::LevelDataAll emptyData{};
    LevelData::MapInfos mapInfos{};
    rs_utl::WRITE_LE_UINT16(mapInfos.map, 99);
    // Leave mapInfos min_x/min_y/max_x/max_y as 0 to simulate blank canvas

    rs_knl::Map *pMap = new rs_knl::Map(&tileMgr, 99);
    int nbTiles = 128 * 96 * 12;
    rs_eng::Tile **tiles = new rs_eng::Tile*[nbTiles];
    for (int i = 0; i < nbTiles; ++i) {
        tiles[i] = tileMgr.getTile(0);
    }
    pMap->setTiles(128, 96, 12, tiles);
    delete[] tiles;

    rs_knl::Mission *pMission = new rs_knl::Mission();
    REQUIRE(pMission->init(pMap));
    pMission->setRawLevelData(emptyData);

    // Spawn 4 squad agents
    pMission->spawnPlayerAgent(0, rs_knl::TilePoint(64, 48, 2), 0);
    pMission->spawnPlayerAgent(1, rs_knl::TilePoint(65, 48, 2), 0);
    pMission->spawnPlayerAgent(2, rs_knl::TilePoint(64, 49, 2), 0);
    pMission->spawnPlayerAgent(3, rs_knl::TilePoint(65, 49, 2), 0);

    std::filesystem::path tempGameFile = std::filesystem::temp_directory_path() / "test_game99_scroll.dat";
    REQUIRE(rs_knl::MissionManager::exportMissionToBinary(pMission, 99, tempGameFile.string()));

    LevelData::LevelDataAll reloadedData{};
    REQUIRE(misMgr.load_level_data(99, reloadedData, tempGameFile.string()));

    uint16_t minX = rs_utl::READ_LE_UINT16(reloadedData.mapinfos.min_x);
    uint16_t minY = rs_utl::READ_LE_UINT16(reloadedData.mapinfos.min_y);
    uint16_t maxX = rs_utl::READ_LE_UINT16(reloadedData.mapinfos.max_x);
    uint16_t maxY = rs_utl::READ_LE_UINT16(reloadedData.mapinfos.max_y);

    CHECK(minX == 0);
    CHECK(minY == 0);
    CHECK(maxX == (128 - 1) * 2);
    CHECK(maxY == (96 - 1) * 2);

    std::filesystem::remove(tempGameFile);
    delete pMission;
}

TEST_CASE("PlayerAgentDriveVehicleTest", "[mission][vehicle]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();

    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(1, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());
    misMgr.setMission(pMission);

    auto *car0 = dynamic_cast<rs_knl::GenericCar*>(pMission->vehicle(0));
    REQUIRE(car0 != nullptr);
    REQUIRE(car0->isCar());
    // In Mission 1, car0 starts parked off-road in a stall at (60, 35, 2)
    REQUIRE(car0->tileX() == 60);
    REQUIRE(car0->tileY() == 35);
    REQUIRE(car0->tileZ() == 2);

    auto *agent0 = pMission->getSquad()->member(0);
    auto *agent1 = pMission->getSquad()->member(1);
    REQUIRE(agent0 != nullptr);
    REQUIRE(agent1 != nullptr);

    // Position agents adjacent to car0
    agent0->setPosition(60, 36, 2);
    agent1->setPosition(60, 36, 2);

    // Agent 0 boards car0
    agent0->addMovementAction(agent0->createActionEnterVehicle(car0), false);
    for (int step = 0; step < 20; ++step) {
        pMission->handleTick(step * 50, 50);
        if (agent0->inVehicle() == car0) break;
    }
    REQUIRE(agent0->inVehicle() == car0);
    // Agent 0 is the first to enter, so Agent 0 becomes the driver
    REQUIRE(car0->getDriver() == agent0);

    // Agent 1 boards car0 as passenger
    agent1->addMovementAction(agent1->createActionEnterVehicle(car0), false);
    for (int step = 0; step < 20; ++step) {
        pMission->handleTick(1000 + step * 50, 50);
        if (agent1->inVehicle() == car0) break;
    }
    REQUIRE(agent1->inVehicle() == car0);
    // Agent 0 remains driver; Agent 1 is a passenger
    REQUIRE(car0->getDriver() == agent0);
    REQUIRE(car0->isDriver(agent0));
    REQUIRE_FALSE(car0->isDriver(agent1));

    // Driver drives car towards destination on the road network
    rs_knl::TilePoint driveTarget(60, 45, 2);
    agent0->addActionDriveVehicle(car0, driveTarget, false);

    int startY = car0->tileY();
    bool moved = false;
    for (int step = 0; step < 100; ++step) {
        pMission->handleTick(2000 + step * 50, 50);
        if (car0->tileY() != startY) {
            moved = true;
            break;
        }
    }
    REQUIRE(moved);
    // Verify both agents remained inside the car while driving
    REQUIRE(agent0->inVehicle() == car0);
    REQUIRE(agent1->inVehicle() == car0);

    // Both agents alight the vehicle cleanly
    car0->dropPassenger(agent0);
    car0->dropPassenger(agent1);
    CHECK(agent0->inVehicle() == nullptr);
    CHECK(agent1->inVehicle() == nullptr);
    CHECK(car0->getDriver() == nullptr);

    delete pMission;
}

TEST_CASE("LargeDoorAnimationOrientationTest", "[mission][gate]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();

    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(3, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());
    misMgr.setMission(pMission);

    // In Mission 3, static 53 is the LargeDoor at (40, 32, 2)
    rs_knl::LargeDoor *gate = nullptr;
    for (size_t i = 0; i < pMission->numStatics(); ++i) {
        auto *s = pMission->statics(i);
        if (s && s->type() == rs_knl::Static::smt_LargeDoor) {
            gate = dynamic_cast<rs_knl::LargeDoor *>(s);
            break;
        }
    }
    REQUIRE(gate != nullptr);
    REQUIRE(gate->tileX() == 40);
    REQUIRE(gate->tileY() == 32);
    REQUIRE(gate->tileZ() == 2);
    REQUIRE(gate->orientation() == rs_knl::Static::kStaticOrientationEW);
    REQUIRE(gate->baseAnim() == 392);
    REQUIRE(gate->doorState() == rs_knl::Static::kStateDoorClosed);

    // Verify covered tiles were detected and registered
    REQUIRE(gate->coveredTiles().size() == 3);
    for (int idx : gate->coveredTiles()) {
        REQUIRE((pMission->mdpoints_[idx].flags & rs_knl::kWalkable) == 0);
    }

    // 1. Closed gate acts as a barrier: pathfinding must navigate AROUND the gate
    auto *ag0 = pMission->getSquad()->member(0);
    ag0->setPosition(38, 31, 2);
    bool pathFound = ag0->initMovementToDestination(pMission, rs_knl::TilePoint(45, 31, 2));
    REQUIRE(pathFound);
    REQUIRE(!ag0->destPath().empty());

    // Verify the path does not pass through the closed gate at (40, 31..33)
    bool detourAroundGate = false;
    for (const auto &wp : ag0->destPath()) {
        if (wp.tx == 40) {
            REQUIRE(wp.ty != 31);
            REQUIRE(wp.ty != 32);
            REQUIRE(wp.ty != 33);
        }
        if (wp.ty >= 34) {
            detourAroundGate = true;
        }
    }
    REQUIRE(detourAroundGate);

    // Agent executes the walk around the gate to reach the other side
    ag0->setPosition(38, 31, 2);
    ag0->addActionWalk(rs_knl::TilePoint(45, 31, 2), false);
    for (int tick = 0; tick < 400; ++tick) {
        pMission->handleTick(tick * 50, 50);
        if (ag0->tileX() >= 44) break;
    }
    REQUIRE(ag0->tileX() >= 44);

    // 2. When a vehicle approaches, the gate opens and stays open
    auto *veh0 = pMission->vehicle(0);
    REQUIRE(veh0 != nullptr);
    veh0->setPosition(38, 32, 2);
    pMission->handleTick(400 * 50, 50);
    REQUIRE((gate->doorState() == rs_knl::Static::kStateDoorOpening || gate->doorState() == rs_knl::Static::kStateDoorOpen));
    gate->setStateTo(rs_knl::Static::kStateDoorOpen); // Once opened

    // When open, the gate tiles must be restored as walkable in the navigation graph
    for (int idx : gate->coveredTiles()) {
        REQUIRE((pMission->mdpoints_[idx].flags & rs_knl::kWalkable) == rs_knl::kWalkable);
    }

    // Pedestrian can now path DIRECTLY through the open gate without detouring
    ag0->setPosition(38, 31, 2);
    pathFound = ag0->initMovementToDestination(pMission, rs_knl::TilePoint(45, 31, 2));
    REQUIRE(pathFound);
    // With gate open, path smoothing collapses the unobstructed line straight along y=31
    REQUIRE(!ag0->destPath().empty());
    for (const auto &wp : ag0->destPath()) {
        REQUIRE(wp.ty == 31);
    }
    ag0->addActionWalk(rs_knl::TilePoint(45, 31, 2), false);
    for (int tick = 1; tick < 200; ++tick) {
        pMission->handleTick((400 + tick) * 50, 50);
        if (ag0->tileX() >= 44) break;
    }
    REQUIRE(ag0->tileX() >= 44);

    // 3. When gate closes again, covered tiles become non-walkable barriers once more
    gate->setStateTo(rs_knl::Static::kStateDoorClosed);
    for (int idx : gate->coveredTiles()) {
        REQUIRE((pMission->mdpoints_[idx].flags & rs_knl::kWalkable) == 0);
    }

    // Initial closed animation frame: must be EW closed (392)
    REQUIRE(gate->animationPlayer()->currentFramesAnimation() == 392);

    // Opening animation: must be EW opening (393), NOT NS opening (396)
    gate->setStateTo(rs_knl::Static::kStateDoorOpening);
    REQUIRE(gate->doorState() == rs_knl::Static::kStateDoorOpening);
    REQUIRE(gate->animationPlayer()->currentFramesAnimation() == 393);

    // Closing animation: must be EW closing (394)
    gate->setStateTo(rs_knl::Static::kStateDoorClosing);
    REQUIRE(gate->doorState() == rs_knl::Static::kStateDoorClosing);
    REQUIRE(gate->animationPlayer()->currentFramesAnimation() == 394);

    // Re-closing
    gate->setStateTo(rs_knl::Static::kStateDoorClosed);
    REQUIRE(gate->doorState() == rs_knl::Static::kStateDoorClosed);
    REQUIRE(gate->animationPlayer()->currentFramesAnimation() == 392);

    // Test changing orientation to NS
    gate->setOrientation(rs_knl::Static::kStaticOrientationNS);
    REQUIRE(gate->orientation() == rs_knl::Static::kStaticOrientationNS);
    REQUIRE(gate->baseAnim() == 395);
    gate->setStateTo(rs_knl::Static::kStateDoorClosed);
    REQUIRE(gate->animationPlayer()->currentFramesAnimation() == 395);

    gate->setStateTo(rs_knl::Static::kStateDoorOpening);
    REQUIRE(gate->doorState() == rs_knl::Static::kStateDoorOpening);
    REQUIRE(gate->animationPlayer()->currentFramesAnimation() == 396);

    gate->setStateTo(rs_knl::Static::kStateDoorClosing);
    REQUIRE(gate->doorState() == rs_knl::Static::kStateDoorClosing);
    REQUIRE(gate->animationPlayer()->currentFramesAnimation() == 397);

    // Test changing back to EW
    gate->setOrientation(rs_knl::Static::kStaticOrientationEW);
    REQUIRE(gate->orientation() == rs_knl::Static::kStaticOrientationEW);
    REQUIRE(gate->baseAnim() == 392);
    gate->setStateTo(rs_knl::Static::kStateDoorOpening);
    REQUIRE(gate->animationPlayer()->currentFramesAnimation() == 393);

    // Standalone instances
    rs_knl::LargeDoor standaloneEW(998, pMission->map(), 392);
    REQUIRE(standaloneEW.orientation() == rs_knl::Static::kStaticOrientationEW);
    REQUIRE(standaloneEW.baseAnim() == 392);
    REQUIRE(standaloneEW.animationPlayer()->currentFramesAnimation() == 392);
    standaloneEW.setStateTo(rs_knl::Static::kStateDoorOpening);
    REQUIRE(standaloneEW.animationPlayer()->currentFramesAnimation() == 393);

    rs_knl::LargeDoor standaloneNS(999, pMission->map(), 395);
    REQUIRE(standaloneNS.orientation() == rs_knl::Static::kStaticOrientationNS);
    REQUIRE(standaloneNS.baseAnim() == 395);
    REQUIRE(standaloneNS.animationPlayer()->currentFramesAnimation() == 395);
    standaloneNS.setStateTo(rs_knl::Static::kStateDoorOpening);
    REQUIRE(standaloneNS.animationPlayer()->currentFramesAnimation() == 396);

    delete pMission;
}

TEST_CASE("WindowObjAnimationOrientationAndDamageTest", "[static][window]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);

    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(3, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);

    // 1. NS window (550)
    rs_knl::WindowObj winNS(1001, pMission->map(), rs_knl::WindowObj::kStateWindowClosed, 550);
    winNS.setOrientation(rs_knl::Static::kStaticOrientationNS);
    REQUIRE(winNS.orientation() == rs_knl::Static::kStaticOrientationNS);
    REQUIRE(winNS.baseAnim() == 550);

    // 2. EW window (551)
    rs_knl::WindowObj winEW(1002, pMission->map(), rs_knl::WindowObj::kStateWindowClosed, 550);
    winEW.setOrientation(rs_knl::Static::kStaticOrientationEW);
    REQUIRE(winEW.orientation() == rs_knl::Static::kStaticOrientationEW);
    REQUIRE(winEW.baseAnim() == 551);

    // Damage EW window with bullets -> plays EW breakingAnim (555), NOT NS breakingAnim (554)
    winEW.setHealth(1);
    winEW.setStartHealth(1);
    rs_knl::DamageToInflict bulletDmg{};
    bulletDmg.dtype = rs_knl::kDmgTypeBullet;
    bulletDmg.dvalue = 10;
    winEW.handleHit(bulletDmg);
    REQUIRE(winEW.isDead());
    REQUIRE_FALSE(winEW.canBlock());
    REQUIRE(winEW.animationPlayer()->currentFramesAnimation() == 555);

    // Ending breaking animation transitions to EW damagedAnim (557), NOT NS damagedAnim (556)
    winEW.handleAnimationEnded();
    REQUIRE(winEW.animationPlayer()->currentFramesAnimation() == 557);

    // Laser damage test on NS window -> plays NS breakingAnim (554)
    winNS.setHealth(1);
    winNS.setStartHealth(1);
    rs_knl::DamageToInflict laserDmg{};
    laserDmg.dtype = rs_knl::kDmgTypeLaser;
    laserDmg.dvalue = 10;
    winNS.handleHit(laserDmg);
    REQUIRE(winNS.isDead());
    REQUIRE_FALSE(winNS.canBlock());
    REQUIRE(winNS.animationPlayer()->currentFramesAnimation() == 554);

    // Ending breaking animation transitions to NS damagedAnim (556)
    winNS.handleAnimationEnded();
    REQUIRE(winNS.animationPlayer()->currentFramesAnimation() == 556);

    delete pMission;
}

TEST_CASE("DoorOrientationAndAnimationTest", "[static][door]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);

    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(3, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);

    // NS door (famBase = 1077 is NS base)
    rs_knl::Door doorNS(1003, pMission->map(), 1077, rs_knl::Static::kStateDoorClosed);
    REQUIRE(doorNS.orientation() == rs_knl::Static::kStaticOrientationNS);
    REQUIRE(doorNS.baseAnim() == 1077);
    REQUIRE(doorNS.animationPlayer()->currentFramesAnimation() == 1077);

    // Switch doorNS to EW
    doorNS.setOrientation(rs_knl::Static::kStaticOrientationEW);
    REQUIRE(doorNS.orientation() == rs_knl::Static::kStaticOrientationEW);
    REQUIRE(doorNS.baseAnim() == 1078);
    REQUIRE(doorNS.animationPlayer()->currentFramesAnimation() == 1078);

    // Switch doorNS back to NS
    doorNS.setOrientation(rs_knl::Static::kStaticOrientationNS);
    REQUIRE(doorNS.orientation() == rs_knl::Static::kStaticOrientationNS);
    REQUIRE(doorNS.baseAnim() == 1077);
    REQUIRE(doorNS.animationPlayer()->currentFramesAnimation() == 1077);

    // EW door constructor (famBase + 1 = 1078)
    rs_knl::Door doorEW(1004, pMission->map(), 1078, rs_knl::Static::kStateDoorClosed);
    REQUIRE(doorEW.orientation() == rs_knl::Static::kStaticOrientationEW);
    REQUIRE(doorEW.baseAnim() == 1078);

    delete pMission;
}

TEST_CASE("NeonSignAndAnimWindowOrientationTest", "[static][neon_window]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);

    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(3, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);

    // NeonSign with NS base anim 560
    rs_knl::NeonSign neon(1005, pMission->map(), 560, 0);
    neon.setOrientation(rs_knl::Static::kStaticOrientationNS);
    REQUIRE(neon.orientation() == rs_knl::Static::kStaticOrientationNS);
    REQUIRE(neon.baseAnim() == 560);

    // Switch to EW: must become 561
    neon.setOrientation(rs_knl::Static::kStaticOrientationEW);
    REQUIRE(neon.orientation() == rs_knl::Static::kStaticOrientationEW);
    REQUIRE(neon.baseAnim() == 561);

    // Switch back to NS: must return to 560
    neon.setOrientation(rs_knl::Static::kStaticOrientationNS);
    REQUIRE(neon.orientation() == rs_knl::Static::kStaticOrientationNS);
    REQUIRE(neon.baseAnim() == 560);

    // AnimWindow with NS base anim 1959
    rs_knl::AnimWindow animWin(1006, pMission->map(), 1959);
    animWin.setOrientation(rs_knl::Static::kStaticOrientationNS);
    REQUIRE(animWin.orientation() == rs_knl::Static::kStaticOrientationNS);
    REQUIRE(animWin.baseAnim() == 1959);

    // Switch to EW: must become 1960
    animWin.setOrientation(rs_knl::Static::kStaticOrientationEW);
    REQUIRE(animWin.orientation() == rs_knl::Static::kStaticOrientationEW);
    REQUIRE(animWin.baseAnim() == 1960);

    // Switch back to NS: must become 1959
    animWin.setOrientation(rs_knl::Static::kStaticOrientationNS);
    REQUIRE(animWin.orientation() == rs_knl::Static::kStaticOrientationNS);
    REQUIRE(animWin.baseAnim() == 1959);

    delete pMission;
}

TEST_CASE("DoorAndLargeDoorSoundAndStateTransitionTest", "[static][door][sound]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);

    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(3, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);

    // 1. Without SoundManager: transitions work safely without crash
    {
        rs_knl::Door door(2001, pMission->map(), 1077, rs_knl::Static::kStateDoorClosed);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorClosed);

        door.setStateTo(rs_knl::Static::kStateDoorOpening);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorOpening);

        door.setStateTo(rs_knl::Static::kStateDoorOpen);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorOpen);

        door.setStateTo(rs_knl::Static::kStateDoorClosing);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorClosing);

        door.setStateTo(rs_knl::Static::kStateDoorClosed);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorClosed);
    }

    // 2. With SoundManager active: transitions exercise sound calls cleanly
    {
        rs_eng::SoundManager soundMgr;
        REQUIRE(rs_eng::SoundManager::singletonPtr() != nullptr);

        rs_knl::Door door(2002, pMission->map(), 1077, rs_knl::Static::kStateDoorClosed);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorClosed);
        REQUIRE(door.doorSound() == rs_eng::DOOR_2);

        door.setStateTo(rs_knl::Static::kStateDoorOpening);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorOpening);

        door.setStateTo(rs_knl::Static::kStateDoorOpen);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorOpen);
        REQUIRE(door.openHoldTimer() == 800);

        door.setStateTo(rs_knl::Static::kStateDoorClosing);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorClosing);

        door.setStateTo(rs_knl::Static::kStateDoorClosed);
        REQUIRE(door.doorState() == rs_knl::Static::kStateDoorClosed);

        // Verify all 5 door families match original sound mapping
        rs_knl::Door door1077(2003, pMission->map(), 1077, rs_knl::Static::kStateDoorClosed);
        REQUIRE(door1077.doorSound() == rs_eng::DOOR_2);

        rs_knl::Door door1101(2004, pMission->map(), 1101, rs_knl::Static::kStateDoorClosed);
        REQUIRE(door1101.doorSound() == rs_eng::DOOR_2);

        rs_knl::Door door1085(2005, pMission->map(), 1085, rs_knl::Static::kStateDoorClosed);
        REQUIRE(door1085.doorSound() == rs_eng::DOOR);

        rs_knl::Door door1093(2006, pMission->map(), 1093, rs_knl::Static::kStateDoorClosed);
        REQUIRE(door1093.doorSound() == rs_eng::DOOR);

        rs_knl::Door door1109(2007, pMission->map(), 1109, rs_knl::Static::kStateDoorClosed);
        REQUIRE(door1109.doorSound() == rs_eng::DOOR);

        // Verify hold timer countdown prevents instant closing
        door1077.setStateTo(rs_knl::Static::kStateDoorOpen);
        REQUIRE(door1077.openHoldTimer() == 800);
        door1077.animate(300);
        REQUIRE(door1077.doorState() == rs_knl::Static::kStateDoorOpen);
        REQUIRE(door1077.openHoldTimer() == 500);
        door1077.animate(500);
        REQUIRE(door1077.doorState() == rs_knl::Static::kStateDoorClosed);
        REQUIRE(door1077.openHoldTimer() == 0);

        rs_knl::LargeDoor gate(2008, pMission->map(), 392);
        REQUIRE(gate.doorState() == rs_knl::Static::kStateDoorClosed);

        gate.setStateTo(rs_knl::Static::kStateDoorOpening);
        REQUIRE(gate.doorState() == rs_knl::Static::kStateDoorOpening);

        gate.setStateTo(rs_knl::Static::kStateDoorOpen);
        REQUIRE(gate.doorState() == rs_knl::Static::kStateDoorOpen);

        gate.setStateTo(rs_knl::Static::kStateDoorClosing);
        REQUIRE(gate.doorState() == rs_knl::Static::kStateDoorClosing);

        gate.setStateTo(rs_knl::Static::kStateDoorClosed);
        REQUIRE(gate.doorState() == rs_knl::Static::kStateDoorClosed);
    }

    delete pMission;
}

TEST_CASE("ViewportAudioCullingTest", "[static][viewport][audio]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);

    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(3, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);

    rs_knl::Door nearDoor(3001, pMission->map(), 1077, rs_knl::Static::kStateDoorClosed);
    nearDoor.setPosition(10, 10, 1);

    rs_knl::Door farDoor(3002, pMission->map(), 1077, rs_knl::Static::kStateDoorClosed);
    farDoor.setPosition(100, 100, 1);

    // 1. Without viewport set: isObjectInView defaults to true
    REQUIRE_FALSE(pMission->hasViewport());
    REQUIRE(pMission->isObjectInView(&nearDoor));
    REQUIRE(pMission->isObjectInView(&farDoor));

    // 2. Set viewport focused around tile (10, 10)
    Point2D nearScreenPos;
    pMission->map()->tileToScreenPoint(nearDoor.position(), &nearScreenPos);
    pMission->setViewport(nearScreenPos, 640, 480, 256, 384);
    REQUIRE(pMission->hasViewport());

    // Near door is inside viewport + margins
    REQUIRE(pMission->isObjectInView(&nearDoor));
    // Far door is far away and culled
    REQUIRE_FALSE(pMission->isObjectInView(&farDoor));

    // 3. SoundManager active: state changes on both near and far doors execute cleanly
    rs_eng::SoundManager soundMgr;
    REQUIRE(rs_eng::SoundManager::singletonPtr() != nullptr);

    nearDoor.setStateTo(rs_knl::Static::kStateDoorOpening);
    REQUIRE(nearDoor.doorState() == rs_knl::Static::kStateDoorOpening);

    farDoor.setStateTo(rs_knl::Static::kStateDoorOpening);
    REQUIRE(farDoor.doorState() == rs_knl::Static::kStateDoorOpening);

    // 4. WindowObj hit & breaking audio with viewport check
    rs_knl::WindowObj winFar(3003, pMission->map(), rs_knl::WindowObj::kStateWindowClosed, 550);
    winFar.setPosition(100, 100, 1);
    winFar.setHealth(1);
    winFar.setStartHealth(1);
    rs_knl::DamageToInflict bulletDmg{};
    bulletDmg.dtype = rs_knl::kDmgTypeBullet;
    bulletDmg.dvalue = 10;
    winFar.handleHit(bulletDmg);
    REQUIRE(winFar.isDead());

    // 5. Clear viewport restores unculled behavior
    pMission->clearViewport();
    REQUIRE_FALSE(pMission->hasViewport());
    REQUIRE(pMission->isObjectInView(&farDoor));

    delete pMission;
}

TEST_CASE("DoorFamilyAnimationSoundTriggersAndDeduplicationTest", "[static][door][sound]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();

    // 1. Verify original HFRA-0.ANI / HSTA-0.ANI sound triggers
    size_t hfraSize = 0, hstaSize = 0;
    uint8_t *hfra = rs_utl::File::loadOriginalFile("HFRA-0.ANI", hfraSize);
    uint8_t *hsta = rs_utl::File::loadOriginalFile("HSTA-0.ANI", hstaSize);
    REQUIRE(hfra != nullptr);
    REQUIRE(hsta != nullptr);

    size_t numAnims = hstaSize / 2;
    size_t numFrames = hfraSize / 8;

    auto getAnimSfx = [&](size_t aid) -> uint8_t {
        if (aid >= numAnims) return 0;
        uint16_t sf = rs_utl::READ_LE_UINT16(hsta + aid * 2);
        if (sf >= numFrames) return 0;
        uint8_t *fe = hfra + sf * 8;
        uint16_t flags = rs_utl::READ_LE_UINT16(fe + 4);
        return flags & 0xFF;
    };

    // Door family 1077 uses sfx 27 (DOOR_2) on both opening and closing
    CHECK(getAnimSfx(1079) == 27);
    CHECK(getAnimSfx(1083) == 27);

    // Door family 1085 uses sfx 25 (DOOR) on both opening and closing
    CHECK(getAnimSfx(1087) == 25);
    CHECK(getAnimSfx(1091) == 25);

    // Door family 1093 uses sfx 25 (DOOR) on both opening and closing
    CHECK(getAnimSfx(1095) == 25);
    CHECK(getAnimSfx(1099) == 25);

    // Door family 1101 uses sfx 27 (DOOR_2) on both opening and closing
    CHECK(getAnimSfx(1103) == 27);
    CHECK(getAnimSfx(1107) == 27);

    // Door family 1109 uses sfx 25 (DOOR) on both opening and closing
    CHECK(getAnimSfx(1111) == 25);
    CHECK(getAnimSfx(1115) == 25);

    // Large road gates (anims 392..397) have NO sound triggers (sfx == 0)
    for (size_t a = 392; a <= 397; ++a) {
        CHECK(getAnimSfx(a) == 0);
    }

    delete[] hfra;
    delete[] hsta;

    // 2. Verify SOUND-0.TAB contains entries 24 (DOOR) and 26 (DOOR_2)
    size_t tabSize = 0;
    uint8_t *tab = rs_utl::File::loadOriginalFile("SOUND-0.TAB", tabSize);
    REQUIRE(tab != nullptr);
    if (tab) {
        size_t entry24Offset = 58 + 24 * 32;
        size_t entry26Offset = 58 + 26 * 32;
        REQUIRE(entry26Offset + 32 <= tabSize);
        uint32_t size24 = rs_utl::READ_LE_UINT32(tab + entry24Offset);
        uint32_t size26 = rs_utl::READ_LE_UINT32(tab + entry26Offset);
        CHECK(size24 > 144);
        CHECK(size26 > 144);
        delete[] tab;
    }
}

TEST_CASE("StaticAnimationAudit", "[static]") {
    initTestDataFolders();

    size_t hfraSize = 0, hstaSize = 0;
    uint8_t *hfra = rs_utl::File::loadOriginalFile("HFRA-0.ANI", hfraSize);
    uint8_t *hsta = rs_utl::File::loadOriginalFile("HSTA-0.ANI", hstaSize);
    REQUIRE(hfra != nullptr);
    REQUIRE(hsta != nullptr);

    size_t numAnims = hstaSize / 2;
    size_t numFrames = hfraSize / 8;

    auto getAnimFrames = [&](size_t aid) -> std::pair<size_t, std::vector<uint8_t>> {
        if (aid >= numAnims) return {0, {}};
        uint16_t sf = rs_utl::READ_LE_UINT16(hsta + aid * 2);
        if (sf >= numFrames) return {0, {}};
        uint16_t curr = sf;
        size_t count = 0;
        std::vector<uint8_t> sfxs;
        while (curr < numFrames && count < 50) {
            count++;
            uint8_t *fe = hfra + curr * 8;
            uint8_t sfx = *(fe + 4);
            uint16_t nextFrame = rs_utl::READ_LE_UINT16(fe + 6);
            if (sfx != 0) sfxs.push_back(sfx);
            if (nextFrame == sf || nextFrame == curr) break;
            curr = nextFrame;
        }
        return {count, sfxs};
    };

    // 1. Verify Window breaking animations (554 NS, 555 EW) have 4 frames and sfx 7
    auto [frames554, sfx554] = getAnimFrames(554);
    CHECK(frames554 == 4);
    REQUIRE(sfx554.size() == 1);
    CHECK(sfx554[0] == 7);

    auto [frames555, sfx555] = getAnimFrames(555);
    CHECK(frames555 == 4);
    REQUIRE(sfx555.size() == 1);
    CHECK(sfx555[0] == 7);

    // 2. Verify SOUND-0.TAB entry 7 is a dummy/empty entry (size <= 144, byte1e == 0),
    // proving original DOS Syndicate binary ignores sfx 7 and plays NO sound for breaking windows.
    size_t tabSize = 0;
    uint8_t *tab = rs_utl::File::loadOriginalFile("SOUND-0.TAB", tabSize);
    REQUIRE(tab != nullptr);
    if (tab) {
        size_t entry7Offset = 58 + 7 * 32;
        REQUIRE(entry7Offset + 32 <= tabSize);
        uint32_t size7 = rs_utl::READ_LE_UINT32(tab + entry7Offset);
        uint8_t byte1e = *(tab + entry7Offset + 0x1e);
        CHECK(size7 <= 144);
        CHECK(byte1e == 0);
        delete[] tab;
    }

    // 3. Verify Trees (227..235) have NO sound triggers (sfx == 0)
    for (size_t a = 227; a <= 235; ++a) {
        auto [count, sfxs] = getAnimFrames(a);
        CHECK(sfxs.empty());
    }

    // 4. Verify Semaphores (1040..1044) have NO sound triggers (sfx == 0)
    for (size_t a = 1040; a <= 1044; ++a) {
        auto [count, sfxs] = getAnimFrames(a);
        CHECK(sfxs.empty());
    }

    // 5. Verify Large Doors (392..397) have NO sound triggers (sfx == 0)
    for (size_t a = 392; a <= 397; ++a) {
        auto [count, sfxs] = getAnimFrames(a);
        CHECK(sfxs.empty());
    }

    delete[] hfra;
    delete[] hsta;

    // 6. Verify WindowObj destruction in ReSyndicated matches original speed and transitions cleanly
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);

    rs_eng::TileManager tileMgr;
    rs_knl::Map map(&tileMgr, 0);
    rs_knl::WindowObj win(2001, &map, rs_knl::WindowObj::kStateWindowClosed, 550);
    win.setOrientation(rs_knl::Static::kStaticOrientationNS);
    win.setHealth(1);
    win.setStartHealth(1);
    rs_knl::DamageToInflict dmg{};
    dmg.dtype = rs_knl::kDmgTypeBullet;
    dmg.dvalue = 10;
    win.handleHit(dmg);
    CHECK(win.isDead());
    CHECK_FALSE(win.canBlock());
    CHECK(win.animationPlayer()->currentFramesAnimation() == 554);
}

TEST_CASE("Tree burning animation duration and Gauss explosion visual overhaul", "[tree][explosion][gauss]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);

    rs_eng::TileManager tileMgr;
    rs_knl::Map map(&tileMgr, 0);

    SECTION("Tree burning duration is 10000ms (~10s) before transitioning to burnt stump") {
        // Base anim 227 -> burning 228 -> burnt stump 229
        rs_knl::Tree tree(3001, &map, 227);
        tree.setHealth(1);
        tree.setStartHealth(1);
        CHECK(tree.isAlive());
        CHECK(tree.canBlock());
        CHECK(tree.animationPlayer()->currentFramesAnimation() == 227);

        // Hit tree with flamer/fire damage
        rs_knl::DamageToInflict dmg{};
        dmg.dtype = rs_knl::kDmgTypeBurn;
        dmg.dvalue = 1;
        tree.handleHit(dmg);

        CHECK(tree.isDead());
        CHECK_FALSE(tree.canBlock());
        CHECK(tree.animationPlayer()->currentFramesAnimation() == 228);
        CHECK(tree.animationPlayer()->isCurrentAnimation(tree.burningAnim()));

        // Simulate 5000ms of elapsed time - should still be burning
        tree.animate(5000);
        CHECK(tree.animationPlayer()->currentFramesAnimation() == 228);
        CHECK(tree.animationPlayer()->isCurrentAnimation(tree.burningAnim()));

        // Simulate another 4900ms (9900ms total) - still burning
        tree.animate(4900);
        CHECK(tree.animationPlayer()->currentFramesAnimation() == 228);
        CHECK(tree.animationPlayer()->isCurrentAnimation(tree.burningAnim()));

        // Advance past 10000ms (e.g. 200ms -> 10100ms total) - should transition to burnt stump (229)
        tree.animate(200);
        CHECK(tree.animationPlayer()->currentFramesAnimation() == 229);
        CHECK(tree.animationPlayer()->isCurrentAnimation(tree.burntAnim()));

        // Verify bullet hits (e.g. Minigun) ignite trees while persuasion does not
        rs_knl::Tree bulletTree(3003, &map, 227);
        bulletTree.setHealth(1);
        bulletTree.setStartHealth(1);
        rs_knl::DamageToInflict persDmg{};
        persDmg.dtype = rs_knl::kDmgTypePersuasion;
        persDmg.dvalue = 1;
        bulletTree.handleHit(persDmg);
        CHECK(bulletTree.isAlive());
        CHECK(bulletTree.canBlock());

        rs_knl::DamageToInflict bulletDmg{};
        bulletDmg.dtype = rs_knl::kDmgTypeBullet;
        bulletDmg.dvalue = 1;
        bulletTree.handleHit(bulletDmg);
        CHECK(bulletTree.isDead());
        CHECK_FALSE(bulletTree.canBlock());
        CHECK(bulletTree.animationPlayer()->isCurrentAnimation(bulletTree.burningAnim()));
    }

    SECTION("Mission::handleTick advances static object animations using simulation diff") {
        MemoryTileManager memTileMgr;
        rs_knl::Map testMap(&memTileMgr, 1);
        REQUIRE(loadMapFromCsv(TEST_DATA_DIR "/map-5x5x4.csv", memTileMgr, testMap));
        rs_knl::Mission testMission;
        REQUIRE(testMission.init(&testMap));

        // Create a burning tree and register it with the mission
        auto pTree = std::make_unique<rs_knl::Tree>(3002, &testMap, 227);
        pTree->setHealth(1);
        pTree->setStartHealth(1);
        rs_knl::DamageToInflict dmg{};
        dmg.dtype = rs_knl::kDmgTypeBurn;
        dmg.dvalue = 1;
        pTree->handleHit(dmg);
        CHECK(pTree->animationPlayer()->currentFramesAnimation() == 228);

        rs_knl::Tree *rawTree = pTree.get();
        testMission.addStatic(std::move(pTree));

        // handleTick(elapsed=16, diff=10100) -> statics must animate using diff, not elapsed
        testMission.handleTick(16, 10100);
        CHECK(rawTree->animationPlayer()->currentFramesAnimation() == 229);
    }

    SECTION("Gauss gun explosion creates organic cluster of tall roaring fire pillars and central fireball") {
        MemoryTileManager memTileMgr;
        rs_knl::Map testMap(&memTileMgr, 1);
        REQUIRE(loadMapFromCsv(TEST_DATA_DIR "/map-5x5x4.csv", memTileMgr, testMap));
        rs_knl::Mission testMission;
        REQUIRE(testMission.init(&testMap));

        ConfigFile config;
        initWeaponConfigFile(config);
        rs_knl::Weapon gaussDef(rs_knl::Weapon::GaussGun, config);
        rs_knl::WeaponInstance gauss(&gaussDef, 100, nullptr);

        size_t initialSfxCount = testMission.numSfxObjects();

        // Detonate Gauss Gun explosion at (500, 500, 0)
        rs_knl::WorldPoint impactLoc(500, 500, 0);
        rs_knl::Explosion::createExplosion(&testMission, &gauss, impactLoc, 384.0, 50);

        size_t newSfxCount = testMission.numSfxObjects();
        REQUIRE(newSfxCount > initialSfxCount);

        // Verify there is an ExplosionBall at impact epicenter
        bool foundCenterExplosionBall = false;
        int largeFireCount = 0;
        int explosionFireCount = 0;

        for (size_t i = initialSfxCount; i < newSfxCount; ++i) {
            rs_knl::SFXObject *sfx = testMission.sfxObjects(i);
            if (sfx->type() == rs_knl::SFXObject::sfxt_ExplosionBall) {
                // Ball origin should match impactLoc X and Y
                if (sfx->position().tx * 256 + sfx->position().ox == impactLoc.x &&
                    sfx->position().ty * 256 + sfx->position().oy == impactLoc.y) {
                    foundCenterExplosionBall = true;
                }
            } else if (sfx->type() == rs_knl::SFXObject::sfxt_LargeFire) {
                largeFireCount++;
            } else if (sfx->type() == rs_knl::SFXObject::sfxt_ExplosionFire) {
                explosionFireCount++;
            }
        }

        CHECK(foundCenterExplosionBall);
        // Gauss explosion must use tall roaring fire pillars (sfxt_LargeFire), NOT sfxt_ExplosionFire
        CHECK(largeFireCount >= 10);
        CHECK(explosionFireCount == 0);
    }

    SECTION("Time bomb explosion uses expanding shockwave fire (sfxt_ExplosionFire)") {
        MemoryTileManager memTileMgr;
        rs_knl::Map testMap(&memTileMgr, 1);
        REQUIRE(loadMapFromCsv(TEST_DATA_DIR "/map-5x5x4.csv", memTileMgr, testMap));
        rs_knl::Mission testMission;
        REQUIRE(testMission.init(&testMap));

        ConfigFile config;
        initWeaponConfigFile(config);
        rs_knl::Weapon bombDef(rs_knl::Weapon::TimeBomb, config);
        CHECK(bombDef.explosionDelay() == 20000);
        rs_knl::WeaponInstance bomb(&bombDef, 101, nullptr);

        size_t initialSfxCount = testMission.numSfxObjects();

        rs_knl::WorldPoint impactLoc(600, 600, 0);
        rs_knl::Explosion::createExplosion(&testMission, &bomb, impactLoc, 432.0, 100);

        size_t newSfxCount = testMission.numSfxObjects();
        REQUIRE(newSfxCount > initialSfxCount);

        int explosionFireCount = 0;
        int largeFireCount = 0;
        for (size_t i = initialSfxCount; i < newSfxCount; ++i) {
            rs_knl::SFXObject *sfx = testMission.sfxObjects(i);
            if (sfx->type() == rs_knl::SFXObject::sfxt_ExplosionFire) {
                explosionFireCount++;
            } else if (sfx->type() == rs_knl::SFXObject::sfxt_LargeFire) {
                largeFireCount++;
            }
        }

        // Time bomb explosion creates concentric shockwave rings of sfxt_ExplosionFire
        CHECK(explosionFireCount > 0);
        CHECK(largeFireCount == 0);
    }
}

TEST_CASE("Mission 2 Ped Actions Factions and Hunter Behavior", "[Mission2]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll levelData;
    REQUIRE(misMgr.load_level_data(2, levelData));

    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    pMission->buildNavigationGraph();
    misMgr.setMission(pMission);

    // Verify Player agents faction
    for (size_t i = 0; i < pMission->getSquad()->size(); ++i) {
        auto *ag = pMission->getSquad()->member(i);
        if (ag) {
            CHECK(ag->factionId() == 0);
        }
    }

    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        auto *p = pMission->ped(i);
        if (p->id() >= 8 && p->id() <= 15) {
            CHECK(p->factionId() == 1);
            auto *pHos = p->behaviour().getComponent<rs_knl::PlayerHostileBehaviourComponent>();
            REQUIRE(pHos != nullptr);
            CHECK(pHos->isHunter() == true);
        } else if (p->id() >= 24 && p->id() <= 39) {
            CHECK((p->factionId() == 3 || p->factionId() == 4));
            auto *pHos = p->behaviour().getComponent<rs_knl::PlayerHostileBehaviourComponent>();
            REQUIRE(pHos != nullptr);
            CHECK(pHos->isHunter() == false);
            // Sentry agents with state 0x00 and no script should have no active movement action
            if ((p->initialLevelState() & LevelData::kPeopleStateWalking) == 0 && !p->hasScript()) {
                CHECK(p->currentAction() == nullptr);
            }
        } else if (p->type() == rs_knl::PedInstance::kPedTypePolice) {
            // Police in Mission 2 with state 0x10 have wandering movement
            if (p->initialLevelState() & LevelData::kPeopleStateWalking) {
                CHECK(p->currentAction() != nullptr);
            }
        }
    }

    struct Coord2D { int x; int y; };
    std::vector<Coord2D> startPositions(pMission->numPeds());
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        startPositions[i] = { pMission->ped(i)->position().tx, pMission->ped(i)->position().ty };
    }

    // Simulate 50 ticks of mission gameplay
    for (int t = 0; t < 50; ++t) {
        pMission->handleTick(100, 100);
    }

    // 1. Blue faction (peds 8..15) are hunters:
    // They should auto-equip weapons, engage the player agents, and move towards them
    int blueAgentsMoved = 0;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        auto *p = pMission->ped(i);
        if (p->id() >= 8 && p->id() <= 15) {
            CHECK(p->getBestRankedWeaponWithAmmo() != nullptr);
            CHECK(p->selectedWeapon() == nullptr); // Holstered while distant
            int sx = startPositions[i].x;
            int sy = startPositions[i].y;
            if (p->position().tx != sx || p->position().ty != sy) {
                blueAgentsMoved++;
            }
        }
    }
    // Hostile hunters should have navigated towards player agents
    CHECK(blueAgentsMoved > 0);

    // 2. Yellow & Orange sentry agents (peds 16..39) with initialLevelState 0x00 and no script:
    // They must remain completely stationary sentries at their exact spawn tiles
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        auto *p = pMission->ped(i);
        if (p->id() >= 16 && p->id() <= 39) {
            if ((p->initialLevelState() & LevelData::kPeopleStateWalking) == 0 && !p->hasScript()) {
                CHECK(p->currentAction() == nullptr);
                CHECK(p->speed() == 0);
                int sx = startPositions[i].x;
                int sy = startPositions[i].y;
                CHECK(p->position().tx == sx);
                CHECK(p->position().ty == sy);
            }
        }
    }
}

TEST_CASE("Mission 1 Guards Inspection Diagnostic", "[Mission1Guards]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll level_data;
    REQUIRE(misMgr.load_level_data(1, level_data));
    rs_knl::Mission *pMission = misMgr.create_mission(level_data);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());
    misMgr.setMission(pMission);

    // Guard #6 (id 10) is in an alcove at (74, 32, 2) behind walls
    auto *pGuard = pMission->ped(6);
    REQUIRE(pGuard != nullptr);
    CHECK(pGuard->type() == rs_knl::PedInstance::kPedTypeGuard);

    auto *pPlayer = pMission->getSquad()->member(0);
    REQUIRE(pPlayer != nullptr);

    // 1. Position player at (74, 28, 2) - separated from guard by solid building wall (y=30)
    pPlayer->setPosition(rs_knl::TilePoint(74, 28, 2));
    
    // Line of sight between guard and player is blocked by the building wall
    rs_knl::WorldPoint gW(pGuard->position());
    rs_knl::WorldPoint pW(pPlayer->position());
    CHECK(pMission->checkBlockedByTile(gW, &pW, false, 3584.0) != 1);

    // Run ticks: guard must NOT fire into the wall
    for (int t = 0; t < 20; ++t) {
        pMission->handleTick(100, 100);
    }
    CHECK_FALSE(pGuard->isUsingWeapon());

    // 2. Position player in front of the alcove at (72, 32, 2) where clear line of sight exists
    pPlayer->setPosition(rs_knl::TilePoint(72, 32, 2));
    rs_knl::WorldPoint pW2(pPlayer->position());
    CHECK(pMission->checkBlockedByTile(gW, &pW2, false, 3584.0) == 1);

    // Guard now has clear line of sight and can engage
    for (int t = 0; t < 20; ++t) {
        pMission->handleTick(100, 100);
    }
    // Guard engaged target
    CHECK(pGuard->selectedWeapon() != nullptr);

    // 3. Move player far away (> 50 tiles) to escape pursuit
    pPlayer->setPosition(rs_knl::TilePoint(20, 20, 2));
    for (int t = 0; t < 30; ++t) {
        pMission->handleTick(100, 100);
    }
    // Guard gives up pursuit and is not actively shooting
    CHECK_FALSE(pGuard->isUsingWeapon());

    misMgr.destroyMission();
}

TEST_CASE("Mission 1 Target Ped Inspection", "[Mission1Target]") {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    LevelData::LevelDataAll level_data;
    REQUIRE(misMgr.load_level_data(1, level_data));
    rs_knl::Mission *pMission = misMgr.create_mission(level_data);
    REQUIRE(pMission != nullptr);
    REQUIRE(pMission->buildNavigationGraph());
    misMgr.setMission(pMission);

    auto *obj = pMission->objective(0);
    auto *ass = dynamic_cast<rs_knl::ObjAssassinate *>(obj);
    REQUIRE(ass != nullptr);
    auto *target = dynamic_cast<rs_knl::PedInstance *>(ass->target());
    REQUIRE(target != nullptr);
    CHECK(target->id() == 8);
    CHECK(target->type() == rs_knl::PedInstance::kPedTypeCivilian);
    CHECK(target->hasScript());
    CHECK(target->isImmunedToPanic());

    // Place armed player right next to the target
    auto *pPlayer = pMission->getSquad()->member(0);
    pPlayer->setPosition(target->position());
    pMission->addArmedPed(pPlayer);
    target->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponOut, pPlayer);

    // Tick mission: target must NOT panic while executing script; continues scenario actions
    for (int t = 0; t < 30; ++t) {
        pMission->handleTick(100, 100);
        CHECK_FALSE(target->isInPanic());
    }

    // Target boards the car
    auto *pCar = dynamic_cast<rs_knl::GenericCar *>(pMission->vehicle(0));
    REQUIRE(pCar != nullptr);
    pCar->addPassenger(target);
    CHECK(target->inVehicle() == pCar);
    CHECK(pCar->getDriver() == target);

    // Player shoots the car
    rs_knl::DamageToInflict hitDmg{};
    hitDmg.dvalue = 50;
    hitDmg.dtype = rs_knl::kDmgTypeBullet;
    hitDmg.d_owner = pPlayer;
    pCar->handleHit(hitDmg);

    // Target is ejected from the car, scenario is aborted, and civilian enters panic mode!
    CHECK(target->inVehicle() == nullptr);
    CHECK_FALSE(target->hasScript());
    CHECK_FALSE(target->isImmunedToPanic());

    for (int t = 0; t < 5; ++t) {
        pMission->handleTick(100, 100);
    }
    CHECK(target->isInPanic());

    misMgr.destroyMission();
}

TEST_CASE( "Custom Mission 22 Destroyed Train State Verification", "[train][mission22][destroyed]" ) {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    LevelData::LevelDataAll level_data;
    if (misMgr.load_level_data(22, level_data, "synd/SYNDICAT/DATA/game22.dat")) {
        rs_knl::Mission *pMission = misMgr.create_mission(level_data, "synd/SYNDICAT/DATA/map22.dat");
        REQUIRE(pMission != nullptr);
        pMission->buildNavigationGraph();
        misMgr.setMission(pMission);

        auto *train0 = pMission->vehicle(0);
        REQUIRE(train0 != nullptr);
        auto *carriage1 = pMission->vehicle(1);
        REQUIRE(carriage1 != nullptr);

        // Put train0 and carriage1 in destroyed state:
        train0->setHealth(-1);
        carriage1->setHealth(-1);

        CHECK(train0->health() <= 0);
        CHECK_FALSE(train0->isAlive());
        CHECK(carriage1->health() <= 0);
        CHECK_FALSE(carriage1->isAlive());

        // Verify burnt animations are active
        if (train0->animationPlayer()) {
            CHECK(train0->animationPlayer()->isCurrentAnimation(train0->burntAnimation()));
        }
        if (carriage1->animationPlayer()) {
            CHECK(carriage1->animationPlayer()->isCurrentAnimation(carriage1->burntAnimation()));
        }

        // Verify destroyed train cannot be driven
        int initialY = train0->position().ty;
        for (int t = 0; t < 50; ++t) {
            pMission->handleTick(100, 100);
        }
        CHECK(train0->speed() == 0);
        CHECK(train0->position().ty == initialY);

        misMgr.destroyMission();
    }
}

TEST_CASE( "Train Round Trip Driving Loop", "[train][roundtrip]" ) {
    rs_eng::AppContext ctx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    REQUIRE(tileMgr.loadTiles());
    rs_knl::MissionManager misMgr(&tileMgr);

    LevelData::LevelDataAll level_data;
    if (misMgr.load_level_data(22, level_data, "synd/SYNDICAT/DATA/game22.dat")) {
        rs_knl::Mission *pMission = misMgr.create_mission(level_data, "synd/SYNDICAT/DATA/map22.dat");
        REQUIRE(pMission != nullptr);
        pMission->buildNavigationGraph();
        misMgr.setMission(pMission);

        auto *train0 = pMission->vehicle(0);
        REQUIRE(train0 != nullptr);

        // Find driver ped (Ped 24 in vanilla Mission 22)
        rs_knl::PedInstance *policePed = pMission->ped(24);
        REQUIRE(policePed != nullptr);

        // Configure policePed with full round-trip route (identical to original Ped 24):
        // 0: Board Train 0
        // 1: Drive to Station 2 (49, 80)
        // 2: Dwell at Station 2 (1000ms for fast test)
        // 3: Drive to Station 1 (49, 24)
        // 4: Dwell at Station 1 (1000ms for fast test)
        // 5: Loop Reset
        policePed->clearScriptSteps();

        rs_knl::ScriptStep s0;
        s0.type = LevelData::kScenarioTypeUseVehicle;
        s0.targetEntityId = 0;
        policePed->addScriptStep(s0);

        rs_knl::ScriptStep s1;
        s1.type = LevelData::kScenarioTypeGoto;
        s1.targetPos = rs_knl::TilePoint(49, 80, 2);
        policePed->addScriptStep(s1);

        rs_knl::ScriptStep s2;
        s2.type = LevelData::kScenarioTypeWait;
        s2.param = 1000;
        policePed->addScriptStep(s2);

        rs_knl::ScriptStep s3;
        s3.type = LevelData::kScenarioTypeGoto;
        s3.targetPos = rs_knl::TilePoint(49, 24, 2);
        policePed->addScriptStep(s3);

        rs_knl::ScriptStep s4;
        s4.type = LevelData::kScenarioTypeWait;
        s4.param = 1000;
        policePed->addScriptStep(s4);

        rs_knl::ScriptStep s5;
        s5.type = LevelData::kScenarioTypeReset;
        policePed->addScriptStep(s5);

        rs_knl::MissionManager::buildScriptedActionsForPed(pMission, policePed);

        // Leg 1: Board train and travel to Station 2 (ty reaches ~80)
        bool reachedStation2 = false;
        for (int t = 0; t < 300; ++t) {
            pMission->handleTick(100, 100);
            if (train0->position().ty >= 79) {
                reachedStation2 = true;
                break;
            }
        }
        CHECK(reachedStation2);

        // Leg 2: Dwell at Station 2, then reverse and return to Station 1 (ty reaches ~24)
        bool reachedStation1Again = false;
        for (int t = 0; t < 400; ++t) {
            pMission->handleTick(100, 100);
            if (reachedStation2 && train0->position().ty <= 25) {
                reachedStation1Again = true;
                break;
            }
        }
        CHECK(reachedStation1Again);

        // Leg 3: Dwell at Station 1, then MUST depart AGAIN for Station 2!
        bool departedStation1Again = false;
        for (int t = 0; t < 400; ++t) {
            pMission->handleTick(100, 100);
            if (train0->position().ty >= 35) {
                departedStation1Again = true;
                break;
            }
        }
        CHECK(departedStation1Again);

        misMgr.destroyMission();
    }
}

TEST_CASE("Mission Palette Resolution matches Campaign Territory Table", "[kernel][mission][palette]") {
    // Mission 1 (West Eur): Palette 2 (HPAL02.DAT - Industrial)
    CHECK(rs_knl::MissionManager::getMissionPaletteId(1) == 2);
    // Mission 2 (Far East): Palette 3 (HPAL03.DAT - Snow)
    CHECK(rs_knl::MissionManager::getMissionPaletteId(2) == 3);
    // Mission 3 (Mongolia): Palette 4 (HPAL04.DAT - Night)
    CHECK(rs_knl::MissionManager::getMissionPaletteId(3) == 4);
    // Mission 4 (Iran): Palette 5 (HPAL05.DAT - Desert)
    CHECK(rs_knl::MissionManager::getMissionPaletteId(4) == 5);
    // Mission 5 (California): Palette 1 (HPAL01.DAT - Temperate)
    CHECK(rs_knl::MissionManager::getMissionPaletteId(5) == 1);
    // Mission 7 (India): Palette 1
    CHECK(rs_knl::MissionManager::getMissionPaletteId(7) == 1);
    // Mission 22 (Siberia): Palette 3
    CHECK(rs_knl::MissionManager::getMissionPaletteId(22) == 3);
    // Mission 50 (China): Palette 1
    CHECK(rs_knl::MissionManager::getMissionPaletteId(50) == 1);

    // Out-of-bounds or custom missions default to Palette 1
    CHECK(rs_knl::MissionManager::getMissionPaletteId(0) == 1);
    CHECK(rs_knl::MissionManager::getMissionPaletteId(99) == 1);
    CHECK(rs_knl::MissionManager::getMissionPaletteId(-5) == 1);
}

TEST_CASE("Exported GAMEXX.DAT populates type, inventory links, and DOS spatial map list for new faction agents", "[kernel][mission][export][dos]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);
    rs_knl::ModManager modMgr;
    modMgr.reset();
    rs_knl::WeaponManager weaponMgr;
    weaponMgr.reset();
    rs_knl::AgentManager agentMgr;
    agentMgr.setModManager(&modMgr);
    agentMgr.setWeaponManager(&weaponMgr);
    agentMgr.reset();
    rs_eng::TileManager tileMgr;
    tileMgr.loadTiles();
    rs_knl::MissionManager misMgr(&tileMgr);
    LevelData::LevelDataAll level_data;
    REQUIRE(misMgr.load_level_data(1, level_data));

    rs_knl::Mission *pMission = misMgr.create_mission(level_data);
    REQUIRE(pMission != nullptr);
    misMgr.setMission(pMission);

    // Count existing peds before spawning our new Blue Faction (faction 1) agent
    size_t origPedCount = pMission->numPeds();
    rs_knl::PedInstance *pBlueAgent = pMission->spawnPed(
        rs_knl::PedInstance::kPedTypeAgent,
        rs_knl::TilePoint(84, 36, 1, 128, 128, 0),
        0,
        1,
        1);
    REQUIRE(pBlueAgent != nullptr);

    rs_knl::Weapon *pPistolDef = weaponMgr.getWeapon(rs_knl::Weapon::Pistol);
    rs_knl::Weapon *pMinigunDef = weaponMgr.getWeapon(rs_knl::Weapon::Minigun);
    REQUIRE(pPistolDef != nullptr);
    REQUIRE(pMinigunDef != nullptr);
    rs_knl::WeaponInstance *pWep1 = rs_knl::WeaponInstance::createInstance(pPistolDef);
    rs_knl::WeaponInstance *pWep2 = rs_knl::WeaponInstance::createInstance(pMinigunDef);
    pWep1->setOwner(pBlueAgent);
    pWep2->setOwner(pBlueAgent);
    pBlueAgent->addWeapon(pWep1);
    pBlueAgent->addWeapon(pWep2);

    // Add Rival Sweep tactic targeting Player Squad on Faction 1 with num_squads initially 1
    pMission->tactics().header.num_squads = 1;
    auto &tactic1 = pMission->tactics().entries[1];
    std::memset(&tactic1, 0, sizeof(tactic1));
    tactic1.faction_id = 1;
    tactic1.mode = LevelData::kAITacticModeRivalSweep;
    tactic1.sub_mode = LevelData::kAIFactionPlayer;
    rs_utl::WRITE_LE_UINT16(tactic1.param1, LevelData::kAIFactionPlayer);

    // Also add a Seek & Destroy tactic targeting Faction 2 and verify mode 1 is preserved verbatim (not silently rewritten)
    auto &tactic2 = pMission->tactics().entries[2];
    std::memset(&tactic2, 0, sizeof(tactic2));
    tactic2.faction_id = 1;
    tactic2.mode = LevelData::kAITacticModeSeekDestroy;
    tactic2.sub_mode = 2;

    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "synd_dos_spatial_test";
    std::filesystem::create_directories(tempDir);
    std::filesystem::path tempGame = tempDir / "game01.dat";
    REQUIRE(rs_knl::MissionManager::exportMissionToBinary(pMission, 1, tempGame.string()));

    uint16_t blueSlot = pBlueAgent->id();
    REQUIRE(blueSlot >= 8);
    REQUIRE(blueSlot < 16);

    misMgr.destroyMission();

    misMgr.mapManager().setCustomDataDir(tempDir.string());
    LevelData::LevelDataAll exportedData;
    REQUIRE(misMgr.load_level_data(1, exportedData));

    // 0. Squad slots must export state == 0x00 (standing) so DOS MAIN.EXE does not switch them into wandering AI mode 0x1D
    for (size_t s = 0; s < 4; ++s) {
        CHECK(exportedData.people[s].state == 0x00);
    }

    // 1. Newly spawned ped must have type == 0x01 and valid initial anim/IPA so DOS MAIN.EXE activates & draws it
    const auto &bp = exportedData.people[blueSlot];
    CHECK(bp.type == 0x01);
    CHECK(bp.type_ped == rs_knl::PedInstance::kPedTypeAgent);
    CHECK(bp.location == LevelData::kPeopleLocOnMap);
    CHECK(rs_utl::READ_LE_UINT16(bp.index_base_anim) == 1);
    CHECK(rs_utl::READ_LE_UINT16(bp.index_current_anim) != 0);
    CHECK(rs_utl::READ_LE_UINT16(bp.index_current_frame) != 0);
    CHECK(bp.adrena_amount == 0x80);
    CHECK(bp.inteli_amount == 0x80);
    CHECK(bp.percep_amount == 0x80);

    // 2. Inventory equipment chain & ammo must link cleanly back to blueSlot
    uint16_t pedOff = static_cast<uint16_t>(blueSlot * 92 + 2);
    uint16_t eqOff = rs_utl::READ_LE_UINT16(bp.offset_equipment);
    REQUIRE(eqOff >= 0x9562);
    size_t w0Idx = (eqOff - 0x9562) / 36;
    REQUIRE(w0Idx < 512);
    CHECK(exportedData.weapons[w0Idx].desc == 0x05);
    CHECK(exportedData.weapons[w0Idx].type == 0x04);
    CHECK(rs_utl::READ_LE_UINT16(exportedData.weapons[w0Idx].offset_owner) == pedOff);
    CHECK(rs_utl::READ_LE_UINT16(exportedData.weapons[w0Idx].offset_prev_inventory) == pedOff);
    CHECK(rs_utl::READ_LE_INT16(exportedData.weapons[w0Idx].unkn5) > 0);
    CHECK(rs_utl::READ_LE_UINT16(exportedData.weapons[w0Idx].index_current_frame) != 0);

    uint16_t w1Off = rs_utl::READ_LE_UINT16(exportedData.weapons[w0Idx].offset_next_inventory);
    REQUIRE(w1Off >= 0x9562);
    size_t w1Idx = (w1Off - 0x9562) / 36;
    REQUIRE(w1Idx < 512);
    CHECK(exportedData.weapons[w1Idx].desc == 0x05);
    CHECK(exportedData.weapons[w1Idx].type == 0x04);
    CHECK(rs_utl::READ_LE_UINT16(exportedData.weapons[w1Idx].offset_owner) == pedOff);
    CHECK(rs_utl::READ_LE_UINT16(exportedData.weapons[w1Idx].offset_prev_inventory) == eqOff);
    CHECK(rs_utl::READ_LE_UINT16(exportedData.weapons[w1Idx].offset_next_inventory) == 0);
    CHECK(rs_utl::READ_LE_UINT16(exportedData.weapons[w1Idx].index_current_frame) != 0);

    // 3. Statics use DOS type 0x05 and valid initial frames on Statics and Cars
    for (size_t s = 0; s < 400; ++s) {
        if (exportedData.statics[s].desc != 0) {
            CHECK(exportedData.statics[s].type == 0x05);
            CHECK(rs_utl::READ_LE_UINT16(exportedData.statics[s].index_current_frame) != 0);
        }
    }
    CHECK(rs_utl::READ_LE_UINT16(exportedData.cars[0].index_current_frame) != 0);

    // 4. Spatial map.objs doubly-linked list contains blueSlot at (84, 36)
    uint16_t cur = rs_utl::READ_LE_UINT16(exportedData.map.objs + (36 * 128 + 84) * 2);
    bool foundBlueInSpatialList = false;
    uint16_t prevOff = 0;
    int guard = 0;
    while (cur != 0 && guard++ < 64) {
        if (cur == pedOff) {
            foundBlueInSpatialList = true;
        }
        uint16_t nextOff = 0, nodePrev = 0;
        if (cur >= 0x0002 && cur < 0x0002 + 256 * 92) {
            const auto &node = exportedData.people[(cur - 0x0002) / 92];
            nextOff = rs_utl::READ_LE_UINT16(node.offset_next);
            nodePrev = rs_utl::READ_LE_UINT16(node.offset_prev);
        } else if (cur >= 0x5C02 && cur < 0x5C02 + 64 * 42) {
            const auto &node = exportedData.cars[(cur - 0x5C02) / 42];
            nextOff = rs_utl::READ_LE_UINT16(node.offset_next);
            nodePrev = rs_utl::READ_LE_UINT16(node.offset_prev);
        } else if (cur >= 0x6682 && cur < 0x6682 + 400 * 30) {
            const auto &node = exportedData.statics[(cur - 0x6682) / 30];
            nextOff = rs_utl::READ_LE_UINT16(node.offset_next);
            nodePrev = rs_utl::READ_LE_UINT16(node.offset_prev);
        } else if (cur >= 0x9562 && cur < 0x9562 + 512 * 36) {
            const auto &node = exportedData.weapons[(cur - 0x9562) / 36];
            nextOff = rs_utl::READ_LE_UINT16(node.offset_next);
            nodePrev = rs_utl::READ_LE_UINT16(node.offset_prev);
        }
        CHECK(nodePrev == prevOff);
        prevOff = cur;
        cur = nextOff;
    }
    CHECK(foundBlueInSpatialList);

    // 5. AI Tactics header & mode preservation (no silent mode rewrite on export)
    CHECK(exportedData.tactics.header.num_squads >= 2);
    CHECK(exportedData.tactics.header.peds_per_squad == 8);
    CHECK(exportedData.tactics.entries[1].mode == LevelData::kAITacticModeRivalSweep);
    CHECK(rs_utl::READ_LE_UINT16(exportedData.tactics.entries[1].param1) == LevelData::kAIFactionPlayer);
    CHECK(exportedData.tactics.entries[2].mode == LevelData::kAITacticModeSeekDestroy);
    CHECK(exportedData.tactics.entries[2].sub_mode == 2);

    // 6. Reloading in MissionManager also reconstructs the added agent cleanly
    rs_knl::Mission *pReloaded = misMgr.create_mission(exportedData);
    REQUIRE(pReloaded != nullptr);
    CHECK(pReloaded->numPeds() == origPedCount + 1);
    misMgr.destroyMission();
    std::filesystem::remove_all(tempDir);
}



