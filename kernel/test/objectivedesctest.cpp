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

#include "rs-engine/appcontext.h"
#include "rs-engine/events/event.h"
#include "rs-kernel/model/objectivedesc.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/squad.h"
#include "rs-kernel/ia/actions.h"
#include "rs-kernel/ia/behaviour.h"

#include "testcase.h"

class ObjectiveDescImpl : public rs_knl::ObjectiveDesc {
public:
    ObjectiveDescImpl() {
        handleStartCalled_ = false;
        objectiveEndedCalled_ = false;
    }

    bool handleStartCalled() { return handleStartCalled_; }
    bool objectiveEndedCalled() { return objectiveEndedCalled_; }
    void onObjectiveEndedEvent(rs_knl::ObjectiveEndedEvent *pEvt) { objectiveEndedCalled_ = true; }
protected:
    void handleStart() override {
        handleStartCalled_ = true;
    }

    void doEvaluate(rs_knl::Mission *pMission) override { endObjective(true); }

private:
    bool handleStartCalled_;
    bool objectiveEndedCalled_;
};

TEST_CASE( "ObjectiveDesc", "[kernel][objectivedesc]" ) {

    rs_eng::AppContext appCtx;
    rs_knl::PedInstance agent1(3, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    rs_knl::PedInstance agent2(4, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    rs_knl::PedInstance agent3(5, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    agent1.setHealth(10);
    agent2.setHealth(10);
    agent3.setHealth(10);
    LevelData::MapInfos infos;
    rs_knl::Mission mission;
    mission.getSquad()->setMember(0, &agent1);
    mission.getSquad()->setMember(1, &agent2);
    mission.getSquad()->setMember(2, &agent3);

    SECTION( "ObjectiveDesc basic") {
        ObjectiveDescImpl cut;

        SECTION( "Should not evaluate when not started") {
            REQUIRE( cut.status == rs_knl::kNotStarted);

            cut.evaluate(&mission);
            REQUIRE_FALSE( cut.isTerminated() );
        }

        SECTION( "Should evaluate when started") {
            EventManager::listen<rs_knl::ObjectiveEndedEvent>(&cut, &ObjectiveDescImpl::onObjectiveEndedEvent);

            cut.start();
            REQUIRE( cut.status == rs_knl::kStarted);
            REQUIRE( cut.handleStartCalled());

            cut.evaluate(&mission);
            REQUIRE( cut.isTerminated() );
            REQUIRE( cut.status == rs_knl::kCompleted );
            REQUIRE( cut.objectiveEndedCalled() );
        }

        SECTION( "Should not end in failed with forceEnd if not started") {
            cut.forceEnd();
            
            REQUIRE( cut.status == rs_knl::kNotStarted );
        }

        SECTION( "Should end in failed with forceEnd") {
            cut.start();
            cut.forceEnd();

            REQUIRE( cut.status == rs_knl::kFailed );
        }
        
        SECTION( "Should end in failed if all agents have died") {
            agent1.setHealth(0);
            agent2.setHealth(0);
            agent3.setHealth(0);

            cut.start();
            cut.evaluate(&mission);

            REQUIRE( cut.status == rs_knl::kFailed );
        }
    }

    SECTION( "ObjEvacuate") {
        rs_knl::PedInstance ped1(1, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
        rs_knl::PedInstance ped2(2, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
        std::vector<rs_knl::PedInstance *> pedsToEvacuate = {&ped1, &ped2};
        rs_knl::ObjEvacuate cut(5200, 6400, 128, pedsToEvacuate);

        ped1.setHealth(10);
        ped2.setHealth(10);
        // By default, all peds and agents are in the zone
        ped1.setPosition(20, 25, 1);
        ped2.setPosition(20, 25, 1);
        agent1.setPosition(20, 25, 1);
        agent2.setPosition(21, 24, 1);
        agent3.setPosition(20, 24, 1, 15, 110);

        SECTION( "Should fail due to ped to evacuate is dead") {
            // Ped1 is dead
            ped1.setHealth(0);
            cut.start();
            cut.evaluate(&mission);
            REQUIRE( cut.status == rs_knl::kFailed );
        }

        SECTION( "Should not be terminated due to ped not in zone") {
            // ped 2 is out of zone
            ped2.setPosition(30, 24, 1);
            cut.start();
            cut.evaluate(&mission);
            REQUIRE( cut.status == rs_knl::kStarted );
        }

        SECTION( "Should not be terminated due to agent not in zone") {
            // agent2 is out of the zone
            agent2.setPosition(31, 24, 1);
            cut.start();
            cut.evaluate(&mission);
            REQUIRE( cut.status == rs_knl::kStarted );
        }

        SECTION( "Should be completed") {
            cut.start();
            cut.evaluate(&mission);
            REQUIRE( cut.status == rs_knl::kCompleted );
        }
    }

    SECTION("ObjAssassinate laser vaporization lifecycle") {
        auto *targetPed = new rs_knl::PedInstance(10, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 50);
        targetPed->setStartHealth(50, true);
        mission.addPed(targetPed);

        rs_knl::ObjAssassinate assassinateObj(targetPed);
        assassinateObj.start();
        REQUIRE(assassinateObj.status == rs_knl::kStarted);

        // Inflict lethal laser damage
        rs_knl::DamageToInflict fatalLaserDmg;
        fatalLaserDmg.dtype = rs_knl::kDmgTypeLaser;
        fatalLaserDmg.dvalue = 100;
        fatalLaserDmg.originLocW = {100, 100, 0};

        targetPed->handleHit(fatalLaserDmg);
        targetPed->executeAction(0, &mission);

        // 1. Immediately on hit: health is 0, dead, dying, unalive, vaporizing
        REQUIRE(targetPed->health() <= 0);
        REQUIRE(targetPed->isDead());
        REQUIRE_FALSE(targetPed->isAlive());
        REQUIRE(targetPed->isDying());
        REQUIRE(targetPed->isVaporizing());

        // 2. Target ped is untargetable: isDead(), !isAlive(), and isDying() are all true
        REQUIRE((!targetPed->isAlive() || targetPed->isDead() || targetPed->isDying()));

        // 3. Objective must NOT complete while target is actively vaporizing (avoids audio overlap)
        assassinateObj.evaluate(&mission);
        REQUIRE_FALSE(assassinateObj.isTerminated());
        REQUIRE(assassinateObj.status == rs_knl::kStarted);

        // 4. Once vaporization animation completes (>1000ms timer / animation finish)
        targetPed->executeAction(1050, &mission);
        REQUIRE_FALSE(targetPed->isVaporizing());
        REQUIRE(targetPed->isDead());

        // 5. Objective now completes successfully
        assassinateObj.evaluate(&mission);
        REQUIRE(assassinateObj.isTerminated());
        REQUIRE(assassinateObj.status == rs_knl::kCompleted);
    }

    SECTION("ObjEliminate laser vaporization lifecycle") {
        rs_knl::Mission elimMission;
        auto *squadAgent = new rs_knl::PedInstance(0, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 50);
        squadAgent->setHealth(100);
        elimMission.getSquad()->setMember(0, squadAgent);
        elimMission.addPed(squadAgent);

        auto *enemyAgent = new rs_knl::PedInstance(20, nullptr, rs_knl::PedInstance::kPedTypeAgent, false, 50);
        enemyAgent->setObjGroupDef(rs_knl::PedInstance::og_dmAgent);
        enemyAgent->setObjGroupID(2);
        enemyAgent->setStartHealth(50, true);
        elimMission.addPed(enemyAgent);

        rs_knl::ObjEliminate eliminateObj(rs_knl::PedInstance::og_dmAgent);
        eliminateObj.start();
        REQUIRE(eliminateObj.status == rs_knl::kStarted);

        // Enemy is alive -> objective not completed
        eliminateObj.evaluate(&elimMission);
        REQUIRE_FALSE(eliminateObj.isTerminated());

        // Laser hit on enemy
        rs_knl::DamageToInflict fatalLaserDmg;
        fatalLaserDmg.dtype = rs_knl::kDmgTypeLaser;
        fatalLaserDmg.dvalue = 100;
        fatalLaserDmg.originLocW = {100, 100, 0};

        enemyAgent->handleHit(fatalLaserDmg);
        enemyAgent->executeAction(0, &elimMission);

        REQUIRE(enemyAgent->isDead());
        REQUIRE(enemyAgent->isVaporizing());

        // While vaporizing, eliminate objective does NOT complete yet
        eliminateObj.evaluate(&elimMission);
        REQUIRE_FALSE(eliminateObj.isTerminated());

        // Once vaporization finishes
        enemyAgent->executeAction(1050, &elimMission);
        REQUIRE_FALSE(enemyAgent->isVaporizing());

        // Now objective completes
        eliminateObj.evaluate(&elimMission);
        REQUIRE(eliminateObj.isTerminated());
        REQUIRE(eliminateObj.status == rs_knl::kCompleted);
    }
}