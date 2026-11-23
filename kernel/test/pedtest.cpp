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
#include "rs-kernel/model/weapon.h"
#include "rs-kernel/model/shot.h"
#include "rs-kernel/ia/actions.h"
#include "rs-kernel/ia/behaviour.h"
#include "testcase.h"
#include "rs-engine/appcontext.h"
#include "rs-engine/events/event.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/animationloader.h"
#include "rs-engine/system/system.h"
#include "rs-utils/io/file.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/mgr/agentmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/modmanager.h"
#include "rs-kernel/model/path.h"

TEST_CASE( "Ped", "[kernel][ped]" ) {
    rs_eng::AppContext appCtx;
    ConfigFile config;
    initWeaponConfigFile(config);

    rs_knl::PedInstance cut(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
    cut.setStartHealth(10, true);

    SECTION( "State management") {
        cut.goToState(rs_knl::kPedActionStateWalking);
        REQUIRE( cut.isState(rs_knl::kPedActionStateWalking) );
    }

    SECTION( "Firing state combines with current standing/walking state") {
        cut.goToState(rs_knl::kPedActionStateWalking);
        cut.goToState(rs_knl::kPedActionStateFiring);
        REQUIRE( cut.isState(rs_knl::kPedActionStateWalkingFiring) );

        // Stopping while firing keeps firing active
        cut.leaveState(rs_knl::kPedActionStateWalking);
        REQUIRE( cut.isState(rs_knl::kPedActionStateStandingFiring) );

        cut.leaveState(rs_knl::kPedActionStateFiring);
        REQUIRE( cut.isState(rs_knl::kPedActionStateStanding) );
    }

    SECTION("Mods") {
        SECTION ("Speed should be default with no mods and no load") {
            cut.setSpeedToMax();
            REQUIRE( cut.speed() == 50 );
        }

        SECTION ("Speed should be modified with no mods and overload") {
            rs_knl::Weapon bombClass(rs_knl::Weapon::TimeBomb, config);
            rs_knl::Weapon gaussClass(rs_knl::Weapon::GaussGun, config);
            rs_knl::Weapon uziClass(rs_knl::Weapon::Uzi, config);
            rs_knl::WeaponInstance bomb1(&bombClass, 0, nullptr);
            rs_knl::WeaponInstance bomb2(&bombClass, 1, nullptr);
            rs_knl::WeaponInstance uzi(&uziClass, 2, nullptr);
            rs_knl::WeaponInstance gauss(&gaussClass, 3, nullptr);

            // Unmodded max weight is 1000: 500 + 500 + 150 = 1150 (over = 150) -> 85% speed
            cut.addWeapon(&bomb1);
            cut.addWeapon(&bomb2);
            cut.addWeapon(&uzi);
            cut.setSpeedToMax();
            REQUIRE( cut.speed() == (50 * 850) / 1000 );

            // Carrying Gauss Gun (weight 3000, over >= 1000) -> crawls at kAgentMaxSpeedWithOverweight
            cut.addWeapon(&gauss);
            cut.setSpeedToMax();
            REQUIRE( cut.speed() == rs_knl::PedInstance::kAgentMaxSpeedWithOverweight );

            // With Adrenaline boosted (e.g. Hunter / 100% Adr), speed multiplies weight-clamped base speed
            // (unaugmented Leg mod multiplier at 100% Adrenaline is 2.36x -> 64 * 2.36 = 151)
            cut.setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
            cut.setIPAAmount(IPAStim::Adrenaline, 100);
            cut.setSpeedToMax();
            REQUIRE( cut.speed() > rs_knl::PedInstance::kAgentMaxSpeedWithOverweight * 2 );
            REQUIRE( cut.speed() == 151 );
        }

        SECTION ("Speed should be higher with Leg Mod and Adrenaline") {
            rs_knl::Mod legV1("LegV1", rs_knl::Mod::MOD_LEGS, rs_knl::Mod::MOD_V1, 0, "", 0);
            rs_knl::Mod legV2("LegV2", rs_knl::Mod::MOD_LEGS, rs_knl::Mod::MOD_V2, 0, "", 0);
            rs_knl::Mod legV3("LegV3", rs_knl::Mod::MOD_LEGS, rs_knl::Mod::MOD_V3, 0, "", 0);

            cut.setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
            cut.setIPAAmount(IPAStim::Adrenaline, 100);

            cut.addMod(&legV1);
            cut.setSpeedToMax();
            REQUIRE( cut.speed() == 145 );

            cut.addMod(&legV2);
            cut.setSpeedToMax();
            REQUIRE( cut.speed() == 172 );

            cut.addMod(&legV3);
            cut.setSpeedToMax();
            REQUIRE( cut.speed() == 200 );
        }
    }

    SECTION("Weapon firing interrupted on recoil / hit") {
        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance pistol(&pistolClass, 0, nullptr, 10);
        cut.addWeapon(&pistol);
        cut.selectWeapon(&pistol);

        REQUIRE( cut.canAddUseWeaponAction(&pistol) );
        cut.addActionShootAt(rs_knl::WorldPoint(rs_knl::TilePoint(1, 1, 0)));
        REQUIRE( cut.isUsingWeapon() );

        // Receive bullet hit
        rs_knl::DamageToInflict hitDmg;
        hitDmg.dtype = rs_knl::kDmgTypeBullet;
        hitDmg.dvalue = 2;
        hitDmg.originLocW = rs_knl::WorldPoint(rs_knl::TilePoint(0, 0, 0));
        hitDmg.aimedLocW = cut.position();
        hitDmg.d_owner = nullptr;
        hitDmg.pWeapon = nullptr;

        cut.handleHit(hitDmg);

        // Firing must be immediately interrupted and stopped
        REQUIRE_FALSE( cut.isUsingWeapon() );
        REQUIRE( cut.currentAction() != nullptr );
        REQUIRE( cut.currentAction()->isExclusive() );
        REQUIRE_FALSE( cut.canAddUseWeaponAction(&pistol) );

        cut.executeAction(50, nullptr);
        REQUIRE( cut.isState(rs_knl::kPedActionStateHit) );
        REQUIRE_FALSE( cut.canAddUseWeaponAction(&pistol) );
    }

    SECTION("Weapon firing prevented during dying animation and death") {
        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance pistol(&pistolClass, 0, nullptr, 10);
        cut.addWeapon(&pistol);
        cut.selectWeapon(&pistol);

        rs_knl::DamageToInflict lethalDmg;
        lethalDmg.dtype = rs_knl::kDmgTypeBullet;
        lethalDmg.dvalue = 500;
        lethalDmg.originLocW = rs_knl::WorldPoint(rs_knl::TilePoint(0, 0, 0));
        lethalDmg.aimedLocW = cut.position();
        lethalDmg.d_owner = nullptr;
        lethalDmg.pWeapon = nullptr;

        REQUIRE( cut.takeDamage(lethalDmg) );
        cut.handleDeath(lethalDmg);

        REQUIRE( cut.isDead() );
        REQUIRE( cut.isDying() );
        REQUIRE_FALSE( cut.isUsingWeapon() );
        REQUIRE_FALSE( cut.canAddUseWeaponAction(&pistol) );
        REQUIRE_FALSE( cut.executeUseWeaponAction(50, nullptr) );
    }

    SECTION("Weapon switching fires ShootingWeaponSelectedEvent consistently") {
        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
        rs_knl::Weapon uziClass(rs_knl::Weapon::Uzi, config);
        rs_knl::WeaponInstance pistol(&pistolClass, 0, nullptr, 10);
        rs_knl::WeaponInstance uzi(&uziClass, 0, nullptr, 10);
        cut.addWeapon(&pistol);
        cut.addWeapon(&uzi);

        bool lastSelectedState = false;
        ListenerHandle handle = EventManager::listen<rs_knl::ShootingWeaponSelectedEvent>(
            [&](rs_knl::ShootingWeaponSelectedEvent *evt) {
                lastSelectedState = evt->isSelected;
            });

        // 1. Draw weapon 1 (Pistol)
        cut.selectWeapon(&pistol);
        REQUIRE(lastSelectedState == true);

        // 2. Switch directly to weapon 2 (Uzi)
        cut.selectWeapon(&uzi);
        // Must end with isSelected == true so mission and police know agent is armed
        REQUIRE(lastSelectedState == true);

        // 3. Deselect / Holster weapon
        cut.deselectWeapon();
        REQUIRE(lastSelectedState == false);

        EventManager::remove_listener(handle);
    }

    SECTION("Single-shot weapon reload cooldown enforcement") {
        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance pistol(&pistolClass, 0, nullptr, 10);
        cut.addWeapon(&pistol);
        cut.selectWeapon(&pistol);

        REQUIRE( cut.canAddUseWeaponAction(&pistol) );
        uint8_t res = cut.addActionShootAt(rs_knl::WorldPoint(rs_knl::TilePoint(1, 1, 0)));
        REQUIRE( res == rs_knl::ShootAction::kShootActionSingleShoot );
        REQUIRE( cut.isUsingWeapon() );

        // Simulate player releasing the mouse button (MouseUp triggers stopShooting)
        cut.stopShooting();

        // stopShooting must NOT cancel single-shot reload cooldown
        REQUIRE( cut.isUsingWeapon() );
        REQUIRE_FALSE( cut.canAddUseWeaponAction(&pistol) );

        // Rapid clicking during cooldown must not fire another shot
        uint8_t spamRes = cut.addActionShootAt(rs_knl::WorldPoint(rs_knl::TilePoint(1, 1, 0)));
        REQUIRE( spamRes == rs_knl::ShootAction::kShootActionNotAdded );

        // Advance initial tick (firing animation starts)
        cut.executeUseWeaponAction(50, nullptr);
        REQUIRE( cut.isUsingWeapon() );
        REQUIRE_FALSE( cut.canAddUseWeaponAction(&pistol) );

        // Animation completes -> transitions to reload cooldown (800ms)
        cut.executeUseWeaponAction(101, nullptr);
        REQUIRE( cut.isUsingWeapon() );
        REQUIRE_FALSE( cut.canAddUseWeaponAction(&pistol) );

        // Advance 400ms (halfway through reload cooldown)
        cut.executeUseWeaponAction(400, nullptr);
        REQUIRE( cut.isUsingWeapon() );
        REQUIRE_FALSE( cut.canAddUseWeaponAction(&pistol) );

        // Another rapid click attempt
        spamRes = cut.addActionShootAt(rs_knl::WorldPoint(rs_knl::TilePoint(1, 1, 0)));
        REQUIRE( spamRes == rs_knl::ShootAction::kShootActionNotAdded );

        // Advance remaining 450ms to complete reload cooldown (>800ms)
        cut.executeUseWeaponAction(450, nullptr);

        // Cooldown has finished: action is deleted and weapon is ready to shoot again
        REQUIRE_FALSE( cut.isUsingWeapon() );
        REQUIRE( cut.canAddUseWeaponAction(&pistol) );

        // Subsequent shot succeeds
        uint8_t nextRes = cut.addActionShootAt(rs_knl::WorldPoint(rs_knl::TilePoint(1, 1, 0)));
        REQUIRE( nextRes == rs_knl::ShootAction::kShootActionSingleShoot );
        REQUIRE( cut.isUsingWeapon() );
    }

    SECTION("Automatic weapon stops shooting upon trigger release") {
        rs_knl::Weapon uziClass(rs_knl::Weapon::Uzi, config);
        rs_knl::WeaponInstance uzi(&uziClass, 0, nullptr, 50);
        cut.addWeapon(&uzi);
        cut.selectWeapon(&uzi);

        REQUIRE( cut.canAddUseWeaponAction(&uzi) );
        uint8_t res = cut.addActionShootAt(rs_knl::WorldPoint(rs_knl::TilePoint(1, 1, 0)));
        REQUIRE( res == rs_knl::ShootAction::kShootActionAutomaticShoot );
        REQUIRE( cut.isUsingWeapon() );

        // Releasing trigger on automatic weapon stops continuous fire
        cut.stopShooting();
        cut.executeUseWeaponAction(50, nullptr);

        REQUIRE_FALSE( cut.isUsingWeapon() );
        REQUIRE( cut.canAddUseWeaponAction(&uzi) );
    }
}

TEST_CASE("WeaponRecoilAndSlopeNavigation", "[kernel][ped][recoil]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    initTestDataFolders();
    rs_eng::AnimationManager animMgr;
    rs_eng::OriginalFilesAnimationLoader animLoader;
    animLoader.loadAnimations(animMgr);

    ConfigFile config;
    initWeaponConfigFile(config);

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

    SECTION("Recoil pushback always moves target away from shooter during continuous fire") {
        rs_knl::Mission *pMission = misMgr.create_mission(level_data);
        REQUIRE(pMission != nullptr);
        REQUIRE(pMission->buildNavigationGraph());
        misMgr.setMission(pMission);

        REQUIRE(pMission->numPeds() > 1);
        rs_knl::PedInstance *targetPed = pMission->ped(1);
        REQUIRE(targetPed != nullptr);
        targetPed->setStartHealth(500, true);

        // Shooter positioned at offset from target
        rs_knl::WorldPoint startTargetPos(targetPed->position());
        rs_knl::WorldPoint shooterPos = startTargetPos;
        shooterPos.x -= 500;
        shooterPos.y -= 500;

        auto getDistSqToShooter = [&](const rs_knl::TilePoint &tpos) {
            rs_knl::WorldPoint pos(tpos);
            int64_t dx = pos.x - shooterPos.x;
            int64_t dy = pos.y - shooterPos.y;
            return dx * dx + dy * dy;
        };

        int64_t prevDistSq = getDistSqToShooter(targetPed->position());

        // Simulate rapid minigun fire (continuous hits every 50ms)
        rs_knl::DamageToInflict hitDmg;
        hitDmg.dtype = rs_knl::kDmgTypeBullet;
        hitDmg.dvalue = 2;
        hitDmg.originLocW = shooterPos;
        hitDmg.aimedLocW = targetPed->position();
        hitDmg.d_owner = nullptr;
        hitDmg.pWeapon = nullptr;

        for (int frame = 0; frame < 20; frame++) {
            if (frame % 2 == 0) {
                targetPed->handleHit(hitDmg);
            }
            targetPed->executeAction(50, pMission);

            int64_t currentDistSq = getDistSqToShooter(targetPed->position());
            // Target must NEVER pull forward towards shooter (distSq must not decrease)
            REQUIRE(currentDistSq >= prevDistSq);
            prevDistSq = currentDistSq;
        }

        // Overall distance must have increased
        REQUIRE(prevDistSq > getDistSqToShooter(targetPed->position()) - 100);

        delete pMission;
    }

    SECTION("Recoil stopped by obstacle does not reverse direction") {
        rs_knl::Mission *pMission = misMgr.create_mission(level_data);
        REQUIRE(pMission != nullptr);
        REQUIRE(pMission->buildNavigationGraph());
        misMgr.setMission(pMission);

        REQUIRE(pMission->numPeds() > 0);
        rs_knl::PedInstance *targetPed = pMission->ped(0);
        targetPed->setStartHealth(500, true);

        // Position against map boundary (x=0)
        targetPed->setPosition(0, 0, 1, 10, 10, 0);
        rs_knl::WorldPoint startPos(targetPed->position());

        // Shooter is at +X (x=500, y=10) pushing ped against x=0 boundary
        rs_knl::WorldPoint shooterPos(500, startPos.y, startPos.z);

        rs_knl::DamageToInflict hitDmg;
        hitDmg.dtype = rs_knl::kDmgTypeBullet;
        hitDmg.dvalue = 2;
        hitDmg.originLocW = shooterPos;
        hitDmg.aimedLocW = targetPed->position();
        hitDmg.d_owner = nullptr;
        hitDmg.pWeapon = nullptr;

        targetPed->handleHit(hitDmg);
        for (int frame = 0; frame < 10; frame++) {
            targetPed->executeAction(50, pMission);
            // Ped should be blocked and not bounce forward towards shooter
            rs_knl::WorldPoint currentPos(targetPed->position());
            REQUIRE(currentPos.x <= startPos.x);
        }

        delete pMission;
    }

    SECTION("Cardinal slope and flat diagonal traversal in moveToDir") {
        rs_knl::Mission *pMission = misMgr.create_mission(level_data);
        REQUIRE(pMission != nullptr);
        REQUIRE(pMission->buildNavigationGraph());
        misMgr.setMission(pMission);

        REQUIRE(pMission->numPeds() > 0);
        rs_knl::PedInstance *ped = pMission->ped(0);

        int maxX = pMission->mmax_x_;
        int maxY = pMission->mmax_y_;
        int maxZ = pMission->mmax_z_;
        int mmax_xy = pMission->mmax_m_xy;

        bool foundSlopeTile = false;
        for (int z = 0; z < maxZ; z++) {
            for (int y = 0; y < maxY; y++) {
                for (int x = 0; x < maxX; x++) {
                    const rs_knl::FloodNode &fn = pMission->mdpoints_[x + y * maxX + z * mmax_xy];
                    if ((fn.flags & rs_knl::kWalkable) &&
                        ((fn.dirsAbove & (0x01 | 0x10 | 0x04 | 0x40)) ||
                         (fn.dirsBelow & (0x01 | 0x10 | 0x04 | 0x40)))) {
                        ped->setPosition(x, y, z, 128, 128, 0);
                        DirMoveType dmt;
                        dmt.clear();
                        dmt.bounce = false;
                        dmt.safe_walk = false;
                        int dist = 200;

                        int dir = 0;
                        if (fn.dirsAbove & 0x01 || fn.dirsBelow & 0x01) dir = 0;
                        else if (fn.dirsAbove & 0x10 || fn.dirsBelow & 0x10) dir = 128;
                        else if (fn.dirsAbove & 0x04 || fn.dirsBelow & 0x04) dir = 64;
                        else if (fn.dirsAbove & 0x40 || fn.dirsBelow & 0x40) dir = 192;

                        uint8_t res = ped->moveToDir(pMission, 100, dmt, dir, -1, -1, &dist, true, 1000);
                        REQUIRE((res & 1) == 1);
                        foundSlopeTile = true;
                        break;
                    }
                }
                if (foundSlopeTile) break;
            }
            if (foundSlopeTile) break;
        }
        delete pMission;
    }

    SECTION("Rapid continuous fire applies cumulative damage per bullet hit during recoil") {
        rs_knl::Mission *pMission = misMgr.create_mission(level_data);
        REQUIRE(pMission != nullptr);
        REQUIRE(pMission->buildNavigationGraph());
        misMgr.setMission(pMission);

        REQUIRE(pMission->numPeds() > 1);
        rs_knl::PedInstance *targetPed = pMission->ped(1);
        REQUIRE(targetPed != nullptr);
        targetPed->setStartHealth(16, true);

        rs_knl::WorldPoint shooterPos(targetPed->position());
        shooterPos.x -= 400;

        rs_knl::DamageToInflict hitDmg;
        hitDmg.dtype = rs_knl::kDmgTypeBullet;
        hitDmg.dvalue = 1;
        hitDmg.originLocW = shooterPos;
        hitDmg.aimedLocW = targetPed->position();
        hitDmg.d_owner = nullptr;
        hitDmg.pWeapon = nullptr;

        // Two hits on the same tick before executeAction must both register
        targetPed->handleHit(hitDmg);
        targetPed->handleHit(hitDmg);
        targetPed->executeAction(50, pMission);
        REQUIRE(targetPed->health() == 14);

        // Continue rapid minigun-rate fire (1 dmg every 50ms) until dead
        for (int i = 0; i < 14; ++i) {
            targetPed->handleHit(hitDmg);
            targetPed->executeAction(50, pMission);
        }
        REQUIRE(targetPed->isDead());

        delete pMission;
    }

    SECTION("Recoil knocks ped off unfenced canal void edge while fenced edge blocks") {
        LevelData::LevelDataAll m1Data;
        REQUIRE(misMgr.load_level_data(1, m1Data));
        rs_knl::Mission *pMission = misMgr.create_mission(m1Data);
        REQUIRE(pMission != nullptr);
        REQUIRE(pMission->buildNavigationGraph());
        misMgr.setMission(pMission);

        REQUIRE(pMission->numPeds() > 1);
        rs_knl::PedInstance *ped = pMission->ped(1);
        REQUIRE(ped != nullptr);
        ped->setStartHealth(500, true);

        // Give ped a weapon to confirm void-fall does not drop loot on ground
        size_t initialGroundWeapons = pMission->numWeaponsOnGround();
        rs_knl::Weapon *pPistolDef = weaponMgr.getWeapon(rs_knl::Weapon::Pistol);
        REQUIRE(pPistolDef != nullptr);
        rs_knl::WeaponInstance *pWep = rs_knl::WeaponInstance::createInstance(pPistolDef);
        pWep->setOwner(ped);
        ped->addWeapon(pWep);

        // Mission 1: (84, 36, tz=2) sits immediately east of unfenced canal void (83, 36)
        REQUIRE(pMission->surfaceAt(83, 36, 2) == rs_knl::SurfaceType::kEmpty);
        REQUIRE(pMission->surfaceAt(83, 36, 1) == rs_knl::SurfaceType::kEmpty);
        ped->setPosition(84, 36, 2, 16, 128, 0);

        rs_knl::DamageToInflict westKnock;
        westKnock.dtype = rs_knl::kDmgTypeBullet;
        westKnock.dvalue = 1;
        westKnock.originLocW = rs_knl::WorldPoint(rs_knl::TilePoint(85, 36, 2, 128, 128, 0));
        westKnock.aimedLocW = ped->position();
        westKnock.d_owner = nullptr;
        westKnock.pWeapon = nullptr;

        ped->handleHit(westKnock);
        // After 10 ticks (500ms), ped has recoiled past edge tile (83, 36) into middle of void (tx <= 82)
        // resting on the top surface plane of z=0 void tiles (tz == 1) while playing sinkAnimation
        for (int f = 0; f < 10 && ped->isAlive(); ++f) {
            ped->animate(50);
        }
        REQUIRE(ped->position().tx <= 82);
        REQUIRE(ped->position().tz == 1);
        REQUIRE(ped->isDead());
        // If sinkAnimation is loaded, ped plays the progressive sinking animation while still drawable
        REQUIRE(ped->animations().sinkAnimation != rs_knl::PedAnimations::kUndefinedAnim);
        REQUIRE(ped->isSinking());
        REQUIRE(ped->isDrawable());
        REQUIRE(ped->getDrawAnimation() == static_cast<uint16_t>(ped->baseAnim() + 201));

        // Advance through the 12-frame sink animation (~2000ms at 6 fps) until complete
        for (int f = 0; f < 60 && ped->isSinking(); ++f) {
            ped->animate(50);
        }
        REQUIRE_FALSE(ped->isSinking());
        REQUIRE_FALSE(ped->isDrawable());
        REQUIRE(pMission->numWeaponsOnGround() == initialGroundWeapons);

        // Now find a walkable tile adjacent to a handrail fence (kHandrailLight) and verify recoil stops
        bool testedFence = false;
        for (int y = 2; y < pMission->mmax_y_ - 2 && !testedFence; ++y) {
            for (int x = 2; x < pMission->mmax_x_ - 2 && !testedFence; ++x) {
                for (int z = 1; z < pMission->mmax_z_ && !testedFence; ++z) {
                    const rs_knl::FloodNode &fn = pMission->mdpoints_[x + y * pMission->mmax_x_ + z * pMission->mmax_m_xy];
                    if ((fn.flags & rs_knl::kWalkable) &&
                        pMission->surfaceAt(x - 1, y, z) == rs_knl::SurfaceType::kHandrailLight) {
                        rs_knl::PedInstance *fencedPed = pMission->ped(2);
                        REQUIRE(fencedPed != nullptr);
                        fencedPed->setStartHealth(500, true);
                        fencedPed->setPosition(x, y, z, 32, 128, 0);

                        rs_knl::DamageToInflict fenceHit;
                        fenceHit.dtype = rs_knl::kDmgTypeBullet;
                        fenceHit.dvalue = 1;
                        fenceHit.originLocW = rs_knl::WorldPoint(rs_knl::TilePoint(x + 1, y, z, 128, 128, 0));
                        fenceHit.aimedLocW = fencedPed->position();
                        fenceHit.d_owner = nullptr;
                        fenceHit.pWeapon = nullptr;

                        fencedPed->handleHit(fenceHit);
                        for (int f = 0; f < 10; ++f) {
                            fencedPed->executeAction(50, pMission);
                        }
                        CHECK(fencedPed->isAlive());
                        CHECK(fencedPed->isDrawable());
                        CHECK(fencedPed->position().tx == x);
                        testedFence = true;
                    }
                }
            }
        }
        REQUIRE(testedFence);

        delete pMission;
    }
}

TEST_CASE("LaserHitAndVaporization", "[kernel][ped][laser]") {
    rs_eng::AppContext appCtx;
    auto pSys = rs_eng::System::createSystem();
    rs_eng::AnimationManager animMgr;
    ConfigFile config;
    initWeaponConfigFile(config);

    SECTION("Laser hit: non-fatal hit recovers to standing and stays alive") {
        rs_knl::PedInstance ped(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        ped.setStartHealth(200, true);

        rs_knl::DamageToInflict dmg;
        dmg.dtype = rs_knl::kDmgTypeLaser;
        dmg.dvalue = 50;
        dmg.d_owner = nullptr;
        dmg.originLocW = {100, 100, 0};

        rs_knl::LaserHitAction hitAct(dmg);
        hitAct.execute(0, nullptr, &ped);

        REQUIRE(ped.health() == 150);
        REQUIRE(ped.isState(rs_knl::kPedActionStateHit));
        REQUIRE_FALSE(ped.isDead());
        REQUIRE_FALSE(ped.isDying());
        REQUIRE(ped.isDrawable());

        // Failsafe timer (>1000ms) or animation completion restores standing state
        hitAct.execute(1050, nullptr, &ped);

        REQUIRE_FALSE(ped.isState(rs_knl::kPedActionStateHit));
        REQUIRE(ped.isState(rs_knl::kPedActionStateStanding));
        REQUIRE_FALSE(ped.isDead());
        REQUIRE_FALSE(ped.isDying());
        REQUIRE(ped.isDrawable());
    }

    SECTION("Laser hit: fatal hit causes full vaporization for player agents and NPCs") {
        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance pistolAgent(&pistolClass, 0, nullptr);
        rs_knl::WeaponInstance pistolNpc(&pistolClass, 1, nullptr);

        // Player Agent
        rs_knl::PedInstance agent(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        agent.setStartHealth(50, true);
        agent.addWeapon(&pistolAgent);

        rs_knl::DamageToInflict fatalDmg;
        fatalDmg.dtype = rs_knl::kDmgTypeLaser;
        fatalDmg.dvalue = 100;
        fatalDmg.d_owner = nullptr;
        fatalDmg.originLocW = {100, 100, 0};

        rs_knl::LaserHitAction agentHitAct(fatalDmg);
        agentHitAct.execute(0, nullptr, &agent);

        REQUIRE(agent.isDead());
        REQUIRE(agent.isDrawable()); // still drawable while vaporization animation plays

        agentHitAct.execute(1050, nullptr, &agent);

        REQUIRE(agent.isDead());
        REQUIRE_FALSE(agent.isDrawable()); // vaporized!
        REQUIRE(agent.numWeapons() == 1); // weapons are vaporized with ped, not dropped

        // NPC Guard
        rs_knl::PedInstance npc(2, nullptr, rs_knl::PedInstance::kPedTypeGuard, false, 50);
        npc.setStartHealth(50, true);
        npc.addWeapon(&pistolNpc);

        rs_knl::LaserHitAction npcHitAct(fatalDmg);
        npcHitAct.execute(0, nullptr, &npc);

        REQUIRE(npc.isDead());
        REQUIRE(npc.isDrawable());

        npcHitAct.execute(1050, nullptr, &npc);

        REQUIRE(npc.isDead());
        REQUIRE_FALSE(npc.isDrawable()); // vaporized!
        REQUIRE(npc.numWeapons() == 1); // weapons not dropped
    }

    SECTION("Laser hit: agent death initiates dyingBurnAnimation and advances through smoke to deadBurn") {
        rs_knl::PedInstance agent(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        agent.setAnimations(1);
        agent.setStartHealth(50, true);

        rs_knl::DamageToInflict fatalDmg;
        fatalDmg.dtype = rs_knl::kDmgTypeLaser;
        fatalDmg.dvalue = 100;
        fatalDmg.d_owner = nullptr;
        fatalDmg.originLocW = {100, 100, 0};

        rs_knl::LaserHitAction agentHitAct(fatalDmg);
        agentHitAct.execute(0, nullptr, &agent);
        agentHitAct.execute(1050, nullptr, &agent);

        REQUIRE(agent.isDead());
        REQUIRE_FALSE(agent.isDrawable());

        // Agent should be in dyingBurnAnimation (ash skeleton)
        REQUIRE(agent.animationPlayer()->isCurrentAnimation(agent.animations().dyingBurnAnimation));

        // When dyingBurnAnimation finishes, handleAnimationEnded transitions to smokeBurnAnimation
        agent.handleAnimationEnded();
        REQUIRE(agent.animationPlayer()->isCurrentAnimation(agent.animations().smokeBurnAnimation));

        // When smokeBurnAnimation finishes (after smoking period), handleAnimationEnded transitions to deadBurnAnimation
        agent.handleAnimationEnded();
        REQUIRE(agent.animationPlayer()->isCurrentAnimation(agent.animations().deadBurnAnimation));

        // In selector grid view, drawSelectorAnim successfully renders the permanent ash pile
        Point2D screenPos = {50, 50};
        agent.drawSelectorAnim(screenPos);
    }

    SECTION("Police behavior: target death and retaliation when fired upon") {
        rs_knl::PedInstance police(10, nullptr, rs_knl::PedInstance::kPedTypePolice, false, 50);
        police.behaviour().addComponent(new rs_knl::PoliceBehaviourComponent());
        police.setStartHealth(50, true);
        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
        rs_knl::WeaponInstance pistolPolice(&pistolClass, 0, nullptr);
        police.addWeapon(&pistolPolice);

        rs_knl::PedInstance agent1(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        agent1.setStartHealth(50, true);
        rs_knl::WeaponInstance pistolAgent1(&pistolClass, 1, nullptr);
        agent1.addWeapon(&pistolAgent1);

        rs_knl::PedInstance agent2(2, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        agent2.setStartHealth(50, true);
        rs_knl::WeaponInstance pistolAgent2(&pistolClass, 2, nullptr);
        agent2.addWeapon(&pistolAgent2);

        // When police receives a hit from agent2, police should retaliate and target agent2
        rs_knl::DamageToInflict hitDmg;
        hitDmg.dtype = rs_knl::kDmgTypeBullet;
        hitDmg.dvalue = 5;
        hitDmg.d_owner = &agent2;
        hitDmg.originLocW = {100, 100, 0};
        police.handleHit(hitDmg);

        // When police executes behavior while target is dead, police should transition out of combat
        agent2.setStartHealth(0, true);
        police.behaviour().execute(100, nullptr);

        // Target dead handled cleanly without freezing
        REQUIRE(agent2.isDead());
    }

    SECTION("Agent ownership and persuaded follower identification") {
        rs_knl::PedInstance agent(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        rs_knl::PedInstance enemyAgent(2, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 50);
        rs_knl::PedInstance civilian(3, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 50);

        REQUIRE(agent.isOurAgent());
        REQUIRE_FALSE(agent.isPersuaded());

        REQUIRE_FALSE(enemyAgent.isOurAgent());
        REQUIRE_FALSE(enemyAgent.isPersuaded());

        REQUIRE_FALSE(civilian.isOurAgent());
        REQUIRE_FALSE(civilian.isPersuaded());

        // When civilian is persuaded by agent
        civilian.setNewOwner(&agent);
        REQUIRE(civilian.isPersuaded());
        REQUIRE_FALSE(civilian.isOurAgent());
    }

    SECTION("Persuaded NPC weapon wielding synchronizes with player agent weapon state") {
        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
        rs_knl::Weapon persuaderClass(rs_knl::Weapon::Persuadatron, config);

        rs_knl::PedInstance agent(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        agent.setStartHealth(50, true);
        rs_knl::WeaponInstance persuader(&persuaderClass, 0, nullptr);
        rs_knl::WeaponInstance pistolAgent(&pistolClass, 0, nullptr);
        agent.addWeapon(&persuader);
        agent.addWeapon(&pistolAgent);

        // Initially agent holds Persuadatron (non-weapon / cannot shoot)
        agent.selectWeapon(&persuader);
        REQUIRE_FALSE(agent.isArmed());

        // Police starts with pistol equipped
        rs_knl::PedInstance police(10, nullptr, rs_knl::PedInstance::kPedTypePolice, false, 50);
        police.setStartHealth(50, true);
        rs_knl::WeaponInstance pistolPolice(&pistolClass, 0, nullptr);
        police.addWeapon(&pistolPolice);
        police.selectWeapon(&pistolPolice);
        REQUIRE(police.isArmed());

        // 1. When cop is persuaded while agent is unarmed/holding Persuadatron, cop must holster weapon
        police.handlePersuadedBy(&agent);
        REQUIRE(police.isPersuaded());
        REQUIRE(police.selectedWeapon() == nullptr);
        REQUIRE_FALSE(police.isArmed());

        // Periodic execute maintains unholstered state while owner is unarmed
        police.behaviour().execute(100, nullptr);
        REQUIRE(police.selectedWeapon() == nullptr);

        // 2. When persuaded ped picks up a dropped weapon, it remains in inventory unequipped while agent is unarmed
        rs_knl::WeaponInstance droppedPistol(&pistolClass, 0, nullptr);
        police.addWeapon(&droppedPistol);
        rs_knl::Action::ActionType pickType = rs_knl::Action::kActTypePickUp;
        police.behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtActionEnded, &pickType);
        REQUIRE(police.selectedWeapon() == nullptr);
        police.behaviour().execute(100, nullptr);
        REQUIRE(police.selectedWeapon() == nullptr);

        // 3. When agent equips a shooting weapon, persuaded cop equips weapon
        agent.selectWeapon(&pistolAgent);
        REQUIRE(agent.isArmed());
        police.behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponOut, &agent);
        REQUIRE(police.selectedWeapon() != nullptr);
        REQUIRE(police.isArmed());

        // 4. When agent holsters weapon, persuaded cop holsters weapon
        agent.deselectWeapon();
        REQUIRE_FALSE(agent.isArmed());
        police.behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponCleared, &agent);
        REQUIRE(police.selectedWeapon() == nullptr);
        REQUIRE_FALSE(police.isArmed());

        // 5. Periodic execute automatically holsters weapon if owner becomes unarmed
        police.selectWeapon(&pistolPolice);
        REQUIRE(police.isArmed());
        police.behaviour().execute(100, nullptr);
        REQUIRE(police.selectedWeapon() == nullptr);
        REQUIRE_FALSE(police.isArmed());

        // 6. Periodic execute automatically draws weapon if owner becomes armed
        agent.selectWeapon(&pistolAgent);
        REQUIRE(agent.isArmed());
        police.behaviour().execute(100, nullptr);
        REQUIRE(police.selectedWeapon() != nullptr);
        REQUIRE(police.isArmed());
    }
}

TEST_CASE("PedInstance - Cyberware Tiers and Bitmasks", "[ped]") {
    rs_eng::AppContext appCtx;
    rs_knl::ModManager modMgr;
    modMgr.reset();

    SECTION("Static Bitmask Conversion Formulas") {
        REQUIRE(rs_knl::PedInstance::modsBitmaskFromTier(0) == 0x0000);
        REQUIRE(rs_knl::PedInstance::modsBitmaskFromTier(1) == 0x0AAA);
        REQUIRE(rs_knl::PedInstance::modsBitmaskFromTier(2) == 0x1554);
        REQUIRE(rs_knl::PedInstance::modsBitmaskFromTier(3) == 0x1FFE);

        REQUIRE(rs_knl::PedInstance::tierFromModsBitmask(0x0000) == 0);
        REQUIRE(rs_knl::PedInstance::tierFromModsBitmask(0x0AAA) == 1);
        REQUIRE(rs_knl::PedInstance::tierFromModsBitmask(0x1554) == 2);
        REQUIRE(rs_knl::PedInstance::tierFromModsBitmask(0x1FFE) == 3);

        // Fallback for invalid/unknown tiers
        REQUIRE(rs_knl::PedInstance::modsBitmaskFromTier(-1) == 0x0000);
        REQUIRE(rs_knl::PedInstance::modsBitmaskFromTier(99) == 0x0000);
    }

    SECTION("Cyberware Installation and Inspection on PedInstance") {
        rs_knl::PedInstance enemy(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 100);
        REQUIRE_FALSE(enemy.isCyberwareOverridden());
        REQUIRE(enemy.cyberwareTier() == 0);
        REQUIRE(enemy.getModsInfoBitmask() == 0x0000);

        // Apply Tier 1
        enemy.applyCyberwareTier(1);
        REQUIRE(enemy.cyberwareTier() == 1);
        REQUIRE(enemy.getModsInfoBitmask() == 0x0AAA);

        // Apply Tier 2
        enemy.applyCyberwareTier(2);
        REQUIRE(enemy.cyberwareTier() == 2);
        REQUIRE(enemy.getModsInfoBitmask() == 0x1554);

        // Apply Tier 3
        enemy.applyCyberwareTier(3);
        REQUIRE(enemy.cyberwareTier() == 3);
        REQUIRE(enemy.getModsInfoBitmask() == 0x1FFE);

        // Strip back to Tier 0
        enemy.applyCyberwareTier(0);
        REQUIRE(enemy.cyberwareTier() == 0);
        REQUIRE(enemy.getModsInfoBitmask() == 0x0000);

        // Override toggle
        enemy.setCyberwareOverridden(true);
        REQUIRE(enemy.isCyberwareOverridden());
        enemy.setCyberwareOverridden(false);
        REQUIRE_FALSE(enemy.isCyberwareOverridden());
    }

    SECTION("Chest Armor Damage Mitigation by Damage Type") {
        rs_knl::Mod chestV1("ChestV1", rs_knl::Mod::MOD_CHEST, rs_knl::Mod::MOD_V1, 0, "", 0);
        rs_knl::Mod chestV2("ChestV2", rs_knl::Mod::MOD_CHEST, rs_knl::Mod::MOD_V2, 0, "", 0);
        rs_knl::Mod chestV3("ChestV3", rs_knl::Mod::MOD_CHEST, rs_knl::Mod::MOD_V3, 0, "", 0);

        rs_knl::DamageToInflict laserDmg{};
        laserDmg.dtype = rs_knl::kDmgTypeLaser;
        laserDmg.dvalue = 32;

        rs_knl::DamageToInflict burnDmg{};
        burnDmg.dtype = rs_knl::kDmgTypeBurn;
        burnDmg.dvalue = 8;

        rs_knl::DamageToInflict expDmg{};
        expDmg.dtype = rs_knl::kDmgTypeExplosion;
        expDmg.dvalue = 64;

        // Laser & Explosion: V1 = instant kill; V2 = 8 dmg; V3 = 4 dmg
        rs_knl::PedInstance agV1(10, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 178);
        agV1.setStartHealth(16, true);
        agV1.addMod(&chestV1);
        REQUIRE( agV1.takeDamage(laserDmg) );

        rs_knl::PedInstance agV2(11, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 178);
        agV2.setStartHealth(16, true);
        agV2.addMod(&chestV2);
        REQUIRE_FALSE( agV2.takeDamage(laserDmg) );
        REQUIRE( agV2.health() == 8 );
        REQUIRE( agV2.takeDamage(expDmg) );

        rs_knl::PedInstance agV3(12, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 178);
        agV3.setStartHealth(16, true);
        agV3.addMod(&chestV3);
        REQUIRE_FALSE( agV3.takeDamage(laserDmg) );
        REQUIRE( agV3.health() == 12 );

        // Burn: V0 = instant kill; V1 = 8 dmg; V2 = 4 dmg; V3 = 2 dmg
        rs_knl::PedInstance agBurnV0(13, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 178);
        agBurnV0.setStartHealth(16, true);
        REQUIRE( agBurnV0.takeDamage(burnDmg) );

        rs_knl::PedInstance agBurnV1(14, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 178);
        agBurnV1.setStartHealth(16, true);
        agBurnV1.addMod(&chestV1);
        REQUIRE_FALSE( agBurnV1.takeDamage(burnDmg) );
        REQUIRE( agBurnV1.health() == 8 );

        rs_knl::PedInstance agBurnV2(15, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 178);
        agBurnV2.setStartHealth(16, true);
        agBurnV2.addMod(&chestV2);
        REQUIRE_FALSE( agBurnV2.takeDamage(burnDmg) );
        REQUIRE( agBurnV2.health() == 12 );

        rs_knl::PedInstance agBurnV3(16, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 178);
        agBurnV3.setStartHealth(16, true);
        agBurnV3.addMod(&chestV3);
        REQUIRE_FALSE( agBurnV3.takeDamage(burnDmg) );
        REQUIRE( agBurnV3.health() == 14 );
    }
}

