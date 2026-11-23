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
 *   Copyright (C) 2016, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-kernel/model/train.h"
#include <vector>
#include <utility>
#include <cmath>

#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/squad.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/ia/actions.h"
#include "rs-kernel/model/pathsurfaces.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-utils/log/log.h"

namespace rs_knl {

const int TrainHead::kTrainRampDistance;
const int TrainHead::kTrainMinRampSpeed;

TrainBody::TrainBody(uint16_t anId, uint8_t aType, Map *pMap, int startHp, bool isMoveOnXAxis, int maxSpeed) :
    Vehicle(anId, aType, pMap, maxSpeed) {

    setHealth(startHp);
    setStartHealth(startHp);

    pNextBody_ = NULL;
    moveOnXaxis_ = isMoveOnXAxis;
}

TrainBody::~TrainBody() {
}

void TrainBody::dropAllPassengers(const Mission &mission, const TilePoint &dropPos) {
    LOG(Log::k_FLG_GAME, "TrainBody", "dropAllPassengers", ("dropAllPassengers for carriage %d, passengers count: %d", id(), (int)passengers_.size()))
    TilePoint platformPos = findWalkablePlatformTile(mission, dropPos);

    auto it = passengers_.begin();
    while (it != passengers_.end()) {
        PedInstance *pPed = *it;
        if (pPed->isOurAgent() || pPed->isPersuaded()) {
            ++it;
            continue;
        }

        LOG(Log::k_FLG_GAME, "TrainBody", "dropAllPassengers", ("Dropping ped %d (type %d) onto platform tile (%d,%d,%d)", pPed->id(), pPed->type(), platformPos.tx, platformPos.ty, platformPos.tz))
        pPed->leaveVehicle();
        pPed->setPosition(platformPos);
        pPed->setTrainBoardingCooldown(15000); // 15 seconds cooldown
        
        int dx = platformPos.tx - dropPos.tx;
        int dy = platformPos.ty - dropPos.ty;
        if (dx == 0 && dy == 0) {
            if (isMovementOnXAxis()) {
                dy = 1;
            } else {
                dx = 1;
            }
        }
        int targetTx = platformPos.tx + dx * 3;
        int targetTy = platformPos.ty + dy * 3;
        if (targetTx < 0) targetTx = 0;
        if (targetTx >= mission.mmax_x_) targetTx = mission.mmax_x_ - 1;
        if (targetTy < 0) targetTy = 0;
        if (targetTy >= mission.mmax_y_) targetTy = mission.mmax_y_ - 1;

        TilePoint targetTp(targetTx, targetTy, platformPos.tz, 128, 128);
        WorldPoint targetW(targetTp);
        pPed->addToDefaultActions(new WalkToDirectionAction(targetW));

        if (pPed->currentAction() == nullptr) {
            pPed->setCurrentActionWithSource(Action::kActionDefault);
        }

        it = passengers_.erase(it);
    }
}

void TrainBody::dropPassenger(PedInstance *pPed) {
    if (MissionManager::singletonPtr() != nullptr) {
        Mission *pMission = g_missionCtrl.mission();
        if (pMission) {
            TilePoint platformPos = findWalkablePlatformTile(*pMission, position());
            pPed->setPosition(platformPos);
            pPed->setTrainBoardingCooldown(15000); // 15 seconds cooldown
        }
    }
    Vehicle::dropPassenger(pPed);
}

void TrainBody::handleHit(DamageToInflict &d) {
    if (health_ <= 0) return;
    decreaseHealth(d.dvalue);
    if (health_ == 0) {
        if (animationPlayer_) {
            animationPlayer_->play(burningAnimation_);
        }
        if (MissionManager::singletonPtr() != nullptr) {
            Mission *pMission = g_missionCtrl.mission();
            if (pMission) {
                dropAllPassengers(*pMission, position());
            }
        }
    }
}

static uint8_t getDirectionBitMask(int dx, int dy) {
    if (dx == 0 && dy == 1) return 0x01;
    if (dx == 1 && dy == 1) return 0x02;
    if (dx == 1 && dy == 0) return 0x04;
    if (dx == 1 && dy == -1) return 0x08;
    if (dx == 0 && dy == -1) return 0x10;
    if (dx == -1 && dy == -1) return 0x20;
    if (dx == -1 && dy == 0) return 0x40;
    if (dx == -1 && dy == 1) return 0x80;
    return 0;
}

struct PathState {
    int tx, ty, tz;
};

static int getReachableAreaSize(const Mission &mission, int startX, int startY, int startZ, int trackX, int trackY, bool moveOnXaxis) {
    int max_xyz = mission.mmax_x_ * mission.mmax_y_ * mission.mmax_z_;
    std::vector<bool> visited(max_xyz, false);
    std::vector<PathState> queue;
    
    queue.push_back({startX, startY, startZ});
    visited[startX + startY * mission.mmax_x_ + startZ * mission.mmax_m_xy] = true;
    
    size_t head = 0;
    while (head < queue.size() && queue.size() < 100) {
        PathState curr = queue[head++];
        int cx = curr.tx;
        int cy = curr.ty;
        int cz = curr.tz;
        
        FloodNode *currNode = &(mission.mdpoints_[cx + cy * mission.mmax_x_ + cz * mission.mmax_m_xy]);
        
        int dxs[] = {0, 1, 1, 1, 0, -1, -1, -1};
        int dys[] = {1, 1, 0, -1, -1, -1, 0, 1};
        
        for (int dz = -1; dz <= 1; ++dz) {
            int nz = cz + dz;
            if (nz < 0 || nz >= mission.mmax_z_) continue;
            
            uint8_t transitions = 0;
            if (dz == -1) transitions = currNode->dirsBelow;
            else if (dz == 0) transitions = currNode->dirsSame;
            else if (dz == 1) transitions = currNode->dirsAbove;
            
            if (transitions == 0) continue;
            
            for (int i = 0; i < 8; ++i) {
                int dx = dxs[i];
                int dy = dys[i];
                uint8_t mask = getDirectionBitMask(dx, dy);
                
                if (transitions & mask) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    
                    // Forbid crossing/stepping onto the track axis
                    if (moveOnXaxis) {
                        if (ny == trackY) continue;
                    } else {
                        if (nx == trackX) continue;
                    }
                    
                    if (nx >= 0 && nx < mission.mmax_x_ && ny >= 0 && ny < mission.mmax_y_) {
                        int idx = nx + ny * mission.mmax_x_ + nz * mission.mmax_m_xy;
                        if (!visited[idx]) {
                            FloodNode *nd = &(mission.mdpoints_[idx]);
                            if ((nd->flags & kWalkable) != 0) {
                                visited[idx] = true;
                                queue.push_back({nx, ny, nz});
                            }
                        }
                    }
                }
            }
        }
    }
    return (int)queue.size();
}

TilePoint TrainBody::findWalkablePlatformTile(const Mission &mission, const TilePoint &dropPos) {
    TilePoint bestTp = dropPos;
    int bestScore = -1;
    int bestReach = -1;
    double bestDist = 99999.0;
    
    int tz = dropPos.tz + 1; // Platform level

    // Search in a 5x5 area around dropPos at platform level
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            // Must step off the track axis to avoid dropping under the train
            if (moveOnXaxis_ ? (dy == 0) : (dx == 0)) {
                continue;
            }

            int tx = dropPos.tx + dx;
            int ty = dropPos.ty + dy;
            
            if (tx >= 0 && tx < mission.mmax_x_ && ty >= 0 && ty < mission.mmax_y_) {
                FloodNode *targetd = &(mission.mdpoints_[tx + ty * mission.mmax_x_ + tz * mission.mmax_m_xy]);
                bool walkable = ((targetd->flags & kWalkable) != 0);
                int score = 0;
                int reach = 0;
                
                if (walkable) {
                    reach = getReachableAreaSize(mission, tx, ty, tz, dropPos.tx, dropPos.ty, moveOnXaxis_);
                    // Calculate score = number of walkable tiles in 5x5 neighborhood
                    for (int nx = -2; nx <= 2; nx++) {
                        for (int ny = -2; ny <= 2; ny++) {
                            int ntx = tx + nx;
                            int nty = ty + ny;
                            if (ntx >= 0 && ntx < mission.mmax_x_ && nty >= 0 && nty < mission.mmax_y_) {
                                FloodNode *nd = &(mission.mdpoints_[ntx + nty * mission.mmax_x_ + tz * mission.mmax_m_xy]);
                                if ((nd->flags & kWalkable) != 0) {
                                    score++;
                                }
                            }
                        }
                    }
                }
                
                double dist = sqrt((double)(dx * dx + dy * dy));
                
                if (walkable) {
                    bool isBetter = false;
                    if (reach > bestReach) {
                        isBetter = true;
                    } else if (reach == bestReach) {
                        if (score > bestScore) {
                            isBetter = true;
                        } else if (score == bestScore && dist < bestDist) {
                            isBetter = true;
                        }
                    }

                    if (isBetter) {
                        bestReach = reach;
                        bestScore = score;
                        bestDist = dist;
                        bestTp = dropPos;
                        bestTp.tx = tx;
                        bestTp.ty = ty;
                        bestTp.tz = tz;
                    }
                }
            }
        }
    }
    
    if (bestScore != -1) {
        return bestTp;
    }
    
    // Fallback: search at tracks level (Z = dropPos.tz)
    bestScore = -1;
    bestReach = -1;
    bestDist = 99999.0;
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            // Must step off the track axis to avoid dropping under the train
            if (moveOnXaxis_ ? (dy == 0) : (dx == 0)) {
                continue;
            }

            int tx = dropPos.tx + dx;
            int ty = dropPos.ty + dy;
            int fallback_tz = dropPos.tz;
            
            if (tx >= 0 && tx < mission.mmax_x_ && ty >= 0 && ty < mission.mmax_y_) {
                FloodNode *targetd = &(mission.mdpoints_[tx + ty * mission.mmax_x_ + fallback_tz * mission.mmax_m_xy]);
                bool walkable = ((targetd->flags & kWalkable) != 0);
                int score = 0;
                int reach = 0;
                
                if (walkable) {
                    reach = getReachableAreaSize(mission, tx, ty, fallback_tz, dropPos.tx, dropPos.ty, moveOnXaxis_);
                    for (int nx = -2; nx <= 2; nx++) {
                        for (int ny = -2; ny <= 2; ny++) {
                            int ntx = tx + nx;
                            int nty = ty + ny;
                            if (ntx >= 0 && ntx < mission.mmax_x_ && nty >= 0 && nty < mission.mmax_y_) {
                                FloodNode *nd = &(mission.mdpoints_[ntx + nty * mission.mmax_x_ + fallback_tz * mission.mmax_m_xy]);
                                if ((nd->flags & kWalkable) != 0) {
                                    score++;
                                }
                            }
                        }
                    }
                }
                
                double dist = sqrt((double)(dx * dx + dy * dy));
                
                if (walkable) {
                    bool isBetter = false;
                    if (reach > bestReach) {
                        isBetter = true;
                    } else if (reach == bestReach) {
                        if (score > bestScore) {
                            isBetter = true;
                        } else if (score == bestScore && dist < bestDist) {
                            isBetter = true;
                        }
                    }

                    if (isBetter) {
                        bestReach = reach;
                        bestScore = score;
                        bestDist = dist;
                        bestTp = dropPos;
                        bestTp.tx = tx;
                        bestTp.ty = ty;
                        bestTp.tz = fallback_tz;
                    }
                }
            }
        }
    }
    
    if (bestScore != -1) {
        return bestTp;
    }
    
    return dropPos; // Absolute fallback
}

void TrainBody::changeTrainAndPassengersPosition(int distanceX, int distanceY) {
    addOffsetToPosition(distanceX, distanceY);

    if (!passengers_.empty()) {
        for (std::list<PedInstance *>::iterator it = passengers_.begin();
            it != passengers_.end(); it++
        ) {
            (*it)->setPosition(pos_);
        }
    }

    if (pNextBody_ != NULL) {
        pNextBody_->changeTrainAndPassengersPosition(distanceX, distanceY);
    }
}

TrainHead::TrainHead(uint16_t anId, uint8_t aType, Map *pMap, int startHp, bool isMoveOnXAxis, int maxSpeed) :
    TrainBody(anId, aType, pMap, startHp, isMoveOnXAxis, maxSpeed),
    distanceTraveled_(0),
    totalDistanceToDestination_(0) {}

TrainHead::~TrainHead() {

}

//! Set the destination to reach at given speed
bool TrainHead::initMovementToDestination([[maybe_unused]] Mission *m, [[maybe_unused]] const TilePoint &destinationPt) {
    if (health_ <= 0) {
        return false;
    }
    TrainBody *pCheck = getNext();
    while (pCheck != nullptr) {
        if (pCheck->health() <= 0) {
            return false;
        }
        pCheck = pCheck->getNext();
    }

    clearDestination();

    dest_path_.push_front(destinationPt);

    WorldPoint destW(destinationPt);
    WorldPoint currW(pos_);
    totalDistanceToDestination_ = isMovementOnXAxis() ? std::abs(destW.x - currW.x) : std::abs(destW.y - currW.y);
    distanceTraveled_ = 0;

    // Start from rest; doMove() applies the ramp and sets speed dynamically.
    stop();

    return true;
}

/*!
 * Moves a vehicle on the map.
 * \param elapsed Elapsed time since last frame.
 */
bool TrainHead::doMove(uint32_t elapsed)
{
    if (health_ <= 0) {
        stop();
        clearDestination();
        return false;
    }
    TrainBody *pCheck = getNext();
    while (pCheck != nullptr) {
        if (pCheck->health() <= 0) {
            stop();
            clearDestination();
            return false;
        }
        pCheck = pCheck->getNext();
    }

    bool updated = false;
    uint32_t remainingTime = elapsed;

    while ((!dest_path_.empty()) && remainingTime != 0) {

        // Get 1D distance along movement axis between train and destination
        WorldPoint destination(dest_path_.front());
        WorldPoint currentPos(pos_);
        int diff = isMovementOnXAxis() ? (destination.x - currentPos.x) : (destination.y - currentPos.y);
        double distanceToNextNode = std::abs((double)diff);

        if (distanceToNextNode < 4.0) {
            stopIfDestinationReached(destination);
            updated = true;
            break;
        }

        // --- Linear accel/decel ramp ---
        // Ramp-up: speed increases from 0 to maxSpeed over kTrainRampDistance wu from departure.
        // Ramp-down: speed decreases from maxSpeed to 0 over kTrainRampDistance wu before arrival.
        // Both ramps are clamped to kTrainMinRampSpeed so the train never fully stalls.
        double ramp = (double)kTrainRampDistance;
        double accelFactor = (distanceTraveled_ < kTrainRampDistance)
            ? ((double)distanceTraveled_ / ramp)
            : 1.0;

        // For decel, use distance remaining across the whole journey (not just this segment),
        // so multi-hop paths decel correctly when approaching the final stop.
        double distRemaining = (totalDistanceToDestination_ > 0)
            ? ((double)totalDistanceToDestination_ - (double)distanceTraveled_)
            : distanceToNextNode;
        if (distRemaining < 0.0) distRemaining = 0.0;

        double decelFactor = (distRemaining < ramp)
            ? (distRemaining / ramp)
            : 1.0;

        double factor = std::min(accelFactor, decelFactor);
        int effectiveSpeed = (int)(maxSpeed() * factor);
        if (effectiveSpeed < kTrainMinRampSpeed) effectiveSpeed = kTrainMinRampSpeed;

        // Apply effective speed for this frame.
        setSpeed(effectiveSpeed);

        // This is the time for all the remaining distance to the next node in the path
        double availableTimeToNextNode = (distanceToNextNode / (double)effectiveSpeed) * 1000.0;
        // We cannot spend more time than the time remaining
        if (availableTimeToNextNode > remainingTime)
            availableTimeToNextNode = remainingTime;

        int step = (int)((diff * (effectiveSpeed * availableTimeToNextNode) / distanceToNextNode) / 1000);
        if (std::abs(step) > (int)distanceToNextNode) {
            step = (diff > 0) ? (int)distanceToNextNode : -(int)distanceToNextNode;
        }

        int distanceX = isMovementOnXAxis() ? step : 0;
        int distanceY = isMovementOnXAxis() ? 0 : step;

        // Track cumulative distance for the ramp.
        int stepDistance = std::abs(step);
        distanceTraveled_ += stepDistance;

        // Updates the remaining time
        remainingTime -= static_cast<uint32_t>(availableTimeToNextNode);

        // Moves vehicle
        changeTrainAndPassengersPosition(distanceX, distanceY);

        stopIfDestinationReached(destination);

        updated = true;
    }

    return updated;
}

void TrainHead::stopIfDestinationReached(const WorldPoint &destinationPt) {
    bool doStop = false;
    WorldPoint currentPos(pos_);

    doStop = isMovementOnXAxis() ?
                abs(destinationPt.x - currentPos.x) < 4:
                abs(destinationPt.y - currentPos.y) < 4;

    if(doStop) {
        dest_path_.pop_front();
        stop();

        if (MissionManager::singletonPtr() != nullptr) {
            Mission *pMission = g_missionCtrl.mission();
            if (pMission) {
                TrainBody *pBody = getNext();
                while (pBody != nullptr && pBody->getType() != Vehicle::kVehicleTypeTrainHead) {
                    pBody->dropAllPassengers(*pMission, pBody->position());
                    pBody = pBody->getNext();
                }
            }
        }
    }
}

bool TrainHead::containsPed(PedInstance *p) {
    if (!p) return false;
    if (Vehicle::containsPed(p)) return true;
    TrainBody *pBody = getNext();
    while (pBody != nullptr && pBody != this) {
        if (pBody->containsPed(p)) return true;
        pBody = pBody->getNext();
    }
    return false;
}

void TrainHead::handleHit(DamageToInflict &d) {
    if (health_ <= 0) return;
    decreaseHealth(d.dvalue);
    if (health_ == 0) {
        clearDestination();
        setSpeed(0);
        if (animationPlayer_) {
            animationPlayer_->play(burningAnimation_);
        }
        if (MissionManager::singletonPtr() != nullptr) {
            Mission *pMission = g_missionCtrl.mission();
            if (pMission) {
                dropAllPassengers(*pMission, position());
                TrainBody *pBody = getNext();
                while (pBody != nullptr) {
                    pBody->setSpeed(0);
                    pBody->dropAllPassengers(*pMission, pBody->position());
                    pBody = pBody->getNext();
                }
            }
        }
    }
}

void TrainHead::appendTrainBody(TrainBody *pTrainBodyToAdd) {
    TrainBody *pBody = this;
    while (pBody->getNext() != NULL) {
        pBody = pBody->getNext();
    }
    pBody->setNext(pTrainBodyToAdd);
}

}
