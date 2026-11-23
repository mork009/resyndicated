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
 *   Copyright (C) 2013, 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-kernel/model/vehicle.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <algorithm>
#include <cmath>
#include <queue>

#include "rs-utils/log/log.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/shot.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/model/train.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/ia/actions.h"
#include "rs-engine/config.h"

namespace rs_knl {

const uint8_t Vehicle::kVehicleTypeLargeArmored = 0x01;
const uint8_t Vehicle::kVehicleTypeLargeArmoredDamaged = 0x04;
const uint8_t Vehicle::kVehicleTypeTrainHead = 0x05;
const uint8_t Vehicle::kVehicleTypeTrainBody = 0x09;
const uint8_t Vehicle::kVehicleTypeRegularCar = 0x0D;
const uint8_t Vehicle::kVehicleTypeFireFighter = 0x11;
const uint8_t Vehicle::kVehicleTypeSmallArmored = 0x1C;
const uint8_t Vehicle::kVehicleTypePolice = 0x24;
const uint8_t Vehicle::kVehicleTypeMedics = 0x28;

const float GenericCar::kInfiniteDistance = 100000.0f;

uint16_t Vehicle::getDirectionAnimOffset() const
{
    uint8_t rawDir = getDiscreteDirection(4);
    if (!isMoving() || lastSpriteDir_ < 0 || lastSpriteDir_ > 3) {
        lastSpriteDir_ = static_cast<int8_t>(rawDir);
    } else if (static_cast<int>(rawDir) != lastSpriteDir_) {
        // Apply +/- 4 direction-unit (~5.6 deg) hysteresis around 45-deg boundaries
        // while moving so sub-tile integer stepping never flickers the 4-way car sprite.
        int delta = std::abs(dir_ - lastSpriteDir_ * 64);
        if (delta > 128) {
            delta = 256 - delta;
        }
        if (delta >= 36) {
            lastSpriteDir_ = static_cast<int8_t>(rawDir);
        }
    }

    uint16_t offsetAnim = static_cast<uint16_t>(lastSpriteDir_);
    if (getType() == kVehicleTypeTrainHead) {
        return offsetAnim;
    }
    if (getType() == kVehicleTypeTrainBody) {
        return (offsetAnim % 2 == 1) ? 1 : 0;
    }
    // For regular cars there is an empty animation between each direction so we skip one out of 2.
    return offsetAnim * 2;
}

void Vehicle::draw(const Point2D &screenPos)
{
    Point2D posWithOffs = addOffs(screenPos);

    // ensure on map
    int minX = (std::abs(rs_eng::g_mapZoom - 1.0) > 0.01) ? -128 : 90;
    if (posWithOffs.x < minX || posWithOffs.y < -128)
        return;

    animationPlayer_->draw(posWithOffs, getDirectionAnimOffset());
}

uint16_t Vehicle::getDrawAnimation() const {
    uint16_t anim = 0;
    if (animationPlayer_ && animationPlayer_->currentFramesAnimation() != 0) {
        anim = animationPlayer_->currentFramesAnimation();
    }
    return anim + getDirectionAnimOffset();
}

int Vehicle::getDrawFrame() const {
    if (animationPlayer_) {
        return animationPlayer_->currentFrame();
    }
    return 0;
}


void Vehicle::doUpdateState(uint32_t elapsed) {
    if (health_ > 0) {
        Mission *pMission = g_missionCtrl.mission();
        bool moved = doMove(elapsed);

        if (moved) {
            if (pMission) {
                checkRunOverPeds(pMission);
            }
        } else if (getType() == Vehicle::kVehicleTypeTrainBody) {
            if (pMission && !isTrainMoving(pMission)) {
                checkBoardingNPCs(pMission);
            }
        }
    }
}

void Vehicle::checkRunOverPeds(Mission *pMission) {
    if (health_ <= 0 || pMission == nullptr) return;

    // Run over peds that physically collide with this vehicle
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        PedInstance *pPed = pMission->ped(i);
        if (pPed->isAlive() && pPed->inVehicle() == NULL) {
            int cz = pos_.tz * 128 + pos_.oz;
            int pz = pPed->position().tz * 128 + pPed->position().oz;
            if (std::abs(cz - pz) <= 160) {
                int cx = pos_.tx * 256 + pos_.ox;
                int cy = pos_.ty * 256 + pos_.oy;
                int px = pPed->position().tx * 256 + pPed->position().ox;
                int py = pPed->position().ty * 256 + pPed->position().oy;
                int dx = px - cx;
                int dy = py - cy;
                if ((dx * dx + dy * dy) <= (64 * 64)) {
                    DamageToInflict d;
                    d.dtype = kDmgTypeCollision;
                    d.d_owner = this;
                    d.dvalue = 1024;
                    d.ddir = -1;
                    d.originLocW = position();
                    pPed->handleHit(d);
                }
            }
        }
    }

    if (getType() == Vehicle::kVehicleTypeTrainHead) {
        TrainHead *pHead = dynamic_cast<TrainHead *>(this);
        if (pHead != nullptr) {
            TrainBody *pBody = pHead->getNext();
            while (pBody != NULL) {
                pBody->checkRunOverPeds(pMission);
                pBody = pBody->getNext();
            }
        }
    }
}

void Vehicle::checkBoardingNPCs(Mission *pMission) {
    if (health_ <= 0 || pMission == nullptr || isTrainMoving(pMission)) return;

    int our_x = pos_.tx * 256 + pos_.ox;
    int our_y = pos_.ty * 256 + pos_.oy;
    int our_z = pos_.tz * 128 + pos_.oz;

    // Stationary train body (carriage) checks for nearby NPCs to board.
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        PedInstance *pPed = pMission->ped(i);
        if (pPed->isAlive() && pPed->inVehicle() == nullptr && !pPed->isOurAgent() && !pPed->isPersuaded() 
            && pPed->trainBoardingCooldown() == 0 && pPed->scriptSteps().empty()) {
            int px = pPed->position().tx * 256 + pPed->position().ox;
            int py = pPed->position().ty * 256 + pPed->position().oy;
            int pz = pPed->position().tz * 128 + pPed->position().oz;
            
            int cx = our_x - px;
            int cy = our_y - py;
            int cz = our_z - pz;
            double dist = sqrt((double)(cx * cx + cy * cy + cz * cz));

            if (dist < 1500.0) {
                LOG(Log::k_FLG_GAME, "Vehicle", "checkBoardingNPCs", ("Ped %d (type %d) dist to carriage %d is %.1f (cx=%d, cy=%d, cz=%d). pPed: (%d,%d,%d), carriage: (%d,%d,%d)", 
                    pPed->id(), pPed->type(), id(), dist, cx, cy, cz, px, py, pz, our_x, our_y, our_z))
            }

            if (isCloseTo(pPed, 550)) {
                pPed->destroyAllActions(true);
                addPassenger(pPed);
                LOG(Log::k_FLG_GAME, "Vehicle", "checkBoardingNPCs", ("NPC %d (type %d) automatically boarded train carriage %d", pPed->id(), pPed->type(), id()))
            }
        }
    }
}

bool Vehicle::isTrainMoving(Mission *pMission) {
    if (pMission == nullptr) return false;
    if (getType() == Vehicle::kVehicleTypeTrainBody) {
        for (size_t i = 0; i < pMission->numVehicles(); ++i) {
            Vehicle *pVehicle = pMission->vehicle(i);
            if (pVehicle->getType() == Vehicle::kVehicleTypeTrainHead) {
                TrainHead *pHead = dynamic_cast<TrainHead *>(pVehicle);
                if (pHead != nullptr) {
                    TrainBody *pCurr = pHead->getNext();
                    while (pCurr != nullptr) {
                        if (pCurr == this) {
                            return pHead->isMoving();
                        }
                        pCurr = pCurr->getNext();
                    }
                }
            }
        }
    }
    return isMoving();
}

/**
 * Adds given ped to the list of passengers.
 * \param pPed PedInstance*
 * \return void
 *
 */
void Vehicle::addPassenger(PedInstance *pPed) {
    if(!containsPed(pPed)) {
        passengers_.push_back(pPed);
        pPed->putInVehicle(this);
    }
}

/*!
 * Removes given passenger from vehicle.
 * \param pPed Ped to remove
 */
void Vehicle::dropPassenger(PedInstance *pPed) {
    if (pDriver_ == pPed) {
        pDriver_ = nullptr;
    }
    for (std::list<PedInstance *>::iterator it = passengers_.begin();
        it != passengers_.end(); it++)
        {
            if ((*it)->id() == pPed->id()) {
                pPed->leaveVehicle();
                passengers_.erase(it);
                return;
            }
        }
}

void Vehicle::dropAllPassengers() {
    while (!passengers_.empty()) {
        dropPassenger(passengers_.front());
    }
}

Vehicle::~Vehicle() {
    dropAllPassengers();
}

/*!
 * Returns true if at least one of our agent is inside the vehicle.
 */
bool Vehicle::containsOurAgents() {
    for (std::list<PedInstance *>::iterator it = passengers_.begin();
        it != passengers_.end(); it++)
    {
        if ((*it)->isOurAgent()) {
            return true;
        }
    }
    return false;
}

/*!
 * Returns true if the vehicle contains peds considered hostile by the given ped.
 * \param pPed The ped evaluating the hostility of the vehicle
 * \param hostile_desc_alt Parameter for evaluating the hostility
 * \return True if at least one hostile ped is found.
 */
bool Vehicle::containsHostilesForPed(PedInstance* p,
                                          unsigned int hostile_desc_alt)
{
    for (std::list<PedInstance *>::iterator it = passengers_.begin();
        it != passengers_.end(); it++)
    {
        if (p->isHostileTo((ShootableMapObject *)(*it), hostile_desc_alt))
            return true;
    }
    return false;
}

/*!
 * Initialize the animations to use for this vehicle
 * @param baseSpriteAnimationId the base animation id to use
 */
void Vehicle::setAnimations(uint16_t baseSpriteAnimationId) {
    baseSpriteAnim_ = baseSpriteAnimationId;
    regularAnimation_ = animationPlayer_->addAnimation(baseSpriteAnimationId);

    if (getType() == kVehicleTypeTrainHead) {
        // Authentic Bullfrog train locomotive animations:
        // 416..419: Normal (SW, SE, NE, NW)
        // 420..423: Burning (SW, SE, NE, NW)
        // 424..427: Burnt Wreckage (SW, SE, NE, NW)
        burningAnimation_ = animationPlayer_->addAnimation(baseSpriteAnimationId + 4,
                                                           rs_eng::kAnimationModeLoop, 4, 10000);
        burntAnimation_ = animationPlayer_->addAnimation(baseSpriteAnimationId + 8);
    } else if (getType() == kVehicleTypeTrainBody) {
        // Authentic Bullfrog train carriage animations:
        // Base (428/434/440): Normal (2 dirs)
        // Base + 2: Burning (2 dirs)
        // Base + 4: Burnt Wreckage (2 dirs)
        burningAnimation_ = animationPlayer_->addAnimation(baseSpriteAnimationId + 2,
                                                           rs_eng::kAnimationModeLoop, 2, 10000);
        burntAnimation_ = animationPlayer_->addAnimation(baseSpriteAnimationId + 4);
    } else {
        burningAnimation_ = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 8,
                                           rs_eng::kAnimationModeLoop, 8, 10000);

        burntAnimation_ = animationPlayer_->addAnimation(baseSpriteAnimationId + 12);
    }
}

void Vehicle::handleAnimationEnded() {
    if (animationPlayer_->isCurrentAnimation(burningAnimation_)) {
        // At the end of burning animation, there is the burnt animation
        animationPlayer_->play(burntAnimation_);
    }
}

void Vehicle::handleHit(DamageToInflict &d) {
    if (health_ <= 0) return;
    decreaseHealth(d.dvalue);
    if (health_ == 0) {
        if (animationPlayer_) {
            animationPlayer_->play(burningAnimation_);
        }
    }
}

GenericCar::GenericCar(uint16_t anId, uint8_t aType, Map *pMap, int maxSpeed, int accelRate):
    Vehicle(anId, aType, pMap, maxSpeed),
    currentSpeed_(0),
    targetSpeed_(0),
    accelRate_(accelRate) {
    unblockPath();
}

/*!
 * Computes a path on the road network from the car's current position to \p destinationPt
 * and stores it in dest_path_.
 *
 * \par Algorithm
 * Implements an \b A* search on the road tile graph. At each iteration the open node with
 * the smallest f-score (f = g + h) is expanded, where g is the cumulative tile cost from
 * the start and h is the Manhattan distance to the destination. The heuristic is admissible
 * (never overestimates), so the algorithm always finds the shortest path when one exists.
 * A min-heap is used for O(log n) node selection; stale heap entries are discarded via lazy
 * deletion when a node is popped after already being closed.
 *
 * \par Road constraints
 * Expansion is limited to tiles that are connected by compatible road exits as determined 
 * by Map::getPossibleConnexionsForRoadTile(). 
 *
 * \par Off-road recovery
 * If the car's current tile is not drivable (e.g. parked on non road), findPathToNearestWalkableTile()
 * is called first to build a short recovery path; this recovery path is prepended to the main path.
 *
 * \par Watchdog
 * If the search exceeds kPathfindingWatchdog iterations without reaching the goal, method returns false
 * to indicate that no path to destination was found. With A* this should only trigger on
 * pathologically disconnected road networks.
 *
 * \param pMission Current mission (unused in the body — kept for interface compatibility).
 * \param destinationPt Target tile point (tx, ty, tz, ox, oy).
 * \return true if a non-empty path was computed; false if the destination is unreachable.
 */
bool GenericCar::initMovementToDestination([[maybe_unused]] Mission *pMission, const TilePoint &destinationPt) {
    // A* search on the road network.
    // At each step, expand the open node with the smallest f-score = g + h,
    // where g is the cumulative tile cost and h is the Manhattan distance to the destination.

    // Min-heap ordered by f-score (smallest first).
    using OpenEntry = std::pair<int, TilePoint>;
    std::priority_queue<OpenEntry, std::vector<OpenEntry>, std::greater<OpenEntry>> openQueue;
    // Cumulative cost from the start node to each visited node
    std::map<TilePoint, int> gScore;

    std::set < TilePoint > closed;
    std::map < TilePoint, TilePoint > parent; // child → parent, for path reconstruction

    TilePoint destPt(destinationPt);

    pMap_->clip(&destPt);

    clearDestination();
    unblockPath();

    if (!isDrawable() || isDead()) {
        LOG(Log::k_FLG_GAME, "GenericCar", "initMovementToDestination", ("Car is invisible or dead"))
        return false;
    }

    int carRoadZ = 0;
    if (pos_.tz > 0 && pMap_->isTileWalkableByCar(pos_.tx, pos_.ty, pos_.tz - 1)) {
        carRoadZ = pos_.tz - 1;
    } else if (pMap_->isTileWalkableByCar(pos_.tx, pos_.ty, pos_.tz)) {
        carRoadZ = pos_.tz;
    } else if (pos_.tz > 0) {
        carRoadZ = pos_.tz - 1;
    } else {
        carRoadZ = 0;
    }
    if (carRoadZ < 0 || carRoadZ >= pMap_->maxTz()) {
        carRoadZ = 0;
    }

    // Determine road Z layer for destination (must match carRoadZ)
    int roadZ = carRoadZ;
    if (!pMap_->isTileWalkableByCar(destPt.tx, destPt.ty, roadZ)) {
        TilePoint nearestDest;
        if (findNearestDrivableTile(TilePoint(destPt.tx, destPt.ty, roadZ), nearestDest)) {
            destPt = nearestDest;
        } else {
            LOG(Log::k_FLG_GAME, "GenericCar", "initMovementToDestination", ("Destination point is not walkable by car %d : %d, %d, %d", id(), destPt.tx, destPt.ty, destPt.tz))
            return false;
        }
    }
    destPt.tz = roadZ;

    int startRoadZ = roadZ;
    TilePoint startNode(pos_.tx, pos_.ty, startRoadZ, pos_.ox, pos_.oy);

    // If the car is already on the destination tile, movement is complete
    if (startNode.tx == destPt.tx && startNode.ty == destPt.ty) {
        dest_path_.clear();
        stop();
        return true;
    }

    // Check whether a tile is passable for off-road recovery without cutting through buildings/walls
    auto isOffRoadGroundPassable = [&](int tx, int ty, bool checkOverheadRoof) -> bool {
        if (tx < 0 || tx >= pMap_->maxTx() || ty < 0 || ty >= pMap_->maxTy()) {
            return false;
        }
        if (tx == pos_.tx && ty == pos_.ty) {
            return true;
        }
        rs_eng::Tile *pTile = pMap_->getTileAt(tx, ty, startRoadZ);
        if (!pTile || pTile->type() == rs_eng::Tile::kWall || pTile->type() == rs_eng::Tile::kRoof ||
            pTile->id() == 119 || pTile->id() == rs_eng::Tile::kTileIdRoundAbout) {
            return false;
        }
        if (checkOverheadRoof && startRoadZ + 1 < pMap_->maxTz()) {
            rs_eng::Tile *pAbove = pMap_->getTileAt(tx, ty, startRoadZ + 1);
            if (pAbove && (pAbove->type() == rs_eng::Tile::kWall || pAbove->type() == rs_eng::Tile::kRoof)) {
                return false;
            }
        }
        return true;
    };

    // Check whether a straight-line off-road glide from startPos to entryTile is free of walls/roofs
    auto isStraightOffRoadClear = [&](const TilePoint &entryTile) -> bool {
        int x0 = pos_.tx * 256 + pos_.ox;
        int y0 = pos_.ty * 256 + pos_.oy;
        int x1 = entryTile.tx * 256 + 128;
        int y1 = entryTile.ty * 256 + 128;
        int dist = static_cast<int>(std::hypot(static_cast<double>(x1 - x0), static_cast<double>(y1 - y0)));
        int steps = std::max(1, dist / 64);
        for (int s = 1; s < steps; ++s) {
            double t = static_cast<double>(s) / static_cast<double>(steps);
            int cx = static_cast<int>(std::round(x0 + (x1 - x0) * t)) / 256;
            int cy = static_cast<int>(std::round(y0 + (y1 - y0) * t)) / 256;
            if (cx == entryTile.tx && cy == entryTile.ty) {
                continue;
            }
            if (!isOffRoadGroundPassable(cx, cy, true)) {
                return false;
            }
        }
        return true;
    };

    struct OffRoadCandidate {
        TilePoint entryTile;
        int initialGScore;
        std::vector<TilePoint> detourPath; // Empty when direct glide is clear
    };

    std::vector<TilePoint> recoveryPath;
    std::vector<OffRoadCandidate> offRoadCandidates;
    bool startedOffRoad = !pMap_->isTileWalkableByCar(pos_.tx, pos_.ty, startRoadZ);

    if (startedOffRoad) {
        // Explore reachable off-road tiles via 8-connected BFS to collect nearby road entry
        // candidates (covering both lanes of adjacent roads) rather than committing blindly
        // to the closest single tile, which can force a U-turn on multi-lane streets.
        auto encodeKey = [maxX = pMap_->maxTx()](int tx, int ty) -> int {
            return ty * maxX + tx;
        };

        std::queue<std::pair<int, int>> bfsQ;
        std::map<int, int> bfsParent;
        std::set<int> visited;
        double minRoadDist = kInfiniteDistance;

        int startKey = encodeKey(pos_.tx, pos_.ty);
        bfsQ.push({pos_.tx, pos_.ty});
        visited.insert(startKey);

        double startWorldX = pos_.tx + pos_.ox / 256.0;
        double startWorldY = pos_.ty + pos_.oy / 256.0;

        static constexpr int kDirs[8][2] = {
            {1, 0}, {-1, 0}, {0, 1}, {0, -1},
            {1, 1}, {1, -1}, {-1, 1}, {-1, -1}
        };

        while (!bfsQ.empty()) {
            auto [cx, cy] = bfsQ.front();
            bfsQ.pop();

            double curDist = std::hypot((cx + 0.5) - startWorldX, (cy + 0.5) - startWorldY);
            if (curDist > static_cast<double>(kMaxWalkableSearchRadius)) {
                continue;
            }
            if (minRoadDist < kInfiniteDistance && curDist > minRoadDist + 4.5) {
                continue;
            }

            if (!(cx == pos_.tx && cy == pos_.ty) && pMap_->isTileWalkableByCar(cx, cy, startRoadZ)) {
                if (curDist < minRoadDist) {
                    minRoadDist = curDist;
                }
                TilePoint candTile(cx, cy, startRoadZ);
                OffRoadCandidate cand;
                cand.entryTile = candTile;

                auto computeAlignmentPenalty = [&](double fromX, double fromY) -> int {
                    std::list<TilePoint> entryExits;
                    expandCandidateNeighbours(candTile, entryExits);
                    double appDx = (cx + 0.5) - fromX;
                    double appDy = (cy + 0.5) - fromY;
                    double appLen = std::hypot(appDx, appDy);
                    if (entryExits.empty() || appLen <= 1e-4) {
                        return 0;
                    }
                    double bestDot = -1.0;
                    for (const TilePoint &exitTile : entryExits) {
                        double ex = static_cast<double>(exitTile.tx - cx);
                        double ey = static_cast<double>(exitTile.ty - cy);
                        double dot = (ex * appDx + ey * appDy) / appLen;
                        if (dot > bestDot) {
                            bestDot = dot;
                        }
                    }
                    return static_cast<int>(std::round((1.0 - std::max(-1.0, bestDot)) * 45.0));
                };

                if (isStraightOffRoadClear(candTile)) {
                    // Weight off-road distance 1.35x vs road tile (100 units/tile) so cars
                    // glide smoothly (up to ~42 deg merge) directly into the proper lane rather
                    // than turning sharply into an adjacent wrong-way/corner tile, while still
                    // preferring roads for long travel.
                    cand.initialGScore = static_cast<int>(std::round(curDist * 135.0))
                        + computeAlignmentPenalty(startWorldX, startWorldY);
                } else {
                    std::vector<TilePoint> detour;
                    int k = encodeKey(cx, cy);
                    while (bfsParent.count(k)) {
                        int pk = bfsParent[k];
                        if (pk == startKey) {
                            break;
                        }
                        int px = pk % pMap_->maxTx();
                        int py = pk / pMap_->maxTx();
                        detour.push_back(TilePoint(px, py, pos_.tz, 128, 128));
                        k = pk;
                    }
                    std::reverse(detour.begin(), detour.end());
                    double detourDist = 0.0;
                    int roadHopPenalty = 0;
                    double prevX = startWorldX;
                    double prevY = startWorldY;
                    for (const TilePoint &wp : detour) {
                        double wx = wp.tx + 0.5;
                        double wy = wp.ty + 0.5;
                        detourDist += std::hypot(wx - prevX, wy - prevY);
                        if (pMap_->isTileWalkableByCar(wp.tx, wp.ty, startRoadZ)) {
                            roadHopPenalty += 80;
                        }
                        prevX = wx;
                        prevY = wy;
                    }
                    detourDist += std::hypot((cx + 0.5) - prevX, (cy + 0.5) - prevY);
                    cand.initialGScore = static_cast<int>(std::round(detourDist * 135.0))
                        + computeAlignmentPenalty(prevX, prevY) + roadHopPenalty;
                    cand.detourPath = std::move(detour);
                }
                offRoadCandidates.push_back(std::move(cand));
            }

            for (const auto &d : kDirs) {
                int nx = cx + d[0];
                int ny = cy + d[1];
                if (nx < 0 || nx >= pMap_->maxTx() || ny < 0 || ny >= pMap_->maxTy()) {
                    continue;
                }
                int nKey = encodeKey(nx, ny);
                if (visited.count(nKey)) {
                    continue;
                }
                if (!isOffRoadGroundPassable(nx, ny, true)) {
                    continue;
                }
                if (d[0] != 0 && d[1] != 0) {
                    if (!isOffRoadGroundPassable(cx + d[0], cy, true) ||
                        !isOffRoadGroundPassable(cx, cy + d[1], true)) {
                        continue;
                    }
                }
                visited.insert(nKey);
                bfsParent[nKey] = encodeKey(cx, cy);
                bfsQ.push({nx, ny});
            }
        }

        if (offRoadCandidates.empty()) {
            // Fallback to legacy cardinal probe if overhead roofs surrounded the spawn
            if (!findPathToNearestWalkableTile(startNode, recoveryPath)) {
                return false;
            }
            if (!recoveryPath.empty()) {
                recoveryPath.pop_back(); // entry tile is produced by A* itself
            }
            offRoadCandidates.push_back({startNode, 0, recoveryPath});
            recoveryPath.clear();
        }
    }

    // Seed the open set: single start tile when on-road, or all nearby road entries when off-road.
    std::map<TilePoint, int> chosenSeedIdx;
    if (!startedOffRoad) {
        gScore[startNode] = 0;
        int hStart = 100 * (abs(destPt.tx - startNode.tx) + abs(destPt.ty - startNode.ty));
        openQueue.push({hStart, startNode});
    } else {
        for (size_t i = 0; i < offRoadCandidates.size(); ++i) {
            const TilePoint &entry = offRoadCandidates[i].entryTile;
            int gInit = offRoadCandidates[i].initialGScore;
            if (!gScore.count(entry) || gInit < gScore[entry]) {
                gScore[entry] = gInit;
                chosenSeedIdx[entry] = static_cast<int>(i);
                int h = 100 * (abs(destPt.tx - entry.tx) + abs(destPt.ty - entry.ty));
                openQueue.push({gInit + h, entry});
            }
        }
    }

    int watchDog = kPathfindingWatchdog;
    TilePoint chosenEntryTile = startNode;

    while (!openQueue.empty()) {
        watchDog--;

        // --- Pop the node with the lowest f-score = g + h ---
        auto [fCurrent, p] = openQueue.top();
        openQueue.pop();

        // Lazy deletion: skip nodes already expanded (stale heap entries)
        if (closed.count(p)) continue;

        closed.insert(p);

        // --- Goal test (or watchdog expiry) ---
        if (p.isSameTile(destPt) || watchDog < 0) {
            if (watchDog < 0) {
                LOG(Log::k_FLG_GAME, "GenericCar", "initMovementToDestination", ("Hit watchdog before finding path for car %d to dest : %d, %d, %d\n", id(), destPt.tx, destPt.ty, destPt.tz))
                dest_path_.clear();
                return false;
            }

            dest_path_.push_front(TilePoint(destPt));

            // Reconstruct path by following parent links back to the start / entry tile
            while (parent.find(p) != parent.end()) {
                p = parent[p];
                if (!startedOffRoad && p.tx == pos_.tx && p.ty == pos_.ty && p.tz == destPt.tz)
                    break;
                dest_path_.push_front(p);
            }
            chosenEntryTile = p;
            break;
        }

        // Get candidate amoung neigbours
        std::list<TilePoint> candidateNeighbors;
        expandCandidateNeighbours(p, candidateNeighbors);

        int gCurrent = gScore.count(p) ? gScore[p] : 0;

        if (candidateNeighbors.empty()) {
            LOG(Log::k_FLG_GAME, "GenericCar", "initMovementToDestination", ("No neigbours for car %d at point %d, %d, %d\n", id(), p.tx, p.ty, p.tz))
        }

        for (auto& neighbor : candidateNeighbors) {
            if (closed.count(neighbor)) {
                continue;
            }

            int gNew = gCurrent + 100;

            // Only enqueue if we found a strictly better path to this neighbour
            if (!gScore.count(neighbor) || gNew < gScore[neighbor]) {
                gScore[neighbor] = gNew;
                parent[neighbor] = p;

                int h = 100 * (abs(destPt.tx - neighbor.tx) + abs(destPt.ty - neighbor.ty));
                openQueue.push({gNew + h, neighbor});
            }
        }
    }

    if(!dest_path_.empty()) {
        targetSpeed_ = maxSpeed();
        if (currentSpeed_ < kMinCarMovingSpeed) {
            currentSpeed_ = kMinCarMovingSpeed;
        }
        setSpeed(currentSpeed_);
        addIntraTileOffsetsToPath(startedOffRoad);

        // Prepend off-road detour waypoints only if direct straight glide to entry was blocked
        if (startedOffRoad && chosenSeedIdx.count(chosenEntryTile)) {
            const auto &detour = offRoadCandidates[chosenSeedIdx[chosenEntryTile]].detourPath;
            for (auto it = detour.rbegin(); it != detour.rend(); ++it) {
                TilePoint wp = *it;
                wp.tz = pos_.tz;
                dest_path_.push_front(wp);
            }
        }
    } else {
        currentSpeed_ = 0;
        targetSpeed_ = 0;
        stop();
    }
    
    return !dest_path_.empty();
}

/*!
 * Find only road-adjacent tiles that respect driving direction
 * @param p is the current tile for which we look candidates
 * @param candidateNeighbors a map of candidates
 */
void GenericCar::expandCandidateNeighbours(const TilePoint &p, std::list<TilePoint> & candidateNeighbors) {
    uint8_t possibleConnexions = pMap_->getPossibleConnexionsForRoadTile(p);
    
    if (rs_utl::isBitsOnWithMask(possibleConnexions, Map::kConnexionMaskExitNorth)) {
        candidateNeighbors.push_back(TilePoint(p.tx, p.ty - 1, p.tz));
    }

    if (rs_utl::isBitsOnWithMask(possibleConnexions, Map::kConnexionMaskExitSouth)) {
        candidateNeighbors.push_back(TilePoint(p.tx, p.ty + 1, p.tz));
    }

    if (rs_utl::isBitsOnWithMask(possibleConnexions, Map::kConnexionMaskExitEast)) {
        candidateNeighbors.push_back(TilePoint(p.tx + 1, p.ty, p.tz));
    }

    if (rs_utl::isBitsOnWithMask(possibleConnexions, Map::kConnexionMaskExitWest)) {
        candidateNeighbors.push_back(TilePoint(p.tx - 1, p.ty, p.tz));
    }
}

/*!
 * Adjust intra-tile offsets (ox, oy) so the car stays centered in the correct lane
 * and generates smooth multi-point rounded curves around 90-degree corners.
 */
void GenericCar::addIntraTileOffsetsToPath(bool startedOffRoad) {
    if (dest_path_.empty()) {
        return;
    }

    std::vector<TilePoint> rawTiles(dest_path_.begin(), dest_path_.end());
    std::list<TilePoint> curvedPath;

    TilePoint prevTile = pos_;

    for (size_t i = 0; i < rawTiles.size(); ++i) {
        const TilePoint &currTile = rawTiles[i];
        const TilePoint *nextTile = (i + 1 < rawTiles.size()) ? &rawTiles[i + 1] : nullptr;

        int inDx = currTile.tx - prevTile.tx;
        int inDy = currTile.ty - prevTile.ty;

        int outDx = nextTile ? (nextTile->tx - currTile.tx) : inDx;
        int outDy = nextTile ? (nextTile->ty - currTile.ty) : inDy;

        if (i == 0 && startedOffRoad) {
            // When transitioning from off-road onto the entry road tile, align the entry
            // waypoint directly to the outgoing road direction so the car glides into the
            // proper traffic lane without reversing or executing a spurious 90-degree turn.
            if (nextTile != nullptr) {
                inDx = outDx;
                inDy = outDy;
            } else if (pMap_) {
                int rz = (pos_.tz > 0 && pMap_->isTileWalkableByCar(currTile.tx, currTile.ty, pos_.tz - 1))
                    ? pos_.tz - 1 : currTile.tz;
                rs_eng::Tile *pTile = pMap_->getTileAt(currTile.tx, currTile.ty, rz);
                if (pTile && pTile->type() == rs_eng::Tile::kRoadSideWE) {
                    inDx = 1; inDy = 0;
                } else if (pTile && pTile->type() == rs_eng::Tile::kRoadSideEW) {
                    inDx = -1; inDy = 0;
                } else if (pTile && pTile->type() == rs_eng::Tile::kRoadSideSN) {
                    inDx = 0; inDy = 1;
                } else if (pTile && pTile->type() == rs_eng::Tile::kRoadSideNS) {
                    inDx = 0; inDy = -1;
                } else if (std::abs(inDx) >= std::abs(inDy)) {
                    inDx = (inDx > 0) ? 1 : (inDx < 0 ? -1 : 0);
                    inDy = 0;
                } else {
                    inDx = 0;
                    inDy = (inDy > 0) ? 1 : -1;
                }
                outDx = inDx;
                outDy = inDy;
            }
        } else if (inDx == 0 && inDy == 0) {
            inDx = outDx;
            inDy = outDy;
        }

        bool isTurn = (inDx != outDx || inDy != outDy) && (inDx != 0 || inDy != 0) && (outDx != 0 || outDy != 0);

        if (!isTurn) {
            // Straight road segment
            int ox = kLaneOffsetMiddle;
            int oy = kLaneOffsetMiddle;
            if (inDx > 0) { // West to East
                ox = kLaneOffsetMiddle;
                oy = kLaneOffsetLow;
            } else if (inDx < 0) { // East to West
                ox = kLaneOffsetMiddle;
                oy = kLaneOffsetHigh;
            } else if (inDy > 0) { // North to South
                ox = kLaneOffsetHigh;
                oy = kLaneOffsetMiddle;
            } else if (inDy < 0) { // South to North
                ox = kLaneOffsetLow;
                oy = kLaneOffsetMiddle;
            }
            curvedPath.push_back(TilePoint(currTile.tx, currTile.ty, pos_.tz, ox, oy));
        } else {
            // 90-degree corner turn: generate rounded Bezier arc waypoints
            int vx = kLaneOffsetMiddle;
            int vy = kLaneOffsetMiddle;

            if (inDx != 0) {
                vy = (inDx > 0) ? kLaneOffsetLow : kLaneOffsetHigh;
                vx = (outDy > 0) ? kLaneOffsetHigh : kLaneOffsetLow;
            } else {
                vx = (inDy > 0) ? kLaneOffsetHigh : kLaneOffsetLow;
                vy = (outDx > 0) ? kLaneOffsetLow : kLaneOffsetHigh;
            }

            int radius = kCornerTurnRadius;
            int p0x = vx - inDx * radius;
            int p0y = vy - inDy * radius;
            int p2x = vx + outDx * radius;
            int p2y = vy + outDy * radius;

            auto clampCoord = [](int val) {
                return std::max(20, std::min(236, val));
            };
            p0x = clampCoord(p0x);
            p0y = clampCoord(p0y);
            p2x = clampCoord(p2x);
            p2y = clampCoord(p2y);

            // Interpolate kCornerArcWaypoints smooth waypoints along quadratic Bezier curve
            int numSteps = std::max(2, kCornerArcWaypoints);
            for (int step = 0; step < numSteps; ++step) {
                double u = (double)step / (double)(numSteps - 1);
                double invU = 1.0 - u;
                int ox = (int)std::round(invU * invU * p0x + 2.0 * invU * u * vx + u * u * p2x);
                int oy = (int)std::round(invU * invU * p0y + 2.0 * invU * u * vy + u * u * p2y);
                curvedPath.push_back(TilePoint(currTile.tx, currTile.ty, pos_.tz, ox, oy));
            }
        }

        prevTile = currTile;
    }

    dest_path_ = std::move(curvedPath);
}

std::vector<TilePoint> GenericCar::calculateRoadPath(Map *pMap, const TilePoint &startPt, const TilePoint &destPt) {
    std::vector<TilePoint> result;
    if (!pMap) return result;

    TilePoint s = startPt;
    TilePoint d = destPt;
    pMap->clip(&s);
    pMap->clip(&d);

    auto findRoadTile = [&](const TilePoint &pt, int preferredZ, TilePoint &out) -> bool {
        std::vector<int> candidateZs;
        auto addZ = [&](int z) {
            if (z >= 0 && z < pMap->maxTz()) {
                if (std::find(candidateZs.begin(), candidateZs.end(), z) == candidateZs.end()) {
                    candidateZs.push_back(z);
                }
            }
        };
        if (preferredZ >= 0) {
            addZ(preferredZ);
        } else {
            if (pt.tz > 0 && pMap->isTileWalkableByCar(pt.tx, pt.ty, pt.tz - 1)) {
                addZ(pt.tz - 1);
            }
            addZ(pt.tz);
            addZ(pt.tz - 1);
            addZ(pt.tz + 1);
        }
        for (int z = 0; z < pMap->maxTz(); ++z) addZ(z);

        // For each candidate Z in priority order:
        // Try exact (pt.tx, pt.ty), then search radius 1..6 on THAT Z layer before moving to next Z layer!
        for (int z : candidateZs) {
            if (pMap->isTileWalkableByCar(pt.tx, pt.ty, z)) {
                out = TilePoint(pt.tx, pt.ty, z, pt.ox, pt.oy);
                return true;
            }
            int bestDistSq = 999999;
            int bestX = -1, bestY = -1;
            for (int r = 1; r <= 6; ++r) {
                int minX = std::max(0, pt.tx - r);
                int maxX = std::min(pMap->maxTx() - 1, pt.tx + r);
                int minY = std::max(0, pt.ty - r);
                int maxY = std::min(pMap->maxTy() - 1, pt.ty + r);
                for (int ty = minY; ty <= maxY; ++ty) {
                    for (int tx = minX; tx <= maxX; ++tx) {
                        if (pMap->isTileWalkableByCar(tx, ty, z)) {
                            int dx = tx - pt.tx;
                            int dy = ty - pt.ty;
                            int d2 = dx * dx + dy * dy;
                            if (d2 < bestDistSq) {
                                bestDistSq = d2;
                                bestX = tx;
                                bestY = ty;
                            }
                        }
                    }
                }
                if (bestX != -1) break;
            }
            if (bestX != -1) {
                out = TilePoint(bestX, bestY, z, pt.ox, pt.oy);
                return true;
            }
        }
        return false;
    };

    TilePoint sRoad = s;
    findRoadTile(s, -1, sRoad);
    TilePoint dRoad = d;
    findRoadTile(d, sRoad.tz, dRoad);

    s = sRoad;
    d = dRoad;

    if (s.tx == d.tx && s.ty == d.ty) {
        result.push_back(startPt);
        result.push_back(destPt);
        return result;
    }

    using NodeScore = std::pair<int, TilePoint>;
    std::priority_queue<NodeScore, std::vector<NodeScore>, std::greater<NodeScore>> openQueue;
    std::map<TilePoint, int> gScore;
    std::set<TilePoint> closed;
    std::map<TilePoint, TilePoint> parent;

    gScore[s] = 0;
    int hStart = abs(d.tx - s.tx) + abs(d.ty - s.ty);
    openQueue.push({hStart, s});

    int watchdog = 4000;
    bool found = false;
    TilePoint endNode = d;

    while (!openQueue.empty() && watchdog-- > 0) {
        auto [fCurr, p] = openQueue.top();
        openQueue.pop();

        if (closed.count(p)) continue;
        closed.insert(p);

        if (p.isSameTile(d)) {
            endNode = p;
            found = true;
            break;
        }

        uint8_t possibleConnexions = pMap->getPossibleConnexionsForRoadTile(p);
        std::vector<TilePoint> neighbors;
        if (rs_utl::isBitsOnWithMask(possibleConnexions, Map::kConnexionMaskExitNorth))
            neighbors.push_back(TilePoint(p.tx, p.ty - 1, p.tz));
        if (rs_utl::isBitsOnWithMask(possibleConnexions, Map::kConnexionMaskExitSouth))
            neighbors.push_back(TilePoint(p.tx, p.ty + 1, p.tz));
        if (rs_utl::isBitsOnWithMask(possibleConnexions, Map::kConnexionMaskExitEast))
            neighbors.push_back(TilePoint(p.tx + 1, p.ty, p.tz));
        if (rs_utl::isBitsOnWithMask(possibleConnexions, Map::kConnexionMaskExitWest))
            neighbors.push_back(TilePoint(p.tx - 1, p.ty, p.tz));

        int gCurr = gScore.count(p) ? gScore[p] : 0;
        for (auto &nbr : neighbors) {
            if (closed.count(nbr)) continue;
            int gNew = gCurr + 1;
            if (!gScore.count(nbr) || gNew < gScore[nbr]) {
                gScore[nbr] = gNew;
                parent[nbr] = p;
                int h = abs(d.tx - nbr.tx) + abs(d.ty - nbr.ty);
                openQueue.push({gNew + h, nbr});
            }
        }
    }

    if (!found) {
        result.push_back(startPt);
        result.push_back(destPt);
        return result;
    }

    std::vector<TilePoint> rawPath;
    TilePoint curr = endNode;
    rawPath.push_back(curr);
    while (parent.find(curr) != parent.end()) {
        curr = parent[curr];
        rawPath.push_back(curr);
        if (curr.isSameTile(s)) break;
    }
    std::reverse(rawPath.begin(), rawPath.end());

    TilePoint prevTile = rawPath.front();
    int visualZ = startPt.tz;

    for (size_t i = 0; i < rawPath.size(); ++i) {
        const TilePoint &cTile = rawPath[i];
        const TilePoint *nTile = (i + 1 < rawPath.size()) ? &rawPath[i + 1] : nullptr;

        int inDx = cTile.tx - prevTile.tx;
        int inDy = cTile.ty - prevTile.ty;
        int outDx = nTile ? (nTile->tx - cTile.tx) : inDx;
        int outDy = nTile ? (nTile->ty - cTile.ty) : inDy;

        if (inDx == 0 && inDy == 0) {
            inDx = outDx;
            inDy = outDy;
        }

        bool isTurn = (inDx != outDx || inDy != outDy) && (inDx != 0 || inDy != 0) && (outDx != 0 || outDy != 0);
        if (!isTurn) {
            int ox = kLaneOffsetMiddle;
            int oy = kLaneOffsetMiddle;
            if (inDx > 0) { ox = kLaneOffsetMiddle; oy = kLaneOffsetLow; }
            else if (inDx < 0) { ox = kLaneOffsetMiddle; oy = kLaneOffsetHigh; }
            else if (inDy > 0) { ox = kLaneOffsetHigh; oy = kLaneOffsetMiddle; }
            else if (inDy < 0) { ox = kLaneOffsetLow; oy = kLaneOffsetMiddle; }
            result.push_back(TilePoint(cTile.tx, cTile.ty, visualZ, ox, oy));
        } else {
            int vx = kLaneOffsetMiddle;
            int vy = kLaneOffsetMiddle;
            if (inDx != 0) {
                vy = (inDx > 0) ? kLaneOffsetLow : kLaneOffsetHigh;
                vx = (outDy > 0) ? kLaneOffsetHigh : kLaneOffsetLow;
            } else {
                vx = (inDy > 0) ? kLaneOffsetHigh : kLaneOffsetLow;
                vy = (outDx > 0) ? kLaneOffsetLow : kLaneOffsetHigh;
            }

            int radius = kCornerTurnRadius;
            int p0x = vx - inDx * radius;
            int p0y = vy - inDy * radius;
            int p2x = vx + outDx * radius;
            int p2y = vy + outDy * radius;

            auto clampCoord = [](int val) { return std::max(20, std::min(236, val)); };
            p0x = clampCoord(p0x);
            p0y = clampCoord(p0y);
            p2x = clampCoord(p2x);
            p2y = clampCoord(p2y);

            for (int step = 0; step <= kCornerArcWaypoints; ++step) {
                float t = static_cast<float>(step) / static_cast<float>(kCornerArcWaypoints);
                float u = 1.0f - t;
                float bx = u * u * p0x + 2.0f * u * t * vx + t * t * p2x;
                float by = u * u * p0y + 2.0f * u * t * vy + t * t * p2y;
                result.push_back(TilePoint(cTile.tx, cTile.ty, visualZ, static_cast<int>(bx), static_cast<int>(by)));
            }
        }
        prevTile = cTile;
    }

    if (!result.empty()) {
        if (!result.front().isSameTile(startPt) || abs(result.front().ox - startPt.ox) > 30 || abs(result.front().oy - startPt.oy) > 30) {
            result.insert(result.begin(), startPt);
        }
        if (!result.back().isSameTile(destPt) || abs(result.back().ox - destPt.ox) > 30 || abs(result.back().oy - destPt.oy) > 30) {
            result.push_back(destPt);
        }
    }

    return result;
}

/*!
 * Checks if the vehicle is currently in or approaching a corner turn within lookahead distance.
 */
bool GenericCar::isUpcomingWaypointCorner(double &distToCorner) {
    if (dest_path_.empty()) return false;

    double cumDist = 0.0;
    int prevX = pos_.tx * 256 + pos_.ox;
    int prevY = pos_.ty * 256 + pos_.oy;

    for (const auto &pt : dest_path_) {
        int curX = pt.tx * 256 + pt.ox;
        int curY = pt.ty * 256 + pt.oy;
        int dx = curX - prevX;
        int dy = curY - prevY;
        double segDist = sqrt((double)(dx * dx + dy * dy));
        cumDist += segDist;

        // Diagonal displacement indicates arc / corner waypoint
        if (std::abs(dx) > 0 && std::abs(dy) > 0) {
            distToCorner = cumDist;
            return (cumDist <= (double)kCornerSlowdownDistance);
        }

        if (cumDist > (double)kCornerSlowdownDistance) {
            break;
        }

        prevX = curX;
        prevY = curY;
    }

    return false;
}

/*!
 * Returns true if the vehicle driver has a subsequent driving action queued (e.g. continuous patrol loop).
 */
bool GenericCar::hasSubsequentDrivingAction() const {
    if (!pDriver_ || !pDriver_->isAlive()) return false;

    MovementAction *act = pDriver_->currentAction();
    if (!act) return false;

    MovementAction *nextAct = act->next();
    if (nextAct) {
        if (dynamic_cast<DriveVehicleAction*>(nextAct)) {
            return true;
        }
        if (dynamic_cast<ResetScriptedAction*>(nextAct)) {
            MovementAction *firstAct = pDriver_->defaultAction();
            if (firstAct && dynamic_cast<DriveVehicleAction*>(firstAct)) {
                return true;
            }
        }
    }
    return false;
}

/*!
 * Finds the nearest drivable road tile to the car's current position and builds a
 * straight-line recovery path to reach it.
 *
 * Called when the car is on a non-drivable tile. Scans up to kMaxWalkableSearchRadius tiles
 * in each of the four cardinal directions (+X, -X, -Y, +Y) and keeps the candidate with the
 * smallest squared distance (to avoid sqrt). The winning path is stored in \p recoveryPath
 * and the tile coordinates of its end point are written into \p startTx / \p startTy so that
 * the main pathfinder can continue from there.
 *
 * \param startPt     The car's current position (off-road).
 * \param startTx     [out] X tile coordinate of the nearest walkable tile found.
 * \param startTy     [out] Y tile coordinate of the nearest walkable tile found.
 * \param recoveryPath [out] Sequence of tile points leading to the nearest walkable tile.
 * \return true if a walkable tile was found within the search radius; false otherwise.
 */
bool GenericCar::findPathToNearestWalkableTile(TilePoint &startPt, std::vector < TilePoint > &recoveryPath) {
    // Scan up to kMaxWalkableSearchRadius tiles in each of the 4 cardinal directions
    // to find the closest drivable tile. Uses squared distance to avoid sqrt.
    int bestDist = (int)kInfiniteDistance, curDist;
    std::vector < TilePoint > candidatePath;
    candidatePath.reserve(kMaxWalkableSearchRadius);
    // we got somewhere we shouldn't, we need to find somewhere that is walkable
    TilePoint probePoint = startPt;
    // Store the final starting point
    int startTx = startPt.tx;
    int startTy = startPt.ty;
    for (int i = 1; i < kMaxWalkableSearchRadius; i++) {
        if (pos_.tx + i >= pMap_->maxTx())
            break;
        probePoint.tx = pos_.tx + i;
        candidatePath.push_back(probePoint);
        if (pMap_->isTileWalkableByCar(pos_.tx + i, pos_.ty, startPt.tz)) {
            curDist = i * i;
            if(curDist < bestDist) {
                bestDist = curDist;
                recoveryPath.assign(candidatePath.begin(), candidatePath.end());
                startTx = pos_.tx + i;
                startTy = pos_.ty;
                break;
            }
        }
    }

    candidatePath.clear();
    probePoint = startPt;
    for (int i = -1; i > -kMaxWalkableSearchRadius; --i) {
        if (pos_.tx + i < 0)
            break;
        probePoint.tx = (pos_.tx + i);
        candidatePath.push_back(probePoint);
        if (pMap_->isTileWalkableByCar(pos_.tx + i, pos_.ty, startPt.tz)) {
            curDist = i * i;
            if(curDist < bestDist) {
                bestDist = curDist;
                recoveryPath.assign(candidatePath.begin(), candidatePath.end());
                startTx = pos_.tx + i;
                startTy = pos_.ty;
                break;
            }
        }
    }

    candidatePath.clear();
    probePoint = startPt;
    for (int i = -1; i > -kMaxWalkableSearchRadius; --i) {
        if (pos_.ty + i < 0)
            break;
        probePoint.ty = (pos_.ty + i);
        candidatePath.push_back(probePoint);
        if (pMap_->isTileWalkableByCar(pos_.tx, pos_.ty + i, startPt.tz)) {
            curDist = i * i;
            if(curDist < bestDist) {
                bestDist = curDist;
                recoveryPath.assign(candidatePath.begin(), candidatePath.end());
                startTx = pos_.tx;
                startTy = pos_.ty + i;
                break;
            }
        }
    }

    candidatePath.clear();
    probePoint = startPt;
    for (int i = 1; i < kMaxWalkableSearchRadius; i++) {
        if (pos_.ty + i >= pMap_->maxTy())
            break;
        probePoint.ty = pos_.ty + i;
        candidatePath.push_back(probePoint);
        if (pMap_->isTileWalkableByCar(pos_.tx, pos_.ty + i, startPt.tz)) {
            curDist = i * i;
            if(curDist < bestDist) {
                bestDist = curDist;
                recoveryPath.assign(candidatePath.begin(), candidatePath.end());
                startTx = pos_.tx;
                startTy = pos_.ty + i;
                break;
            }
        }
    }

    startPt.tx = startTx;
    startPt.ty = startTy;
    return (bestDist != (int)kInfiniteDistance);
}

/*!
 * Finds the nearest drivable road tile to the given target position in 3D map space.
 * Searches in a radius of up to kMaxWalkableSearchRadius around targetPt.
 */
bool GenericCar::findNearestDrivableTile(const TilePoint &targetPt, TilePoint &nearestDrivablePt) {
    int bestDistSq = 999999;
    int bestTx = -1, bestTy = -1;

    int maxR = kMaxWalkableSearchRadius;
    int minX = std::max(0, targetPt.tx - maxR);
    int maxX = std::min(pMap_->maxTx() - 1, targetPt.tx + maxR);
    int minY = std::max(0, targetPt.ty - maxR);
    int maxY = std::min(pMap_->maxTy() - 1, targetPt.ty + maxR);

    int roadZ = targetPt.tz;
    if (roadZ < 0 || roadZ >= pMap_->maxTz()) {
        if (pos_.tz > 0 && pMap_->isTileWalkableByCar(pos_.tx, pos_.ty, pos_.tz - 1)) {
            roadZ = pos_.tz - 1;
        } else if (pMap_->isTileWalkableByCar(pos_.tx, pos_.ty, pos_.tz)) {
            roadZ = pos_.tz;
        } else if (pos_.tz > 0) {
            roadZ = pos_.tz - 1;
        } else {
            roadZ = 0;
        }
        if (roadZ < 0 || roadZ >= pMap_->maxTz()) {
            roadZ = 0;
        }
    }

    for (int ty = minY; ty <= maxY; ++ty) {
        for (int tx = minX; tx <= maxX; ++tx) {
            if (pMap_->isTileWalkableByCar(tx, ty, roadZ)) {
                int dx = tx - targetPt.tx;
                int dy = ty - targetPt.ty;
                int distSq = dx * dx + dy * dy;
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    bestTx = tx;
                    bestTy = ty;
                }
            }
        }
    }

    if (bestTx != -1) {
        nearestDrivablePt = TilePoint(bestTx, bestTy, roadZ, targetPt.ox, targetPt.oy);
        return true;
    }
    return false;
}

/*!
 * Returns true if the given pedestrian is inside the driving path corridor towards targetPt
 */
bool GenericCar::isPedInDrivingPath(PedInstance *pPed, const TilePoint &targetPt) {
    if (pPed == nullptr || !pPed->isAlive() || pPed->inVehicle() != nullptr) {
        return false;
    }

    int cz = pos_.tz * 128 + pos_.oz;
    int pz = pPed->position().tz * 128 + pPed->position().oz;
    if (std::abs(cz - pz) > 160) {
        return false;
    }

    int cx = pos_.tx * 256 + pos_.ox;
    int cy = pos_.ty * 256 + pos_.oy;
    int px = pPed->position().tx * 256 + pPed->position().ox;
    int py = pPed->position().ty * 256 + pPed->position().oy;

    double pdx = px - cx;
    double pdy = py - cy;
    double distSq = pdx * pdx + pdy * pdy;
    double lookahead = (double)kCarYieldDistance;
    if (distSq > (lookahead * lookahead)) {
        return false;
    }

    int wx = targetPt.tx * 256 + targetPt.ox;
    int wy = targetPt.ty * 256 + targetPt.oy;
    int vdx = wx - cx;
    int vdy = wy - cy;

    double vx = 0.0;
    double vy = 0.0;

    if (vdx != 0 || vdy != 0) {
        double pathLen = sqrt((double)(vdx * vdx + vdy * vdy));
        vx = vdx / pathLen;
        vy = vdy / pathLen;
    } else {
        double angleRad = (dir_ / 128.0) * rs_utl::kPI;
        vx = sin(angleRad);
        vy = cos(angleRad);
    }

    // Longitudinal projection ahead of car: look ahead along the driving line
    double s = pdx * vx + pdy * vy;
    if (s < -32.0 || s > lookahead) {
        return false;
    }

    // Lateral distance from the driving line:
    // Standard road is 256 units wide; driving corridor of 96 units covers the full driving lane
    // and road center, ensuring cars yield to agents and crossing pedestrians.
    double l = std::abs(pdx * vy - pdy * vx);
    return (l <= 96.0);
}

/*!
 * @return true if this vehicle is currently blocked by something
 */
bool GenericCar::isBlocked() {
    if (hold_on_.pathBlocker != nullptr) { // blocked by something
        if (hold_on_.terminatePath) {
            // Must stop : clear destination and stop
            clearDestination();
            return true;
        }
        if (dest_path_.empty()) {
            unblockPath();
            return false;
        }
        if (hold_on_.pathBlocker->is(MapObject::kNatureVehicle)) {
            // For vehicle blockers: check if the blocking car has left the tile
            if (hold_on_.pathBlocker->tileX() == hold_on_.tilex
                && hold_on_.pathBlocker->tileY() == hold_on_.tiley) {
                return true;  // still occupying the tile — wait
            }
            unblockPath();  // blocker has moved away — resume
        } else if (hold_on_.pathBlocker->is(MapObject::kNaturePed)) {
            // If driven by player agents, do not yield to pedestrians
            if (isDrivenByPlayer()) {
                unblockPath();
                return false;
            }
            // For ped blockers: resume when the ped has cleared the driving lane, entered a vehicle, or died
            PedInstance *pBlockingPed = static_cast<PedInstance *>(hold_on_.pathBlocker);
            TilePoint nextPt = dest_path_.front();
            if (!pBlockingPed->isAlive()
                    || pBlockingPed->inVehicle() != nullptr
                    || !isPedInDrivingPath(pBlockingPed, nextPt)) {
                unblockPath();  // ped has cleared the driving path — resume
            } else {
                return true;  // ped still blocking the driving path — wait
            }
        } else {
            return true;  // non-vehicle blocker (door, obstacle…) — unchanged behaviour
        }
    }

    return false;
}

/*!
 * Check if the next tile is already occupied by another vehicle,
 * or if there are living pedestrians in the car's forward driving path (NPC drivers only).
 * @param checkForCrossings True to check crossings
 * @return true if there is a blocker ahead
 */
bool GenericCar::checkForBlockers(bool checkForCrossings) {
    Mission *pMission = g_missionCtrl.mission();
    if (!pMission) return false;

    // Determine the effective waypoint to check for blockers.
    auto it = dest_path_.begin();
    if (dest_path_.size() > 1) {
        const TilePoint &nextPt = *it;
        int diffx = nextPt.tx * 256 + nextPt.ox - (pos_.tx * 256 + pos_.ox);
        int diffy = nextPt.ty * 256 + nextPt.oy - (pos_.ty * 256 + pos_.oy);
        int halfSize = std::max(sizeX(), sizeY());
        if (diffx * diffx + diffy * diffy < halfSize * halfSize) {
            ++it;  // vehicle front is past nextPt → check the following waypoint
        }
    }

    const TilePoint &checkPt = *it;

    // Check for vehicle blockers on waypoint tile (applies to both NPC and player drivers)
    const std::vector<MapObject*> &occupants = pMission->getObjectsAtTile(checkPt);
    for (MapObject *obj : occupants) {
        if (obj != this && obj->is(MapObject::kNatureVehicle)) {
            Vehicle *pBlocker = static_cast<Vehicle *>(obj);
            if (pBlocker->isBlockedBy(this)) { 
                // trying to prevent interblocking of two cars
                continue;
            }
            // Blocker actually occupies the checkPt tile
            if (pBlocker->tileX() == checkPt.tx && pBlocker->tileY() == checkPt.ty) {
                bool terminate = pBlocker->isDead();
                blockPathWith(obj, terminate, checkPt.tx, checkPt.ty, checkPt.tz);
                return true;
            }
            // If both vehicles are on the same tile, break deadlock by ID priority so one leaves first
            if (pBlocker->tileX() == pos_.tx && pBlocker->tileY() == pos_.ty) {
                if (pBlocker->id() < this->id()) {
                    bool terminate = pBlocker->isDead();
                    blockPathWith(obj, terminate, pos_.tx, pos_.ty, pos_.tz);
                    return true;
                }
            }
        }
    }

    // Pedestrian yielding logic applies ONLY to NPC-driven vehicles.
    // When driven by player agents, cars do NOT yield to pedestrians (they run them over).
    if (!isDrivenByPlayer()) {
        for (size_t i = 0; i < pMission->numPeds(); ++i) {
            PedInstance *pPed = pMission->ped(i);
            if (pPed && pPed->isAlive() && pPed->inVehicle() == nullptr) {
                if (isPedInDrivingPath(pPed, checkPt)) {
                    blockPathWith(pPed, false, pPed->tileX(), pPed->tileY(), pPed->tileZ(), pPed->offX(), pPed->offY());
                    return true;
                }
            }
        }
    }

    return false;
}

/*!
 * Moves a vehicle on the map with smooth acceleration, progressive yielding deceleration,
 * and realistic rounded cornering kinematics.
 * \param elapsed Elapsed time since last frame (ms).
 */
bool GenericCar::doMove(uint32_t elapsed) {
    if (dest_path_.empty()) {
        currentSpeed_ = 0;
        targetSpeed_ = 0;
        stop();
        return false;
    }

    if (currentSpeed_ == 0 && speed() > 0) {
        currentSpeed_ = speed();
    }

    double dt = (double)elapsed / 1000.0;

    // 1. Determine target speed based on blockers, cornering, and straightaways
    bool blocked = isBlocked() || checkForBlockers(true);

    if (blocked) {
        targetSpeed_ = 0; // Decelerate / brake progressively

        // If very close to blocker, halt immediately to avoid touching
        if (hold_on_.pathBlocker != nullptr) {
            int cx = pos_.tx * 256 + pos_.ox;
            int cy = pos_.ty * 256 + pos_.oy;
            int bx = hold_on_.tilex * 256 + hold_on_.xadj;
            int by = hold_on_.tiley * 256 + hold_on_.yadj;
            int dx = bx - cx;
            int dy = by - cy;
            if (dx * dx + dy * dy <= (110 * 110)) {
                currentSpeed_ = 0;
                setSpeed(0);
                return false;
            }
        }
    } else {
        double distToCorner = 0.0;
        if (isUpcomingWaypointCorner(distToCorner)) {
            targetSpeed_ = kCornerCarSpeed; // Decelerate for corner
        } else {
            targetSpeed_ = maxSpeed(); // Cruising speed on straightaways
        }

        // If approaching a final destination (no subsequent driving loop), decelerate progressively to stop
        if (!hasSubsequentDrivingAction()) {
            double distToFinal = 0.0;
            int prevX = pos_.tx * 256 + pos_.ox;
            int prevY = pos_.ty * 256 + pos_.oy;
            for (const auto &pt : dest_path_) {
                int curX = pt.tx * 256 + pt.ox;
                int curY = pt.ty * 256 + pt.oy;
                int dx = curX - prevX;
                int dy = curY - prevY;
                distToFinal += sqrt((double)(dx * dx + dy * dy));
                prevX = curX;
                prevY = curY;
            }
            if (distToFinal <= (double)kCornerSlowdownDistance) {
                double ratio = distToFinal / (double)kCornerSlowdownDistance;
                int finalTarget = (int)(kMinCarMovingSpeed + (maxSpeed() - kMinCarMovingSpeed) * ratio);
                if (finalTarget < targetSpeed_) {
                    targetSpeed_ = finalTarget;
                }
            }
        }
    }

    // 2. Adjust currentSpeed_ towards targetSpeed_ smoothly
    if (currentSpeed_ < targetSpeed_) {
        currentSpeed_ += (int)(accelRate_ * dt);
        if (currentSpeed_ > targetSpeed_) currentSpeed_ = targetSpeed_;
    } else if (currentSpeed_ > targetSpeed_) {
        currentSpeed_ -= (int)(kCarDecelRate * dt);
        if (currentSpeed_ < targetSpeed_) currentSpeed_ = targetSpeed_;
    }

    setSpeed(currentSpeed_);

    // If fully stopped due to blocker or destination reached
    if (currentSpeed_ <= 0) {
        return false;
    }

    bool updated = false;
    int used_time = static_cast<int>(elapsed);

    while (!dest_path_.empty() && used_time > 0 && currentSpeed_ > 0) {
        const TilePoint &nextPt = dest_path_.front();
        int adx = nextPt.tx * 256 + nextPt.ox;
        int ady = nextPt.ty * 256 + nextPt.oy;
        int diffx = adx - (pos_.tx * 256 + pos_.ox);
        int diffy = ady - (pos_.ty * 256 + pos_.oy);

        if (abs(diffx) >= 8 || abs(diffy) >= 8) {
            // Look ahead along dest_path_ (~64 world units) when updating facing direction so
            // short Bezier arc chords and integer truncation near 45 deg turn monotonically.
            const int curWx = pos_.tx * 256 + pos_.ox;
            const int curWy = pos_.ty * 256 + pos_.oy;
            double prevWx = static_cast<double>(curWx);
            double prevWy = static_cast<double>(curWy);
            double remLook = 64.0;
            int lookDx = diffx;
            int lookDy = diffy;
            for (const TilePoint &wp : dest_path_) {
                const double wpx = static_cast<double>(wp.tx * 256 + wp.ox);
                const double wpy = static_cast<double>(wp.ty * 256 + wp.oy);
                const double segLen = std::hypot(wpx - prevWx, wpy - prevWy);
                if (segLen >= remLook && segLen > 1e-6) {
                    const double t = remLook / segLen;
                    lookDx = static_cast<int>(std::lround(prevWx + (wpx - prevWx) * t)) - curWx;
                    lookDy = static_cast<int>(std::lround(prevWy + (wpy - prevWy) * t)) - curWy;
                    break;
                }
                remLook -= segLen;
                prevWx = wpx;
                prevWy = wpy;
                lookDx = wp.tx * 256 + wp.ox - curWx;
                lookDy = wp.ty * 256 + wp.oy - curWy;
            }
            if (lookDx != 0 || lookDy != 0) {
                setDirection(lookDx, lookDy, &dir_);
            } else {
                setDirection(diffx, diffy, &dir_);
            }

            int dx = 0, dy = 0;
            double d = sqrt((double)(diffx * diffx + diffy * diffy));
            // This is the time for all the remaining distance to the node
            double avail_time_use = (d / (double)currentSpeed_) * 1000.0;
            // correcting time available regarding the time we have
            if (avail_time_use > used_time)
                avail_time_use = used_time;

            // computes distance travelled by vehicle in the available time
            if (abs(diffx) > 0)
                dx = (int)((diffx * (currentSpeed_ * avail_time_use) / d) / 1000);
            if (abs(diffy) > 0)
                dy = (int)((diffy * (currentSpeed_ * avail_time_use) / d) / 1000);

            // Updates the available time
            if (dx || dy) {
                int prv_time = used_time;
                if (dx) {
                    used_time -= (int)(((double) dx * 1000.0 * d)
                        / (double)(diffx * currentSpeed_));
                } else if (dy) {
                    used_time -= (int)(((double) dy * 1000.0 * d)
                        / (double)(diffy * currentSpeed_));
                } else {
                    used_time = 0;
                }
                if (used_time < 0 || prv_time == used_time)
                    used_time = 0;
            } else {
                used_time = 0;
            }

            addOffsetToPosition(dx, dy);

            // Recompute diff after movement (addOffsetToPosition may have changed tx/ty)
            diffx = adx - (pos_.tx * 256 + pos_.ox);
            diffy = ady - (pos_.ty * 256 + pos_.oy);
        }

        // Waypoint arrival check
        if (abs(diffx) < 14 && abs(diffy) < 14) {
            pos_.tx = nextPt.tx; pos_.ty = nextPt.ty;
            pos_.ox = nextPt.ox; pos_.oy = nextPt.oy;
            dest_path_.pop_front();
            if (dest_path_.empty()) {
                if (!hasSubsequentDrivingAction()) {
                    stop();
                    currentSpeed_ = 0;
                    targetSpeed_ = 0;
                }
            }
        }

        updated = true;
    }

    if (dest_path_.empty() && isMoving()) {
        if (!hasSubsequentDrivingAction()) {
            stop();
            currentSpeed_ = 0;
            targetSpeed_ = 0;
        }
    }

    // Update passengers position to be in sync with car position
    for (auto passenger : passengers_) {
        passenger->setPosition(pos_);
    }

    return updated;
}


/*!
 * Method called when object is hit by a weapon shot.
 * \param d Damage description
 */
void GenericCar::handleHit(DamageToInflict &d) {
    if (health_ <= 0)
        return;

    decreaseHealth(d.dvalue);
    if (health_ == 0) {
        clearDestination();
        animationPlayer_->play(burningAnimation_);
        pDriver_ = NULL;
        PedInstance *pShooter = dynamic_cast<PedInstance *>(d.d_owner);
        while (passengers_.size() != 0)
        {
            PedInstance *p = *(passengers_.begin());
            dropPassenger(p);
            p->destroyAllActions(true);
            p->clearScriptSteps();
            p->addToDefaultActions(new WalkToDirectionAction());
            p->addMovementAction(p->defaultAction(), false);
            if (p->type() == PedInstance::kPedTypeCivilian && pShooter) {
                p->behaviour().handleBehaviourEvent(Behaviour::kBehvEvtWeaponOut, pShooter);
                p->behaviour().handleBehaviourEvent(Behaviour::kBehvEvtHit, &d);
            }
        }
    } else if (pDriver_ != NULL && !pDriver_->isOurAgent()) {
        // in case the car is drived by someone else than our agents
        // and one of our agent shot the car then
        // the driver is ejected from the car
        // Usually he is alone in the car so don't bother with any passengers
        PedInstance *pShooter = dynamic_cast<PedInstance *>(d.d_owner);
        if (pShooter && pShooter->isOurAgent()) {
            PedInstance *pPed = pDriver_;
            dropPassenger(pPed);
            // Cancel and clear scripted actions so ped is no longer locked into scenario
            pPed->destroyAllActions(true);
            pPed->clearScriptSteps();
            // Make the ped start walking
            pPed->addToDefaultActions(new WalkToDirectionAction());
            pPed->addMovementAction(pPed->defaultAction(), false);
            if (pPed->type() == PedInstance::kPedTypeCivilian) {
                pPed->behaviour().handleBehaviourEvent(Behaviour::kBehvEvtWeaponOut, pShooter);
                pPed->behaviour().handleBehaviourEvent(Behaviour::kBehvEvtHit, &d);
            }
        }
    }
}

/*!
 * Adds the given ped to the passenger but if the vehicle
 * has no driver, ped becomes the driver.
 * If our agent enters, any NPC driver is ejected.
 * \param p The ped
 */
void GenericCar::addPassenger(PedInstance *p) {
    if (p != nullptr && p->isOurAgent()) {
        // If there is an existing non-player driver, eject them so they alight the vehicle
        if (pDriver_ != nullptr && !pDriver_->isOurAgent() && pDriver_ != p) {
            PedInstance *pOldDriver = pDriver_;
            dropPassenger(pOldDriver);
            pOldDriver->destroyAllActions(true);
            pOldDriver->clearScriptSteps();
            pOldDriver->addToDefaultActions(new WalkToDirectionAction());
            pOldDriver->addMovementAction(pOldDriver->defaultAction(), false);
            if (pOldDriver->type() == PedInstance::kPedTypeCivilian) {
                pOldDriver->behaviour().handleBehaviourEvent(Behaviour::kBehvEvtWeaponOut, p);
            }
        }
        if (pDriver_ == nullptr || !pDriver_->isOurAgent()) {
            pDriver_ = p;
        }
    }

    Vehicle::addPassenger(p);

    if (pDriver_ == NULL && !p->isPersuaded()) {
        // Ped becomes the driver
        pDriver_ = p;
    }
}

/*!
 * Overload initial method to manage driver.
 * \param pPed The ped to remove.
 */
void GenericCar::dropPassenger(PedInstance *pPed) {
    Vehicle::dropPassenger(pPed);
    if (pDriver_ == pPed) {
        pDriver_ = NULL;
        clearDestination();

        // find another driver in the remaining passengers
        for (std::list<PedInstance *>::iterator it = passengers_.begin();
            it != passengers_.end(); it++) {
            // take the first non persuaded
            if (!(*it)->isPersuaded()) {
                pDriver_ = *it;
                break;
            }
        }
    }
}

/**
 * Set this ped as the driver of the vehicle and add him as a passenger
 * if he's not already in the vehicle.
 * \param pPed PedInstance*
 * \param forceDriver bool if true, set the driver even if there is already
 * another driver
 * \return void
 *
 */
void Vehicle::setDriver(PedInstance *pPed, bool forceDriver) {
    if (pPed != NULL) {
        if (pDriver_ != nullptr && pDriver_ != pPed) {
            if (!pDriver_->isOurAgent() && (pPed->isOurAgent() || forceDriver)) {
                PedInstance *pOldDriver = pDriver_;
                pDriver_ = nullptr;
                dropPassenger(pOldDriver);
                pOldDriver->destroyAllActions(true);
                pOldDriver->addToDefaultActions(new WalkToDirectionAction());
            }
        }

        if (pDriver_ == NULL || forceDriver) {
            pDriver_ = pPed;
        }

        if (!containsPed(pPed)) {
            Vehicle::addPassenger(pPed);
        }
    } else {
        if (pDriver_ != nullptr) {
            PedInstance *pOldDriver = pDriver_;
            pDriver_ = nullptr;
            dropPassenger(pOldDriver);
        }
    }
}

}