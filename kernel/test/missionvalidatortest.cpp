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

#include <unordered_set>
#include <catch2/catch_test_macros.hpp>
#include "mission_validator.h"
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
#include "rs-kernel/model/static.h"
#include "rs-kernel/model/objectivedesc.h"
#include "rs-kernel/ia/behaviour.h"

TEST_CASE("MissionValidator Collective Baseline & Boundary Tests", "[editor][validator]") {
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

    SECTION("All 50 Original Missions Pass Validation Cleanly (Collective Baseline)") {
        for (int mId = 1; mId <= 50; ++mId) {
            auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
            auto &ld = *ldPtr;
            if (!misMgr.load_level_data(mId, ld)) continue;
            rs_knl::Mission *m = misMgr.create_mission(ld);
            if (!m) continue;

            auto report = rs_win_editor::MissionValidator::validate(m);
            if (!report.issues.empty()) {
                for (const auto &iss : report.issues) {
                    UNSCOPED_INFO("Mission " << mId << " issue: " << iss.entityDesc << " - " << iss.message);
                }
            }
            INFO("Mission " << mId << " reported " << report.issues.size() << " issue(s)");
            CHECK(report.passed());
            CHECK(report.errorCount == 0);
            CHECK(report.warningCount == 0);
            CHECK(report.issues.empty());

            misMgr.destroyMission();
        }
    }

    SECTION("Custom Mission 99 Delay Timer and Cybernetic Speed Scaling") {
        if (std::filesystem::exists("synd/SYNDICAT/DATA/custom/game99.dat")) {
            auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
            bool loaded = misMgr.load_level_data(99, *ldPtr, "synd/SYNDICAT/DATA/custom/game99.dat");
            REQUIRE(loaded);
            rs_knl::Mission *m = misMgr.create_mission(*ldPtr, "synd/SYNDICAT/DATA/custom/map99.dat");
            REQUIRE(m != nullptr);

            misMgr.setMission(m);
            m->buildNavigationGraph();

            // Verify mission properties: Tier 3 enemy cybernetics (if game99 is currently configured with Delay Timer)
            int rootBlue = m->getRootTacticIndex(1);
            if (m->techLevel() == 3 && rootBlue > 0 && m->tactics().entries[rootBlue].mode == LevelData::kAITacticModeDelayTimer) {
                CHECK(m->techLevel() == 3);
                const auto &rootTactic = m->tactics().entries[rootBlue];
                CHECK(rootTactic.mode == LevelData::kAITacticModeDelayTimer);
                CHECK(rs_utl::READ_LE_UINT16(rootTactic.param1) == 50);

                // Simulate ticks 0 to 60 (100ms per tick)
                for (int tick = 0; tick <= 60; ++tick) {
                    m->handleTick(100, 100);

                    // During Delay Timer (ticks 0..49 / 0..4.9s), Blue agents must remain stationary
                    if (tick < 50) {
                        for (size_t i = 0; i < m->numPeds(); ++i) {
                            auto *p = m->ped(i);
                            if (!p || p->factionId() != 1 || p->type() != rs_knl::PedInstance::kPedTypeAgent) continue;
                            CHECK(p->speed() == 0);
                        }
                    }
                    // Once Delay Timer expires (tick 51+ / 5.1s+), Blue agents advance to Hunter AI and accelerate to V3 mod speed (712)
                    if (tick >= 52) {
                        for (size_t i = 0; i < m->numPeds(); ++i) {
                            auto *p = m->ped(i);
                            if (!p || p->factionId() != 1 || p->type() != rs_knl::PedInstance::kPedTypeAgent) continue;
                            CHECK(p->isHunter());
                            CHECK(p->speed() == 712);
                        }
                    }
                }
            }

            misMgr.destroyMission();
        }
    }

    SECTION("Missing Player Squad is Flagged as Error") {
        auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
        auto &ld = *ldPtr;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);

        std::vector<rs_knl::PedInstance*> playerAgents;
        for (size_t i = 0; i < m->numPeds(); ++i) {
            if (m->ped(i) && m->ped(i)->isOurAgent()) {
                playerAgents.push_back(m->ped(i));
            }
        }
        for (auto *p : playerAgents) {
            m->removePed(p);
        }

        auto report = rs_win_editor::MissionValidator::validate(m);
        CHECK(report.hasErrors());
        bool foundSquadError = false;
        for (const auto &iss : report.issues) {
            if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
                iss.entityDesc.find("Player Squad") != std::string::npos) {
                foundSquadError = true;
                break;
            }
        }
        CHECK(foundSquadError);
        misMgr.destroyMission();
    }

    SECTION("Squad Agent Placed on Unwalkable Tile is Flagged as Error") {
        auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
        auto &ld = *ldPtr;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);
        m->buildNavigationGraph();

        // Find an unwalkable tile coordinate with non-zero tile
        int unwalkX = -1, unwalkY = -1;
        for (int y = 0; y < m->map()->maxTy(); ++y) {
            for (int x = 0; x < m->map()->maxTx(); ++x) {
                if (m->map()->getTileAt(x, y, 1) && m->map()->getTileAt(x, y, 1)->id() != 0 && !m->isTileWalkable(x, y, 1)) {
                    unwalkX = x;
                    unwalkY = y;
                    break;
                }
            }
            if (unwalkX >= 0) break;
        }

        if (unwalkX >= 0) {
            // Reposition Agent #1 to the unwalkable position
            for (size_t i = 0; i < m->numPeds(); ++i) {
                auto *p = m->ped(i);
                if (p && p->isOurAgent()) {
                    p->setTileX(unwalkX);
                    p->setTileY(unwalkY);
                    p->setTileZ(1);
                    break;
                }
            }

            auto report = rs_win_editor::MissionValidator::validate(m);
            CHECK(report.hasErrors());
            bool foundUnwalkableError = false;
            for (const auto &iss : report.issues) {
                if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
                    iss.position.tx == unwalkX && iss.position.ty == unwalkY) {
                    foundUnwalkableError = true;
                    break;
                }
            }
            CHECK(foundUnwalkableError);
        }
        misMgr.destroyMission();
    }

    SECTION("Squad Agent Floating Over Empty Void Space is Flagged as Error") {
        auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
        auto &ld = *ldPtr;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);

        int voidX = 10, voidY = 10;
        for (int z = 0; z < m->map()->maxTz(); ++z) {
            m->map()->setTileAt(voidX, voidY, z, 0);
        }

        for (size_t i = 0; i < m->numPeds(); ++i) {
            auto *p = m->ped(i);
            if (p && p->isOurAgent()) {
                p->setTileX(voidX);
                p->setTileY(voidY);
                p->setTileZ(1);
                break;
            }
        }

        auto report = rs_win_editor::MissionValidator::validate(m);
        CHECK(report.hasErrors());
        bool foundVoidError = false;
        for (const auto &iss : report.issues) {
            if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
                iss.position.tx == voidX && iss.position.ty == voidY) {
                foundVoidError = true;
                break;
            }
        }
        CHECK(foundVoidError);
        misMgr.destroyMission();
    }

    SECTION("Entity Placed Outside Map Boundaries is Flagged as Error") {
        auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
        auto &ld = *ldPtr;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);

        rs_knl::TilePoint outOfBoundsPos(-5, -5, 0);
        rs_knl::PedInstance *badPed = m->spawnPed(rs_knl::PedInstance::kPedTypeCivilian, outOfBoundsPos, 0, 0);
        REQUIRE(badPed != nullptr);

        auto report = rs_win_editor::MissionValidator::validate(m);
        CHECK(report.hasErrors());
        bool foundOobError = false;
        for (const auto &iss : report.issues) {
            if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
                iss.position.tx == -5 && iss.position.ty == -5) {
                foundOobError = true;
                break;
            }
        }
        CHECK(foundOobError);
        misMgr.destroyMission();
    }

    SECTION("Civilian Floating Over Empty Void Column is Flagged as Warning") {
        auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
        auto &ld = *ldPtr;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);

        int voidX = 15, voidY = 15;
        for (int z = 0; z < m->map()->maxTz(); ++z) {
            m->map()->setTileAt(voidX, voidY, z, 0);
        }

        rs_knl::TilePoint voidPos(voidX, voidY, 1);
        rs_knl::PedInstance *badCiv = m->spawnPed(rs_knl::PedInstance::kPedTypeCivilian, voidPos, 0, 0);
        REQUIRE(badCiv != nullptr);

        auto report = rs_win_editor::MissionValidator::validate(m);
        CHECK(report.hasIssues());
        bool foundVoidWarn = false;
        for (const auto &iss : report.issues) {
            if (iss.severity == rs_win_editor::ValidationSeverity::Warning &&
                iss.position.tx == voidX && iss.position.ty == voidY) {
                foundVoidWarn = true;
                break;
            }
        }
        CHECK(foundVoidWarn);
        misMgr.destroyMission();
    }

    SECTION("Vehicle Embedded Inside Solid Wall is Flagged as Warning") {
        auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
        auto &ld = *ldPtr;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);

        // Find a solid wall tile
        int wallX = -1, wallY = -1, wallZ = -1;
        for (int z = 1; z < m->map()->maxTz(); ++z) {
            for (int y = 0; y < m->map()->maxTy(); ++y) {
                for (int x = 0; x < m->map()->maxTx(); ++x) {
                    auto *t = m->map()->getTileAt(x, y, z);
                    if (t && t->id() != 0 && t->type() == rs_eng::Tile::kWall) {
                        wallX = x;
                        wallY = y;
                        wallZ = z;
                        break;
                    }
                }
                if (wallX >= 0) break;
            }
            if (wallX >= 0) break;
        }

        if (wallX >= 0) {
            rs_knl::TilePoint wallPos(wallX, wallY, wallZ);
            rs_knl::Vehicle *v = m->spawnVehicle(0, wallPos, 0, 0);
            REQUIRE(v != nullptr);

            auto report = rs_win_editor::MissionValidator::validate(m);
            CHECK(report.hasIssues());
            bool foundWallWarn = false;
            for (const auto &iss : report.issues) {
                if (iss.severity == rs_win_editor::ValidationSeverity::Warning &&
                    iss.position.tx == wallX && iss.position.ty == wallY && iss.position.tz == wallZ) {
                    foundWallWarn = true;
                    break;
                }
            }
            CHECK(foundWallWarn);
        }
        misMgr.destroyMission();
    }

    SECTION("Missing Objective Target is Flagged as Error") {
        auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
        auto &ld = *ldPtr;
        REQUIRE(misMgr.load_level_data(1, ld));
        rs_knl::Mission *m = misMgr.create_mission(ld);
        REQUIRE(m != nullptr);

        // Add an assassinate objective with nullptr target
        rs_knl::ObjAssassinate *brokenObj = new rs_knl::ObjAssassinate(nullptr);
        m->insertObjective(m->numObjectives(), brokenObj);

        auto report = rs_win_editor::MissionValidator::validate(m);
        CHECK(report.hasErrors());
        bool foundObjError = false;
        for (const auto &iss : report.issues) {
            if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
                iss.entityDesc.find("Assassinate") != std::string::npos) {
                foundObjError = true;
                break;
            }
        }
        CHECK(foundObjError);
        misMgr.destroyMission();
    }
}

TEST_CASE("New Blank Canvas Mission Setup with 4 Squad Spawn Points Passes Validation", "[editor][validator]") {
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

    auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
    auto &ld = *ldPtr;
    REQUIRE(misMgr.load_level_data(1, ld));
    rs_knl::Mission *m = misMgr.create_mission(ld);
    REQUIRE(m != nullptr);

    // Clear all entities (matching MainWindow::newMission)
    while (m->numPeds() > 0) {
        m->removePed(m->ped(0));
    }
    while (m->numVehicles() > 0) {
        m->removeVehicle(m->vehicle(0));
    }
    while (m->numWeaponsOnGround() > 0) {
        m->removeGroundWeapon(m->weaponOnGround(0));
    }
    while (m->numStatics() > 0) {
        m->removeStatic(m->statics(0));
    }

    // Clear map and build concrete platform at (64, 48) on Z=1
    rs_knl::Map *pMap = m->map();
    REQUIRE(pMap != nullptr);
    for (int y = 0; y < pMap->maxTy(); ++y) {
        for (int x = 0; x < pMap->maxTx(); ++x) {
            for (int z = 0; z < pMap->maxTz(); ++z) {
                pMap->setTileAt(x, y, z, 0);
            }
        }
    }

    int cx = 64;
    int cy = 48;
    int spawnZ = 2;
    int groundZ = spawnZ - 1; // Z=1
    for (int y = cy - 2; y <= cy + 3; ++y) {
        for (int x = cx - 2; x <= cx + 3; ++x) {
            pMap->setTileAt(x, y, groundZ, 12);
        }
    }

    // Spawn 4 squad player agents
    m->spawnPlayerAgent(0, rs_knl::TilePoint(cx, cy, spawnZ), 0);
    m->spawnPlayerAgent(1, rs_knl::TilePoint(cx + 1, cy, spawnZ), 0);
    m->spawnPlayerAgent(2, rs_knl::TilePoint(cx, cy + 1, spawnZ), 0);
    m->spawnPlayerAgent(3, rs_knl::TilePoint(cx + 1, cy + 1, spawnZ), 0);

    REQUIRE(m->numPeds() == 4);
    for (size_t i = 0; i < 4; ++i) {
        rs_knl::PedInstance *p = m->ped(i);
        REQUIRE(p != nullptr);
        CHECK(p->isOurAgent());
        CHECK(p->id() == i);
    }

    m->buildNavigationGraph();

    auto report = rs_win_editor::MissionValidator::validate(m);
    for (const auto &iss : report.issues) {
        UNSCOPED_INFO("Issue: " << (iss.severity == rs_win_editor::ValidationSeverity::Error ? "ERROR" : "WARNING") << " [" << iss.entityDesc << "] at (" << iss.position.tx << ", " << iss.position.ty << ", " << iss.position.tz << "): " << iss.message);
    }
    CHECK(report.errorCount == 0);
    CHECK(!report.hasErrors());
    bool foundSquadError = false;
    for (const auto &iss : report.issues) {
        if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
            iss.entityDesc.find("Squad") != std::string::npos) {
            foundSquadError = true;
            break;
        }
    }
    CHECK(!foundSquadError);

    misMgr.destroyMission();
}
TEST_CASE("MissionValidator - Faction Agent Capacity and Slot Allocation", "[validator][factions]") {
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

    auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
    auto &ld = *ldPtr;
    REQUIRE(misMgr.load_level_data(1, ld));
    rs_knl::Mission *m = misMgr.create_mission(ld);
    REQUIRE(m != nullptr);

    // In Mission 1, there are initially 0 enemy agents
    CHECK(m->countFactionAgents(1) == 0);

    // Spawn 8 agents in Faction 1 (Blue)
    rs_knl::TilePoint spawnPos(10, 10, 1);
    std::vector<rs_knl::PedInstance*> f1Agents;
    for (int i = 0; i < 8; ++i) {
        rs_knl::PedInstance *p = m->spawnPed(rs_knl::PedInstance::kPedTypeAgent, spawnPos, 0, 0, 1);
        REQUIRE(p != nullptr);
        CHECK(p->factionId() == 1);
        CHECK(p->id() >= 8);
        CHECK(p->id() <= 15);
        f1Agents.push_back(p);
    }
    CHECK(m->countFactionAgents(1) == 8);

    // Attempting to spawn a 9th agent in Faction 1 should fail (returns nullptr)
    rs_knl::PedInstance *ninth = m->spawnPed(rs_knl::PedInstance::kPedTypeAgent, spawnPos, 0, 0, 1);
    CHECK(ninth == nullptr);
    CHECK(m->countFactionAgents(1) == 8);

    // Now validate: 8 agents in Faction 1 should be valid (no faction error)
    auto report = rs_win_editor::MissionValidator::validate(m);
    bool foundFacError = false;
    for (const auto &iss : report.issues) {
        if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
            iss.message.find("exceeds the maximum limit of 8 agents") != std::string::npos) {
            foundFacError = true;
            break;
        }
    }
    CHECK(!foundFacError);

    // Manually force a 9th agent to Faction 1 in memory to test validator detection
    rs_knl::PedInstance *forced = m->spawnPed(rs_knl::PedInstance::kPedTypeAgent, spawnPos, 0, 0, 0);
    REQUIRE(forced != nullptr);
    forced->setFactionId(1);
    CHECK(m->countFactionAgents(1) == 9);

    auto reportOver = rs_win_editor::MissionValidator::validate(m);
    CHECK(reportOver.hasErrors());
    bool foundOverError = false;
    for (const auto &iss : reportOver.issues) {
        if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
            iss.message.find("Faction 1 has 9 agents") != std::string::npos) {
            foundOverError = true;
            break;
        }
    }
    CHECK(foundOverError);

    misMgr.destroyMission();
}

TEST_CASE("MissionValidator - Pedestrian Total Capacity (256) and Duplicate ID Detection", "[validator][capacity]") {
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

    auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
    auto &ld = *ldPtr;
    REQUIRE(misMgr.load_level_data(1, ld));
    rs_knl::Mission *m = misMgr.create_mission(ld);
    REQUIRE(m != nullptr);

    // Initial mission 1 ped count
    size_t initialPeds = m->numPeds();
    CHECK(initialPeds < 256);

    // Spawn civilians until all 248 non-squad slots (8..255) are exhausted
    // (Slots 0..3 are squad agents, slots 4..7 are reserved Player Syndicate slots)
    rs_knl::TilePoint spawnPos(15, 15, 1);
    while (true) {
        rs_knl::PedInstance *p = m->spawnPed(rs_knl::PedInstance::kPedTypeCivilian, spawnPos, 0, 0, 0);
        if (!p) break;
    }
    // All 248 non-squad slots + 4 squad agents = 252 peds total
    CHECK(m->numPeds() == 252);

    // When slots are exhausted, spawning another ped must return nullptr
    rs_knl::PedInstance *overflowPed = m->spawnPed(rs_knl::PedInstance::kPedTypeCivilian, spawnPos, 0, 0, 0);
    CHECK(overflowPed == nullptr);
    CHECK(m->numPeds() == 252);

    // Verify all 252 peds have unique IDs
    std::unordered_set<uint16_t> idSet;
    for (size_t i = 0; i < m->numPeds(); ++i) {
        idSet.insert(m->ped(i)->id());
    }
    CHECK(idSet.size() == 252);

    // Validator should pass capacity check
    auto report252 = rs_win_editor::MissionValidator::validate(m);
    bool foundCapError = false;
    for (const auto &iss : report252.issues) {
        if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
            iss.message.find("exceeds the maximum limit of 256") != std::string::npos) {
            foundCapError = true;
            break;
        }
    }
    CHECK(!foundCapError);

    // Test validator flagging capacity when peds exceed 256
    // We add dummy peds with valid positions to exceed 256
    std::vector<rs_knl::PedInstance*> extraPeds;
    for (int i = 0; i < 5; ++i) {
        auto *dummy = new rs_knl::PedInstance(static_cast<uint16_t>(256 + i), m->map(), rs_knl::PedInstance::kPedTypeCivilian, false, 10);
        dummy->setPosition(spawnPos.tx, spawnPos.ty, spawnPos.tz, 0, 0, 0);
        m->addPed(dummy);
        extraPeds.push_back(dummy);
    }
    CHECK(m->numPeds() == 257);
    auto reportOver256 = rs_win_editor::MissionValidator::validate(m);
    CHECK(reportOver256.hasErrors());
    bool foundOver256 = false;
    for (const auto &iss : reportOver256.issues) {
        if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
            iss.message.find("exceeds the maximum limit of 256") != std::string::npos) {
            foundOver256 = true;
            break;
        }
    }
    CHECK(foundOver256);

    // Remove the extra peds (removePed automatically deletes each ped)
    for (auto *p : extraPeds) {
        m->removePed(p);
    }
    CHECK(m->numPeds() == 252);

    // Inject a duplicate ID to test duplicate detection
    uint16_t origId = m->ped(10)->id();
    m->ped(10)->setId(m->ped(11)->id());
    auto reportDup = rs_win_editor::MissionValidator::validate(m);
    CHECK(reportDup.hasErrors());
    bool foundDupError = false;
    for (const auto &iss : reportDup.issues) {
        if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
            iss.message.find("is duplicated") != std::string::npos) {
            foundDupError = true;
            break;
        }
    }
    CHECK(foundDupError);
    m->ped(10)->setId(origId); // restore

    misMgr.destroyMission();
}

TEST_CASE("MissionValidator Vehicle Dependencies and Route Step Validation", "[editor][validator][vehicle]") {
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

    auto ldPtr = std::make_unique<LevelData::LevelDataAll>();
    auto &ld = *ldPtr;
    REQUIRE(misMgr.load_level_data(1, ld));
    rs_knl::Mission *m = misMgr.create_mission(ld);
    REQUIRE(m != nullptr);

    // Baseline mission 1 should pass cleanly
    auto baseReport = rs_win_editor::MissionValidator::validate(m);
    REQUIRE(baseReport.passed());

    // 1. Pedestrian with UseVehicle targeting invalid/deleted vehicle ID (-1)
    rs_knl::PedInstance *ped = m->ped(4);
    REQUIRE(ped != nullptr);
    rs_knl::ScriptStep invalidStep;
    invalidStep.type = LevelData::kScenarioTypeUseVehicle;
    invalidStep.targetEntityId = -1;
    ped->addScriptStep(invalidStep);

    auto errReport = rs_win_editor::MissionValidator::validate(m);
    CHECK(errReport.hasErrors());
    bool foundInvalidVehErr = false;
    for (const auto &iss : errReport.issues) {
        if (iss.severity == rs_win_editor::ValidationSeverity::Error &&
            iss.message.find("References invalid or deleted Vehicle") != std::string::npos) {
            foundInvalidVehErr = true;
            break;
        }
    }
    CHECK(foundInvalidVehErr);

    // Clear step
    ped->clearScriptSteps();

    // 2. Pedestrian targeting a destroyed vehicle
    rs_knl::Vehicle *v = m->vehicle(0);
    REQUIRE(v != nullptr);
    int origHealth = v->health();
    v->setHealth(0);

    rs_knl::ScriptStep destroyedStep;
    destroyedStep.type = LevelData::kScenarioTypeUseVehicle;
    destroyedStep.targetEntityId = 0;
    ped->addScriptStep(destroyedStep);

    auto warnReport = rs_win_editor::MissionValidator::validate(m);
    CHECK(warnReport.hasWarnings());
    bool foundDestroyedVehWarn = false;
    for (const auto &iss : warnReport.issues) {
        if (iss.severity == rs_win_editor::ValidationSeverity::Warning &&
            iss.message.find("is currently destroyed") != std::string::npos) {
            foundDestroyedVehWarn = true;
            break;
        }
    }
    CHECK(foundDestroyedVehWarn);

    // 3. Destroyed vehicle with assigned driver
    v->setDriver(ped);
    auto driverWarnReport = rs_win_editor::MissionValidator::validate(m);
    CHECK(driverWarnReport.hasWarnings());
    bool foundDriverWarn = false;
    for (const auto &iss : driverWarnReport.issues) {
        if (iss.severity == rs_win_editor::ValidationSeverity::Warning &&
            iss.message.find("assigned driver") != std::string::npos &&
            iss.message.find("destroyed") != std::string::npos) {
            foundDriverWarn = true;
            break;
        }
    }
    CHECK(foundDriverWarn);

    // Cleanup
    v->setDriver(nullptr);
    v->setHealth(origHealth);
    ped->clearScriptSteps();

    misMgr.destroyMission();
}
