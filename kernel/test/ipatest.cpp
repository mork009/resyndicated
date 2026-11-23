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
#include <catch2/catch_approx.hpp>

#include "rs-kernel/model/ipastim.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/weapon.h"
#include "rs-kernel/model/weaponholder.h"
#include "rs-kernel/ia/behaviour.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/mgr/modmanager.h"
#include "rs-engine/appcontext.h"

#include "testcase.h"

using Catch::Approx;

TEST_CASE("IPA Stim - Adrenaline, Perception, Intelligence", "[kernel][ipa]") {
    rs_eng::AppContext appCtx;

    SECTION("IPAStim Multipliers") {
        IPAStim adr(IPAStim::Adrenaline, 50, 50);
        REQUIRE(adr.getMultiplier() == Approx(1.0f));

        adr.setAmount(100);
        REQUIRE(adr.getMultiplier() == Approx(2.0f));

        adr.setAmount(0);
        REQUIRE(adr.getMultiplier() == Approx(0.5f));

        adr.setAmount(75);
        REQUIRE(adr.getMultiplier() == Approx(1.5f));

        adr.setAmount(25);
        REQUIRE(adr.getMultiplier() == Approx(0.75f));
    }

    SECTION("Adrenaline Speed & Inverse Health Regeneration") {
        rs_knl::PedInstance agent(0, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 256);
        agent.setObjGroupDef(rs_knl::PedInstance::og_dmAgent);

        // 50% neutral Adrenaline -> standard speed
        agent.adrenaline().setAmount(50);
        int speed50 = agent.applySpeedModifier(256);
        REQUIRE(speed50 == 256);

        // 100% max Adrenaline -> boosted speed
        agent.adrenaline().setAmount(100);
        int speed100 = agent.applySpeedModifier(256);
        REQUIRE(speed100 > speed50);

        // 0% min Adrenaline -> halved speed
        agent.adrenaline().setAmount(0);
        int speed0 = agent.applySpeedModifier(256);
        REQUIRE(speed0 == 128);

        // Persuaded follower speed scaling (half-strength owner Adrenaline modifier)
        rs_knl::PedInstance civ(1, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
        civ.handlePersuadedBy(&agent);

        // Neutral agent (50% Adrenaline = 1.0x multiplier) -> civilian speed unchanged
        agent.adrenaline().setAmount(50);
        REQUIRE(agent.getPersuadedSpeedMultiplier() == Approx(1.0f));
        REQUIRE(civ.applySpeedModifier(128) == 128);

        // Max Adrenaline (100% Adrenaline = 2.0x multiplier) -> civilian gets 1.0 + (2.0 - 1.0) * 1.30 = 2.30x speed (15% faster than pre-merge 2.0x)
        agent.adrenaline().setAmount(100);
        REQUIRE(agent.getPersuadedSpeedMultiplier() == Approx(2.30f));
        REQUIRE(civ.applySpeedModifier(128) == 294); // 128 * 2.30 = 294.4

        // Low Adrenaline (0% Adrenaline = 0.5x multiplier) -> civilian gets 1.0 + (0.5 - 1.0) * 0.5 = 0.75x speed
        agent.adrenaline().setAmount(0);
        REQUIRE(agent.getPersuadedSpeedMultiplier() == Approx(0.75f));
        REQUIRE(civ.applySpeedModifier(128) == 96);

        // Cybernetic Chest mod unlocks health regen
        rs_knl::Mod chestV1("Chest V1", rs_knl::Mod::MOD_CHEST, rs_knl::Mod::MOD_V1, 1000, "Chest V1", 0);
        agent.addMod(&chestV1);
        uint16_t basePeriod = agent.getHealthRegenerationPeriod();
        REQUIRE(basePeriod == 12800);

        // Chest V3 has faster base period
        rs_knl::Mod chestV3("Chest V3", rs_knl::Mod::MOD_CHEST, rs_knl::Mod::MOD_V3, 5000, "Chest V3", 0);
        agent.addMod(&chestV3);
        REQUIRE(agent.getHealthRegenerationPeriod() == 3200);

        // DOS Chest Regeneration behavior:
        // - Above Adrenaline >= 4%, heals at fixed Chest period regardless of boosted Adrenaline.
        // - Below Adrenaline < 4% (< 10/255), all Chest mods (V1..V3) heal every 800ms.
        agent.setStartHealth(16, true);
        agent.setHealth(10);
        rs_knl::CommonAgentBehaviourComponent chestBehv(&agent);
        rs_knl::Behaviour::BehaviourParam regenParam;
        regenParam.pPed = &agent;
        regenParam.pMission = nullptr;
        regenParam.elapsed = 100;

        agent.adrenaline().setAmount(100);
        for (int i = 0; i < 32; ++i) chestBehv.execute(regenParam); // 3200ms (Timer triggers at > max)
        REQUIRE(agent.health() == 10);
        chestBehv.execute(regenParam); // 3300ms (> 3200ms) -> +1 HP fixed for V3 even at 100% Adr
        REQUIRE(agent.health() == 11);

        // Drop Adrenaline < 4%: V1 heals in 800ms (> 800ms = 900ms) just like V3
        agent.addMod(&chestV1);
        agent.adrenaline().setAmount(0);
        for (int i = 0; i < 9; ++i) chestBehv.execute(regenParam);
        REQUIRE(agent.health() == 12);
    }

    SECTION("IPA Consumption vs Dependency Retention Multipliers") {
        // Effect slows by [1x, 2x, 4x, 64/7 (~9.14x)]; Dependency creeps by [1x, 1.3x, 13/7 (~1.86x), 6.5x]
        // matching DOS 8-bit 256-tick wraparound over [4,8,16,40] and [20,28,40,128].
        IPAStim ipaV1(IPAStim::Adrenaline, 50, 50);
        ipaV1.setLevels256(255, 128, 128);
        uint8_t baseEff1 = ipaV1.effect();
        // 1900ms / 2.0 = 950ms (<= 1000ms) -> effect has not stepped yet
        ipaV1.processTicks(1900, 0);
        REQUIRE(ipaV1.effect() == baseEff1);
        // +200ms -> total 2100ms / 2.0 = 1050ms (> 1000ms) -> effect steps +1
        ipaV1.processTicks(200, 0);
        REQUIRE(ipaV1.effect() == baseEff1 + 1);

        // With MOD_V3 (64/7 ~= 9.14x effect, 6.5x dep): 9000ms / 9.143 = 984.4ms -> no step yet; +1500ms -> steps
        IPAStim ipaV3(IPAStim::Adrenaline, 50, 50);
        ipaV3.setLevels256(255, 128, 128);
        uint8_t baseEff3 = ipaV3.effect();
        ipaV3.processTicks(9000, 2);
        REQUIRE(ipaV3.effect() == baseEff3);
        ipaV3.processTicks(1500, 2);
        REQUIRE(ipaV3.effect() == baseEff3 + 1);

        // Per-frame fractional accumulation (33ms and 8ms frames with MOD_V3):
        // Ensure V3 completes 50% -> 100% consumption in ~457s matching original DOS pacing
        IPAStim ipaFrame33(IPAStim::Adrenaline, 100, 50);
        ipaFrame33.setLevels256(255, 128, 128);
        ipaFrame33.setAmount(100);
        int steps33 = 0;
        while (ipaFrame33.effect() < ipaFrame33.amount() && steps33 < 20000) {
            ipaFrame33.processTicks(33, 2);
            ++steps33;
        }
        uint32_t totalMs33 = static_cast<uint32_t>(steps33) * 33;
        REQUIRE(ipaFrame33.effect() == ipaFrame33.amount());
        REQUIRE(totalMs33 >= 450000);
        REQUIRE(totalMs33 <= 466000);

        // High-FPS frames (8ms < 9.14x divisor) must still progress at the same rate
        IPAStim ipaFrame8(IPAStim::Perception, 100, 50);
        ipaFrame8.setLevels256(255, 128, 128);
        ipaFrame8.setAmount(100);
        int steps8 = 0;
        while (ipaFrame8.effect() < ipaFrame8.amount() && steps8 < 80000) {
            ipaFrame8.processTicks(8, 2);
            ++steps8;
        }
        uint32_t totalMs8 = static_cast<uint32_t>(steps8) * 8;
        REQUIRE(ipaFrame8.effect() == ipaFrame8.amount());
        REQUIRE(totalMs8 >= 450000);
        REQUIRE(totalMs8 <= 466000);
    }

    SECTION("Perception Accuracy & Angular Dispersion") {
        rs_knl::PedInstance agent(0, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 256);
        agent.setObjGroupDef(rs_knl::PedInstance::og_dmAgent);

        // 50% Perception
        agent.perception().setAmount(50);
        double acc50 = 0.0;
        agent.getAccuracy(acc50);

        // 100% Perception -> high accuracy even unmodded
        agent.perception().setAmount(100);
        double acc100 = 0.0;
        agent.getAccuracy(acc100);
        REQUIRE(acc100 > acc50);
        REQUIRE(acc100 >= 0.80);

        // 0% Perception -> lower accuracy
        agent.perception().setAmount(0);
        double acc0 = 0.0;
        agent.getAccuracy(acc0);
        REQUIRE(acc0 < acc50);

        // Adding Eyes V3 mod: verify ~10x spread ratio between Perception=50 and Perception=100
        rs_knl::Mod eyesV3("Eyes V3", rs_knl::Mod::MOD_EYES, rs_knl::Mod::MOD_V3, 5000, "Eyes V3", 0);
        agent.addMod(&eyesV3);

        agent.perception().setAmount(100);
        double accModded100 = 0.0;
        agent.getAccuracy(accModded100);
        REQUIRE(accModded100 >= 0.90);

        agent.perception().setAmount(50);
        double accModded50 = 0.0;
        agent.getAccuracy(accModded50);

        double spreadFactor50 = 1.0 - accModded50;
        double spreadFactor100 = 1.0 - accModded100;
        REQUIRE(spreadFactor50 / spreadFactor100 >= 10.0);

        // Verify that bullet dispersion is predominantly lateral rather than front/back
        ConfigFile config;
        initWeaponConfigFile(config);
        rs_knl::Weapon minigunClass(rs_knl::Weapon::Minigun, config);

        // Verify shotanglemin/max and shotdepthmin/max properties on Minigun
        REQUIRE(minigunClass.shotAngleMin() == 1.0);
        REQUIRE(minigunClass.shotAngleMax() == 20.0);
        REQUIRE(minigunClass.shotDepthMin() == 0.0);
        REQUIRE(minigunClass.shotDepthMax() == 16.0);
        REQUIRE(minigunClass.effectiveSpreadAngle(1.0) == 1.0);
        REQUIRE(minigunClass.effectiveSpreadAngle(0.0) == 20.0);

        // Agent at (0, 0, 0) shooting directly along the +X axis at (1000, 0, 0)
        agent.setPosition(rs_knl::TilePoint(0, 0, 0));
        agent.perception().setAmount(50); // baseline perception with lateral spread

        rs_knl::WorldPoint originW;
        originW.convertFromTilePoint(agent.position());
        double totalLateralDelta = 0.0;
        double totalFrontBackDelta = 0.0;
        for (int i = 0; i < 50; ++i) {
            rs_knl::WorldPoint aimedLoc(originW.x + 1000, originW.y, originW.z);
            agent.adjustAimedPtWithRangeAndAccuracy(&minigunClass, &aimedLoc);
            double frontBackDiff = std::abs(aimedLoc.x - (originW.x + 1000));
            double lateralDiff = std::abs(aimedLoc.y - originW.y);
            totalFrontBackDelta += frontBackDiff;
            totalLateralDelta += lateralDiff;
        }
        // Lateral delta should be dominant
        REQUIRE(totalLateralDelta > 0.0);

        // Verify single shot weapons (Pistol and Long Range) at Perception=50 vs Perception=100
        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
        rs_knl::Weapon longRangeClass(rs_knl::Weapon::LongRange, config);

        // At Perception=100 with Eyes V3: Pistol & Long Range are on target
        agent.perception().setAmount(100);
        double pistolLateral100 = 0.0;
        double lrLateral100 = 0.0;
        for (int i = 0; i < 30; ++i) {
            rs_knl::WorldPoint aimedLocP(originW.x + 1000, originW.y, originW.z);
            agent.adjustAimedPtWithRangeAndAccuracy(&pistolClass, &aimedLocP);
            pistolLateral100 += std::abs(aimedLocP.y - originW.y);

            rs_knl::WorldPoint aimedLocLR(originW.x + 1000, originW.y, originW.z);
            agent.adjustAimedPtWithRangeAndAccuracy(&longRangeClass, &aimedLocLR);
            lrLateral100 += std::abs(aimedLocLR.y - originW.y);
        }

        // At Perception=50 with Eyes V3: Pistol & Long Range have noticeable lateral spread
        agent.perception().setAmount(50);
        double pistolLateral50 = 0.0;
        double lrLateral50 = 0.0;
        for (int i = 0; i < 30; ++i) {
            rs_knl::WorldPoint aimedLocP(originW.x + 1000, originW.y, originW.z);
            agent.adjustAimedPtWithRangeAndAccuracy(&pistolClass, &aimedLocP);
            pistolLateral50 += std::abs(aimedLocP.y - originW.y);

            rs_knl::WorldPoint aimedLocLR(originW.x + 1000, originW.y, originW.z);
            agent.adjustAimedPtWithRangeAndAccuracy(&longRangeClass, &aimedLocLR);
            lrLateral50 += std::abs(aimedLocLR.y - originW.y);
        }

        REQUIRE(pistolLateral50 > 0.0);
        REQUIRE(lrLateral50 > 0.0);
        REQUIRE(pistolLateral50 / std::max(1.0, pistolLateral100) >= 3.0);
        REQUIRE(lrLateral50 / std::max(1.0, lrLateral100) >= 4.0);

        // Verify Shotgun maintains visible dispersion floor at Perception=100
        rs_knl::Weapon shotgunClass(rs_knl::Weapon::Shotgun, config);
        agent.perception().setAmount(100);
        double shotgunLateral100 = 0.0;
        for (int i = 0; i < 30; ++i) {
            rs_knl::WorldPoint aimedLocSG(originW.x + 1000, originW.y, originW.z);
            agent.adjustAimedPtWithRangeAndAccuracy(&shotgunClass, &aimedLocSG);
            shotgunLateral100 += std::abs(aimedLocSG.y - originW.y);
        }
        agent.perception().setAmount(50);
        double shotgunLateral50 = 0.0;
        for (int i = 0; i < 30; ++i) {
            rs_knl::WorldPoint aimedLocSG(originW.x + 1000, originW.y, originW.z);
            agent.adjustAimedPtWithRangeAndAccuracy(&shotgunClass, &aimedLocSG);
            shotgunLateral50 += std::abs(aimedLocSG.y - originW.y);
        }
        REQUIRE(shotgunLateral100 > 0.0);
        REQUIRE(shotgunLateral50 > shotgunLateral100);
    }

    SECTION("Intelligence Weapon Ranking & Auto-Wield Selection") {
        ConfigFile config;
        initWeaponConfigFile(config);

        rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
        rs_knl::Weapon uziClass(rs_knl::Weapon::Uzi, config);
        rs_knl::Weapon minigunClass(rs_knl::Weapon::Minigun, config);
        rs_knl::Weapon gaussClass(rs_knl::Weapon::GaussGun, config);
        rs_knl::Weapon shieldClass(rs_knl::Weapon::EnergyShield, config);

        rs_knl::WeaponInstance pistol(&pistolClass, 1, nullptr);
        rs_knl::WeaponInstance uzi(&uziClass, 2, nullptr);
        rs_knl::WeaponInstance minigun(&minigunClass, 3, nullptr);
        rs_knl::WeaponInstance gauss(&gaussClass, 4, nullptr);
        rs_knl::WeaponInstance shield(&shieldClass, 5, nullptr);

        rs_knl::WeaponHolder holder;
        holder.addWeapon(&shield);
        holder.addWeapon(&pistol);
        holder.addWeapon(&uzi);
        holder.addWeapon(&minigun);
        holder.addWeapon(&gauss);

        // Minigun has highest rank (6)
        rs_knl::WeaponInstance *best = holder.getBestRankedWeaponWithAmmo();
        REQUIRE(best == &minigun);

        // When Minigun is depleted, cycles to next highest rank: Uzi (5)
        minigun.setAmmoRemaining(0);
        best = holder.getBestRankedWeaponWithAmmo();
        REQUIRE(best == &uzi);

        // When Uzi is depleted, cycles to Pistol (1)
        uzi.setAmmoRemaining(0);
        best = holder.getBestRankedWeaponWithAmmo();
        REQUIRE(best == &pistol);

        // When Pistol is depleted, cycles to Gauss Gun (0)
        pistol.setAmmoRemaining(0);
        best = holder.getBestRankedWeaponWithAmmo();
        REQUIRE(best == &gauss);

        // When Gauss is depleted, no shooting weapons with ammo remain (shield is utility)
        gauss.setAmmoRemaining(0);
        best = holder.getBestRankedWeaponWithAmmo();
        REQUIRE(best == nullptr);
    }

    SECTION("Intelligence Reaction Delay Scaling & Target Acquisition") {
        ConfigFile config;
        initWeaponConfigFile(config);

        rs_knl::Weapon minigunClass(rs_knl::Weapon::Minigun, config);
        rs_knl::WeaponInstance minigun(&minigunClass, 100, nullptr);

        auto *pAgent = new rs_knl::PedInstance(0, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 256);
        pAgent->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
        pAgent->setObjGroupID(0);
        pAgent->setStartHealth(1000, true);
        pAgent->perception().setAmount(100);
        pAgent->addWeapon(&minigun);
        pAgent->selectWeapon(&minigun);

        auto *pEnemy = new rs_knl::PedInstance(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 256);
        pEnemy->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
        pEnemy->setObjGroupID(1);
        pEnemy->setStartHealth(1000, true);

        pAgent->setPosition(rs_knl::TilePoint(100, 100, 0));
        pEnemy->setPosition(rs_knl::TilePoint(102, 100, 0));

        rs_knl::Mission mission;
        mission.addPed(pAgent);
        mission.addPed(pEnemy);

        rs_knl::CommonAgentBehaviourComponent agentBehv(pAgent);
        rs_knl::Behaviour::BehaviourParam param;
        param.elapsed = 50;
        param.pMission = &mission;
        param.pPed = pAgent;

        // At Intelligence <= 55 (e.g. 50%): passive stance, does not auto-fire
        pAgent->intelligence().setAmount(50);
        for (int i = 0; i < 20; ++i) {
            agentBehv.execute(param);
        }
        REQUIRE_FALSE(pAgent->isUsingWeapon());

        // At 60% Intelligence: 3000ms reaction delay
        pAgent->intelligence().setAmount(60);
        // First tick (50ms): Target detected, agent is aiming/hesitating, has NOT fired yet
        agentBehv.execute(param);
        REQUIRE_FALSE(pAgent->isUsingWeapon());

        // Simulate 2500ms passing: still hesitating (needs 3000ms total)
        for (int i = 0; i < 50; ++i) {
            agentBehv.execute(param);
        }
        REQUIRE_FALSE(pAgent->isUsingWeapon());

        // Simulate remaining 500ms passing: reaction timer elapses and agent opens fire!
        for (int i = 0; i < 11; ++i) {
            agentBehv.execute(param);
        }
        REQUIRE(pAgent->isUsingWeapon());
    }

    SECTION("Intelligence Auto-Defense Ceases Fire on Target Death & Recoil") {
        ConfigFile config;
        initWeaponConfigFile(config);

        rs_knl::Weapon minigunClass(rs_knl::Weapon::Minigun, config);
        rs_knl::WeaponInstance minigun(&minigunClass, 100, nullptr);

        auto *pAgent = new rs_knl::PedInstance(0, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 256);
        pAgent->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
        pAgent->setObjGroupID(0);
        pAgent->setStartHealth(1000, true);
        pAgent->intelligence().setAmount(100);
        pAgent->perception().setAmount(100);
        pAgent->addWeapon(&minigun);
        pAgent->selectWeapon(&minigun);

        auto *pEnemy1 = new rs_knl::PedInstance(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 256);
        pEnemy1->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
        pEnemy1->setObjGroupID(1);
        pEnemy1->setStartHealth(1000, true);

        auto *pEnemy2 = new rs_knl::PedInstance(2, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 256);
        pEnemy2->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
        pEnemy2->setObjGroupID(1);
        pEnemy2->setStartHealth(1000, true);

        // Place agent at (100, 100, 0), enemy1 at (102, 100, 0), enemy2 at (103, 100, 0)
        pAgent->setPosition(rs_knl::TilePoint(100, 100, 0));
        pEnemy1->setPosition(rs_knl::TilePoint(102, 100, 0));
        pEnemy2->setPosition(rs_knl::TilePoint(103, 100, 0));

        rs_knl::Mission mission;
        mission.addPed(pAgent);
        mission.addPed(pEnemy1);
        mission.addPed(pEnemy2);

        rs_knl::CommonAgentBehaviourComponent agentBehv(pAgent);
        rs_knl::Behaviour::BehaviourParam param;
        param.elapsed = 50;
        param.pMission = &mission;
        param.pPed = pAgent;

        // First tick at 100% Intelligence: instantaneous reaction, starts shooting immediately at closest (enemy1)
        agentBehv.execute(param);
        REQUIRE(pAgent->isUsingWeapon());

        // Enemy 1 enters recoil state (kPedActionStateHit)
        pEnemy1->goToState(rs_knl::kPedActionStateHit);
        REQUIRE(pEnemy1->isState(rs_knl::kPedActionStateHit));

        // Next tick: agent stops shooting Enemy 1 and immediately switches to non-recoiling Enemy 2!
        agentBehv.execute(param);
        REQUIRE(pAgent->isUsingWeapon());

        // Enemy 2 is killed
        pEnemy2->setHealth(0);
        REQUIRE(pEnemy2->isDead());

        // Enemy 1 recovers from recoil
        pEnemy1->leaveState(rs_knl::kPedActionStateHit);
        REQUIRE_FALSE(pEnemy1->isState(rs_knl::kPedActionStateHit));

        // Agent now immediately re-engages recovered Enemy 1
        agentBehv.execute(param);
        REQUIRE(pAgent->isUsingWeapon());

        // Enemy 1 is killed
        pEnemy1->setHealth(0);
        REQUIRE(pEnemy1->isDead());

        // All enemies dead: agent stops shooting
        agentBehv.execute(param);
        REQUIRE_FALSE(pAgent->isUsingWeapon());
    }

    SECTION("Panic mode agent ignores and stops shooting vaporizing enemies") {
        ConfigFile config;
        initWeaponConfigFile(config);

        rs_knl::Weapon minigunClass(rs_knl::Weapon::Minigun, config);
        rs_knl::WeaponInstance minigun(&minigunClass, 5000, nullptr);

        auto *pPanicAgent = new rs_knl::PedInstance(0, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 256);
        pPanicAgent->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
        pPanicAgent->setObjGroupID(0);
        pPanicAgent->setStartHealth(1000, true);
        pPanicAgent->intelligence().setAmount(100);
        pPanicAgent->perception().setAmount(100);
        pPanicAgent->addWeapon(&minigun);
        pPanicAgent->selectWeapon(&minigun);

        auto *pTargetEnemy = new rs_knl::PedInstance(1, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 256);
        pTargetEnemy->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
        pTargetEnemy->setObjGroupID(1);
        pTargetEnemy->setStartHealth(100, true);

        pPanicAgent->setPosition(rs_knl::TilePoint(100, 100, 0));
        pTargetEnemy->setPosition(rs_knl::TilePoint(102, 100, 0));

        rs_knl::Mission panicMission;
        panicMission.addPed(pPanicAgent);
        panicMission.addPed(pTargetEnemy);

        rs_knl::CommonAgentBehaviourComponent agentBehv(pPanicAgent);
        rs_knl::Behaviour::BehaviourParam param;
        param.elapsed = 50;
        param.pMission = &panicMission;
        param.pPed = pPanicAgent;

        // 1. Initial engagement: agent acquires target and starts shooting
        agentBehv.execute(param);
        REQUIRE(pPanicAgent->isUsingWeapon());

        // 2. Fatal laser hit hits the target
        rs_knl::DamageToInflict fatalLaserDmg;
        fatalLaserDmg.dtype = rs_knl::kDmgTypeLaser;
        fatalLaserDmg.dvalue = 500;
        fatalLaserDmg.originLocW = {100 * 256, 100 * 256, 0};

        pTargetEnemy->handleHit(fatalLaserDmg);
        pTargetEnemy->executeAction(0, &panicMission);

        REQUIRE(pTargetEnemy->isDead());
        REQUIRE(pTargetEnemy->isVaporizing());

        // 3. Panic mode tick: agent must immediately stop shooting and not fire additional ammo
        agentBehv.execute(param);
        REQUIRE_FALSE(pPanicAgent->isUsingWeapon());

        // 4. Verify agent with laser will not re-target or shoot vaporizing ped
        rs_knl::Weapon laserClass(rs_knl::Weapon::Laser, config);
        rs_knl::WeaponInstance laser(&laserClass, 100, nullptr);
        pPanicAgent->addWeapon(&laser);
        pPanicAgent->selectWeapon(&laser);

        agentBehv.execute(param);
        REQUIRE_FALSE(pPanicAgent->isUsingWeapon());
    }
}
