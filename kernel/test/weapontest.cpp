/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#include <cmath>
#include "rs-kernel/model/weapon.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/shot.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-engine/appcontext.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/animationloader.h"
#include "rs-engine/gfx/base_weapon_sprites.h"
#include "rs-utils/io/file.h"

#include "testcase.h"

TEST_CASE( "WeaponInstance", "[kernel][weapon]" ) {
    rs_eng::AppContext appCtx;
    ConfigFile config;
    initWeaponConfigFile(config);
    rs_knl::Weapon energyClass(rs_knl::Weapon::EnergyShield, config);

    // weapon.13.auto.fire_rate=75, weapon.13.ammopershot=1, weapon.13.ammo.nb=200
    REQUIRE( energyClass.fireRate() == 75 );
    REQUIRE( energyClass.ammoPerShot() == 1 );

    rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
    REQUIRE( pistolClass.reloadTime() == 600 );

    rs_knl::Weapon shotgunClass(rs_knl::Weapon::Shotgun, config);
    REQUIRE( shotgunClass.reloadTime() == 200 );

    SECTION( "consumeAmmoForEnergyShield" ) {
        SECTION( "Should not consume ammo before fireRate is elapsed" ) {
            rs_knl::WeaponInstance shield(&energyClass, 0, nullptr);

            REQUIRE_FALSE( shield.consumeAmmoForEnergyShield(50) );

            REQUIRE( shield.ammoRemaining() == 200 );
        }

        SECTION( "Should consume one shot of ammo once fireRate is elapsed" ) {
            rs_knl::WeaponInstance shield(&energyClass, 0, nullptr);

            shield.consumeAmmoForEnergyShield(50);
            REQUIRE_FALSE( shield.consumeAmmoForEnergyShield(30) );

            REQUIRE( shield.ammoRemaining() == 199 );
        }

        SECTION( "Should consume at most one shot of ammo per call, however large elapsed is" ) {
            rs_knl::WeaponInstance shield(&energyClass, 0, nullptr);

            REQUIRE_FALSE( shield.consumeAmmoForEnergyShield(1000) );

            REQUIRE( shield.ammoRemaining() == 199 );
        }

        SECTION( "Should return true once ammo is depleted" ) {
            rs_knl::WeaponInstance shield(&energyClass, 0, nullptr, 1);

            REQUIRE( shield.consumeAmmoForEnergyShield(80) );

            REQUIRE( shield.ammoRemaining() == 0 );
        }

        SECTION( "Should keep returning true and not go negative once ammo is depleted" ) {
            rs_knl::WeaponInstance shield(&energyClass, 0, nullptr, 1);
            shield.consumeAmmoForEnergyShield(80);

            REQUIRE( shield.consumeAmmoForEnergyShield(80) );

            REQUIRE( shield.ammoRemaining() == 0 );
        }
    }

    SECTION( "canShoot classification for weapons vs non-weapon items" ) {
        rs_knl::Weapon pistol(rs_knl::Weapon::Pistol, config);
        rs_knl::Weapon uzi(rs_knl::Weapon::Uzi, config);
        rs_knl::Weapon minigun(rs_knl::Weapon::Minigun, config);
        rs_knl::Weapon laser(rs_knl::Weapon::Laser, config);
        rs_knl::Weapon flamer(rs_knl::Weapon::Flamer, config);
        rs_knl::Weapon longRange(rs_knl::Weapon::LongRange, config);
        rs_knl::Weapon gauss(rs_knl::Weapon::GaussGun, config);
        rs_knl::Weapon shotgun(rs_knl::Weapon::Shotgun, config);

        REQUIRE( pistol.canShoot() );
        REQUIRE( uzi.canShoot() );
        REQUIRE( minigun.canShoot() );
        REQUIRE( laser.canShoot() );
        REQUIRE( flamer.canShoot() );
        REQUIRE( longRange.canShoot() );
        REQUIRE( gauss.canShoot() );
        REQUIRE( shotgun.canShoot() );

        rs_knl::Weapon persuader(rs_knl::Weapon::Persuadatron, config);
        rs_knl::Weapon scanner(rs_knl::Weapon::Scanner, config);
        rs_knl::Weapon medikit(rs_knl::Weapon::MediKit, config);
        rs_knl::Weapon timebomb(rs_knl::Weapon::TimeBomb, config);
        rs_knl::Weapon accesscard(rs_knl::Weapon::AccessCard, config);
        rs_knl::Weapon shield(rs_knl::Weapon::EnergyShield, config);

        REQUIRE_FALSE( persuader.canShoot() );
        REQUIRE_FALSE( scanner.canShoot() );
        REQUIRE_FALSE( medikit.canShoot() );
        REQUIRE_FALSE( timebomb.canShoot() );
        REQUIRE_FALSE( accesscard.canShoot() );
        REQUIRE_FALSE( shield.canShoot() );

        rs_knl::WeaponInstance persuaderInst(&persuader, 0, nullptr);
        rs_knl::WeaponInstance minigunInst(&minigun, 1, nullptr);
        REQUIRE_FALSE( persuaderInst.canShoot() );
        REQUIRE( minigunInst.canShoot() );
    }

    SECTION( "usesAmmo classification for ammunition-consuming vs non-ammo items" ) {
        rs_knl::Weapon pistol(rs_knl::Weapon::Pistol, config);
        rs_knl::Weapon shield(rs_knl::Weapon::EnergyShield, config);
        rs_knl::Weapon timebomb(rs_knl::Weapon::TimeBomb, config);
        rs_knl::Weapon scanner(rs_knl::Weapon::Scanner, config);
        rs_knl::Weapon accesscard(rs_knl::Weapon::AccessCard, config);
        rs_knl::Weapon persuader(rs_knl::Weapon::Persuadatron, config);

        REQUIRE( pistol.usesAmmo() );
        REQUIRE( shield.usesAmmo() );
        REQUIRE_FALSE( timebomb.usesAmmo() );
        REQUIRE_FALSE( scanner.usesAmmo() );
        REQUIRE_FALSE( accesscard.usesAmmo() );
        REQUIRE_FALSE( persuader.usesAmmo() );

        rs_knl::Weapon airStrike(rs_knl::Weapon::AirStrike, config);
        rs_knl::Weapon cloneShield(rs_knl::Weapon::CloneShield, config);
        REQUIRE_FALSE( airStrike.canShoot() );
        REQUIRE_FALSE( airStrike.usesAmmo() );
        REQUIRE_FALSE( cloneShield.canShoot() );
        REQUIRE( cloneShield.usesAmmo() );
    }

    SECTION( "Base weapons and American Revolt weapons have distinct icons and selectors" ) {
        rs_knl::Weapon pistol(rs_knl::Weapon::Pistol, config);
        rs_knl::Weapon accessCard(rs_knl::Weapon::AccessCard, config);
        rs_knl::Weapon airStrike(rs_knl::Weapon::AirStrike, config);
        rs_knl::Weapon cloneShield(rs_knl::Weapon::CloneShield, config);

        REQUIRE( pistol.getSmallIconId() == 15 );
        REQUIRE( pistol.getBigIconId() == 65 );
        REQUIRE( pistol.selector() == 1603 );

        REQUIRE( accessCard.getSmallIconId() == 26 );
        REQUIRE( accessCard.getBigIconId() == 76 );
        REQUIRE( accessCard.selector() == 1614 );

        REQUIRE( airStrike.getSmallIconId() == 27 );
        REQUIRE( airStrike.getBigIconId() == 1180 );
        REQUIRE( airStrike.selector() == 1615 );

        REQUIRE( cloneShield.getSmallIconId() == 29 );
        REQUIRE( cloneShield.getBigIconId() == 83 );
        REQUIRE( cloneShield.selector() == 1617 );
    }

    SECTION( "CloneShield auto-activates on selection, disguises agent as civilian, and hides hostility" ) {
        rs_knl::Weapon cloneShieldClass(rs_knl::Weapon::CloneShield, config);
        rs_knl::WeaponInstance cloneShield(&cloneShieldClass, 0, nullptr);

        rs_knl::PedInstance agent(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        agent.setStartHealth(16, true);
        agent.setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
        agent.addEnemyGroupDef(2);
        agent.addWeapon(&cloneShield);

        rs_knl::PedInstance guard(8, nullptr, rs_knl::PedInstance::kPedTypeGuard, false, 50);
        guard.setStartHealth(16, true);
        guard.setObjGroupID(2);
        guard.addEnemyGroupDef(1);

        rs_knl::PedInstance enemyAgent(9, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 50);
        enemyAgent.setStartHealth(16, true);
        enemyAgent.setObjGroupID(2);
        enemyAgent.addEnemyGroupDef(1);
        agent.setObjGroupID(1);

        REQUIRE( guard.isHostileTo(&agent) );
        REQUIRE( enemyAgent.isHostileTo(&agent) );
        REQUIRE_FALSE( agent.isCloneShieldActivated() );
        REQUIRE_FALSE( agent.isEnergyShieldActivated() );

        // Selecting Clone Shield automatically activates it and disguises the agent
        agent.selectWeapon(&cloneShield);
        REQUIRE( agent.selectedWeapon() == &cloneShield );
        REQUIRE( agent.isCloneShieldActivated() );
        REQUIRE_FALSE( agent.isEnergyShieldActivated() );
        // Guards are fooled by the civilian clone disguise
        REQUIRE_FALSE( guard.isHostileTo(&agent) );
        // But rival enemy agents see through the disguise!
        REQUIRE( enemyAgent.isHostileTo(&agent) );

        // Deselecting Clone Shield restores normal agent state and hostility
        agent.deselectWeapon();
        REQUIRE_FALSE( agent.isCloneShieldActivated() );
        REQUIRE( guard.isHostileTo(&agent) );
        REQUIRE( enemyAgent.isHostileTo(&agent) );
    }

    SECTION( "AirStrikeShot bomber flight trajectory and progression" ) {
        rs_knl::Weapon airStrikeClass(rs_knl::Weapon::AirStrike, config);
        rs_knl::WeaponInstance airStrike(&airStrikeClass, 0, nullptr);

        rs_knl::DamageToInflict dmg;
        dmg.dtype = rs_knl::kDmgTypeExplosion;
        dmg.dvalue = 128;
        dmg.range = 1024;
        dmg.d_owner = nullptr;
        dmg.pWeapon = &airStrike;
        dmg.originLocW = rs_knl::WorldPoint(10 * 256, 20 * 256, 512);
        dmg.aimedLocW = rs_knl::WorldPoint(18 * 256, 20 * 256, 512);

        rs_knl::AirStrikeShot shot(dmg, 0.0);
        REQUIRE( shot.isAirStrike() );
        REQUIRE( shot.getPlanePositions().size() == 5 );
        REQUIRE( shot.dirX() > 0.99 );
        REQUIRE( std::abs(shot.dirY()) < 0.01 );
        // Starts 31616 world units (9.88s approach at 3200 units/s) upstream of aimedLocW
        REQUIRE( shot.currentPosition().x == dmg.aimedLocW.x - 31616 );
        REQUIRE( shot.currentPosition().y == dmg.aimedLocW.y );

        // Advance 9880 ms at 3200 units/s = 31616 units -> reaches aimedLocW
        shot.animate(9880, nullptr);
        REQUIRE_FALSE( shot.isLifeOver() );
        REQUIRE( shot.currentPosition().x == dmg.aimedLocW.x );

        // Advance through bullet strafe (~1.5s), doubled bombing sweep (~6.8s), and sprite dissipation (~5.0s) -> finishes pass
        shot.animate(20000, nullptr);
        REQUIRE( shot.isLifeOver() );
        REQUIRE( shot.currentPosition().x == dmg.aimedLocW.x + 2500 + 8192 );
    }

    SECTION( "FlamerShot DOS substep range, muzzle offset, and ambient wind drift" ) {
        MemoryTileManager tileMgr;
        rs_knl::Map testMap(&tileMgr, 1);
        const int kMaxX = 20, kMaxY = 20, kMaxZ = 2;
        std::vector<rs_eng::Tile *> tiles(kMaxX * kMaxY * kMaxZ,
            tileMgr.getTile(rs_eng::TileManager::kIndexTransparentTile));
        for (int ty = 0; ty < kMaxY; ++ty) {
            for (int tx = 0; tx < kMaxX; ++tx) {
                tiles[(ty * kMaxX + tx) * kMaxZ + 0] = tileMgr.getTile(225);
            }
        }
        testMap.setTiles(kMaxX, kMaxY, kMaxZ, tiles.data());

        rs_knl::Weapon flamerClass(rs_knl::Weapon::Flamer, config);
        rs_knl::WeaponInstance flamer(&flamerClass, 1000, nullptr);

        // Zero wind: 96 barrel-to-muzzle + 14 * (38..48) -> ~628..768 world units along +Y (dir=0)
        // (+64 barrel offset in ShootAction::fillDamageDesc yields ~692..832 from shooter center)
        rs_knl::Mission missionNoWind;
        REQUIRE( missionNoWind.init(&testMap) );
        missionNoWind.setWind(0, 0);

        rs_knl::DamageToInflict dmg{};
        dmg.dtype = rs_knl::kDmgTypeBurn;
        dmg.dvalue = flamerClass.damagePerShot();
        dmg.range = flamerClass.range();
        dmg.pWeapon = &flamer;
        dmg.originLocW = rs_knl::WorldPoint(rs_knl::TilePoint(10, 5, 1, 128, 128, 0));
        dmg.aimedLocW = rs_knl::WorldPoint(dmg.originLocW.x, dmg.originLocW.y + 512, dmg.originLocW.z);

        rs_knl::FlamerShot shotNeutral(&missionNoWind, dmg);
        REQUIRE( shotNeutral.currentPosition().y - dmg.originLocW.y == 0 );
        shotNeutral.animate(0, &missionNoWind); // first tick skips move
        // Advance halfway (~150ms * 2400 = 360 units) -> continuous 32-unit trail spawned (~11 sprites)
        shotNeutral.animate(150, &missionNoWind);
        REQUIRE( !shotNeutral.isLifeOver() );
        REQUIRE( missionNoWind.numSfxObjects() >= 10 );
        // Finish stream (~250ms more at 2400 u/s) -> completes and adds endpoint linger sprite
        shotNeutral.animate(250, &missionNoWind);
        REQUIRE( shotNeutral.isLifeOver() );
        int dyNeutral = shotNeutral.currentPosition().y - dmg.originLocW.y;
        REQUIRE( dyNeutral >= 625 );
        REQUIRE( dyNeutral <= 775 );
        REQUIRE( missionNoWind.numSfxObjects() >= 20 );

        // Tailwind (+Y windAngle=0, speed=16): adds 14 * 16 = +224 -> ~852..992 units from barrel
        rs_knl::Mission missionTailWind;
        REQUIRE( missionTailWind.init(&testMap) );
        missionTailWind.setWind(0, 16);
        rs_knl::FlamerShot shotTail(&missionTailWind, dmg);
        shotTail.animate(0, &missionTailWind);
        shotTail.animate(450, &missionTailWind);
        REQUIRE( shotTail.isLifeOver() );
        int dyTail = shotTail.currentPosition().y - dmg.originLocW.y;
        REQUIRE( dyTail >= 845 );
        REQUIRE( dyTail <= 1005 );

        // Headwind (-Y windAngle=128, speed=16): subtracts 14 * 16 = -224 -> ~404..544 units from barrel
        rs_knl::Mission missionHeadWind;
        REQUIRE( missionHeadWind.init(&testMap) );
        missionHeadWind.setWind(128, 16);
        rs_knl::FlamerShot shotHead(&missionHeadWind, dmg);
        shotHead.animate(0, &missionHeadWind);
        shotHead.animate(300, &missionHeadWind);
        REQUIRE( shotHead.isLifeOver() );
        int dyHead = shotHead.currentPosition().y - dmg.originLocW.y;
        REQUIRE( dyHead >= 395 );
        REQUIRE( dyHead <= 555 );
        REQUIRE( dyTail - dyHead >= 350 );
    }

    SECTION( "Inspect Base vs AR sprites" ) {
        initTestDataFolders();
        size_t baseTabSize = 0, baseDatSize = 0;
        uint8_t *baseTab = rs_utl::File::loadOriginalFile("mspr-0.tab", baseTabSize);
        uint8_t *baseDat = rs_utl::File::loadOriginalFile("mspr-0.dat", baseDatSize);
        REQUIRE( baseTab != nullptr );
        REQUIRE( baseDat != nullptr );

        size_t arTabSize = 0, arDatSize = 0;
        std::string arDir = "synd/DATADISK/DATA";
        if (!std::filesystem::exists(arDir + "/MSPR-0-D.TAB") && !std::filesystem::exists(arDir + "/mspr-0-d.tab")) {
            arDir = "C:/Games/Syndicate Plus/DATADISK/DATA";
        }
        uint8_t *arTab = rs_utl::File::loadOriginalFile(arDir + "/MSPR-0-D.TAB", arTabSize);
        if (!arTab) arTab = rs_utl::File::loadOriginalFile(arDir + "/mspr-0-d.tab", arTabSize);
        uint8_t *arDat = rs_utl::File::loadOriginalFile(arDir + "/MSPR-0-D.DAT", arDatSize);
        if (!arDat) arDat = rs_utl::File::loadOriginalFile(arDir + "/mspr-0-d.dat", arDatSize);

        uint32_t bOff15 = rs_utl::READ_LE_UINT32(baseTab + 15 * 6);
        uint32_t bOff65 = rs_utl::READ_LE_UINT32(baseTab + 65 * 6);
        uint32_t bOff26 = rs_utl::READ_LE_UINT32(baseTab + 26 * 6);
        uint32_t bOff76 = rs_utl::READ_LE_UINT32(baseTab + 76 * 6);

        CHECK( std::memcmp(rs_eng::kBaseMsprPistolSmall, baseDat + bOff15, 288) == 0 );
        CHECK( std::memcmp(rs_eng::kBaseMsprPistolBig, baseDat + bOff65, 2752) == 0 );
        CHECK( std::memcmp(rs_eng::kBaseMsprAccessCardSmall, baseDat + bOff26, 288) == 0 );
        CHECK( std::memcmp(rs_eng::kBaseMsprAccessCardBig, baseDat + bOff76, 2752) == 0 );

        if (arTab && arDat) {
            uint32_t aOff15 = rs_utl::READ_LE_UINT32(arTab + 15 * 6);
            uint32_t aOff65 = rs_utl::READ_LE_UINT32(arTab + 65 * 6);
            uint32_t aOff26 = rs_utl::READ_LE_UINT32(arTab + 26 * 6);
            uint32_t aOff76 = rs_utl::READ_LE_UINT32(arTab + 76 * 6);

            CHECK( std::memcmp(rs_eng::kArMsprAirStrikeSmall, arDat + aOff15, 288) == 0 );
            CHECK( std::memcmp(rs_eng::kArMsprAirStrikeBig, arDat + aOff65, 2752) == 0 );
            CHECK( std::memcmp(rs_eng::kArMsprCloneShieldSmall, arDat + aOff26, 288) == 0 );
            CHECK( std::memcmp(rs_eng::kArMsprCloneShieldBig, arDat + aOff76, 2752) == 0 );
        }

        size_t bHsprTabSize = 0, bHsprDatSize = 0;
        uint8_t *bHsprTab = rs_utl::File::loadOriginalFile("hspr-0.tab", bHsprTabSize);
        uint8_t *bHsprDat = rs_utl::File::loadOriginalFile("hspr-0.dat", bHsprDatSize);
        REQUIRE( bHsprTab != nullptr );
        REQUIRE( bHsprDat != nullptr );

        uint32_t bOff1603 = rs_utl::READ_LE_UINT32(bHsprTab + 1603 * 6);
        uint32_t bOff1643 = rs_utl::READ_LE_UINT32(bHsprTab + 1643 * 6);
        uint32_t bOff1614 = rs_utl::READ_LE_UINT32(bHsprTab + 1614 * 6);
        uint32_t bOff1654 = rs_utl::READ_LE_UINT32(bHsprTab + 1654 * 6);

        CHECK( std::memcmp(rs_eng::kBaseHsprPistolUnsel, bHsprDat + bOff1603, 640) == 0 );
        CHECK( std::memcmp(rs_eng::kBaseHsprPistolSel, bHsprDat + bOff1643, 640) == 0 );
        CHECK( std::memcmp(rs_eng::kBaseHsprAccessCardUnsel, bHsprDat + bOff1614, 640) == 0 );
        CHECK( std::memcmp(rs_eng::kBaseHsprAccessCardSel, bHsprDat + bOff1654, 640) == 0 );

        size_t aHsprTabSize = 0, aHsprDatSize = 0;
        uint8_t *aHsprTab = rs_utl::File::loadOriginalFile("C:/Games/Syndicate Plus/DATADISK/DATA/HSPR-0-D.TAB", aHsprTabSize);
        uint8_t *aHsprDat = rs_utl::File::loadOriginalFile("C:/Games/Syndicate Plus/DATADISK/DATA/HSPR-0-D.DAT", aHsprDatSize);
        if (aHsprTab && aHsprDat) {
            uint32_t aOff1603 = rs_utl::READ_LE_UINT32(aHsprTab + 1603 * 6);
            uint32_t aOff1643 = rs_utl::READ_LE_UINT32(aHsprTab + 1643 * 6);
            uint32_t aOff1614 = rs_utl::READ_LE_UINT32(aHsprTab + 1614 * 6);
            uint32_t aOff1654 = rs_utl::READ_LE_UINT32(aHsprTab + 1654 * 6);

            CHECK( std::memcmp(rs_eng::kArHsprAirStrikeUnsel, aHsprDat + aOff1603, 640) == 0 );
            CHECK( std::memcmp(rs_eng::kArHsprAirStrikeSel, aHsprDat + aOff1643, 640) == 0 );
            CHECK( std::memcmp(rs_eng::kArHsprCloneShieldUnsel, aHsprDat + aOff1614, 640) == 0 );
            CHECK( std::memcmp(rs_eng::kArHsprCloneShieldSel, aHsprDat + aOff1654, 640) == 0 );
        }

        delete[] baseTab; delete[] baseDat;
        delete[] arTab; delete[] arDat;
        delete[] bHsprTab; delete[] bHsprDat;
        delete[] aHsprTab; delete[] aHsprDat;
    }

    SECTION( "AutomaticShootAction preserves sub-interval remainder to sustain true fire rate" ) {
        rs_knl::Weapon minigunClass(rs_knl::Weapon::Minigun, config); // fire_rate=40, ammopershot=1
        rs_knl::WeaponInstance minigun(&minigunClass, 0, nullptr, 500);
        rs_knl::PedInstance agent(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        agent.setStartHealth(16, true);
        agent.addWeapon(&minigun);
        agent.selectWeapon(&minigun);

        rs_knl::AutomaticShootAction shootAct(rs_knl::WorldPoint(1000, 1000, 0), &minigun);
        shootAct.execute(30, nullptr, &agent); // initial shot
        REQUIRE( minigun.ammoRemaining() == 499 );

        shootAct.execute(30, nullptr, &agent); // accum = 30 < 40
        REQUIRE( minigun.ammoRemaining() == 499 );

        shootAct.execute(30, nullptr, &agent); // accum = 60 >= 40 -> fires, rem = 20
        REQUIRE( minigun.ammoRemaining() == 498 );

        shootAct.execute(30, nullptr, &agent); // accum = 50 >= 40 -> fires, rem = 10
        REQUIRE( minigun.ammoRemaining() == 497 );

        shootAct.execute(30, nullptr, &agent); // accum = 40 >= 40 -> fires, rem = 0
        REQUIRE( minigun.ammoRemaining() == 496 );
    }

}
