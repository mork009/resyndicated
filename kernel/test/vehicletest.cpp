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
#include "rs-engine/system/system.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/animationloader.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/train.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/mgr/vehiclemanager.h"
#include "rs-kernel/mgr/modmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/agentmanager.h"
#include "testcase.h"

TEST_CASE( "Vehicle", "[kernel][vehicle]" ) {
    rs_knl::GenericCar cut(1, rs_knl::Vehicle::kVehicleTypeRegularCar, nullptr, 100);

    SECTION( "Should set animations with right offset") {
        // Test is not complete
        cut.setAnimations(2);
        REQUIRE( cut.regularAnimation() == 0 );
        REQUIRE( cut.burntAnimation() == 2 );
    }
}

TEST_CASE( "Train Animations and Non-Combustion", "[kernel][vehicle][train]" ) {
    rs_knl::TrainHead head(1, rs_knl::Vehicle::kVehicleTypeTrainHead, nullptr, 100, true, 1280);
    rs_knl::TrainBody body(2, rs_knl::Vehicle::kVehicleTypeTrainBody, nullptr, 80, true);

    REQUIRE_FALSE( head.isCar() );
    REQUIRE_FALSE( body.isCar() );

    head.setAnimations(416);
    body.setAnimations(428);

    // TrainHead authentic animations: regular (416), burning (420), burnt wreckage (424)
    REQUIRE( head.regularAnimation() != head.burningAnimation() );
    REQUIRE( head.regularAnimation() != head.burntAnimation() );
    REQUIRE( head.burningAnimation() != head.burntAnimation() );

    // TrainBody authentic animations: regular (428), burning (430), burnt wreckage (432)
    REQUIRE( body.regularAnimation() != body.burningAnimation() );
    REQUIRE( body.regularAnimation() != body.burntAnimation() );
    REQUIRE( body.burningAnimation() != body.burntAnimation() );
}

/*!
 * Test subclass that removes external dependencies from doMove():
 * - overrides checkForBlockers() so it never needs g_missionCtrl
 * - exposes addPathPoint() to push directly into the protected dest_path_
 */
class TestGenericCar : public rs_knl::GenericCar {
public:
    explicit TestGenericCar(int maxSpeed = 100)
        : rs_knl::GenericCar(1, rs_knl::Vehicle::kVehicleTypeRegularCar, nullptr, maxSpeed) {}

    void addPathPoint(const rs_knl::TilePoint &pt) {
        dest_path_.push_back(pt);
    }

    bool checkForBlockers([[maybe_unused]] bool checkForCrossings) override {
        return false;
    }

    void runAddIntraTileOffsetsToPath() {
        addIntraTileOffsetsToPath();
    }

    using rs_knl::GenericCar::isBlocked;
    using rs_knl::Vehicle::doUpdateState;
};

TEST_CASE( "GenericCar::doMove", "[kernel][vehicle]" ) {

    SECTION( "returns false when path is empty" ) {
        TestGenericCar cut;
        REQUIRE( !cut.doMove(100) );
        REQUIRE( !cut.hasDestination() );
    }

    SECTION( "snaps to waypoint when already within arrival threshold" ) {
        // diffx = 6, diffy = 0 — both below the 16-unit threshold,
        // so the movement branch is skipped and the car snaps directly.
        TestGenericCar cut;
        cut.setPosition(5, 5, 2, 128, 128, 0);
        cut.setSpeedToMax();
        cut.addPathPoint(rs_knl::TilePoint(5, 5, 2, 134, 128, 0));

        REQUIRE( cut.doMove(100) );
        REQUIRE( !cut.hasDestination() );
        REQUIRE( !cut.isMoving() );
        REQUIRE( cut.tileX() == 5 );
        REQUIRE( cut.tileY() == 5 );
        REQUIRE( cut.offX() == 134 );
        REQUIRE( cut.offY() == 128 );
    }

    SECTION( "moves toward distant waypoint within one tick" ) {
        // diffx = 256, speed = 100 units/s, elapsed = 100 ms
        // dx = (256 * 100 * 100 / 256) / 1000 = 10 units moved
        TestGenericCar cut(100);
        cut.setPosition(5, 5, 2, 128, 128, 0);
        cut.setSpeedToMax();
        cut.addPathPoint(rs_knl::TilePoint(6, 5, 2, 128, 128, 0));

        REQUIRE( cut.doMove(100) );
        REQUIRE( cut.hasDestination() );   // waypoint not yet reached
        REQUIRE( cut.tileX() == 5 );       // still on the same tile
        REQUIRE( cut.offX() == 138 );      // 128 + 10
    }

    SECTION( "reaches waypoint and clears path when elapsed time is sufficient" ) {
        // diffx = 256, speed = 100, full travel time = 2560 ms
        // elapsed 10000 ms covers it entirely: car lands exactly on the waypoint,
        // triggers the arrival check, snaps, pops the path, and stops.
        TestGenericCar cut(100);
        cut.setPosition(5, 5, 2, 128, 128, 0);
        cut.setSpeedToMax();
        cut.addPathPoint(rs_knl::TilePoint(6, 5, 2, 128, 128, 0));

        REQUIRE( cut.doMove(10000) );
        REQUIRE( !cut.hasDestination() );
        REQUIRE( !cut.isMoving() );
        REQUIRE( cut.tileX() == 6 );
        REQUIRE( cut.offX() == 128 );
    }

    SECTION( "traverses multiple waypoints when elapsed time covers the whole path" ) {
        TestGenericCar cut(100);
        cut.setPosition(5, 5, 2, 128, 128, 0);
        cut.setSpeedToMax();
        cut.addPathPoint(rs_knl::TilePoint(6, 5, 2, 128, 128, 0));
        cut.addPathPoint(rs_knl::TilePoint(7, 5, 2, 128, 128, 0));

        REQUIRE( cut.doMove(100000) );
        REQUIRE( !cut.hasDestination() );
        REQUIRE( !cut.isMoving() );
        REQUIRE( cut.tileX() == 7 );
        REQUIRE( cut.offX() == 128 );
    }

    SECTION( "car yielding to ped blocker in driving path and ignoring ped on sidewalk" ) {
        TestGenericCar car(100);
        car.setPosition(5, 5, 2, 128, 128, 0);
        car.addPathPoint(rs_knl::TilePoint(6, 5, 2, 128, 128, 0));

        rs_knl::PedInstance ped(2, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
        ped.setStartHealth(10);
        ped.resetHealth();

        // Ped in center of road lane (128, 128) -> is in driving path
        ped.setPosition(6, 5, 2, 128, 128, 0);
        REQUIRE( car.isPedInDrivingPath(&ped, rs_knl::TilePoint(6, 5, 2, 128, 128, 0)) );

        car.blockPathWith(&ped, false, 6, 5, 2);
        REQUIRE( car.isBlocked() );

        // Ped on sidewalk (128, 280) -> lateral offset = 152 >= 144 -> not in driving path
        ped.setPosition(6, 5, 2, 128, 280, 0);
        REQUIRE_FALSE( car.isPedInDrivingPath(&ped, rs_knl::TilePoint(6, 5, 2, 128, 128, 0)) );
        REQUIRE_FALSE( car.isBlocked() );
    }

    SECTION( "smooth acceleration and cornering kinematics" ) {
        TestGenericCar car(1024);
        car.setPosition(5, 5, 2, 128, 32, 0); // Eastbound in right lane
        car.addPathPoint(rs_knl::TilePoint(6, 5, 2, 128, 32, 0));
        car.addPathPoint(rs_knl::TilePoint(6, 6, 2, 200, 128, 0)); // Turn South
        car.runAddIntraTileOffsetsToPath();

        // Should expand 90-degree turn into rounded curve points
        REQUIRE( car.destPath().size() > 2 );

        // Step through turn: starts at initial speed and ramps up/down smoothly
        REQUIRE( car.doMove(50) );
        REQUIRE( car.isMoving() );
    }

    SECTION( "90-degree cornering transitions sprite direction cleanly without flickering" ) {
        struct TurnCase {
            int inDx, inDy;
            int outDx, outDy;
            int startOx, startOy;
            int endOx, endOy;
        };
        const TurnCase turns[] = {
            { 1,  0,  0,  1, 128,  32, 200, 128}, // Right/Down (+X) -> Left/Down (+Y)
            { 0,  1,  1,  0, 200, 128, 128,  32}, // Left/Down (+Y)  -> Right/Down (+X)
            {-1,  0,  0, -1, 128, 200,  32, 128}, // Left/Up (-X)    -> Right/Up (-Y)
            { 0, -1, -1,  0,  32, 128, 128, 200}, // Right/Up (-Y)   -> Left/Up (-X)
            { 1,  0,  0, -1, 128,  32,  32, 128}, // +X -> -Y
            {-1,  0,  0,  1, 128, 200, 200, 128}, // -X -> +Y
            { 0,  1, -1,  0, 200, 128, 128, 200}, // +Y -> -X
            { 0, -1,  1,  0,  32, 128, 128,  32}, // -Y -> +X
        };
        for (const auto &tc : turns) {
            for (uint32_t dt : {16u, 33u, 46u}) {
                TestGenericCar car(480);
                car.setPosition(5 - tc.inDx, 5 - tc.inDy, 2, tc.startOx, tc.startOy, 0);
                car.addPathPoint(rs_knl::TilePoint(5, 5, 2, 128, 128, 0));
                car.addPathPoint(rs_knl::TilePoint(5 + tc.outDx, 5 + tc.outDy, 2, tc.endOx, tc.endOy, 0));
                car.runAddIntraTileOffsetsToPath();
                car.setSpeed(rs_knl::GenericCar::kMinCarMovingSpeed);

                std::vector<uint8_t> discSeq;
                std::vector<uint16_t> animSeq;
                for (int tick = 0; tick < 250 && !car.destPath().empty(); ++tick) {
                    car.doMove(dt);
                    uint8_t d4 = car.getDiscreteDirection(4);
                    uint16_t animOff = car.getDrawAnimation();
                    if (discSeq.empty() || discSeq.back() != d4) {
                        discSeq.push_back(d4);
                    }
                    if (animSeq.empty() || animSeq.back() != animOff) {
                        animSeq.push_back(animOff);
                    }
                }
                REQUIRE(car.destPath().empty());
                CHECK(discSeq.size() <= 2);
                CHECK(animSeq.size() <= 2);
            }
        }
    }
}

namespace {

void initMockAnimationManager(rs_eng::AnimationManager &animMgr) {
    rs_eng::GameSpriteFrameElement elem;
    elem.sprite = 0;
    elem.nextElement = 0;
    animMgr.addFrameElement(elem);
    for (size_t i = 0; i < 50; ++i) {
        rs_eng::GameSpriteFrame frame;
        frame.flags = 0x0100;
        frame.firstElement = 0;
        frame.nextFrame = i;
        animMgr.addFrame(frame);
        animMgr.addFramesAnimation(static_cast<uint16_t>(i));
    }
}

} // namespace

TEST_CASE( "GenericCar::handleHit and Destruction", "[kernel][vehicle]" ) {
    rs_eng::AnimationManager animMgr;
    initMockAnimationManager(animMgr);
    rs_knl::GenericCar car(1, rs_knl::Vehicle::kVehicleTypeRegularCar, nullptr, 100);
    car.setStartHealth(50);
    car.resetHealth();
    car.setAnimations(10);

    rs_knl::PedInstance passenger(2, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    passenger.setStartHealth(10);
    passenger.resetHealth();
    car.addPassenger(&passenger);
    REQUIRE( car.containsPed(&passenger) );

    rs_knl::DamageToInflict dmg;
    dmg.dtype = rs_knl::kDmgTypeBullet;
    dmg.dvalue = 50;
    dmg.ddir = -1;
    dmg.d_owner = nullptr;

    car.handleHit(dmg);

    // Car health reached 0
    REQUIRE( car.health() == 0 );
    REQUIRE( car.isDead() );
    // Passengers were ejected / dropped
    REQUIRE_FALSE( car.containsPed(&passenger) );
}

TEST_CASE( "PedInstance::CollisionDeath", "[kernel][ped]" ) {
    rs_eng::AnimationManager animMgr;
    initMockAnimationManager(animMgr);

    rs_knl::PedInstance ped(1, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    ped.setStartHealth(10);
    ped.resetHealth();
    ped.goToState(rs_knl::kPedActionStateStanding);

    rs_knl::DamageToInflict collisionDmg;
    collisionDmg.dtype = rs_knl::kDmgTypeCollision;
    collisionDmg.dvalue = 1024;
    collisionDmg.ddir = -1;
    collisionDmg.d_owner = nullptr;

    ped.handleHit(collisionDmg);
    REQUIRE( ped.currentAction() != nullptr );

    ped.executeAction(100, nullptr);
    REQUIRE( ped.isState(rs_knl::kPedActionStateHit) );
}

TEST_CASE( "Vehicle::checkRunOverPeds", "[kernel][vehicle]" ) {
    rs_eng::AnimationManager animMgr;
    initMockAnimationManager(animMgr);

    rs_knl::Mission mission;
    rs_knl::GenericCar car(1, rs_knl::Vehicle::kVehicleTypeRegularCar, nullptr, 100);
    car.setPosition(5, 5, 2, 128, 128, 0);
    car.setStartHealth(50);
    car.resetHealth();

    rs_knl::PedInstance *pPed = new rs_knl::PedInstance(2, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    pPed->setPosition(5, 5, 2, 130, 128, 0);
    pPed->setStartHealth(10);
    pPed->resetHealth();
    pPed->goToState(rs_knl::kPedActionStateStanding);
    mission.addPed(pPed);

    car.checkRunOverPeds(&mission);

    // Ped in center received lethal hit action from collision
    REQUIRE( pPed->currentAction() != nullptr );

    // Ped on sidewalk (128, 280) is NOT hit by passing car
    rs_knl::PedInstance *pSidewalkPed = new rs_knl::PedInstance(3, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    pSidewalkPed->setPosition(5, 5, 2, 128, 280, 0);
    pSidewalkPed->setStartHealth(10);
    pSidewalkPed->resetHealth();
    pSidewalkPed->goToState(rs_knl::kPedActionStateStanding);
    mission.addPed(pSidewalkPed);

    car.checkRunOverPeds(&mission);
    REQUIRE( pSidewalkPed->currentAction() == nullptr );
}

TEST_CASE( "GenericCar::doMove stationary vs moving collision", "[kernel][vehicle]" ) {
    rs_eng::AnimationManager animMgr;
    initMockAnimationManager(animMgr);

    rs_knl::Mission mission;

    TestGenericCar car(100);
    car.setPosition(5, 5, 2, 128, 128, 0);
    car.setStartHealth(50);
    car.resetHealth();

    rs_knl::PedInstance *pPed = new rs_knl::PedInstance(2, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    pPed->setPosition(5, 5, 2, 130, 128, 0);
    pPed->setStartHealth(10);
    pPed->resetHealth();
    pPed->goToState(rs_knl::kPedActionStateStanding);
    mission.addPed(pPed);

    // Car is stationary without destination: doMove returns false
    REQUIRE_FALSE( car.doMove(50) );

    // Only when car actually moves does checkRunOverPeds get called
    car.addPathPoint(rs_knl::TilePoint(6, 5, 2, 128, 128, 0));
    REQUIRE( car.doMove(50) );
    car.checkRunOverPeds(&mission);
    REQUIRE( pPed->currentAction() != nullptr );
}

TEST_CASE( "GenericCar::addPassenger NPC driver alights when player enters", "[kernel][vehicle]" ) {
    rs_knl::GenericCar car(1, rs_knl::Vehicle::kVehicleTypeRegularCar, nullptr, 100);
    car.setPosition(5, 5, 2, 128, 128, 0);

    // Initial civilian driver
    rs_knl::PedInstance civilian(2, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    car.addPassenger(&civilian);
    REQUIRE( car.getDriver() == &civilian );
    REQUIRE( car.containsPed(&civilian) );
    REQUIRE( !civilian.isDrawable() );

    // Player agent enters car
    rs_knl::PedInstance agent(3, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    car.addPassenger(&agent);

    // NPC driver alights and agent becomes new driver
    REQUIRE( car.getDriver() == &agent );
    REQUIRE( car.isDrivenByPlayer() );
    REQUIRE( car.containsPed(&agent) );
    REQUIRE_FALSE( car.containsPed(&civilian) );
    REQUIRE( civilian.isDrawable() );
}

TEST_CASE( "GenericCar player driven vehicle does not yield to peds", "[kernel][vehicle]" ) {
    TestGenericCar car(100);
    car.setPosition(5, 5, 2, 128, 128, 0);

    // Pedestrian in driving path
    rs_knl::PedInstance ped(2, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    ped.setStartHealth(10);
    ped.resetHealth();
    ped.setPosition(6, 5, 2, 128, 128, 0);

    car.addPathPoint(rs_knl::TilePoint(6, 5, 2, 128, 128, 0));

    // Block car with ped
    car.blockPathWith(&ped, false, 6, 5, 2);
    REQUIRE( car.isBlocked() );

    // Player agent takes driver seat
    rs_knl::PedInstance agent(3, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    car.addPassenger(&agent);

    // Car is driven by player -> does not yield to ped blocker
    REQUIRE_FALSE( car.isBlocked() );
}

TEST_CASE( "Pedestrian Car Avoidance", "[kernel][ped][vehicle]" ) {
    rs_eng::AnimationManager animMgr;
    initMockAnimationManager(animMgr);

    rs_knl::Mission mission;

    // Moving car at (5, 5, 2, 128, 128) driving East (+X) toward tile 6
    TestGenericCar *pCar = new TestGenericCar(100);
    pCar->setPosition(5, 5, 2, 128, 128, 0);
    pCar->setStartHealth(50);
    pCar->resetHealth();
    pCar->setSpeed(100);
    pCar->addPathPoint(rs_knl::TilePoint(6, 5, 2, 128, 128, 0));
    mission.addVehicle(pCar);

    // Pedestrian standing in front of car at (5, 5, 2, 220, 138) — ahead and slightly right of centerline
    rs_knl::PedInstance ped(2, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    ped.setStartHealth(10);
    ped.resetHealth();
    ped.setPosition(5, 5, 2, 220, 138, 0);
    ped.goToState(rs_knl::kPedActionStateStanding);

    int startY = ped.offY();

    // Avoidance check triggered
    bool avoided = ped.checkAndAvoidMovingVehicles(&mission, 100);
    REQUIRE( avoided );
    // Pedestrian nudged laterally away from car's trajectory
    REQUIRE( ped.offY() != startY );

    // Yielding/stopped car with destination ahead also triggers avoidance
    pCar->setSpeed(0);
    int beforeY = ped.offY();
    bool avoidedYielding = ped.checkAndAvoidMovingVehicles(&mission, 100);
    REQUIRE( avoidedYielding );
    REQUIRE( ped.offY() != beforeY );

    // Player agent standing in front of car avoids
    rs_knl::PedInstance agent(4, nullptr, rs_knl::PedInstance::kPedTypeAgent, true, 128);
    agent.setStartHealth(10);
    agent.resetHealth();
    agent.setPosition(5, 5, 2, 220, 138, 0);
    agent.goToState(rs_knl::kPedActionStateStanding);
    REQUIRE( agent.checkAndAvoidMovingVehicles(&mission, 100) );

    // Player agent actively walking does NOT get interrupted
    agent.setSpeed(128);
    agent.addActionWalk(rs_knl::TilePoint(6, 5, 2, 128, 128, 0), false);
    REQUIRE_FALSE( agent.checkAndAvoidMovingVehicles(&mission, 100) );

    // Pedestrian standing far away at (10, 10) does NOT trigger avoidance
    rs_knl::PedInstance farPed(3, nullptr, rs_knl::PedInstance::kPedTypeCivilian, false, 128);
    farPed.setPosition(10, 10, 2, 128, 128, 0);
    REQUIRE_FALSE( farPed.checkAndAvoidMovingVehicles(&mission, 100) );
}

TEST_CASE( "VehicleManager Configuration and Attributes", "[kernel][vehicle][config]" ) {
    rs_knl::VehicleManager::loadVehicleStats();

    SECTION( "Regular Car attributes" ) {
        const auto &st = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeRegularCar);
        REQUIRE( st.type == rs_knl::Vehicle::kVehicleTypeRegularCar );
        REQUIRE( st.health == 30 );
        REQUIRE( st.speed == 1024 );
        REQUIRE( st.acceleration == 1400 );
        REQUIRE( st.base_anim == 446 );
    }

    SECTION( "Small Armored attributes" ) {
        const auto &st = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeSmallArmored);
        REQUIRE( st.type == rs_knl::Vehicle::kVehicleTypeSmallArmored );
        REQUIRE( st.health == 10 );
        REQUIRE( st.speed == 768 );
        REQUIRE( st.acceleration == 1000 );
        REQUIRE( st.base_anim == 534 );
    }

    SECTION( "Large Armored attributes" ) {
        const auto &st = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeLargeArmored);
        REQUIRE( st.type == rs_knl::Vehicle::kVehicleTypeLargeArmored );
        REQUIRE( st.health == 600 );
        REQUIRE( st.speed == 640 );
        REQUIRE( st.acceleration == 800 );
        REQUIRE( st.base_anim == 400 );
    }

    SECTION( "Police Car attributes" ) {
        const auto &st = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypePolice);
        REQUIRE( st.type == rs_knl::Vehicle::kVehicleTypePolice );
        REQUIRE( st.health == 120 );
        REQUIRE( st.speed == 1280 );
        REQUIRE( st.acceleration == 1800 );
        REQUIRE( st.base_anim == 470 );
    }

    SECTION( "Ambulance attributes" ) {
        const auto &st = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeMedics);
        REQUIRE( st.type == rs_knl::Vehicle::kVehicleTypeMedics );
        REQUIRE( st.health == 115 );
        REQUIRE( st.speed == 1150 );
        REQUIRE( st.acceleration == 1400 );
        REQUIRE( st.base_anim == 494 );
    }

    SECTION( "Firetruck attributes" ) {
        const auto &st = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeFireFighter);
        REQUIRE( st.type == rs_knl::Vehicle::kVehicleTypeFireFighter );
        REQUIRE( st.health == 40 );
        REQUIRE( st.speed == 1024 );
        REQUIRE( st.acceleration == 1000 );
        REQUIRE( st.base_anim == 518 );
    }

    SECTION( "Train Locomotive (Lead) attributes" ) {
        const auto &st = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeTrainHead);
        REQUIRE( st.type == rs_knl::Vehicle::kVehicleTypeTrainHead );
        REQUIRE( st.health == 100 );
        REQUIRE( st.speed == 1280 );
        REQUIRE( st.acceleration == 1600 );
        REQUIRE( st.dwell_time == 10000 );
        REQUIRE( st.base_anim == 416 );
    }

    SECTION( "Train Carriage attributes" ) {
        const auto &st = rs_knl::VehicleManager::getStats(rs_knl::Vehicle::kVehicleTypeTrainBody);
        REQUIRE( st.type == rs_knl::Vehicle::kVehicleTypeTrainBody );
        REQUIRE( st.health == 80 );
        REQUIRE( st.dwell_time == 10000 );
        REQUIRE( st.base_anim == 428 );
    }
}

TEST_CASE( "Train Damage and Destruction Handling", "[kernel][vehicle][train][damage]" ) {
    rs_knl::TrainHead head(1, rs_knl::Vehicle::kVehicleTypeTrainHead, nullptr, 100, true, 1280);
    rs_knl::TrainBody body(2, rs_knl::Vehicle::kVehicleTypeTrainBody, nullptr, 80, true);
    head.appendTrainBody(&body);

    SECTION( "TrainHead takes partial damage" ) {
        rs_knl::DamageToInflict d;
        d.dvalue = 40;
        head.handleHit(d);
        REQUIRE( head.health() == 60 );
    }

    SECTION( "TrainHead destruction halts train" ) {
        rs_knl::DamageToInflict d;
        d.dvalue = 100;
        head.handleHit(d);
        REQUIRE( head.health() <= 0 );
        REQUIRE( head.speed() == 0 );
        REQUIRE( !head.hasDestination() );
        REQUIRE_FALSE( head.doMove(100) );
    }

    SECTION( "Carriage destruction halts linked train locomotive" ) {
        rs_knl::DamageToInflict d;
        d.dvalue = 80;
        body.handleHit(d);
        REQUIRE( body.health() <= 0 );
        REQUIRE_FALSE( head.doMove(100) );
    }
}

TEST_CASE( "Train Driver Assignment and Movement Inhibition", "[kernel][vehicle][train][driver]" ) {
    rs_knl::TrainHead head(1, rs_knl::Vehicle::kVehicleTypeTrainHead, nullptr, 100, true, 1280);
    rs_knl::TrainBody body(2, rs_knl::Vehicle::kVehicleTypeTrainBody, nullptr, 80, true);
    head.appendTrainBody(&body);

    rs_knl::PedInstance driver(10, nullptr, rs_knl::PedInstance::kPedTypePolice, false, 128);

    SECTION( "TrainHead supports driver assignment and detachment" ) {
        REQUIRE( head.getDriver() == nullptr );
        head.setDriver(&driver);
        REQUIRE( head.getDriver() == &driver );
        REQUIRE( head.isDriver(&driver) );
        REQUIRE( driver.inVehicle() == &head );

        // Clearing driver
        head.setDriver(nullptr);
        REQUIRE( head.getDriver() == nullptr );
        REQUIRE_FALSE( head.isDriver(&driver) );
        REQUIRE( driver.inVehicle() == nullptr );
    }

    SECTION( "Train movement fails when TrainHead is destroyed" ) {
        head.setStartHealth(0);
        head.setHealth(-1);
        REQUIRE_FALSE( head.isAlive() );
        REQUIRE_FALSE( head.initMovementToDestination(nullptr, rs_knl::TilePoint(10, 10, 2)) );
    }

    SECTION( "Train movement fails when Carriage is destroyed" ) {
        body.setStartHealth(0);
        body.setHealth(-1);
        REQUIRE_FALSE( body.isAlive() );
        REQUIRE_FALSE( head.initMovementToDestination(nullptr, rs_knl::TilePoint(10, 10, 2)) );
    }
}

TEST_CASE( "Mission 3 Off-Road Car Glides Into Correct Eastbound Lane", "[kernel][vehicle][mission3]" ) {
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
    REQUIRE(misMgr.load_level_data(3, levelData));
    rs_knl::Mission *pMission = misMgr.create_mission(levelData);
    REQUIRE(pMission != nullptr);
    misMgr.setMission(pMission);

    auto *pCar = dynamic_cast<rs_knl::GenericCar *>(pMission->vehicle(0));
    REQUIRE(pCar != nullptr);
    REQUIRE(pCar->tileX() == 21);
    REQUIRE(pCar->tileY() == 31);

    SECTION( "Parked car at spawn (21, 31) glides smoothly into Eastbound lane without entering Westbound lane or cornering" ) {
        pMission->start(weaponMgr);
        // Navigate East toward the city gate along the two-lane street (y=31 Westbound, y=33 Eastbound)
        REQUIRE(pCar->initMovementToDestination(pMission, rs_knl::TilePoint(36, 33, 2)));
        REQUIRE_FALSE(pCar->destPath().empty());

        // Car must glide directly into the Eastbound lane (y=33, oy=kLaneOffsetLow),
        // NOT into the wrong-direction Westbound lane (y=31) or Southbound turnaround (23,32) followed by a corner/reversal.
        const rs_knl::TilePoint &firstWp = pCar->destPath().front();
        CHECK(firstWp.ty == 33);
        CHECK(firstWp.tx >= 24);
        CHECK(firstWp.ox == rs_knl::GenericCar::kLaneOffsetMiddle);
        CHECK(firstWp.oy == rs_knl::GenericCar::kLaneOffsetLow);

        // Every waypoint from parked position along the initial straightaway moves strictly East (no West reversal)
        int prevWorldX = pCar->tileX() * 256 + pCar->offX();
        for (const auto &wp : pCar->destPath()) {
            int curWorldX = wp.tx * 256 + wp.ox;
            CHECK(curWorldX >= prevWorldX);
            prevWorldX = curWorldX;
        }

        // Simulate ticks through the off-road -> on-road merge and verify facing direction stays SE/East (1..64), never flipping West (>128)
        for (int tick = 0; tick < 80 && !pCar->destPath().empty(); ++tick) {
            pCar->doMove(50);
            int dir = pCar->direction();
            CHECK(dir > 0);
            CHECK(dir <= 64);
        }
    }

    SECTION( "Off-road car North of Westbound lane crosses divided street cleanly to Eastbound or glides Westbound" ) {
        pCar->setPosition(28, 30, 2, 128, 128);
        REQUIRE(pCar->initMovementToDestination(pMission, rs_knl::TilePoint(35, 33, 2)));
        REQUIRE_FALSE(pCar->destPath().empty());
        const rs_knl::TilePoint &eastEntry = pCar->destPath().front();
        CHECK(eastEntry.ty == 33);
        CHECK(eastEntry.tx >= 28);
        CHECK(eastEntry.oy == rs_knl::GenericCar::kLaneOffsetLow);

        pCar->setPosition(28, 30, 2, 128, 128);
        REQUIRE(pCar->initMovementToDestination(pMission, rs_knl::TilePoint(25, 31, 2)));
        REQUIRE_FALSE(pCar->destPath().empty());
        const rs_knl::TilePoint &westEntry = pCar->destPath().front();
        CHECK(westEntry.ty == 31);
        CHECK(westEntry.tx <= 28);
        CHECK(westEntry.oy == rs_knl::GenericCar::kLaneOffsetHigh);
    }

    misMgr.destroyMission();
}



