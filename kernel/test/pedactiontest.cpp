/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
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
#include <catch2/catch_test_macros.hpp>

#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/weapon.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/mgr/pedmanager.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/ia/actions.h"
#include "rs-engine/events/event.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/appcontext.h"
#include "testcase.h"

namespace {

/*! Test-only action that always fails and records whether it was executed.*/
class FakeFailingAction : public rs_knl::MovementAction {
public:
    explicit FakeFailingAction(bool blocking) :
        rs_knl::MovementAction(rs_knl::Action::kActTypeUndefined, false, false, blocking) {}

    bool executed = false;

protected:
    bool doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] rs_knl::Mission *pMission,
            [[maybe_unused]] rs_knl::PedInstance *pPed) override {
        executed = true;
        setFailed();
        return true;
    }
};

/*! Test-only action that always succeeds and records whether it was executed.*/
class FakeSpyAction : public rs_knl::MovementAction {
public:
    FakeSpyAction() : rs_knl::MovementAction(rs_knl::Action::kActTypeUndefined) {}

    bool executed = false;

protected:
    bool doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] rs_knl::Mission *pMission,
            [[maybe_unused]] rs_knl::PedInstance *pPed) override {
        executed = true;
        setSucceeded();
        return true;
    }
};

}  // namespace

TEST_CASE( "PedAction", "[kernel][ped]" ) {
    rs_knl::PedInstance cut(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    cut.setStartHealth(10);
    cut.goToState(rs_knl::kPedActionStateStanding);

    SECTION( "can take hit action") {
        // Reject cause health is zero
        REQUIRE_FALSE( cut.canTakeAction(rs_knl::Action::kActTypeHit) );

        // Should accept
        cut.resetHealth();
        REQUIRE( cut.canTakeAction(rs_knl::Action::kActTypeHit) );

        // Reject cause state is already hit
        cut.goToState(rs_knl::kPedActionStateHit);
        REQUIRE_FALSE( cut.canTakeAction(rs_knl::Action::kActTypeHit) );
    }

    SECTION( "a blocking action failure stops the rest of the chain") {
        cut.resetHealth();
        FakeFailingAction *pFail = new FakeFailingAction(true);
        FakeSpyAction *pSpy = new FakeSpyAction();
        pFail->link(pSpy);
        cut.addMovementAction(pFail, false);

        cut.executeAction(100, nullptr);

        REQUIRE( pFail->executed );
        REQUIRE_FALSE( pSpy->executed );
        REQUIRE( cut.currentAction() == nullptr );
    }

    SECTION( "a non-blocking action failure lets the chain continue") {
        cut.resetHealth();
        FakeFailingAction *pFail = new FakeFailingAction(false);
        FakeSpyAction *pSpy = new FakeSpyAction();
        pFail->link(pSpy);
        cut.addMovementAction(pFail, false);

        cut.executeAction(100, nullptr);

        REQUIRE( pFail->executed );
        REQUIRE( pSpy->executed );
    }
}

TEST_CASE( "PersuadedWeaponPickup", "[kernel][ped][behaviour]" ) {
    rs_eng::AppContext appCtx;
    rs_eng::AnimationManager animMgr;
    ConfigFile config;
    initWeaponConfigFile(config);

    MemoryTileManager tileMgr;
    rs_knl::Map map(&tileMgr, 1);
    const int kTestMapMaxTx = 32;
    const int kTestMapMaxTy = 32;
    const int kTestMapMaxTz = 3;
    int nbTiles = kTestMapMaxTx * kTestMapMaxTy * kTestMapMaxTz;
    rs_eng::Tile **tiles = new rs_eng::Tile*[nbTiles];
    for (int i=0; i<nbTiles; i++) {
        tiles[i] = tileMgr.getTile(rs_eng::TileManager::kIndexTransparentTile);
    }
    map.setTiles(kTestMapMaxTx, kTestMapMaxTy, kTestMapMaxTz, tiles);
    delete[] tiles;

    rs_knl::Mission mission;
    REQUIRE( mission.init(&map) );

    rs_knl::PedInstance *pAgent = new rs_knl::PedInstance(1, &map, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    pAgent->setStartHealth(10, true);
    pAgent->setPosition(10, 10, 1, 0, 0, 0);
    mission.addPed(pAgent);
    mission.getSquad()->setMember(rs_knl::Squad::kSlot1, pAgent);

    rs_knl::PedInstance *pPersuaded = new rs_knl::PedInstance(2, &map, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    pPersuaded->setStartHealth(10, true);
    pPersuaded->setPosition(10, 10, 1, 0, 0, 0);
    mission.addPed(pPersuaded);
    pPersuaded->handlePersuadedBy(pAgent);

    SECTION( "Unarmed persuaded NPC picks up nearby dropped weapon" ) {
        // Add a dropped pistol on the ground 2 tiles away (approx 512 units, < 768 units)
        rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance *pGroundWeap = new rs_knl::WeaponInstance(&pistolDef, 0, nullptr);
        pGroundWeap->setPosition(12, 10, 1, 0, 0, 0);
        mission.addWeaponToGround(pGroundWeap);

        pPersuaded->behaviour().execute(600, &mission);

        // Persuaded ped should now have alternative action to go pick up the weapon
        REQUIRE( pPersuaded->altAction() != nullptr );
        REQUIRE( pPersuaded->altAction()->type() == rs_knl::Action::kActTypeWalk );
    }

    SECTION( "Persuaded NPC with depleted weapon picks up nearby dropped weapon" ) {
        rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance *pEmptyWeap = new rs_knl::WeaponInstance(&pistolDef, 0, nullptr, 0);
        pPersuaded->addWeapon(pEmptyWeap);
        pPersuaded->selectWeapon((size_t) 0);

        rs_knl::WeaponInstance *pGroundWeap = new rs_knl::WeaponInstance(&pistolDef, 0, nullptr);
        pGroundWeap->setPosition(12, 10, 1, 0, 0, 0);
        mission.addWeaponToGround(pGroundWeap);

        pPersuaded->behaviour().execute(600, &mission);

        REQUIRE( pPersuaded->altAction() != nullptr );
        REQUIRE( pPersuaded->altAction()->type() == rs_knl::Action::kActTypeWalk );
    }
}

TEST_CASE( "CivilianPanicBehavior", "[kernel][ped][behaviour]" ) {
    rs_eng::AppContext appCtx;
    rs_eng::AnimationManager animMgr;
    ConfigFile config;
    initWeaponConfigFile(config);

    MemoryTileManager tileMgr;
    rs_knl::MissionManager misMgr(&tileMgr);
    rs_knl::Map map(&tileMgr, 1);
    const int kTestMapMaxTx = 32;
    const int kTestMapMaxTy = 32;
    const int kTestMapMaxTz = 3;
    int nbTiles = kTestMapMaxTx * kTestMapMaxTy * kTestMapMaxTz;
    rs_eng::Tile **tiles = new rs_eng::Tile*[nbTiles];
    for (int i = 0; i < nbTiles; i++) {
        tiles[i] = tileMgr.getTile(rs_eng::TileManager::kIndexTransparentTile);
    }
    map.setTiles(kTestMapMaxTx, kTestMapMaxTy, kTestMapMaxTz, tiles);
    delete[] tiles;

    rs_knl::Mission mission;
    REQUIRE( mission.init(&map) );
    misMgr.setMission(&mission);

    rs_knl::PedInstance *pAgent = new rs_knl::PedInstance(1, &map, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    pAgent->setStartHealth(10, true);
    pAgent->setPosition(10, 10, 1, 0, 0, 0);
    mission.addPed(pAgent);
    mission.getSquad()->setMember(rs_knl::Squad::kSlot1, pAgent);

    rs_knl::PedInstance *pCiv = new rs_knl::PedInstance(2, &map, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    pCiv->setStartHealth(2, true);
    pCiv->setPosition(10, 12, 1, 0, 0, 0);
    mission.addPed(pCiv);
    rs_knl::PedManager::initCivilian(pCiv, 2);
    pCiv->addToDefaultActions(new rs_knl::WalkToDirectionAction());
    pCiv->addMovementAction(pCiv->defaultAction(), false);

    SECTION( "Civilian panics when armed hostile is nearby, and calms down when threat is eliminated" ) {
        CHECK_FALSE( pCiv->isInPanic() );

        // Agent arms weapon
        rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance *pPistol = new rs_knl::WeaponInstance(&pistolDef, 0, nullptr);
        pAgent->addWeapon(pPistol);
        pAgent->selectWeapon((size_t) 0);
        mission.addArmedPed(pAgent);
        pCiv->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponOut, pAgent);

        // Update civilian behaviour: armed threat in range -> panics!
        pCiv->behaviour().execute(600, &mission);
        CHECK( pCiv->isInPanic() );
        CHECK( pCiv->altAction() != nullptr );

        // Now remove armed threat (agent holsters weapon or dies)
        pAgent->deselectWeapon();
        mission.removeArmedPed(pAgent);

        // Update civilian behaviour: threat gone -> calms down immediately!
        pCiv->behaviour().execute(600, &mission);
        CHECK_FALSE( pCiv->isInPanic() );
        CHECK( pCiv->altAction() == nullptr );
        CHECK( pCiv->isCurrentActionFromSource(rs_knl::Action::kActionDefault) );
    }

    SECTION( "Civilian blocked against barrier does not get stuck in perpetual panic" ) {
        // Arm weapon to trigger panic
        rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance *pPistol = new rs_knl::WeaponInstance(&pistolDef, 0, nullptr);
        pAgent->addWeapon(pPistol);
        pAgent->selectWeapon((size_t) 0);
        mission.addArmedPed(pAgent);
        pCiv->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponOut, pAgent);

        pCiv->behaviour().execute(600, &mission);
        CHECK( pCiv->isInPanic() );
        CHECK( pCiv->altAction() != nullptr );

        // Simulate blocked panic walk: executing frames where no distance is covered
        // Execute for 1200ms total (> kMaxBlockedTime 1000ms)
        pCiv->executeAction(600, &mission);
        pCiv->executeAction(600, &mission);

        // Remove armed threat
        pAgent->deselectWeapon();
        mission.removeArmedPed(pAgent);
        pCiv->behaviour().execute(600, &mission);

        CHECK_FALSE( pCiv->isInPanic() );
        CHECK( pCiv->altAction() == nullptr );
        CHECK( pCiv->isCurrentActionFromSource(rs_knl::Action::kActionDefault) );
    }

    SECTION( "cancelAltActions cleanly restores default action chain" ) {
        rs_knl::WalkToDirectionAction *pAlt = new rs_knl::WalkToDirectionAction();
        pAlt->setMaxDistanceToWalk(1000);
        pCiv->addToAltActions(pAlt);
        pCiv->setCurrentActionWithSource(rs_knl::Action::kActionAlt);

        CHECK( pCiv->isCurrentActionFromSource(rs_knl::Action::kActionAlt) );
        CHECK( pCiv->altAction() != nullptr );

        pCiv->cancelAltActions();

        CHECK_FALSE( pCiv->isCurrentActionFromSource(rs_knl::Action::kActionAlt) );
        CHECK( pCiv->altAction() == nullptr );
        CHECK( pCiv->isCurrentActionFromSource(rs_knl::Action::kActionDefault) );
    }

    SECTION( "Civilian boarding a vehicle ignores panic (DOS Status 5 parity)" ) {
        rs_knl::GenericCar car(10, rs_knl::Vehicle::kVehicleTypeRegularCar, &map, 100);
        car.setPosition(10, 15, 1, 0, 0, 0);

        // Assign civilian to enter vehicle
        pCiv->addMovementAction(pCiv->createActionEnterVehicle(&car), false);

        CHECK( pCiv->isEnteringVehicle() );
        CHECK( pCiv->isImmunedToPanic() );

        // Hostile agent draws weapon nearby
        rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance *pPistol = new rs_knl::WeaponInstance(&pistolDef, 0, nullptr);
        pAgent->addWeapon(pPistol);
        pAgent->selectWeapon((size_t) 0);
        mission.addArmedPed(pAgent);
        pCiv->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponOut, pAgent);

        // Behaviour execution does NOT trigger panic because ped is boarding vehicle
        pCiv->behaviour().execute(600, &mission);
        CHECK_FALSE( pCiv->isInPanic() );
        CHECK( pCiv->altAction() == nullptr );

        // Clean up
        pAgent->deselectWeapon();
        mission.removeArmedPed(pAgent);
    }

    SECTION( "Civilian inside vehicle or persuaded is immune to panic" ) {
        rs_knl::GenericCar car(10, rs_knl::Vehicle::kVehicleTypeRegularCar, &map, 100);
        pCiv->putInVehicle(&car);

        CHECK( pCiv->isInVehicle() );
        CHECK( pCiv->isImmunedToPanic() );

        pCiv->leaveVehicle();
        CHECK_FALSE( pCiv->isInVehicle() );
        CHECK_FALSE( pCiv->isImmunedToPanic() );

        // Persuading civilian grants panic immunity
        pCiv->handlePersuadedBy(pAgent);
        CHECK( pCiv->isPersuaded() );
        CHECK( pCiv->isImmunedToPanic() );
    }

    misMgr.setMission(nullptr);
}

TEST_CASE("PoliceWarningAudioTriggers", "[ped][police][sound]") {
    rs_eng::AppContext appCtx;
    ConfigFile config;
    initWeaponConfigFile(config);
    rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);

    rs_knl::PedInstance playerAgent(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    playerAgent.setStartHealth(100, true);
    rs_knl::WeaponInstance pistol1(&pistolDef, 0, nullptr);
    playerAgent.addWeapon(&pistol1);
    playerAgent.selectWeapon((size_t)0);

    rs_knl::PedInstance enemyAgent(2, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 128);
    enemyAgent.setStartHealth(100, true);
    rs_knl::WeaponInstance pistol2(&pistolDef, 0, nullptr);
    enemyAgent.addWeapon(&pistol2);
    enemyAgent.selectWeapon((size_t)0);

    rs_knl::PedInstance police(3, nullptr, rs_knl::PedInstance::kPedTypePolice, false, 128);
    police.setStartHealth(50, true);
    rs_knl::WeaponInstance pistol3(&pistolDef, 0, nullptr);
    police.addWeapon(&pistol3);
    police.selectWeapon((size_t)0);

    int warningCount = 0;
    ListenerHandle handle = EventManager::listen<rs_knl::PoliceWarningEmittedEvent>(
        [&](rs_knl::PoliceWarningEmittedEvent *) {
            warningCount++;
        });

    SECTION("Police confronting player agent emits warning") {
        warningCount = 0;
        rs_knl::WaitBeforeShootingAction waitAction(&playerAgent);
        waitAction.execute(100, nullptr, &police);
        CHECK(warningCount == 1);
        waitAction.execute(100, nullptr, &police);
        CHECK(warningCount == 1); // Only once per target engagement
    }

    SECTION("Enemy agent confronting player agent does NOT emit police warning") {
        warningCount = 0;
        rs_knl::WaitBeforeShootingAction waitAction(&playerAgent);
        waitAction.execute(100, nullptr, &enemyAgent);
        CHECK(warningCount == 0);
        waitAction.execute(100, nullptr, &enemyAgent);
        CHECK(warningCount == 0);
    }

    SECTION("Police confronting enemy agent does NOT emit player warning") {
        warningCount = 0;
        rs_knl::WaitBeforeShootingAction waitAction(&enemyAgent);
        waitAction.execute(100, nullptr, &police);
        CHECK(warningCount == 0);
        waitAction.execute(100, nullptr, &police);
        CHECK(warningCount == 0);
    }

    SECTION("Persuaded police confronting player agent does NOT emit warning") {
        warningCount = 0;
        police.handlePersuadedBy(&playerAgent);
        rs_knl::WaitBeforeShootingAction waitAction(&playerAgent);
        waitAction.execute(100, nullptr, &police);
        CHECK(warningCount == 0);
        waitAction.execute(100, nullptr, &police);
        CHECK(warningCount == 0);
    }

    EventManager::remove_listener(handle);
}

TEST_CASE("EnemyAgentDelayedWeaponWielding", "[ped][agent][ai]") {
    rs_eng::AppContext appCtx;
    MemoryTileManager tileMgr;
    rs_knl::MissionManager misMgr(&tileMgr);
    ConfigFile config;
    initWeaponConfigFile(config);
    rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);

    // Create player agent at distant position (15, 0, 0) -> 3840 world units
    rs_knl::TilePoint playerPos(15, 0, 0, 128, 128, 0);
    rs_knl::PedInstance playerAgent(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    playerAgent.setPosition(playerPos);
    playerAgent.setStartHealth(100, true);

    // Create enemy agent at (0, 0, 0) with a loaded pistol in inventory
    rs_knl::TilePoint enemyPos(0, 0, 0, 128, 128, 0);
    rs_knl::PedInstance enemyAgent(2, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 128);
    enemyAgent.setPosition(enemyPos);
    enemyAgent.setStartHealth(100, true);
    rs_knl::WeaponInstance pistol(&pistolDef, 0, nullptr);
    enemyAgent.addWeapon(&pistol);
    enemyAgent.deselectWeapon();

    CHECK(enemyAgent.selectedWeapon() == nullptr);

    auto *pHostile = new rs_knl::PlayerHostileBehaviourComponent();
    pHostile->setHunter(true);
    enemyAgent.behaviour().addComponent(pHostile);

    SECTION("Enemy agent does NOT wield weapon when engaging distant player") {
        pHostile->followAndShootTarget(&enemyAgent, &playerAgent);
        CHECK(enemyAgent.selectedWeapon() == nullptr);

        rs_knl::FollowToShootAction followAction(&playerAgent);
        followAction.execute(100, nullptr, &enemyAgent);
        CHECK(enemyAgent.selectedWeapon() == nullptr);

        // Advance while still beyond wield threshold (e.g. at 10 tiles = 2560 units > 2048)
        rs_knl::TilePoint farPos(10, 0, 0, 128, 128, 0);
        playerAgent.setPosition(farPos);
        followAction.execute(100, nullptr, &enemyAgent);
        CHECK(enemyAgent.selectedWeapon() == nullptr);

        // Move within wield threshold (5 tiles = 1280 units <= 1536)
        rs_knl::TilePoint closePos(5, 0, 0, 128, 128, 0);
        playerAgent.setPosition(closePos);
        followAction.execute(100, nullptr, &enemyAgent);
        CHECK(enemyAgent.selectedWeapon() != nullptr);
    }

    SECTION("Enemy agent holsters weapon when target is eliminated") {
        // Force weapon drawn
        enemyAgent.selectShootingWeaponWithAmmo();
        CHECK(enemyAgent.selectedWeapon() != nullptr);

        pHostile->followAndShootTarget(&enemyAgent, &playerAgent);
        playerAgent.setHealth(0);

        rs_knl::Behaviour::BehaviourParam param;
        param.elapsed = 100;
        param.pMission = nullptr;
        param.pPed = &enemyAgent;
        pHostile->execute(param);

        CHECK(enemyAgent.selectedWeapon() == nullptr);
    }
}

TEST_CASE("CornerBounceDoesNotSpinForever", "[ped][pathfinding][bounce]") {
    rs_eng::AppContext appCtx;
    MemoryTileManager tileMgr;
    rs_knl::Map map(&tileMgr, 1);

    // 5x5x2 map where tile (2,2,1) has open neighbors North (2,1,1), East (3,2,1), NE (3,1,1)
    // and blocked neighbors West (1,2,1) and South (2,3,1) (matching Mission 5 Ped #47 corner at (27,33)).
    const int kMaxX = 5, kMaxY = 5, kMaxZ = 2;
    std::vector<rs_eng::Tile *> tiles(kMaxX * kMaxY * kMaxZ,
        tileMgr.getTile(rs_eng::TileManager::kIndexTransparentTile));
    auto setFloor = [&](int tx, int ty) {
        tiles[(ty * kMaxX + tx) * kMaxZ + 0] = tileMgr.getTile(225);
    };
    setFloor(2, 1);
    setFloor(3, 1);
    setFloor(2, 2);
    setFloor(3, 2);
    setFloor(3, 3);
    setFloor(4, 2);
    setFloor(4, 1);
    map.setTiles(kMaxX, kMaxY, kMaxZ, tiles.data());

    rs_knl::Mission mission;
    REQUIRE(mission.init(&map));

    auto *pGuard = new rs_knl::PedInstance(47, &map, rs_knl::PedInstance::kPedTypePolice, false, 128);
    pGuard->setStartHealth(1, true);
    pGuard->setPosition(rs_knl::TilePoint(2, 2, 1, 0, 82, 0));
    pGuard->setDirection(1);
    pGuard->addToDefaultActions(new rs_knl::WalkToDirectionAction());
    pGuard->setCurrentActionWithSource(rs_knl::Action::kActionDefault);
    mission.addPed(pGuard);

    for (int tick = 0; tick < 120; ++tick) {
        pGuard->executeAction(40, &mission);
    }

    rs_knl::TilePoint endPos = pGuard->position();
    bool escapedCorner = (endPos.tx != 2 || endPos.ty != 2 || endPos.ox > 32 || endPos.oy < 200);
    CHECK(escapedCorner);
}

TEST_CASE("PolicePerceptionScaledDetectionRange", "[ped][police][perception]") {
    rs_eng::AppContext appCtx;
    ConfigFile config;
    initWeaponConfigFile(config);
    rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);

    rs_knl::Mission mission;

    auto *pArmedAgent = new rs_knl::PedInstance(0, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 178);
    pArmedAgent->setStartHealth(16, true);
    pArmedAgent->setPosition(rs_knl::TilePoint(0, 0, 0, 128, 128, 0));
    auto *pWpn = new rs_knl::WeaponInstance(&pistolDef, 0, nullptr);
    pArmedAgent->addWeapon(pWpn);
    pArmedAgent->selectWeapon((size_t)0);
    mission.addPed(pArmedAgent);
    mission.addArmedPed(pArmedAgent);

    // Distance = 2048 units (8 tiles):
    // Far East Police (dep=80, amt=208 -> ~1855 range) should NOT spot armed agent at 2048;
    // California Police (dep=110, amt=238 -> ~2207 range) SHOULD spot armed agent at 2048.
    auto *pFarEastCop = new rs_knl::PedInstance(10, nullptr, rs_knl::PedInstance::kPedTypePolice, false, 128);
    pFarEastCop->setStartHealth(1, true);
    pFarEastCop->setSightRange(1500);
    pFarEastCop->initAllLevelsForIPAType(IPAStim::Perception, 208, 80, 0);
    pFarEastCop->setPosition(rs_knl::TilePoint(8, 0, 0, 128, 128, 0));
    auto *pFeBehv = new rs_knl::PoliceBehaviourComponent();
    pFarEastCop->behaviour().addComponent(pFeBehv);
    mission.addPed(pFarEastCop);

    auto *pCaliCop = new rs_knl::PedInstance(11, nullptr, rs_knl::PedInstance::kPedTypePolice, false, 128);
    pCaliCop->setStartHealth(1, true);
    pCaliCop->setSightRange(1500);
    pCaliCop->initAllLevelsForIPAType(IPAStim::Perception, 238, 110, 0);
    pCaliCop->setPosition(rs_knl::TilePoint(8, 0, 0, 128, 128, 0));
    auto *pCaBehv = new rs_knl::PoliceBehaviourComponent();
    pCaliCop->behaviour().addComponent(pCaBehv);
    mission.addPed(pCaliCop);

    pFarEastCop->behaviour().execute(250, &mission);
    pCaliCop->behaviour().execute(250, &mission);

    CHECK(pFarEastCop->currentAction() == nullptr);
    CHECK(pCaliCop->currentAction() != nullptr);
}

namespace {
void initTransparentTestMap(MemoryTileManager &tileMgr, rs_knl::Map &map, int maxTx, int maxTy, int maxTz) {
    int nbTiles = maxTx * maxTy * maxTz;
    std::vector<rs_eng::Tile *> tiles(nbTiles, tileMgr.getTile(rs_eng::TileManager::kIndexTransparentTile));
    map.setTiles(maxTx, maxTy, maxTz, tiles.data());
}
} // namespace

TEST_CASE("NpcDosIpaDetectionAndPanicRanges", "[ped][perception][ipa]") {
    rs_eng::AppContext appCtx;
    rs_eng::AnimationManager animMgr;
    MemoryTileManager tileMgr;
    rs_knl::MissionManager misMgr(&tileMgr);
    ConfigFile config;
    initWeaponConfigFile(config);
    rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);

    rs_knl::Map map(&tileMgr, 1);
    initTransparentTestMap(tileMgr, map, 20, 20, 3);

    rs_knl::Mission mission;
    REQUIRE(mission.init(&map));
    misMgr.setMission(&mission);

    auto makeNpcRecord = [](uint8_t typePed, uint8_t dep, uint8_t eff, uint8_t lock) {
        LevelData::People ppl{};
        ppl.type_ped = typePed;
        ppl.location = LevelData::kPeopleLocAboveWalkSurf;
        ppl.mapposx[1] = 0;
        ppl.mapposx[0] = 128;
        ppl.mapposy[1] = 0;
        ppl.mapposy[0] = 128;
        ppl.mapposz[0] = 128; // z=1, oz=0
        ppl.health[0] = 4;
        ppl.adrena_amount = 128;
        ppl.adrena_dependency = dep;
        ppl.adrena_effect = eff;
        ppl.unkn11 = lock;
        ppl.inteli_amount = 128;
        ppl.inteli_dependency = dep;
        ppl.inteli_effect = eff;
        ppl.unkn12 = lock;
        ppl.percep_amount = 128;
        ppl.percep_dependency = dep;
        ppl.percep_effect = eff;
        ppl.unkn13 = lock;
        return ppl;
    };

    rs_knl::PedManager pedMgr;

    auto *pAgent = new rs_knl::PedInstance(0, &map, rs_knl::PedInstance::kPedTypeAgent, true, 178);
    pAgent->setStartHealth(16, true);
    pAgent->setPosition(rs_knl::TilePoint(0, 0, 1, 128, 128, 0));
    auto *pAgentPistol = new rs_knl::WeaponInstance(&pistolDef, 0, nullptr);
    pAgent->addWeapon(pAgentPistol);
    pAgent->selectWeapon((size_t)0);
    mission.addPed(pAgent);
    mission.addArmedPed(pAgent);
    mission.getSquad()->setMember(rs_knl::Squad::kSlot1, pAgent);

    SECTION("GAMEXX.DAT Police IPA effect/lock scales range to 1855 (mid) and 2207 (high)") {
        // Mid-tier cop: amount=128, dep=80, eff=208, lock=80 -> ~1855 range
        LevelData::People midRec = makeNpcRecord(0x04, 80, 208, 80);
        rs_knl::PedInstance *pMidCop = pedMgr.loadInstance(midRec, 64, &map, 1, false);
        REQUIRE(pMidCop != nullptr);
        CHECK(pMidCop->perception().amount() == 81);
        CHECK(pMidCop->perception().dependency() == 31);

        // Lock-only override (file has 128/128/128, lock=110 -> MAIN.EXE forces 238/110 -> ~2207 range)
        LevelData::People highRec = makeNpcRecord(0x04, 128, 128, 110);
        rs_knl::PedInstance *pHighCop = pedMgr.loadInstance(highRec, 65, &map, 1, false);
        REQUIRE(pHighCop != nullptr);
        CHECK(pHighCop->perception().amount() == 92);
        CHECK(pHighCop->perception().dependency() == 42);

        pMidCop->setPosition(rs_knl::TilePoint(7, 0, 1, 128, 128, 0)); // 1792 < 1855
        pHighCop->setPosition(rs_knl::TilePoint(8, 0, 1, 128, 128, 0)); // 2048 < 2207
        mission.addPed(pMidCop);
        mission.addPed(pHighCop);

        pMidCop->behaviour().execute(250, &mission);
        pHighCop->behaviour().execute(250, &mission);

        CHECK(pMidCop->currentAction() != nullptr);
        CHECK(pHighCop->currentAction() != nullptr);
    }

    SECTION("GAMEXX.DAT Guard IPA uses 2048 max range -> 1484 (mid) and 1765 (high)") {
        LevelData::People midGuardRec = makeNpcRecord(0x08, 80, 208, 80);
        rs_knl::PedInstance *pMidGuard = pedMgr.loadInstance(midGuardRec, 66, &map, 1, false);
        REQUIRE(pMidGuard != nullptr);
        pMidGuard->addWeapon(new rs_knl::WeaponInstance(&pistolDef, 0, nullptr));

        LevelData::People highGuardRec = makeNpcRecord(0x08, 110, 238, 110);
        rs_knl::PedInstance *pHighGuard = pedMgr.loadInstance(highGuardRec, 67, &map, 1, false);
        REQUIRE(pHighGuard != nullptr);
        pHighGuard->addWeapon(new rs_knl::WeaponInstance(&pistolDef, 0, nullptr));

        // 1600 units: beyond mid-guard (1484), inside high-guard (1765)
        pMidGuard->setPosition(rs_knl::TilePoint(6, 0, 1, 192, 128, 0)); // 6*256 + 64 = 1600
        pHighGuard->setPosition(rs_knl::TilePoint(6, 0, 1, 192, 128, 0));
        mission.addPed(pMidGuard);
        mission.addPed(pHighGuard);

        pMidGuard->behaviour().execute(100, &mission);
        pHighGuard->behaviour().execute(100, &mission);

        CHECK(pMidGuard->currentAction() == nullptr);
        CHECK(pHighGuard->currentAction() != nullptr);
    }

    SECTION("GAMEXX.DAT Civilian IPA uses 2560 max range -> 2207 (high)") {
        LevelData::People highCivRec = makeNpcRecord(0x01, 110, 238, 110);
        rs_knl::PedInstance *pCiv = pedMgr.loadInstance(highCivRec, 68, &map, 1, false);
        REQUIRE(pCiv != nullptr);
        pCiv->addToDefaultActions(new rs_knl::WalkToDirectionAction());
        pCiv->addMovementAction(pCiv->defaultAction(), false);
        pCiv->setPosition(rs_knl::TilePoint(8, 0, 1, 128, 128, 0)); // 2048 < 2207
        mission.addPed(pCiv);

        pCiv->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponOut, pAgent);
        pCiv->behaviour().execute(600, &mission);
        CHECK(pCiv->isInPanic());
    }
}

TEST_CASE("PoliceVariableDisengagementCooldown", "[ped][police][cooldown]") {
    rs_eng::AppContext appCtx;
    rs_eng::AnimationManager animMgr;
    MemoryTileManager tileMgr;
    rs_knl::MissionManager misMgr(&tileMgr);
    ConfigFile config;
    initWeaponConfigFile(config);
    rs_knl::Weapon pistolDef(rs_knl::Weapon::Pistol, config);

    rs_knl::Map map(&tileMgr, 1);
    initTransparentTestMap(tileMgr, map, 12, 12, 3);

    rs_knl::Mission mission;
    REQUIRE(mission.init(&map));
    misMgr.setMission(&mission);

    auto *pAgent = new rs_knl::PedInstance(0, &map, rs_knl::PedInstance::kPedTypeAgent, true, 178);
    pAgent->setStartHealth(16, true);
    pAgent->setPosition(rs_knl::TilePoint(2, 2, 1, 128, 128, 0));
    pAgent->addWeapon(new rs_knl::WeaponInstance(&pistolDef, 0, nullptr));
    pAgent->selectWeapon((size_t)0);
    mission.addPed(pAgent);
    mission.addArmedPed(pAgent);

    auto *pCop = new rs_knl::PedInstance(64, &map, rs_knl::PedInstance::kPedTypePolice, false, 128);
    pCop->setStartHealth(1, true);
    pCop->setSightRange(1500);
    pCop->initAllLevelsForIPAType(IPAStim::Adrenaline, 208, 80, 208);
    pCop->initAllLevelsForIPAType(IPAStim::Perception, 208, 80, 208);
    pCop->setPosition(rs_knl::TilePoint(4, 2, 1, 128, 128, 0)); // 512 units away
    auto *pCopPistol = new rs_knl::WeaponInstance(&pistolDef, 0, nullptr);
    pCop->addWeapon(pCopPistol);
    pCop->addToDefaultActions(new rs_knl::WalkToDirectionAction());
    pCop->addMovementAction(pCop->defaultAction(), false);
    auto *pCopBehv = new rs_knl::PoliceBehaviourComponent();
    pCop->behaviour().addComponent(pCopBehv);
    mission.addPed(pCop);

    // Engage armed agent
    pCop->behaviour().execute(250, &mission);
    REQUIRE(pCop->isCurrentActionFromSource(rs_knl::Action::kActionAlt));

    SECTION("Unwielding weapon starts cooldown rather than disengaging instantaneously, and cop can still shoot") {
        const int initialAmmo = pCopPistol->ammoRemaining();

        // Advance near end of WaitBeforeShootingAction (2000ms) so cop hasn't fired yet
        for (int i = 0; i < 19; ++i) {
            pCop->behaviour().execute(100, &mission);
            pCop->executeAction(100, &mission);
            pCop->executeUseWeaponAction(100, &mission);
        }
        REQUIRE(pCopPistol->ammoRemaining() == initialAmmo);

        // Player unwields weapon right before shot
        pAgent->deselectWeapon();
        mission.removeArmedPed(pAgent);
        pCop->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponCleared, pAgent);

        // Immediately and after a short 300ms step (< 750ms minimum cooldown), cop is STILL engaged
        // and finishes WaitBeforeShootingAction -> FollowToShootAction -> FireWeaponAction, firing a shot
        CHECK(pCop->isCurrentActionFromSource(rs_knl::Action::kActionAlt));
        for (int i = 0; i < 3; ++i) {
            pCop->behaviour().execute(100, &mission);
            pCop->executeAction(100, &mission);
            pCop->executeUseWeaponAction(100, &mission);
        }
        CHECK(pCop->isCurrentActionFromSource(rs_knl::Action::kActionAlt));
        CHECK(pCopPistol->ammoRemaining() < initialAmmo);

        // Advance well past maximum cooldown (2600ms) and weapon reload -> cop disengages to patrol
        for (int i = 0; i < 45; ++i) {
            pCop->behaviour().execute(100, &mission);
            pCop->executeAction(100, &mission);
            pCop->executeUseWeaponAction(100, &mission);
        }
        CHECK(pCop->isCurrentActionFromSource(rs_knl::Action::kActionDefault));
        CHECK(pCop->selectedWeapon() == nullptr);
    }

    SECTION("Re-arming during cooldown cancels disengagement") {
        pAgent->deselectWeapon();
        mission.removeArmedPed(pAgent);
        pCop->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponCleared, pAgent);

        pCop->behaviour().execute(400, &mission);
        CHECK(pCop->isCurrentActionFromSource(rs_knl::Action::kActionAlt));

        // Re-arm before cooldown expires
        pAgent->selectWeapon((size_t)0);
        mission.addArmedPed(pAgent);
        pCop->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponOut, pAgent);

        // Advance 3000ms — cop must remain engaged because target is armed again
        for (int i = 0; i < 30; ++i) {
            pCop->behaviour().execute(100, &mission);
        }
        CHECK(pCop->isCurrentActionFromSource(rs_knl::Action::kActionAlt));
    }

    SECTION("Killing target still disengages immediately") {
        pAgent->setHealth(0);
        pAgent->deselectWeapon();
        mission.removeArmedPed(pAgent);
        pCop->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponCleared, pAgent);

        pCop->behaviour().execute(100, &mission);
        pCop->executeAction(100, &mission);
        pCop->behaviour().execute(100, &mission);
        CHECK(pCop->isCurrentActionFromSource(rs_knl::Action::kActionDefault));
    }
}
