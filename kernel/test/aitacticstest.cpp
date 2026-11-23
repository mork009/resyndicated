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
#include <cstddef>
#include <cstdio>

#include "rs-kernel/model/leveldata.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/ia/behaviour.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/modmanager.h"
#include "rs-kernel/mgr/agentmanager.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/animationloader.h"
#include "rs-engine/gfx/tilemanager.h"
#include "rs-engine/system/system.h"
#include "rs-engine/appcontext.h"
#include "rs-utils/common.h"
#include "rs-utils/io/file.h"
#include "testcase.h"

TEST_CASE("AI Tactics Binary Layout & Sizing", "[aitactics]") {
    // 15-byte packed tactic entry
    CHECK(sizeof(LevelData::AITactic) == 15);

    // 1952-byte AI tactics block at file offset 114058
    CHECK(sizeof(LevelData::AITacticsBlock) == 1952);

    // Total level data structure size must match original Bullfrog Syndicate 116010 bytes
    CHECK(sizeof(LevelData::LevelDataAll) == 116010);

    // Offset in LevelDataAll must be exactly 114058
    CHECK(offsetof(LevelData::LevelDataAll, tactics) == 114058);
}

TEST_CASE("AI Tactics Mission Model API", "[aitactics]") {
    rs_knl::Mission mission;

    SECTION("Default tactics block is initialized correctly") {
        mission.initDefaultTactics();
        CHECK_FALSE(mission.hasCustomTactics());
        CHECK(mission.numTactics() == 0);
        CHECK(mission.getRootTacticIndex(LevelData::kAIFactionBlue) == -1);

        const auto &tactics = mission.tactics();
        CHECK(tactics.header.num_squads == 5);
        CHECK(tactics.header.peds_per_squad == 8);
        CHECK((tactics.entries[0].sub_filter & LevelData::kAITacticFlagDisabled) != 0);
    }

    SECTION("Adding and chaining tactics nodes") {
        mission.initDefaultTactics();

        // Node 1: Blue faction Proximity Tripwire at range 50 tiles
        int n1 = mission.addTactic(LevelData::kAIFactionBlue, LevelData::kAITacticModeProximityTripwire, 50, 0, 0, 0);
        REQUIRE(n1 > 0);
        CHECK(mission.hasCustomTactics());
        CHECK(mission.numTactics() == 1);
        CHECK(mission.getRootTacticIndex(LevelData::kAIFactionBlue) == n1);

        // Node 2: Blue faction Seek & Destroy linked after Node 1
        int n2 = mission.addTactic(LevelData::kAIFactionBlue, LevelData::kAITacticModeSeekDestroy, 0, 0, 0, n1);
        REQUIRE(n2 > 0);
        CHECK(mission.numTactics() == 2);

        // Verify chain
        auto chain = mission.getTacticChain(LevelData::kAIFactionBlue);
        REQUIRE(chain.size() == 2);
        CHECK(chain[0] == n1);
        CHECK(chain[1] == n2);

        // Verify linkages
        const auto &e1 = mission.tactics().entries[n1];
        const auto &e2 = mission.tactics().entries[n2];
        CHECK(rs_utl::READ_LE_UINT16(e1.prev_tactic) == 0);
        CHECK(rs_utl::READ_LE_UINT16(e1.next_tactic) == n2);
        CHECK(rs_utl::READ_LE_UINT16(e2.prev_tactic) == n1);
        CHECK(rs_utl::READ_LE_UINT16(e2.next_tactic) == 0);

        // Add Node 3: Delay timer after Node 2
        int n3 = mission.addTactic(LevelData::kAIFactionBlue, LevelData::kAITacticModeDelayTimer, 150, 0, 0, n2);
        REQUIRE(n3 > 0);
        chain = mission.getTacticChain(LevelData::kAIFactionBlue);
        REQUIRE(chain.size() == 3);
        CHECK(chain[0] == n1);
        CHECK(chain[1] == n2);
        CHECK(chain[2] == n3);

        // Delete middle Node 2: n1 should link directly to n3
        CHECK(mission.deleteTactic(n2));
        chain = mission.getTacticChain(LevelData::kAIFactionBlue);
        REQUIRE(chain.size() == 2);
        CHECK(chain[0] == n1);
        CHECK(chain[1] == n3);
        CHECK(rs_utl::READ_LE_UINT16(mission.tactics().entries[n1].next_tactic) == n3);
        CHECK(rs_utl::READ_LE_UINT16(mission.tactics().entries[n3].prev_tactic) == n1);

        // Clear Blue faction tactics
        mission.clearTactics(LevelData::kAIFactionBlue);
        CHECK(mission.getTacticChain(LevelData::kAIFactionBlue).empty());
        CHECK(mission.numTactics() == 0);
    }

    SECTION("Disabled tactic nodes are retained in chain and bypassed at runtime") {
        mission.initDefaultTactics();

        // Node 1: Delay Timer
        int n1 = mission.addTactic(LevelData::kAIFactionBlue, LevelData::kAITacticModeDelayTimer, 50, 0, 0, 0);
        // Node 2: Seek & Destroy
        int n2 = mission.addTactic(LevelData::kAIFactionBlue, LevelData::kAITacticModeSeekDestroy, 0, 0, 0, n1);
        REQUIRE(n1 > 0);
        REQUIRE(n2 > 0);

        // Case 1: Disable Node 1 (Root). Chain should still contain both nodes, but getRootTacticIndex bypasses n1 to n2
        mission.tactics().entries[n1].sub_filter |= LevelData::kAITacticFlagDisabled;
        auto chain = mission.getTacticChain(LevelData::kAIFactionBlue);
        REQUIRE(chain.size() == 2);
        CHECK(chain[0] == n1);
        CHECK(chain[1] == n2);
        CHECK(mission.getRootTacticIndex(LevelData::kAIFactionBlue) == n2);

        // Case 2: Re-enable Node 1, disable Node 2. Chain contains both nodes, getRootTacticIndex returns n1
        mission.tactics().entries[n1].sub_filter &= ~LevelData::kAITacticFlagDisabled;
        mission.tactics().entries[n2].sub_filter |= LevelData::kAITacticFlagDisabled;
        chain = mission.getTacticChain(LevelData::kAIFactionBlue);
        REQUIRE(chain.size() == 2);
        CHECK(chain[0] == n1);
        CHECK(chain[1] == n2);
        CHECK(mission.getRootTacticIndex(LevelData::kAIFactionBlue) == n1);
    }
}

TEST_CASE("PlayerHostileBehaviourComponent AI Tactics FSM Integration", "[aitactics]") {
    rs_knl::Mission mission;
    mission.initDefaultTactics();

    // Create a 2-node FSM: Tripwire (n1) -> SeekDestroy (n2)
    int n1 = mission.addTactic(LevelData::kAIFactionBlue, LevelData::kAITacticModeProximityTripwire, 30, 0, 0, 0);
    int n2 = mission.addTactic(LevelData::kAIFactionBlue, LevelData::kAITacticModeSeekDestroy, 0, 0, 0, n1);
    REQUIRE(n1 > 0);
    REQUIRE(n2 > 0);

    // Heap-allocate ped so Mission takes ownership cleanly
    auto *enemyPed = new rs_knl::PedInstance(10, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 128);
    auto *pHos = new rs_knl::PlayerHostileBehaviourComponent();
    enemyPed->behaviour().addComponent(pHos);
    mission.addPed(enemyPed);

    SECTION("Initialization from tactic node configures correct state and mode") {
        pHos->setFactionId(LevelData::kAIFactionBlue);
        pHos->initFromTactic(&mission, n1);

        CHECK(pHos->currentTacticIndex() == n1);
        CHECK(pHos->tacticMode() == LevelData::kAITacticModeProximityTripwire);
        CHECK(pHos->tacticParam1() == 30);
        CHECK_FALSE(pHos->isHunter());
    }

    SECTION("Hunter mode activation from Seek & Destroy tactic") {
        pHos->setFactionId(LevelData::kAIFactionBlue);
        pHos->initFromTactic(&mission, n2);

        CHECK(pHos->currentTacticIndex() == n2);
        CHECK(pHos->tacticMode() == LevelData::kAITacticModeSeekDestroy);
        CHECK(pHos->isHunter());
    }

    SECTION("FSM Advancement follows node transitions") {
        pHos->setFactionId(LevelData::kAIFactionBlue);
        pHos->initFromTactic(&mission, n1);
        CHECK(pHos->currentTacticIndex() == n1);

        // Trigger advance
        pHos->advanceTactic(&mission);

        // Since updateSquadTactic updates all members of faction, enemyPed advances to n2
        CHECK(pHos->currentTacticIndex() == n2);
        CHECK(pHos->tacticMode() == LevelData::kAITacticModeSeekDestroy);
        CHECK(pHos->isHunter());
    }

    SECTION("Objective Sweep (Mode 3) activates hunter unconditionally unless self-targeted") {
        int nObj = mission.addTactic(LevelData::kAIFactionOrange, LevelData::kAITacticModeObjectiveSweep, 0, 0, 0, 0);
        int nSelf = mission.addTactic(LevelData::kAIFactionOrange, LevelData::kAITacticModeObjectiveSweep, LevelData::kAIFactionOrange, 0, 0, 0);
        REQUIRE(nObj > 0);
        REQUIRE(nSelf > 0);

        // Even in a non-combat-sweep mission (e.g. Persuade/Assassinate), Mode 3 targeting Player sweeps immediately
        pHos->setHunter(false);
        pHos->setFactionId(LevelData::kAIFactionOrange);
        pHos->initFromTactic(&mission, nObj);
        CHECK(pHos->tacticMode() == LevelData::kAITacticModeObjectiveSweep);
        CHECK(pHos->isHunter());

        // Self-targeted Mode 3 (param1 == factionId) suppresses hunter sweep (Bullfrog 0x14724)
        pHos->setHunter(true);
        pHos->initFromTactic(&mission, nSelf);
        CHECK(pHos->tacticMode() == LevelData::kAITacticModeObjectiveSweep);
        CHECK_FALSE(pHos->isHunter());

        // Combat Sweep mission also activates hunter on Mode 3 targeting Player
        rs_utl::WRITE_LE_UINT16(mission.getRawLevelData().objectives[0].type, LevelData::kObjectiveTypeCombatSweep);
        CHECK(mission.isCombatSweep());

        pHos->setHunter(false);
        pHos->initFromTactic(&mission, nObj);
        CHECK(pHos->tacticMode() == LevelData::kAITacticModeObjectiveSweep);
        CHECK(pHos->isHunter());
    }
}

TEST_CASE("AI Tactics Wildcard and Multi-Faction Sweep Resolution", "[aitactics]") {
    rs_knl::Mission mission;
    mission.initDefaultTactics();

    // Set mission objective to Combat Sweep
    rs_utl::WRITE_LE_UINT16(mission.getRawLevelData().objectives[0].type, LevelData::kObjectiveTypeCombatSweep);
    REQUIRE(mission.isCombatSweep());

    // 1. Add Wildcard tactic (faction 255) with Mode 3 (Objective Sweep)
    int nWild = mission.addTactic(LevelData::kAIFactionWildcard, LevelData::kAITacticModeObjectiveSweep, 0, 0, 0, 0);
    REQUIRE(nWild > 0);

    // 2. Add specific tactic for Faction 3 with Mode 7 (Tripwire)
    int nF3 = mission.addTactic(LevelData::kAIFactionYellow1, LevelData::kAITacticModeProximityTripwire, 50, 0, 0, 0);
    REQUIRE(nF3 > 0);

    SECTION("Specific faction tactic takes precedence over wildcard") {
        // Faction 3 has explicit Mode 7 tactic
        CHECK(mission.getRootTacticIndex(LevelData::kAIFactionYellow1) == nF3);
        // Factions 1, 2, 4, 5, 6, 7 fall back to Wildcard
        CHECK(mission.getRootTacticIndex(LevelData::kAIFactionBlue) == nWild);
        CHECK(mission.getRootTacticIndex(LevelData::kAIFactionOrange) == nWild);
        CHECK(mission.getRootTacticIndex(LevelData::kAIFactionYellow2) == nWild);
        CHECK(mission.getRootTacticIndex(5) == nWild);
        CHECK(mission.getRootTacticIndex(6) == nWild);
        CHECK(mission.getRootTacticIndex(7) == nWild);
    }

    SECTION("Combat Sweep activates Wildcard Objective Sweep as hunters while preserving sentry tripwire") {
        auto *pedWild = new rs_knl::PedInstance(16, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 128);
        auto *pHosWild = new rs_knl::PlayerHostileBehaviourComponent();
        pedWild->behaviour().addComponent(pHosWild);
        mission.addPed(pedWild);

        // Faction 2 inherits Wildcard Mode 3 in Combat Sweep -> becomes hunter!
        int rootF2 = mission.getRootTacticIndex(LevelData::kAIFactionOrange);
        REQUIRE(rootF2 == nWild);
        pHosWild->setFactionId(LevelData::kAIFactionOrange);
        pHosWild->initFromTactic(&mission, rootF2);
        CHECK(pHosWild->isHunter());

        // Faction 3 has explicit Mode 7 Tripwire -> stays sentry non-hunter!
        auto *pedF3 = new rs_knl::PedInstance(24, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 128);
        auto *pHosF3 = new rs_knl::PlayerHostileBehaviourComponent();
        pedF3->behaviour().addComponent(pHosF3);
        mission.addPed(pedF3);

        int rootF3 = mission.getRootTacticIndex(LevelData::kAIFactionYellow1);
        REQUIRE(rootF3 == nF3);
        pHosF3->setFactionId(LevelData::kAIFactionYellow1);
        pHosF3->initFromTactic(&mission, rootF3);
        CHECK_FALSE(pHosF3->isHunter());
    }
}

TEST_CASE("Mission 2 AI Tactics Far East Sentry Verification", "[aitactics][m2]") {
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
    LevelData::LevelDataAll levelData2;
    REQUIRE(misMgr.load_level_data(2, levelData2));

    rs_knl::Mission *pMission2 = misMgr.create_mission(levelData2);
    REQUIRE(pMission2 != nullptr);
    pMission2->buildNavigationGraph();
    misMgr.setMission(pMission2);

    // In Mission 2 (Far East):
    // Only Faction 1 (Blue, peds 8..15) rushes the player squad
    // Faction 2 (Orange, peds 16..23) is capped by maxActive (param1[1] == 1) and remains sentry
    // Faction 3 & 4 (Yellow, peds 24..39) have Mode 7 (Tripwire range 200) and remain sentries
    for (size_t i = 0; i < pMission2->numPeds(); ++i) {
        auto *p = pMission2->ped(i);
        if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && !p->isOurAgent()) {
            if (p->factionId() == LevelData::kAIFactionBlue) {
                CHECK(p->isHunter());
            } else {
                CHECK_FALSE(p->isHunter());
            }
        }
    }

    // Sentry Engagement on Walkway (Mission 2):
    // Find a Yellow sentry agent (e.g. Ped 24)
    rs_knl::PedInstance *sentryPed = nullptr;
    for (size_t i = 0; i < pMission2->numPeds(); ++i) {
        auto *p = pMission2->ped(i);
        if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && p->factionId() == LevelData::kAIFactionYellow1) {
            sentryPed = p;
            break;
        }
    }
    REQUIRE(sentryPed != nullptr);
    auto *pHos = sentryPed->behaviour().getComponent<rs_knl::PlayerHostileBehaviourComponent>();
    REQUIRE(pHos != nullptr);
    CHECK_FALSE(sentryPed->isHunter());

    // Initially player squad is distant at southwest spawn (~7000 units away), sentry holds position
    pMission2->handleTick(100, 100);
    CHECK(pHos->status() == rs_knl::PlayerHostileBehaviourComponent::kHostileStatusDefault);

    // Place an unarmed player agent on the walkway right in front of the sentry (2 tiles away)
    auto *playerPed = pMission2->getSquad()->member(0);
    REQUIRE(playerPed != nullptr);
    playerPed->deselectWeapon();
    CHECK_FALSE(playerPed->isArmed());
    playerPed->setPosition(sentryPed->tileX() + 2, sentryPed->tileY(), sentryPed->tileZ(),
                           sentryPed->offX(), sentryPed->offY(), sentryPed->offZ());

    // Execute ticks: proximity breach of Faction 3 anchor triggers tripwire advancing to Tactic 0 (Combat Sweep)
    for (int t = 0; t < 35; ++t) {
        pMission2->handleTick(100, 100);
        if (pHos->status() == rs_knl::PlayerHostileBehaviourComponent::kHostileStatusFollowAndShoot) {
            break;
        }
    }
    CHECK(pHos->status() == rs_knl::PlayerHostileBehaviourComponent::kHostileStatusFollowAndShoot);
    CHECK(sentryPed->isHunter());

    // Sentry Engagement on Street Level (Mission 2 Faction 2 - Ped 18):
    // Ped 18 is at (83, 39, 256), far away (>14 tiles) from Ped 16 at (79, 25, 1024).
    // Approaching Ped 18 directly triggers the tripwire for Faction 2, proving that each
    // sentry acts as a sensor rather than requiring proximity to an artificial squad leader.
    rs_knl::PedInstance *ped18 = pMission2->ped(18);
    REQUIRE(ped18 != nullptr);
    CHECK(ped18->factionId() == LevelData::kAIFactionOrange);
    CHECK_FALSE(ped18->isHunter());

    // Move player agent within 3 tiles of Ped 18
    playerPed->setPosition(ped18->tileX() + 3, ped18->tileY(), ped18->tileZ(),
                           ped18->offX(), ped18->offY(), ped18->offZ());

    for (int t = 0; t < 10; ++t) {
        pMission2->handleTick(100, 100);
        if (ped18->isHunter()) break;
    }
    CHECK(ped18->isHunter());

    // Check that other Faction 2 agents (e.g. Ped 16 on roof) also mobilized into Combat Sweep
    rs_knl::PedInstance *ped16 = pMission2->ped(16);
    REQUIRE(ped16 != nullptr);
    CHECK(ped16->isHunter());

    misMgr.setMission(nullptr);
    delete pMission2;
}

TEST_CASE("Mission 9 AI Tactics Kazakhstan Verification", "[aitactics][m9]") {
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
    REQUIRE(misMgr.load_level_data(9, levelData));

    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    pMission->buildNavigationGraph();
    misMgr.setMission(pMission);

    // Verify initial roles in Mission 9 (Kazakhstan):
    // Header max_rush_factions is 0 (no global hunter rush forced from start).
    // Faction 1 (Blue, 8..15): Sentry holding southwest (Tactic 7: Mode 7 Proximity Tripwire 50 tiles)
    // Faction 2 (Orange, 16..23): Sentry holding northwest (Tactic 4: Mode 7 Proximity Tripwire 10 tiles)
    // Faction 3 (Yellow 1, 24..31): Sentry holding central area (Tactic 2: Mode 7 Proximity Tripwire 10 tiles)
    // Faction 4 (Yellow 2, 32..39): Active Hunter targeting Faction 3 (Tactic 1: Mode 3 Objective Sweep, p1=3)
    CHECK(pMission->tactics().header.max_rush_factions == 0);

    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        auto *p = pMission->ped(i);
        if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && !p->isOurAgent()) {
            auto *pHos = p->behaviour().getComponent<rs_knl::PlayerHostileBehaviourComponent>();
            REQUIRE(pHos != nullptr);
            if (p->factionId() == LevelData::kAIFactionBlue) {
                CHECK_FALSE(p->isHunter());
                CHECK(pHos->tacticMode() == LevelData::kAITacticModeProximityTripwire);
            } else if (p->factionId() == LevelData::kAIFactionOrange) {
                CHECK_FALSE(p->isHunter());
                CHECK(pHos->tacticMode() == LevelData::kAITacticModeProximityTripwire);
            } else if (p->factionId() == LevelData::kAIFactionYellow1) {
                CHECK_FALSE(p->isHunter());
                CHECK(pHos->tacticMode() == LevelData::kAITacticModeProximityTripwire);
            } else if (p->factionId() == LevelData::kAIFactionYellow2) {
                CHECK(p->isHunter());
                CHECK(pHos->tacticMode() == LevelData::kAITacticModeObjectiveSweep);
                CHECK(pHos->targetFactionId() == LevelData::kAIFactionYellow1);
            }
        }
    }

    struct Pos { int x, y; };
    std::map<int, Pos> startPos;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        auto *p = pMission->ped(i);
        if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && !p->isOurAgent()) {
            startPos[p->id()] = { p->position().tx, p->position().ty };
        }
    }

    // Simulate 350 ticks (35 seconds)
    int f2ActivateTick = -1;
    bool f3Activated = false;
    int f3ActivateTick = -1;

    for (int t = 0; t < 350; ++t) {
        pMission->handleTick(100, 100);

        // At t = 80 (8.0s):
        // Faction 4 is in transit moving west towards Faction 3 with weapons holstered
        // Faction 3 is still holding position in Mode 7 Proximity Tripwire with weapons holstered
        if (t == 80) {
            for (size_t i = 0; i < pMission->numPeds(); ++i) {
                auto *p = pMission->ped(i);
                if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && !p->isOurAgent()) {
                    auto *pHos = p->behaviour().getComponent<rs_knl::PlayerHostileBehaviourComponent>();
                    if (p->factionId() == LevelData::kAIFactionYellow2) {
                        // In transit: weapon holstered
                        CHECK_FALSE(p->isArmed());
                    } else if (p->factionId() == LevelData::kAIFactionYellow1) {
                        // Holding in tripwire: weapon holstered, not engaged
                        CHECK_FALSE(p->isArmed());
                        CHECK(pHos->tacticMode() == LevelData::kAITacticModeProximityTripwire);
                    }
                }
            }
        }

        // Track Faction 3 tripwire breach activation
        if (!f3Activated) {
            for (size_t i = 0; i < pMission->numPeds(); ++i) {
                auto *p = pMission->ped(i);
                if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && p->factionId() == LevelData::kAIFactionYellow1) {
                    auto *pHos = p->behaviour().getComponent<rs_knl::PlayerHostileBehaviourComponent>();
                    if (pHos->tacticMode() == LevelData::kAITacticModeObjectiveSweep) {
                        f3Activated = true;
                        f3ActivateTick = t;
                        break;
                    }
                }
            }
        }

        if (f2ActivateTick < 0) {
            for (size_t i = 0; i < pMission->numPeds(); ++i) {
                auto *p = pMission->ped(i);
                if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && p->factionId() == LevelData::kAIFactionOrange) {
                    if (p->isHunter()) {
                        f2ActivateTick = t;
                        break;
                    }
                }
            }
        }
    }

    // Faction 3 trips tripwire and engages Faction 4 at ~12-18s when Faction 4 arrives near Ped 24
    CHECK(f3Activated);
    CHECK(f3ActivateTick >= 110);
    CHECK(f3ActivateTick <= 220);

    // Faction 2 holds northwest for ~30s before sweeping player
    CHECK(f2ActivateTick >= 280);
    CHECK(f2ActivateTick <= 360);

    // Verify movement after 60 ticks:
    // Faction 4 (Yellow 2) rushes west toward Faction 3 (X decreasing from start at X=82 towards ~60)
    // Faction 1 (Blue) remained in the southwest (sentry, no rush toward player at southeast)
    // Faction 3 (Yellow 1) remained in place until Faction 4 arrives
    bool f4MovedWest = false;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        auto *p = pMission->ped(i);
        if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && !p->isOurAgent()) {
            int sx = startPos[p->id()].x;
            int cx = p->position().tx;
            if (p->factionId() == LevelData::kAIFactionYellow2 && cx < sx) {
                f4MovedWest = true;
            }
            if (p->factionId() == LevelData::kAIFactionBlue) {
                // Faction 1 does not run east toward the player
                CHECK(std::abs(p->position().tx - sx) <= 2);
            }
        }
    }
    CHECK(f4MovedWest);

    delete pMission;
}

TEST_CASE("Mission 2 AI Tactics Elevated Sentry LoS and Proximity Breach", "[aitactics][m2][elevation]") {
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
    LevelData::LevelDataAll levelData2;
    REQUIRE(misMgr.load_level_data(2, levelData2));

    rs_knl::Mission *pMission2 = misMgr.create_mission(levelData2);
    REQUIRE(pMission2 != nullptr);
    pMission2->buildNavigationGraph();
    misMgr.setMission(pMission2);

    // 1. Verify 64-bit non-overflow guarantee in isCloseTo with large distance (e.g. 51200)
    auto *playerPed0 = pMission2->getSquad()->member(0);
    REQUIRE(playerPed0 != nullptr);
    rs_knl::WorldPoint playerW0(playerPed0->position());
    rs_knl::WorldPoint farPoint(playerW0.x + 1000, playerW0.y + 1000, playerW0.z);
    CHECK(playerPed0->isCloseTo(farPoint, 51200));

    // 2. Elevated sentry line of sight from walkway (Faction 4, Ped 32 at Z=8)
    // Ped 32 is on walkway at (64, 68, Z=8 / 1024)
    rs_knl::PedInstance *sentryWalkway = nullptr;
    for (size_t i = 0; i < pMission2->numPeds(); ++i) {
        auto *p = pMission2->ped(i);
        if (p && p->id() == 32) {
            sentryWalkway = p;
            break;
        }
    }
    REQUIRE(sentryWalkway != nullptr);
    CHECK(sentryWalkway->tileZ() == 8);

    // Place a player agent along the walkway approach (3 tiles away)
    auto *playerPed = pMission2->getSquad()->member(0);
    REQUIRE(playerPed != nullptr);
    playerPed->setPosition(sentryWalkway->tileX(), sentryWalkway->tileY() - 3, sentryWalkway->tileZ(),
                           sentryWalkway->offX(), sentryWalkway->offY(), sentryWalkway->offZ());

    // Test line-of-sight with torso-height offset (+64):
    rs_knl::WorldPoint sentryEye(sentryWalkway->position());
    sentryEye.z += 64;
    rs_knl::WorldPoint playerEye(playerPed->position());
    playerEye.z += 64;

    uint8_t losRes = pMission2->checkBlockedByTile(sentryEye, &playerEye, false, 2560.0);
    CHECK(losRes == 1);

    // 3. Proximity trigger of Mode 7 squad anchor (Ped 32) when player approaches within 12 tiles advances to Tactic 0
    auto *pHos = sentryWalkway->behaviour().getComponent<rs_knl::PlayerHostileBehaviourComponent>();
    REQUIRE(pHos != nullptr);
    CHECK(pHos->tacticMode() == LevelData::kAITacticModeProximityTripwire);

    for (int t = 0; t < 35; ++t) {
        pMission2->handleTick(100, 100);
        if (pHos->tacticMode() == LevelData::kAITacticModeObjectiveSweep) break;
    }
    CHECK(pHos->tacticMode() == LevelData::kAITacticModeObjectiveSweep);
    CHECK(sentryWalkway->isHunter());

    delete pMission2;
}

TEST_CASE("Mission 5 AI Tactics Persuade Immediate Orange Engagement & Yellow Sentry Verification", "[aitactics][m5]") {
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
    LevelData::LevelDataAll levelData5;
    REQUIRE(misMgr.load_level_data(5, levelData5));

    rs_knl::Mission *pMission5 = misMgr.create_mission(levelData5);
    REQUIRE(pMission5 != nullptr);
    pMission5->buildNavigationGraph();
    misMgr.setMission(pMission5);

    // Mission 5 is Persuade + Evacuate (non-combat-sweep)
    CHECK_FALSE(pMission5->isCombatSweep());

    // Multi-squad wildcard assignment (Bullfrog MAIN.EXE 0x14588):
    // Squad 1 claims Wildcard Tactic 1 (Mode 7 tripwire),
    // Squad 2 (Orange, peds 16..23) claims Wildcard Tactic 2 (Mode 3 Objective Sweep Player),
    // Squad 3 (Yellow, peds 24..31) wraps around in Pass 2 to Wildcard Tactic 1 (Mode 7 tripwire).
    CHECK(pMission5->getRootTacticIndex(LevelData::kAIFactionOrange) == 2);
    CHECK(pMission5->getRootTacticIndex(LevelData::kAIFactionYellow1) == 1);

    struct StartPos { int tx, ty; };
    std::map<int, StartPos> orangeStart, yellowStart;

    for (size_t i = 0; i < pMission5->numPeds(); ++i) {
        auto *p = pMission5->ped(i);
        if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && !p->isOurAgent()) {
            auto *pHos = p->behaviour().getComponent<rs_knl::PlayerHostileBehaviourComponent>();
            REQUIRE(pHos != nullptr);
            if (p->factionId() == LevelData::kAIFactionOrange) {
                CHECK(p->isHunter());
                CHECK(pHos->currentTacticIndex() == 2);
                CHECK(pHos->tacticMode() == LevelData::kAITacticModeObjectiveSweep);
                CHECK(pHos->targetFactionId() == LevelData::kAIFactionPlayer);
                orangeStart[p->id()] = { p->position().tx, p->position().ty };
            } else if (p->factionId() == LevelData::kAIFactionYellow1) {
                CHECK_FALSE(p->isHunter());
                CHECK(pHos->currentTacticIndex() == 1);
                CHECK(pHos->tacticMode() == LevelData::kAITacticModeProximityTripwire);
                yellowStart[p->id()] = { p->position().tx, p->position().ty };
            }
        }
    }
    REQUIRE(orangeStart.size() == 8);
    REQUIRE(yellowStart.size() == 8);

    // Simulate 60 ticks (6.0 seconds, past 3.0s initial reaction latency)
    for (int t = 0; t < 60; ++t) {
        pMission5->handleTick(100, 100);
    }

    // Verify Orange agents moved toward the player squad with weapons initially holstered,
    // while Yellow agents remained stationary at their sentry posts.
    int orangeMovedCount = 0;
    for (size_t i = 0; i < pMission5->numPeds(); ++i) {
        auto *p = pMission5->ped(i);
        if (p && p->type() == rs_knl::PedInstance::kPedTypeAgent && !p->isOurAgent()) {
            if (p->factionId() == LevelData::kAIFactionOrange) {
                auto sp = orangeStart[p->id()];
                if (p->position().tx != sp.tx || p->position().ty != sp.ty) {
                    orangeMovedCount++;
                }
                CHECK_FALSE(p->isArmed());
            } else if (p->factionId() == LevelData::kAIFactionYellow1) {
                auto sp = yellowStart[p->id()];
                CHECK(p->position().tx == sp.tx);
                CHECK(p->position().ty == sp.ty);
                CHECK_FALSE(p->isArmed());
            }
        }
    }
    CHECK(orangeMovedCount >= 6);

    misMgr.setMission(nullptr);
    delete pMission5;
}

