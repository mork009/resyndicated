/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net>
 *   Copyright (C) 2013, 2025-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef VEHICLE_H
#define VEHICLE_H

#include <string>

#include "rs-kernel/model/mapobject.h"
#include "rs-kernel/model/map.h"
#include "ped.h"

namespace rs_knl {

/*!
 * Generic class for all transports.
 * Transport can be driven or not.
 */
class Vehicle : public ShootableMovableMapObject{
public:
    static const uint8_t kVehicleTypeLargeArmored;
    static const uint8_t kVehicleTypeLargeArmoredDamaged;
    static const uint8_t kVehicleTypeTrainHead;
    static const uint8_t kVehicleTypeTrainBody;
    static const uint8_t kVehicleTypeRegularCar;
    static const uint8_t kVehicleTypeFireFighter;
    static const uint8_t kVehicleTypeSmallArmored;
    static const uint8_t kVehicleTypePolice;
    static const uint8_t kVehicleTypeMedics;

    Vehicle(uint16_t anId, uint8_t aType, Map *pMap, int maxSpeed) :
        ShootableMovableMapObject(anId, pMap, MapObject::kNatureVehicle, maxSpeed) {
        type_ = aType;
    }

    virtual ~Vehicle();

    //! Animation used when the vehicle is ok
    uint16_t regularAnimation() { return regularAnimation_; }
    //! Animation used when the vehicle is burning
    uint16_t burningAnimation() const { return burningAnimation_; }
    //! Animation used when the vehicle is burnt
    uint16_t burntAnimation() { return burntAnimation_; }
    //! Base sprite animation ID (e.g. 400, 416, 428, 434, 446)
    uint16_t baseSpriteAnim() const { return baseSpriteAnim_; }

    void draw(const Point2D &screenPos) override;
    uint16_t getDrawAnimation() const override;
    int getDrawFrame() const override;

    //! Return type of vehicle
    uint8_t getType() const { return type_; }
    //! Return true if vehicle is a car
    bool isCar() const { return type_ != kVehicleTypeTrainHead && type_ != kVehicleTypeTrainBody; }

    //! Adds the given ped to the list of passengers
    virtual void addPassenger(PedInstance *p);
    //! Removes the passenger from the vehicle
    virtual void dropPassenger(PedInstance *p);
    //! Drops all passengers from the vehicle cleanly
    void dropAllPassengers();
    const std::list<PedInstance*>& passengers() const { return passengers_; }

    PedInstance *getDriver(void) {
        return (pDriver_ != NULL && containsPed(pDriver_)) ? pDriver_ : NULL;
    }
    //! Set this ped as the new driver
    virtual void setDriver(PedInstance *pPed, bool forceDriver = true);

    /*!
     * Return true given ped is the driver of this vehicle.
     * \param pPed a Ped.
     */
    bool isDriver(PedInstance *pPed) {
        return (pPed != NULL && pDriver_ == pPed && containsPed(pPed));
    }

    //! Returns true if this vehicle is currently driven by one of the player's agents
    bool isDrivenByPlayer() const {
        return (pDriver_ != nullptr && pDriver_->isOurAgent() && const_cast<Vehicle*>(this)->containsPed(pDriver_));
    }

    //! Returns true if given ped is in the vehicle
    virtual bool containsPed(PedInstance *p) {
        for (std::list<PedInstance *>::iterator it = passengers_.begin();
        it != passengers_.end(); it++)
        {
            if ((*it)->id() == p->id()) {
                return true;
            }
        }
        return false;
    }
    //! Returns true if the vehicle contains one of our agent
    bool containsOurAgents();
    //! Returns true if the vehicle contains peds considered hostile by the given ped
    bool containsHostilesForPed(PedInstance *p, unsigned int hostile_desc_alt);

    void checkRunOverPeds(Mission *pMission);
    void checkBoardingNPCs(Mission *pMission);
    bool isTrainMoving(Mission *pMission);

    void handleHit(DamageToInflict &d) override;

    /*!
     * @brief Build vehicle animations using the base animation.
     */
    void setAnimations(uint16_t baseSpriteAnimationId);

protected:
    void doUpdateState([[maybe_unused]] uint32_t elapsed) override;
    void handleAnimationEnded() override;
    //! Returns animation index offset (with deadband hysteresis while moving to prevent 45-degree sprite flicker)
    uint16_t getDirectionAnimOffset() const;

protected:
    //! Vehicle driver
    PedInstance *pDriver_{nullptr};
    /*! The passengers of the vehicle.*/
    std::list <PedInstance *> passengers_;
    //! Animation used when the vehicle is ok
    uint16_t regularAnimation_;
    //! Animation used when the vehicle is burning
    uint16_t burningAnimation_;
    //! Animation used after the vehicle has finished burning
    uint16_t burntAnimation_;
    //! Base sprite animation ID (e.g. 400, 416, 428, 434, 446)
    uint16_t baseSpriteAnim_{0};
    //! Cached 4-way sprite index (0..3) used for hysteresis while moving
    mutable int8_t lastSpriteDir_{-1};

private:
    /*! Type of vehicle.*/
    uint8_t type_;
};

/*!
 * This class represents a playable car on a map.
 */
class GenericCar : public Vehicle
{
public:
    GenericCar(uint16_t id, uint8_t aType, Map *pMap, int maxSpeed = 1024, int accelRate = kCarAccelRate);
    virtual ~GenericCar() {}

    //! See ShootableMovableMapObject::initMovementToDestination()
    bool initMovementToDestination(Mission *m, const TilePoint &destinationPt) override;

    //! @copydoc ShootableMovableMapObject::doMove()
    bool doMove(uint32_t elapsed) override;

    //! Adds the given ped to the list of passengers
    void addPassenger(PedInstance *p) override;
    //! Removes the passenger from the vehicle
    void dropPassenger(PedInstance *p) override;

    void handleHit(DamageToInflict &d) override;

    int accelRate() const { return accelRate_; }
    void setAccelRate(int a) { accelRate_ = a; }

    //! Returns true if the pedestrian is inside the vehicle's driving path corridor
    bool isPedInDrivingPath(PedInstance *pPed, const TilePoint &targetPt);

    const std::list<TilePoint>& destPath() const { return dest_path_; }

    //! Computes the detailed road path between two tile points (A* on road grid + lane curves)
    static std::vector<TilePoint> calculateRoadPath(Map *pMap, const TilePoint &startPt, const TilePoint &destPt);

    // Intra-tile lane offsets (ox/oy range 0-255)
    static const int kLaneOffsetLow  = 32;  ///< Position on the low side of the lane
    static const int kLaneOffsetHigh = 200; ///< Position on the high side of the lane
    static const int kLaneOffsetMiddle = 128;

    // Kinematics and speed tuning constants
    static const int kMaxCarSpeed            = 1024; ///< Cruising speed on straight roads (units/s)
    static const int kCornerCarSpeed         = 480;  ///< Speed when navigating corners (units/s)
    static const int kMinCarMovingSpeed      = 120;  ///< Minimum initial speed when starting motion (units/s)
    static const int kCarAccelRate           = 1400; ///< Linear acceleration rate (units/s^2)
    static const int kCarDecelRate           = 2800; ///< Linear braking / yielding deceleration rate (units/s^2)
    static const int kCornerSlowdownDistance = 256;  ///< Distance before a turn to begin decelerating (units)
    static const int kCarYieldDistance       = 768;  ///< Lookahead distance for progressive braking on blockers (units)
    static const int kCornerTurnRadius       = 256;  ///< Sub-tile radius for cornering curve (higher = wider, more pronounced curve)
    static const int kCornerArcWaypoints     = 6;    ///< Number of waypoints along the curve (higher = smoother resolution)

    // Pathfinding limits
    static const int   kMaxWalkableSearchRadius = 16;       ///< Max tile search radius in findPathToNearestWalkableTile()
    static const int   kPathfindingWatchdog     = 5000;     ///< Max A* search iterations before giving up
    static const float kInfiniteDistance;                   ///< Sentinel "infinity" for distance comparisons

protected:
    bool findPathToNearestWalkableTile(TilePoint &startPt, std::vector < TilePoint > &recoveryPath);
    bool findNearestDrivableTile(const TilePoint &targetPt, TilePoint &nearestDrivablePt);
    //! Return true if car is blocked by something
    bool isBlocked();
    //! Check for potential blockers (vehicles or pedestrians) on the road
    virtual bool checkForBlockers(bool checkForCrossings = true);
    //! In pathfinding, find possible tiles around p that can be crossed when leaving p
    void expandCandidateNeighbours(const TilePoint &p, std::list<TilePoint> & candidateNeighbors);

    //! Adjust intra-tile offsets and interpolate smooth rounded corner curves
    void addIntraTileOffsetsToPath(bool startedOffRoad = false);

    //! Returns true if the next path node is part of a corner turn or turn approach
    bool isUpcomingWaypointCorner(double &distToCorner);

    //! Returns true if the vehicle driver has a subsequent driving action (e.g. patrol loop)
    bool hasSubsequentDrivingAction() const;

protected:
    //! Current physical speed for continuous acceleration/deceleration ramping
    int currentSpeed_;
    //! Target speed dictated by straightaways, corners, or obstacle yielding
    int targetSpeed_;
    //! Acceleration rate (units/s^2)
    int accelRate_;
};

}
#endif
