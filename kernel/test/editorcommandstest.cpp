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

#include <catch2/catch_test_macros.hpp>

#include "editor_commands.h"
#include "testcase.h"
#include "rs-engine/appcontext.h"
#include "rs-engine/system/system.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/animationloader.h"
#include "rs-kernel/mgr/agentmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/modmanager.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/weapon.h"
#include "rs-kernel/model/leveldata.h"

TEST_CASE("Editor Commands for Vehicles and Routes", "[editor][commands][vehicle]") {
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

    SECTION("DeleteVehicleCommand route pruning, index shifting, and driver handling") {
        LevelData::LevelDataAll ld;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);
        REQUIRE(m->buildNavigationGraph());

        // Spawn Vehicle 0 (Regular car)
        rs_knl::TilePoint v0Pos(20, 20, 1);
        rs_knl::Vehicle *v0 = m->spawnVehicle(rs_knl::Vehicle::kVehicleTypeRegularCar, v0Pos, 0, 100);
        REQUIRE(v0 != nullptr);

        // Spawn Vehicle 1 (Police car)
        rs_knl::TilePoint v1Pos(25, 25, 1);
        rs_knl::Vehicle *v1 = m->spawnVehicle(rs_knl::Vehicle::kVehicleTypePolice, v1Pos, 0, 120);
        REQUIRE(v1 != nullptr);

        size_t v0Idx = 0;
        while (v0Idx < m->numVehicles() && m->vehicle(v0Idx) != v0) ++v0Idx;
        REQUIRE(v0Idx < m->numVehicles());

        size_t v1Idx = 0;
        while (v1Idx < m->numVehicles() && m->vehicle(v1Idx) != v1) ++v1Idx;
        REQUIRE(v1Idx < m->numVehicles());
        REQUIRE(v0Idx < v1Idx);

        // Setup Driver (Ped 4) for Vehicle 0
        rs_knl::PedInstance *pDriver = m->ped(4);
        REQUIRE(pDriver != nullptr);
        pDriver->clearScriptSteps();
        v0->setDriver(pDriver);
        pDriver->putInVehicle(v0);

        // Driver route: 2 driving waypoints
        rs_knl::ScriptStep driveStep1;
        driveStep1.type = LevelData::kScenarioTypeGoto;
        driveStep1.targetPos = rs_knl::TilePoint(22, 20, 1);
        pDriver->addScriptStep(driveStep1);

        rs_knl::ScriptStep driveStep2;
        driveStep2.type = LevelData::kScenarioTypeGoto;
        driveStep2.targetPos = rs_knl::TilePoint(24, 20, 1);
        pDriver->addScriptStep(driveStep2);

        REQUIRE(pDriver->scriptSteps().size() == 2);

        // Setup Passenger (Ped 5) taking a trip in Vehicle 0:
        // Step 0: Walk to car
        // Step 1: Board Vehicle v0Idx
        // Step 2: In-car waypoint 1
        // Step 3: In-car waypoint 2
        // Step 4: Alight Vehicle
        // Step 5: Walk to destination
        rs_knl::PedInstance *pPassenger = m->ped(5);
        REQUIRE(pPassenger != nullptr);
        pPassenger->clearScriptSteps();

        rs_knl::ScriptStep stepWalkPre;
        stepWalkPre.type = LevelData::kScenarioTypeGoto;
        stepWalkPre.targetPos = rs_knl::TilePoint(10, 10, 1);
        pPassenger->addScriptStep(stepWalkPre);

        rs_knl::ScriptStep stepBoard;
        stepBoard.type = LevelData::kScenarioTypeUseVehicle;
        stepBoard.targetEntityId = static_cast<int>(v0Idx);
        pPassenger->addScriptStep(stepBoard);

        rs_knl::ScriptStep stepDriveA;
        stepDriveA.type = LevelData::kScenarioTypeGoto;
        stepDriveA.targetPos = rs_knl::TilePoint(22, 20, 1);
        pPassenger->addScriptStep(stepDriveA);

        rs_knl::ScriptStep stepDriveB;
        stepDriveB.type = LevelData::kScenarioTypeGoto;
        stepDriveB.targetPos = rs_knl::TilePoint(24, 20, 1);
        pPassenger->addScriptStep(stepDriveB);

        rs_knl::ScriptStep stepAlight;
        stepAlight.type = LevelData::kScenarioTypeLeaveVehicle;
        pPassenger->addScriptStep(stepAlight);

        rs_knl::ScriptStep stepWalkPost;
        stepWalkPost.type = LevelData::kScenarioTypeGoto;
        stepWalkPost.targetPos = rs_knl::TilePoint(30, 30, 1);
        pPassenger->addScriptStep(stepWalkPost);

        REQUIRE(pPassenger->scriptSteps().size() == 6);

        // Setup Ped 6 using Vehicle 1 (v1Idx)
        rs_knl::PedInstance *pOtherPed = m->ped(6);
        REQUIRE(pOtherPed != nullptr);
        pOtherPed->clearScriptSteps();

        rs_knl::ScriptStep stepOtherBoard;
        stepOtherBoard.type = LevelData::kScenarioTypeUseVehicle;
        stepOtherBoard.targetEntityId = static_cast<int>(v1Idx);
        pOtherPed->addScriptStep(stepOtherBoard);

        rs_knl::ScriptStep stepOtherAlight;
        stepOtherAlight.type = LevelData::kScenarioTypeLeaveVehicle;
        pOtherPed->addScriptStep(stepOtherAlight);

        REQUIRE(pOtherPed->scriptSteps().size() == 2);
        REQUIRE(pOtherPed->scriptSteps()[0].targetEntityId == static_cast<int>(v1Idx));

        size_t origVehCount = m->numVehicles();

        // Execute DeleteVehicleCommand on Vehicle 0
        rs_win_editor::DeleteVehicleCommand delCmd(m, v0);
        bool execSuccess = delCmd.execute();
        REQUIRE(execSuccess);

        // Verify vehicle count decreased
        REQUIRE(m->numVehicles() == origVehCount - 1);

        // Verify driver was detached and driving route cleared
        CHECK(pDriver->inVehicle() == nullptr);
        CHECK(pDriver->scriptSteps().empty());

        // Verify passenger's in-vehicle route block (steps 1..4) was cleanly removed!
        // Remaining steps should be only stepWalkPre and stepWalkPost (size 2).
        REQUIRE(pPassenger->scriptSteps().size() == 2);
        CHECK(pPassenger->scriptSteps()[0].type == LevelData::kScenarioTypeGoto);
        CHECK(pPassenger->scriptSteps()[0].targetPos.tx == 10);
        CHECK(pPassenger->scriptSteps()[1].type == LevelData::kScenarioTypeGoto);
        CHECK(pPassenger->scriptSteps()[1].targetPos.tx == 30);

        // Verify other ped's targetEntityId was shifted down from v1Idx to v1Idx - 1!
        REQUIRE(pOtherPed->scriptSteps().size() == 2);
        CHECK(pOtherPed->scriptSteps()[0].targetEntityId == static_cast<int>(v1Idx - 1));

        // Now test Undo
        bool undoSuccess = delCmd.undo();
        REQUIRE(undoSuccess);

        // Verify vehicle count restored
        REQUIRE(m->numVehicles() == origVehCount);

        // Verify vehicle 0 restored at exact slot v0Idx
        rs_knl::Vehicle *restoredV0 = m->vehicle(v0Idx);
        REQUIRE(restoredV0 != nullptr);
        CHECK(restoredV0->getDriver() == pDriver);
        CHECK(pDriver->inVehicle() == restoredV0);

        // Verify driver's route restored
        REQUIRE(pDriver->scriptSteps().size() == 2);
        CHECK(pDriver->scriptSteps()[0].targetPos.tx == 22);
        CHECK(pDriver->scriptSteps()[1].targetPos.tx == 24);

        // Verify passenger's full route restored
        REQUIRE(pPassenger->scriptSteps().size() == 6);
        CHECK(pPassenger->scriptSteps()[0].type == LevelData::kScenarioTypeGoto);
        CHECK(pPassenger->scriptSteps()[1].type == LevelData::kScenarioTypeUseVehicle);
        CHECK(pPassenger->scriptSteps()[1].targetEntityId == static_cast<int>(v0Idx));
        CHECK(pPassenger->scriptSteps()[4].type == LevelData::kScenarioTypeLeaveVehicle);
        CHECK(pPassenger->scriptSteps()[5].type == LevelData::kScenarioTypeGoto);

        // Verify other ped's targetEntityId restored to v1Idx
        REQUIRE(pOtherPed->scriptSteps().size() == 2);
        CHECK(pOtherPed->scriptSteps()[0].targetEntityId == static_cast<int>(v1Idx));

        misMgr.destroyMission();
    }

    SECTION("ModifyEntityAttributesCommand driver assignment and route transfer") {
        LevelData::LevelDataAll ld;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);
        REQUIRE(m->buildNavigationGraph());

        rs_knl::TilePoint vPos(20, 20, 1);
        rs_knl::Vehicle *v = m->spawnVehicle(rs_knl::Vehicle::kVehicleTypeRegularCar, vPos, 0, 100);
        REQUIRE(v != nullptr);

        rs_knl::PedInstance *pDriverA = m->ped(4);
        rs_knl::PedInstance *pDriverB = m->ped(5);
        REQUIRE(pDriverA != nullptr);
        REQUIRE(pDriverB != nullptr);
        pDriverA->clearScriptSteps();
        pDriverB->clearScriptSteps();

        v->setDriver(pDriverA);
        pDriverA->putInVehicle(v);

        // Add route to driver A
        rs_knl::ScriptStep step1;
        step1.type = LevelData::kScenarioTypeGoto;
        step1.targetPos = rs_knl::TilePoint(22, 20, 1);
        pDriverA->addScriptStep(step1);

        rs_knl::ScriptStep step2;
        step2.type = LevelData::kScenarioTypeGoto;
        step2.targetPos = rs_knl::TilePoint(24, 20, 1);
        pDriverA->addScriptStep(step2);

        REQUIRE(pDriverA->scriptSteps().size() == 2);
        REQUIRE(pDriverB->scriptSteps().empty());

        // 1. Detach driver (change driver to nullptr)
        rs_win_editor::ModifyEntityAttributesCommand::Attrs oldAttrs;
        oldAttrs.health = v->health();
        oldAttrs.direction = v->direction();
        oldAttrs.pDriver = pDriverA;

        rs_win_editor::ModifyEntityAttributesCommand::Attrs newAttrsNoDriver = oldAttrs;
        newAttrsNoDriver.pDriver = nullptr;

        rs_win_editor::ModifyEntityAttributesCommand detachCmd(m, v, oldAttrs, newAttrsNoDriver);
        bool detachOk = detachCmd.execute();
        REQUIRE(detachOk);

        CHECK(v->getDriver() == nullptr);
        CHECK(pDriverA->inVehicle() == nullptr);
        CHECK(pDriverA->scriptSteps().empty()); // Route cleared so former driver does not wander

        // Undo detach
        bool undoDetachOk = detachCmd.undo();
        REQUIRE(undoDetachOk);

        CHECK(v->getDriver() == pDriverA);
        CHECK(pDriverA->inVehicle() == v);
        REQUIRE(pDriverA->scriptSteps().size() == 2); // Route restored

        // 2. Reassign driver from Driver A to Driver B
        rs_win_editor::ModifyEntityAttributesCommand::Attrs newAttrsDriverB = oldAttrs;
        newAttrsDriverB.pDriver = pDriverB;

        rs_win_editor::ModifyEntityAttributesCommand reassignCmd(m, v, oldAttrs, newAttrsDriverB);
        bool reassignOk = reassignCmd.execute();
        REQUIRE(reassignOk);

        CHECK(v->getDriver() == pDriverB);
        CHECK(pDriverB->inVehicle() == v);
        CHECK(pDriverA->inVehicle() == nullptr);
        CHECK(pDriverA->scriptSteps().empty()); // Old driver's route cleared
        REQUIRE(pDriverB->scriptSteps().size() == 2); // New driver inherited vehicle route!
        CHECK(pDriverB->scriptSteps()[0].targetPos.tx == 22);

        // Undo reassignment
        bool undoReassignOk = reassignCmd.undo();
        REQUIRE(undoReassignOk);

        CHECK(v->getDriver() == pDriverA);
        CHECK(pDriverA->inVehicle() == v);
        CHECK(pDriverB->inVehicle() == nullptr);
        REQUIRE(pDriverA->scriptSteps().size() == 2); // Driver A route restored
        CHECK(pDriverB->scriptSteps().empty());       // Driver B route cleared back to original empty

        misMgr.destroyMission();
    }

    SECTION("SpawnPedCommand and DeletePedCommand for Player's Agents") {
        LevelData::LevelDataAll ld;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);
        REQUIRE(m->buildNavigationGraph());

        int initialPlayerAgents = m->countPlayerAgents();
        REQUIRE(initialPlayerAgents == 4);

        // Delete player agent slot 1
        rs_knl::PedInstance *pAgent1 = nullptr;
        for (size_t i = 0; i < m->numPeds(); ++i) {
            auto *p = m->ped(i);
            if (p && p->isOurAgent() && p->id() == 1) {
                pAgent1 = p;
                break;
            }
        }
        REQUIRE(pAgent1 != nullptr);

        rs_win_editor::DeletePedCommand delCmd(m, pAgent1);
        REQUIRE(delCmd.execute());
        CHECK(m->countPlayerAgents() == 3);
        CHECK(m->findNextFreePlayerSlot() == 1);
        CHECK(m->getSquadSpawnPoint(1).tx == -1);

        // Spawn player agent into slot 1 using SpawnPedCommand
        rs_knl::TilePoint spawnPos(15, 25, 2);
        rs_win_editor::SpawnPedCommand spawnCmd(
            m, rs_knl::PedInstance::kPedTypeAgent, spawnPos, 96, 1,
            false, 0, 0x10, 0, true, 1);
        REQUIRE(spawnCmd.execute());
        CHECK(m->countPlayerAgents() == 4);
        CHECK(m->findNextFreePlayerSlot() == -1);
        rs_knl::PedInstance *spawnedPed = spawnCmd.getSpawnedPed();
        REQUIRE(spawnedPed != nullptr);
        CHECK(spawnedPed->isOurAgent());
        CHECK(spawnedPed->id() == 1);
        CHECK(spawnedPed->direction() == 96);
        CHECK(m->getSquadSpawnPoint(1).tx == 15);
        CHECK(m->getSquadSpawnPoint(1).ty == 25);
        CHECK(m->getSquadSpawnPoint(1).tz == 2);
        CHECK(spawnedPed->tileZ() == 2);

        // Undo spawn
        REQUIRE(spawnCmd.undo());
        CHECK(m->countPlayerAgents() == 3);
        CHECK(m->findNextFreePlayerSlot() == 1);
        CHECK(m->getSquadSpawnPoint(1).tx == -1);

        // Redo spawn
        REQUIRE(spawnCmd.execute());
        CHECK(m->countPlayerAgents() == 4);
        CHECK(m->findNextFreePlayerSlot() == -1);

        misMgr.destroyMission();
    }

    SECTION("Mission 1 deleting and re-adding player agents preserves walkable layer") {
        LevelData::LevelDataAll ld;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);
        REQUIRE(m->buildNavigationGraph());

        // Verify initial squad agents are at tz=2 and on walkable tiles
        for (size_t i = 0; i < 4; ++i) {
            rs_knl::PedInstance *p = m->ped(i);
            REQUIRE(p != nullptr);
            REQUIRE(p->isOurAgent());
            CHECK(p->tileZ() == 2);
            CHECK(m->isTileWalkable(p->tileX(), p->tileY(), p->tileZ()));
        }

        // Delete player agent 0
        rs_knl::PedInstance *p0 = m->ped(0);
        rs_knl::TilePoint origPos = p0->position();
        uint8_t origDir = p0->direction();
        rs_win_editor::DeletePedCommand delCmd(m, p0);
        REQUIRE(delCmd.execute());
        CHECK(m->countPlayerAgents() == 3);
        CHECK(m->findNextFreePlayerSlot() == 0);

        // Re-spawn player agent 0 at origPos (tz=2) using SpawnPedCommand
        rs_win_editor::SpawnPedCommand spawnCmd(
            m, rs_knl::PedInstance::kPedTypeAgent, origPos, origDir, 1,
            false, 0, 0x10, 0, true, 0);
        REQUIRE(spawnCmd.execute());
        CHECK(m->countPlayerAgents() == 4);

        rs_knl::PedInstance *reAdded = spawnCmd.getSpawnedPed();
        REQUIRE(reAdded != nullptr);
        CHECK(reAdded->isOurAgent());
        CHECK(reAdded->id() == 0);
        CHECK(reAdded->tileZ() == 2);
        CHECK(m->getSquadSpawnPoint(0).tz == 2);
        CHECK(m->isTileWalkable(reAdded->tileX(), reAdded->tileY(), reAdded->tileZ()));

        misMgr.destroyMission();
    }

    SECTION("ModifyPedRouteCommand properly manages loop reset without duplicating it") {
        LevelData::LevelDataAll ld;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);
        REQUIRE(m->buildNavigationGraph());

        rs_knl::PedInstance *pCiv = m->ped(4);
        REQUIRE(pCiv != nullptr);
        pCiv->clearScriptSteps();
        pCiv->setScriptLooping(false);

        // Step 1: Initial spawn waypoint and loop = true
        rs_knl::ScriptStep wp1;
        wp1.type = LevelData::kScenarioTypeGoto;
        wp1.targetPos = pCiv->position();

        std::vector<rs_knl::ScriptStep> steps1 = { wp1 };
        rs_win_editor::ModifyPedRouteCommand cmd1(m, pCiv, {}, steps1, false, true);
        REQUIRE(cmd1.execute());
        REQUIRE(pCiv->scriptSteps().size() == 2);
        CHECK(pCiv->scriptSteps()[0].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->scriptSteps()[1].type == LevelData::kScenarioTypeReset);
        CHECK(pCiv->isScriptLooping() == true);

        // Step 2: Add Waypoint 2. Even if oldSteps/newSteps contained Reset, command sanitizes it
        rs_knl::ScriptStep wp2;
        wp2.type = LevelData::kScenarioTypeGoto;
        wp2.targetPos = rs_knl::TilePoint(pCiv->tileX() + 2, pCiv->tileY(), pCiv->tileZ());

        auto oldSteps2 = pCiv->scriptSteps(); // contains [0] wp1, [1] Reset
        auto newSteps2 = oldSteps2;
        newSteps2.push_back(wp2); // simulates appending after Reset

        rs_win_editor::ModifyPedRouteCommand cmd2(m, pCiv, oldSteps2, newSteps2, true, true);
        REQUIRE(cmd2.execute());

        // Must have exactly 3 steps: wp1, wp2, Reset (no intermediate reset!)
        REQUIRE(pCiv->scriptSteps().size() == 3);
        CHECK(pCiv->scriptSteps()[0].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->scriptSteps()[1].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->scriptSteps()[2].type == LevelData::kScenarioTypeReset);
        CHECK(pCiv->isScriptLooping() == true);

        // Step 3: Add Waypoint 3
        rs_knl::ScriptStep wp3;
        wp3.type = LevelData::kScenarioTypeGoto;
        wp3.targetPos = rs_knl::TilePoint(pCiv->tileX() + 4, pCiv->tileY(), pCiv->tileZ());

        auto oldSteps3 = pCiv->scriptSteps();
        auto newSteps3 = oldSteps3;
        newSteps3.push_back(wp3);

        rs_win_editor::ModifyPedRouteCommand cmd3(m, pCiv, oldSteps3, newSteps3, true, true);
        REQUIRE(cmd3.execute());

        REQUIRE(pCiv->scriptSteps().size() == 4);
        CHECK(pCiv->scriptSteps()[0].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->scriptSteps()[1].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->scriptSteps()[2].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->scriptSteps()[3].type == LevelData::kScenarioTypeReset);
        CHECK(pCiv->isScriptLooping() == true);

        // Undo Step 3
        REQUIRE(cmd3.undo());
        REQUIRE(pCiv->scriptSteps().size() == 3);
        CHECK(pCiv->scriptSteps()[0].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->scriptSteps()[1].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->scriptSteps()[2].type == LevelData::kScenarioTypeReset);

        // Toggle loop to false
        auto stepsNoLoop = pCiv->scriptSteps();
        rs_win_editor::ModifyPedRouteCommand cmdNoLoop(m, pCiv, stepsNoLoop, stepsNoLoop, true, false);
        REQUIRE(cmdNoLoop.execute());
        REQUIRE(pCiv->scriptSteps().size() == 2);
        CHECK(pCiv->scriptSteps()[0].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->scriptSteps()[1].type == LevelData::kScenarioTypeGoto);
        CHECK(pCiv->isScriptLooping() == false);

        misMgr.destroyMission();
    }

    SECTION("Route starting at spawn point advances directly to Waypoint 2 at mission start") {
        LevelData::LevelDataAll ld;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);
        REQUIRE(m->buildNavigationGraph());

        rs_knl::PedInstance *pCiv = m->ped(4);
        REQUIRE(pCiv != nullptr);
        pCiv->setPosition(30, 30, 1, 128, 128, 0);

        // WP1 at spawn point (30, 30, 1, 128, 128)
        rs_knl::ScriptStep wp1;
        wp1.type = LevelData::kScenarioTypeGoto;
        wp1.targetPos = rs_knl::TilePoint(30, 30, 1, 128, 128);

        // WP2 at destination (35, 30, 1, 128, 128)
        rs_knl::ScriptStep wp2;
        wp2.type = LevelData::kScenarioTypeGoto;
        wp2.targetPos = rs_knl::TilePoint(35, 30, 1, 128, 128);

        rs_knl::ScriptStep resetStep;
        resetStep.type = LevelData::kScenarioTypeReset;

        pCiv->clearScriptSteps();
        pCiv->addScriptStep(wp1);
        pCiv->addScriptStep(wp2);
        pCiv->addScriptStep(resetStep);
        pCiv->setScriptLooping(true);

        rs_knl::MissionManager::buildScriptedActionsForPed(m, pCiv);

        // Verify that defaultAction() still starts at WP1
        rs_knl::MovementAction *act1 = pCiv->defaultAction();
        REQUIRE(act1 != nullptr);
        CHECK(dynamic_cast<rs_knl::WalkAction*>(act1) != nullptr);

        // Verify that currentAction on mission start is set to WP2 (the next waypoint),
        // skipping redundant movement to spawn where the ped already is
        rs_knl::MovementAction *curAct = pCiv->currentAction();
        REQUIRE(curAct != nullptr);
        CHECK(curAct == act1->next());

        // Verify that WalkAction at spawn succeeds immediately
        rs_knl::WalkAction immediateWalk(rs_knl::TilePoint(30, 30, 1, 128, 128));
        immediateWalk.execute(10, m, pCiv);
        CHECK(immediateWalk.isFinished());
        CHECK_FALSE(immediateWalk.hasFailed());

        misMgr.destroyMission();
    }
}

TEST_CASE("Editor Commands for Ground Weapons", "[editor][commands][weapon]") {
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

    SECTION("Mission 22 contains authentic dropped shotgun near dead police officer") {
        LevelData::LevelDataAll ld22;
        REQUIRE(misMgr.load_level_data(22, ld22));
        rs_knl::Mission *m22 = misMgr.create_mission(ld22);
        REQUIRE(m22 != nullptr);

        // In Mission 22, at X:49 Y:80 there is a dropped Shotgun next to a dead police officer
        REQUIRE(m22->numWeaponsOnGround() >= 1);
        rs_knl::WeaponInstance *pFoundWeapon = nullptr;
        for (size_t i = 0; i < m22->numWeaponsOnGround(); ++i) {
            auto *w = m22->weaponOnGround(i);
            if (w && w->tileX() == 49 && w->tileY() == 80) {
                pFoundWeapon = w;
                break;
            }
        }
        REQUIRE(pFoundWeapon != nullptr);
        CHECK(pFoundWeapon->type() == rs_knl::Weapon::Shotgun);
        CHECK(pFoundWeapon->getDrawAnimation() == 370);
        CHECK(pFoundWeapon->isDrawable() == true);
        CHECK(pFoundWeapon->tileX() == 49);
        CHECK(pFoundWeapon->tileY() == 80);

        auto *p154 = m22->ped(154);
        REQUIRE(p154 != nullptr);
        CHECK(p154->type() == rs_knl::PedInstance::kPedTypePolice);
        CHECK(p154->health() == -1);
        CHECK(p154->tileX() == 49);
        CHECK(p154->tileY() == 80);

        rs_win_editor::DeletePedCommand delPedCmd(m22, p154);
        REQUIRE(delPedCmd.execute());
        REQUIRE(delPedCmd.undo());

        // Test deleting it with DeleteWeaponCommand and undoing
        size_t initialCount = m22->numWeaponsOnGround();
        void *selectedRaw = static_cast<void*>(pFoundWeapon);

        rs_win_editor::DeleteWeaponCommand delCmd(m22, pFoundWeapon);
        REQUIRE(delCmd.execute());
        CHECK(m22->numWeaponsOnGround() == initialCount - 1);

        // Safe pointer validation without dynamic_cast on deleted memory
        bool valid = false;
        for (size_t i = 0; i < m22->numWeaponsOnGround(); ++i) {
            if (static_cast<void*>(m22->weaponOnGround(i)) == selectedRaw) { valid = true; break; }
        }
        CHECK(valid == false);

        REQUIRE(delCmd.undo());
        CHECK(m22->numWeaponsOnGround() == initialCount);
        auto *restoredWeapon = m22->weaponOnGround(m22->numWeaponsOnGround() - 1);
        REQUIRE(restoredWeapon != nullptr);
        CHECK(restoredWeapon->type() == rs_knl::Weapon::Shotgun);
        CHECK(restoredWeapon->tileX() == 49);
        CHECK(restoredWeapon->tileY() == 80);

        misMgr.destroyMission();
    }

    SECTION("SpawnWeaponCommand, MoveEntityCommand, and DeleteWeaponCommand undo/redo lifecycle") {
        LevelData::LevelDataAll ld1;
        REQUIRE(misMgr.load_level_data(1, ld1));
        rs_knl::Mission *m = misMgr.create_mission(ld1);
        REQUIRE(m != nullptr);

        size_t baselineCount = m->numWeaponsOnGround();

        // 1. Spawn a ground weapon (Minigun) at (25, 30, 1, 100, 150)
        rs_knl::TilePoint spawnPt(25, 30, 1, 100, 150);
        rs_win_editor::SpawnWeaponCommand spawnCmd(m, rs_knl::Weapon::Minigun, spawnPt);
        REQUIRE(spawnCmd.execute());
        REQUIRE(m->numWeaponsOnGround() == baselineCount + 1);

        auto *pSpawned = m->weaponOnGround(m->numWeaponsOnGround() - 1);
        REQUIRE(pSpawned != nullptr);
        CHECK(pSpawned->type() == rs_knl::Weapon::Minigun);
        CHECK(pSpawned->getDrawAnimation() == 372); // Minigun on ground anim
        CHECK(pSpawned->tileX() == 25);
        CHECK(pSpawned->tileY() == 30);
        CHECK(pSpawned->tileZ() == 1);
        CHECK(pSpawned->position().ox == 100);
        CHECK(pSpawned->position().oy == 150);

        // Undo spawn
        REQUIRE(spawnCmd.undo());
        CHECK(m->numWeaponsOnGround() == baselineCount);

        // Redo spawn
        REQUIRE(spawnCmd.execute());
        CHECK(m->numWeaponsOnGround() == baselineCount + 1);
        pSpawned = m->weaponOnGround(m->numWeaponsOnGround() - 1);
        REQUIRE(pSpawned != nullptr);

        // 2. Move weapon drop to (28, 35, 2, 50, 60)
        rs_knl::TilePoint oldPos = pSpawned->position();
        rs_knl::TilePoint newPos(28, 35, 2, 50, 60);
        rs_win_editor::MoveEntityCommand moveCmd(pSpawned, oldPos, newPos);
        REQUIRE(moveCmd.execute());
        CHECK(pSpawned->tileX() == 28);
        CHECK(pSpawned->tileY() == 35);
        CHECK(pSpawned->tileZ() == 2);

        REQUIRE(moveCmd.undo());
        CHECK(pSpawned->tileX() == 25);
        CHECK(pSpawned->tileY() == 30);
        CHECK(pSpawned->tileZ() == 1);

        REQUIRE(moveCmd.execute());
        CHECK(pSpawned->tileX() == 28);
        CHECK(pSpawned->tileY() == 35);

        // 3. Delete weapon
        rs_win_editor::DeleteWeaponCommand delCmd(m, pSpawned);
        REQUIRE(delCmd.execute());
        CHECK(m->numWeaponsOnGround() == baselineCount);

        REQUIRE(delCmd.undo());
        CHECK(m->numWeaponsOnGround() == baselineCount + 1);
        auto *pRestored = m->weaponOnGround(m->numWeaponsOnGround() - 1);
        REQUIRE(pRestored != nullptr);
        CHECK(pRestored->type() == rs_knl::Weapon::Minigun);
        CHECK(pRestored->tileX() == 28);
        CHECK(pRestored->tileY() == 35);

        misMgr.destroyMission();
    }
}
