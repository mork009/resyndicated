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

#include "rs-kernel/ia/actions.h"

#include "rs-utils/log/log.h"
#include "rs-engine/sound/soundmanager.h"
#include "rs-engine/events/event.h"
#include "rs-kernel/model/weapon.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/train.h"
#include "rs-kernel/model/squad.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/ia/behaviour.h"

namespace rs_knl {
//*************************************
// Constant definition
//*************************************
const int FollowAction::kFollowDistance = 192;
const int WalkBurnHitAction::kTimeToWalkBurning = 6000;
const int WalkBurnHitAction::kDeathScreamDelay = 3000;
const int FireWeaponAction::kMinAutomaticShootingTime = 500;
const int FireWeaponAction::kMaxAutomaticShootingTime = 2000;
const uint8_t ShootAction::kShootActionNotAdded = 0;
const uint8_t ShootAction::kShootActionAutomaticShoot = 1;
const uint8_t ShootAction::kShootActionSingleShoot = 2;

/*!
 * Default constructor.
 * \param aType What type of action.
 * \param origin Who has created this action.
 */
Action::Action(ActionType aType) : timeToWaitTimer_(0), animWaitTimer_(0) {
    status_ = kActStatusNotStarted;
    type_ = aType;
    source_ = kActionNotScripted;
}

/*!
 * Reset the action.
 */
void Action::reset() {
    status_ = kActStatusNotStarted;
}

/*!
 * Default constructor.
 * \param aType What type of action.
 * \param exclusive Does action allow shooting
 * \param canExecVehicle Is action is allowed while ped is in vehicle
 */
MovementAction::MovementAction(ActionType aType, bool exclusive, bool canExecVehicle, bool blocking) :
Action(aType) {
    pNext_ = NULL;
    pPrevious_ = NULL;
    isExclusive_ = exclusive;
    canExecInVehicle_ = canExecVehicle;
    targetState_ = kPedActionStateStanding;
    warnBehaviour_ = false;
    blocking_ = blocking;
}

/*!
 * Action execution.
 * If action is not started :
 *   - if ped is in a vehicle and action cannot be executed inside, fail
 *   - else call doStart()
 * If action has failed, exit.
 * Else change Ped's state to targetState.
 * Calls doExecute() method.
 * If action is finished (failed or succeeded), quits state.
 * \return True to update screen.
 */
bool MovementAction::execute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    if (status_ == kActStatusNotStarted) {
        // Action is not started so start it
        status_ = kActStatusRunning;
        if (pPed->inVehicle() && !canExecInVehicle()) {
            if (type() == kActTypeWalk) {
                WalkAction *pWalk = dynamic_cast<WalkAction *>(this);
                if (pWalk && pWalk->targetObject() != nullptr && pPed->inVehicle() == pWalk->targetObject()) {
                    setSucceeded();
                    return true;
                }
            }
            // Ped is in a vehicle and action cannot be executed in a vehicle
            setFailed();
        } else {
            doStart(pMission, pPed);
        }
        if (status_ == kActStatusFailed) {
            // action failed so nothing changed
            return false;
        } else {
            // action has started correctly -> change state
            pPed->goToState(targetState_);
        }
    }

    if (status_ == kActStatusWaitForAnim && animWaitTimer_.update(elapsed)) {
        setRunning();
    }

    bool update = false;
    if (!isFinished()) {
        update = doExecute(elapsed, pMission, pPed);
    }

    if (isFinished()) {
        pPed->leaveState(targetState_);
    }

    return update;
}

/*! \brief
 * Suspend the action if possible. Subclasses must call this method.
 * By default actions are suspendable but they are not suspended if they
 * are not started.
 * \param pPed PedInstance*
 * \return bool True if action has been suspended or if it was not started.
 *
 */
bool MovementAction::suspend(PedInstance *pPed) {
    if (status_ != kActStatusNotStarted && status_ != kActStatusSuspended) {
        pPed->leaveState(targetState_);
        savedStatus_ = status_;
        status_ = kActStatusSuspended;
    }

    return true;
}

void MovementAction::resume([[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    status_ = savedStatus_;
    pPed->goToState(targetState_);
}

/*!
 * Insert the action between this action and his current next
 * \param pAction MovementAction*
 * \return void
 *
 */
void MovementAction::insertNext(MovementAction *pAction) {
    if (pNext_ != NULL) {
        pAction->link(pNext_);
    }

    this->link(pAction);
}

/*!
 * Insert the action between this action and his current previous.
 * \param pAction MovementAction*
 * \return void
 *
 */
void MovementAction::insertPrevious(MovementAction *pAction) {
    if (pAction != NULL) {
        if (pPrevious_ != NULL) {
            pPrevious_->link(pAction);
        }

        pAction->link(this);
    }
}

void MovementAction::unlinkNext() {
    if (pNext_ != NULL) {
        pNext_->setPrevious(NULL);
        pNext_ = NULL;
    }
}

/*!
 * Remove this action from the chain of actions.
 * Link the previous to the next.
 * \return void
 *
 */
void MovementAction::removeAndJoinChain() {
    MovementAction *pPrev = previous();
    MovementAction *pNext = next();

    // remove link to next
    unlinkNext();
    // remove link to previous
    if (pPrev != NULL) {
        pPrev->unlinkNext();
        // link previous and next
        pPrev->link(pNext);
    }

    pPrevious_ = NULL;
    pNext_ = NULL;
}

WalkAction::WalkAction(const TilePoint &locT) :
    MovementAction(kActTypeWalk) {
    destLocT_ = locT;
    targetState_ = kPedActionStateWalking;
}

/*! \brief
 *
 * \param smo ShootableMapObject*
 * \param speed int
 *
 */
WalkAction::WalkAction(ShootableMapObject *smo) : MovementAction(kActTypeWalk) {
    targetState_ = kPedActionStateWalking;
    setDestination(smo);
}

void WalkAction::setDestination(ShootableMapObject *smo) {
    pTarget_ = smo;
    if (smo) {
        destLocT_ = smo->position();
    }
}

bool WalkAction::suspend(PedInstance *pPed) {
    pPed->stop();
    return MovementAction::suspend(pPed);
}

/*!
 * Resume action by reinitializing the path to destination.
 * @param pMission 
 * @param pPed 
 */
void WalkAction::resume(Mission *pMission, PedInstance *pPed) {
    doStart(pMission, pPed);
    MovementAction::resume(pMission, pPed);
}

void WalkAction::doStart(Mission *pMission, PedInstance *pPed) {
    if (pTarget_ != nullptr && pPed != nullptr && pPed->inVehicle() == pTarget_) {
        setSucceeded();
        return;
    }

    TilePoint targetPos = pTarget_ ? pTarget_->position() : destLocT_;
    if (pTarget_ == nullptr && pPed != nullptr) {
        if (targetPos.tx == pPed->tileX() && targetPos.ty == pPed->tileY() && targetPos.tz == pPed->tileZ()) {
            int dx = targetPos.ox - pPed->position().ox;
            int dy = targetPos.oy - pPed->position().oy;
            if (abs(dx) < 32 && abs(dy) < 32) {
                setSucceeded();
                return;
            }
        }
    }
    if (pMission != nullptr && pMission->map() != nullptr && !pMission->isTileWalkable(targetPos.tx, targetPos.ty, targetPos.tz)) {
            TilePoint bestWalkable = targetPos;
            int bestDistSq = 999999;
            static const int dxy[8][2] = {
                { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
                { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 }
            };
            // Check radius 0 (same tile at nearby Z) first
            for (int dz = -2; dz <= 2; ++dz) {
                int nz = targetPos.tz + dz;
                if (pMission->isTileWalkable(targetPos.tx, targetPos.ty, nz)) {
                    bestDistSq = 0;
                    bestWalkable = TilePoint(targetPos.tx, targetPos.ty, nz, 128, 128, 0);
                    break;
                }
            }
            if (bestDistSq == 999999) {
                for (int r = 1; r <= 3; ++r) {
                    for (int i = 0; i < 8; ++i) {
                        int nx = targetPos.tx + dxy[i][0] * r;
                        int ny = targetPos.ty + dxy[i][1] * r;
                        for (int dz = -2; dz <= 2; ++dz) {
                            int nz = targetPos.tz + dz;
                            if (pMission->isTileWalkable(nx, ny, nz)) {
                                int dx = (nx * 256 + 128) - (pPed->tileX() * 256 + pPed->position().ox);
                                int dy = (ny * 256 + 128) - (pPed->tileY() * 256 + pPed->position().oy);
                                int distSq = dx * dx + dy * dy;
                                if (distSq < bestDistSq) {
                                    bestDistSq = distSq;
                                    bestWalkable = TilePoint(nx, ny, nz, 128, 128, 0);
                                }
                            }
                        }
                    }
                    if (bestDistSq < 999999) break;
                }
            }
            if (bestDistSq < 999999) {
                targetPos = bestWalkable;
            }
        }

    if (!pPed->initMovementToDestination(pMission, targetPos)) {
        if (pTarget_ && pPed->isCloseTo(pTarget_, 768)) {
            setSucceeded();
            return;
        }
        setFailed();
        return;
    }
}

/*!
 * This method first updates movement for the ped.
 * Then if the ped has no more destination point, it means that
 * he has arrived.
 * Else action continue.
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
bool WalkAction::doExecute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    bool updated = pPed->doMove(elapsed);
    if (!pPed->hasDestination()) {
        // Ped has arrived at destination
        setSucceeded();
    }
    return updated;
}

WalkToDirectionAction::WalkToDirectionAction(const WorldPoint &destLocW) :
MovementAction(kActTypeWalk), blockedElapsed_(0) {
    maxDistanceToWalk_ = 0;
    destLocW_ = destLocW;
    targetState_ = kPedActionStateWalking;
}

WalkToDirectionAction::WalkToDirectionAction() :
MovementAction(kActTypeWalk), blockedElapsed_(0) {
    maxDistanceToWalk_ = 0;
    destLocW_.x = -1;
    destLocW_.y = -1;
    destLocW_.z = -1;
    targetState_ = kPedActionStateWalking;
}

bool WalkToDirectionAction::suspend(PedInstance *pPed) {
    pPed->stop();
    return MovementAction::suspend(pPed);
}

void WalkToDirectionAction::doStart([[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    moveDirdesc_.clear();
    moveDirdesc_.bounce = true;
    if (pPed->isInPanic()) {
        moveDirdesc_.safe_walk = false;
    }
    pPed->setSpeedToMax();
    distWalked_ = 0;
    blockedElapsed_ = 0;
}

/*!
 * Ped moves until he reaches destination.
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
bool WalkToDirectionAction::doExecute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    bool endAction = false;
    int distanceToWalk = 0;
    if (destLocW_.x != -1) {
        int diffx = destLocW_.x - pPed->tileX() * 256 - pPed->offX();
        int diffy = destLocW_.y - pPed->tileY() * 256 - pPed->offY();

        // In this case, distance is the distance between ped and destination location
        distanceToWalk = (int)sqrt((double)(diffx * diffx + diffy * diffy));

        if (distanceToWalk > 0) {
            pPed->moveToDir(pMission,
                            elapsed,
                            moveDirdesc_,
                            -1,
                            destLocW_.x, destLocW_.y,
                            &distanceToWalk);

            if (pPed->tileX() * 256 - pPed->offX() == destLocW_.x
                && pPed->tileY() * 256 - pPed->offY() == destLocW_.y)
                // TODO: add correct z or ignore it?
            {
                endAction = true;
            }
        } else {
            endAction = true;
        }
    } else if (maxDistanceToWalk_ != 0) {
        distanceToWalk = maxDistanceToWalk_ - distWalked_;
        pPed->moveToDir(pMission,
                        elapsed,
                        moveDirdesc_,
                        -1,
                        -1, -1,
                        &distanceToWalk, true);

        distWalked_ += distanceToWalk;
        if (distanceToWalk == 0) {
            blockedElapsed_ += elapsed;
        } else {
            blockedElapsed_ = 0;
        }
        endAction = (distWalked_ >= maxDistanceToWalk_) || (blockedElapsed_ >= kMaxBlockedTime);
    } else {
        // just keep moving
        int spd = (pPed->speed() > 0) ? pPed->speed() : pPed->maxSpeed();
        pPed->moveToDir(pMission,
                        elapsed,
                        moveDirdesc_,
                        -1, -1, -1,
                        nullptr, false,
                        spd);
    }

    if (endAction) {
        setSucceeded();
        pPed->clearDestination();
        blockedElapsed_ = 0;
    }

    return true;
}

/*!
 * Constructor.
 * \param range Radius of the trigger zone
 * \param loc Center of the trigger zone
 */
TriggerAction::TriggerAction(int32_t range, const WorldPoint &loc) :
        MovementAction(kActTypeUndefined, false, true) {
    range_ = range;
    centerLoc_ = loc;
}

/*!
 * Check that at least one agent enters the zone.
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
bool TriggerAction::doExecute([[maybe_unused]] uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    if (!pMission || !pPed || !pMission->getSquad()) return false;

    // Check weapon range or ambush awareness range (at least 15 tiles = 3840 units)
    double maxDist = 3840.0;
    WeaponInstance *pW = pPed->selectedWeapon();
    if (!pW) {
        pW = pPed->getBestRankedWeaponWithAmmo();
    }
    if (pW && pW->range() > 0) {
        maxDist = std::max(maxDist, static_cast<double>(pW->range()) * 0.9);
    }

    for (auto it = pMission->getSquad()->alive_begin(); it != pMission->getSquad()->alive_end(); ++it) {
        rs_knl::PedInstance *pAgent = *it;
        if (!pAgent || !pAgent->isAlive() || pAgent->isDead()) continue;

        WorldPoint pedW(pPed->position());
        WorldPoint agentW(pAgent->position());
        double dx = (double)(pedW.x - agentW.x);
        double dy = (double)(pedW.y - agentW.y);
        double dist = sqrt(dx * dx + dy * dy);

        bool triggered = false;
        if (dist <= maxDist) {
            double pathLen = 0.0;
            if (pMission->getPathLengthBetween(pPed, pAgent, maxDist, &pathLen) == 0) {
                triggered = true;
            }
        }
        if (!triggered && range_ > 0 && pAgent->isCloseTo(centerLoc_, range_)) {
            triggered = true;
        }

        if (triggered) {
            setSucceeded();
            if (pPed->type() != PedInstance::kPedTypeCivilian) {
                if (pPed->type() != PedInstance::kPedTypeAgent) {
                    pPed->selectShootingWeaponWithAmmo();
                }
                if (pPed->type() == PedInstance::kPedTypeAgent) {
                    pPed->setIPAAmount(IPAStim::Adrenaline, 100);
                }
                pPed->behaviour().engageHostileTarget(pAgent);
                if (!pPed->behaviour().hasHostileEngaged()) {
                    if (pPed->type() != PedInstance::kPedTypeAgent) {
                        WaitBeforeShootingAction *pWarnAction = new WaitBeforeShootingAction(pAgent);
                        pPed->addToAltActions(pWarnAction);
                    }
                    FollowToShootAction* pFollowAction = new FollowToShootAction(pAgent);
                    pPed->addToAltActions(pFollowAction);
                    pPed->addToAltActions(new FireWeaponAction(pAgent));
                    pPed->addToAltActions(new ResetScriptedAction(Action::kActionAlt));
                    pPed->setCurrentActionWithSource(Action::kActionAlt);
                }
            }
            return true;
        }
    }
    return false;
}

/*!
 * This action only sets the ped's state to "Escaped".
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
bool EscapeAction::doExecute([[maybe_unused]] uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    setSucceeded();
    pMission->escape(pPed);
    return true;
}

/*!
 * Class constructor.
 * \param pTarget The ped to follow.
 */
FollowAction::FollowAction(PedInstance *pTarget) :
MovementAction(kActTypeFollow), orbitAngleTimer_(0) {
    pTarget_ = pTarget;
    targetState_ = kPedActionStateWalking;
}

void FollowAction::updateLastTargetPos() {
    pTargetLastPosRaw_ = pTarget_->position();
    targetLastPos_ = pTargetLastPosRaw_;
}

void FollowAction::updateLastTargetPos(Mission *pMission, PedInstance *pPed) {
    pTargetLastPosRaw_ = pTarget_->position();
    TilePoint base = pTargetLastPosRaw_;
    if (pPed) {
        int id = pPed->id();
        static const int offsets[8][2] = {
            { 0,  1}, { 1,  0}, { 0, -1}, {-1,  0},
            { 1,  1}, { 1, -1}, {-1,  1}, {-1, -1}
        };
        int idx = (id * 3) % 8;
        TilePoint cand = base;
        cand.tx += offsets[idx][0];
        cand.ty += offsets[idx][1];
        if (pMission && pPed->initMovementToDestination(pMission, cand)) {
            targetLastPos_ = cand;
            return;
        }
    }
    targetLastPos_ = base;
}

void FollowAction::updateOrbitTargetPos(Mission *pMission, PedInstance *pPed, uint32_t elapsed) {
    orbitAngleTimer_ += elapsed;
    pTargetLastPosRaw_ = pTarget_->position();
    TilePoint base = pTargetLastPosRaw_;

    if (pPed) {
        int id = pPed->id();
        int leaderX = base.tx * 256 + base.ox;
        int leaderY = base.ty * 256 + base.oy;

        double dirSign = (id % 2 == 0) ? 1.0 : -1.0;
        double omega = dirSign * (1.2 + (id % 4) * 0.15);

        double timeSec = static_cast<double>(orbitAngleTimer_) / 1000.0;
        double angle = (timeSec * omega) + (id * 1.73);
        double aheadAngle = angle + dirSign * 0.45;

        double rx = 110.0 + ((id * 43) % 90);
        double ry = 70.0 + ((id * 61) % 70);
        double tilt = (id * 0.85);

        double uX = rx * std::cos(aheadAngle);
        double uY = ry * std::sin(aheadAngle);

        double rotX = uX * std::cos(tilt) - uY * std::sin(tilt);
        double rotY = uX * std::sin(tilt) + uY * std::cos(tilt);

        int targetX = leaderX + static_cast<int>(std::round(rotX));
        int targetY = leaderY + static_cast<int>(std::round(rotY));

        if (targetX < 0) targetX = 0;
        if (targetY < 0) targetY = 0;

        TilePoint cand;
        cand.tx = targetX / 256;
        cand.ty = targetY / 256;
        cand.tz = base.tz;
        cand.ox = targetX % 256;
        cand.oy = targetY % 256;
        cand.oz = base.oz;

        if (pMission && pPed->initMovementToDestination(pMission, cand)) {
            targetLastPos_ = cand;
            return;
        }
        if (pMission && pPed->initMovementToDestination(pMission, base)) {
            targetLastPos_ = base;
            return;
        }
    }
    targetLastPos_ = base;
}

void FollowAction::doStart(Mission *pMission, PedInstance *pPed) {
    updateLastTargetPos(pMission, pPed);
    if (!pPed->isCloseTo(pTarget_, kFollowDistance)) {
        if (!pPed->initMovementToDestination(pMission, targetLastPos_)) {
            targetLastPos_ = pTarget_->position();
            if (!pPed->initMovementToDestination(pMission, targetLastPos_)) {
                setFailed();
                return;
            }
        }
        targetState_ = kPedActionStateWalking;
        pPed->goToState(kPedActionStateWalking, true);
    } else {
        pPed->clearDestination();
        targetState_ = kPedActionStateStanding;
        pPed->goToState(kPedActionStateStanding, true);
    }
}

bool FollowAction::doExecute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    bool updated = false;

    if (pTarget_->isDead()) {
        pPed->clearDestination();
        pPed->goToState(kPedActionStateStanding, true);
        setFailed();
    } else {
        bool targetMoved = !pTarget_->sameTile(pTargetLastPosRaw_);

        if (targetMoved) {
            orbitAngleTimer_ = 0;
            updateLastTargetPos(pMission, pPed);
            if (pPed->initMovementToDestination(pMission, targetLastPos_)) {
                targetState_ = kPedActionStateWalking;
                pPed->goToState(kPedActionStateWalking, true);
            } else {
                targetLastPos_ = pTarget_->position();
                if (pPed->initMovementToDestination(pMission, targetLastPos_)) {
                    targetState_ = kPedActionStateWalking;
                    pPed->goToState(kPedActionStateWalking, true);
                } else {
                    setFailed();
                }
            }
        } else if (pPed->isPersuaded()) {
            if (pPed->isEvadingVehicle()) {
                pPed->clearDestination();
                targetState_ = kPedActionStateWalking;
            } else {
                updateOrbitTargetPos(pMission, pPed, elapsed);
                if (pPed->hasDestination() && pPed->speed() > 0) {
                    targetState_ = kPedActionStateWalking;
                    pPed->goToState(kPedActionStateWalking, false);
                } else {
                    targetState_ = kPedActionStateStanding;
                    pPed->goToState(kPedActionStateStanding, false);
                }
            }
        } else {
            if (pPed->isCloseTo(pTarget_, kFollowDistance)) {
                if (pPed->speed() != 0 || !pPed->isState(kPedActionStateStanding)) {
                    pPed->clearDestination();
                    targetState_ = kPedActionStateStanding;
                    pPed->goToState(kPedActionStateStanding, true);
                }
            } else if (pPed->speed() == 0 || !pPed->isState(kPedActionStateWalking)) {
                updateLastTargetPos(pMission, pPed);
                if (pPed->initMovementToDestination(pMission, targetLastPos_)) {
                    targetState_ = kPedActionStateWalking;
                    pPed->goToState(kPedActionStateWalking, true);
                }
            }
        }

        if (pPed->speed() != 0) {
            updated = pPed->doMove(elapsed);
        }
    }
    return updated;
}

/*!
 * Class constructor.
 * \param pTarget The ped to follow.
 */
FollowToShootAction::FollowToShootAction(PedInstance *pTarget) :
MovementAction(kActTypeFollowToShoot) {
    pTarget_ = pTarget;
    targetState_ = kPedActionStateWalking;
    followDistance_ = 0;
}

/*!
 * Saves the target current position in the targetLastPos_ field.
 */
void FollowToShootAction::updateLastTargetPos() {
    targetLastPosW_.convertFromTilePoint(pTarget_->position());
}

/*!
 * Init Action :
 * - If the ped's has no weapon, action fails
 * - If the target is close enough, action fails
 * - else init movement towards target
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
void FollowToShootAction::doStart(Mission *pMission, PedInstance *pPed) {
    WeaponInstance *pWeapon = pPed->selectedWeapon();
    if (!pWeapon) {
        pWeapon = pPed->getBestRankedWeaponWithAmmo();
    }
    if (!pWeapon) {
        setFailed();
        return;
    } else {
        followDistance_ = (pWeapon->range() / 3) * 2;
    }

    int wieldThreshold = pWeapon->range() + 256; // ~1 tile before weapon's range
    if (pPed->type() == PedInstance::kPedTypeAgent) {
        if (pPed->isCloseTo(pTarget_, wieldThreshold)) {
            if (pPed->selectedWeapon() == nullptr) {
                pPed->selectShootingWeaponWithAmmo();
            }
        }
    } else {
        if (pPed->selectedWeapon() == nullptr) {
            pPed->selectShootingWeaponWithAmmo();
        }
    }

    updateLastTargetPos();
    WorldPoint pedPosW(pPed->position());
    pedPosW.z += 64;
    WorldPoint targetPosW(pTarget_->position());
    targetPosW.z += 64;
    bool hasLineOfSight = (pMission && pMission->checkBlockedByTile(pedPosW, &targetPosW, false, followDistance_) == 1);

    // If target is close enough AND line of sight is clear, we are already in firing position
    if (pPed->isCloseTo(pTarget_, followDistance_) && hasLineOfSight) {
        if (pPed->selectedWeapon() == nullptr) {
            pPed->selectShootingWeaponWithAmmo();
        }
        setSucceeded();
    } else {
        // Else, start moving towards the target
        if (pMission && !pPed->initMovementToDestination(pMission, pTarget_->position())) {
            setFailed();
        }
    }
}

bool FollowToShootAction::doExecute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    bool updated = false;

    if (!pTarget_ || pTarget_->isDead()) {
        // target is dead so stop moving and terminate action
        pPed->clearDestination();
        setFailed();
    } else {
        // If weapon is not yet drawn, check if we're now close enough (a few tiles before weapon range)
        if (pPed->selectedWeapon() == nullptr) {
            WeaponInstance *pWeapon = pPed->getBestRankedWeaponWithAmmo();
            int wpnRange = pWeapon ? pWeapon->range() : 1280;
            int wieldThreshold = wpnRange + 256; // ~1 tile before weapon range
            if (pPed->isCloseTo(pTarget_, wieldThreshold)) {
                pPed->selectShootingWeaponWithAmmo();
            }
        }

        WorldPoint pedPosW(pPed->position());
        pedPosW.z += 64;
        WorldPoint targetNewPosW(pTarget_->position());
        targetNewPosW.z += 64;
        bool hasLineOfSight = (pMission && pMission->checkBlockedByTile(pedPosW, &targetNewPosW, false, followDistance_) == 1);

        // Ped stops walking if the target is in range of fire (ie close enough and not
        // hiding behing something)
        if (pPed->isCloseTo(pTarget_, followDistance_) && hasLineOfSight) {
            if (pPed->selectedWeapon() == nullptr) {
                pPed->selectShootingWeaponWithAmmo();
            }
            // We reached the target so stop moving
            setSucceeded();
            pPed->clearDestination();
        } else {
            // If target has moved significantly since last time, or if path was finished but LOS is still blocked, update path
            if (!pTarget_->isCloseTo(targetLastPosW_, 256) || (pPed->destPath().empty() && !hasLineOfSight)) {
                updateLastTargetPos();
                pPed->clearDestination();
                if (pMission && !pPed->initMovementToDestination(pMission, pTarget_->position())) {
                    setFailed();
                    return true;
                }
            }
            // move towards target
            updated = pPed->doMove(elapsed);
        }

    }
    return updated;
}

PutdownWeaponAction::PutdownWeaponAction(uint8_t weaponIdx) : MovementAction(kActTypeDrop, true) {
    weaponIdx_ = weaponIdx;
    targetState_ = kPedActionStatePutDown;
}

void PutdownWeaponAction::doStart([[maybe_unused]] Mission *pMission, [[maybe_unused]] PedInstance *pPed) {
    pPed->playPickupOrDropAnimation();
    waitForAnimation();
}

/*!
 * Action can be executed only when the animation of dropping a weapon is done.
 */
bool PutdownWeaponAction::doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (status_ != kActStatusWaitForAnim) {
        WeaponInstance *pWeapon = pPed->dropWeapon(weaponIdx_);
        // Dropping a timebomb means activate it to explode
        if (pWeapon->isInstanceOf(Weapon::TimeBomb)) {
            pWeapon->activate();
        }

        setSucceeded();
    }

    return true;
}

PickupWeaponAction::PickupWeaponAction(WeaponInstance *pWeapon) :
    MovementAction(kActTypePickUp, true) {
    pWeapon_ = pWeapon;
    targetState_ = kPedActionStatePickUp;
}

void PickupWeaponAction::doStart([[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    // recheck here in case weapon was pickup between the time the action
    // was added and now
    if (pWeapon_->hasOwner() || pWeapon_->isDead() ||
        pPed->numWeapons() == WeaponHolder::kMaxHoldedWeapons ||
        !pPed->isCloseTo(pWeapon_, 256)) {
        setFailed();
    } else {
        // the animation must run first then the object will be picked up
        pPed->playPickupOrDropAnimation();
        waitForAnimation();
    }
}

bool PickupWeaponAction::doExecute([[maybe_unused]] uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    if (status_ != kActStatusWaitForAnim) {
        pWeapon_->setOwner(pPed);
        pWeapon_->deactivate();
        pPed->addWeapon(pWeapon_);
        pMission->removeWeaponOnGround(pWeapon_);

        setSucceeded();
    }
    return true;
}

EnterVehicleAction::EnterVehicleAction(Vehicle *pVehicle) :
        MovementAction(kActTypeUndefined, true, true, true) {
    pVehicle_ = pVehicle;
}

void EnterVehicleAction::doStart([[maybe_unused]] Mission *pMission, [[maybe_unused]] PedInstance *pPed) {
    if (pVehicle_ == nullptr || pVehicle_->isDead()) {
        setFailed();
        return;
    }
    if (pPed != nullptr && pPed->inVehicle() == pVehicle_) {
        setSucceeded();
        return;
    }
    if (pPed != nullptr && pPed->isInVehicle()) {
        setFailed();
        return;
    }
}

bool EnterVehicleAction::doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (pPed != nullptr && pPed->inVehicle() == pVehicle_) {
        setSucceeded();
        return true;
    }
    // Ped needs to be near vehicle to get in
    int cz = pVehicle_->tileZ() * 128 + pVehicle_->position().oz;
    int pz = pPed->position().tz * 128 + pPed->position().oz;
    bool zClose = std::abs(cz - pz) <= 256;
    if (pPed->sameTile(pVehicle_->position()) || (zClose && (pPed->isCloseTo(pVehicle_, 768) || pPed->isCloseTo(pVehicle_, 1024)))) {
        // state of ped is set in addPassenger
        pVehicle_->addPassenger(pPed);
        if (auto *pCar = dynamic_cast<GenericCar*>(pVehicle_)) {
            if (pCar->getDriver() == nullptr) {
                pCar->setDriver(pPed, true);
            }
        }
        setSucceeded();
    } else {
        // Vehicle is no longer where the ped expected it to be
        setFailed();
    }
    return true;
}

LeaveVehicleAction::LeaveVehicleAction() :
    MovementAction(kActTypeUndefined, false, true, true) {
}

void LeaveVehicleAction::doStart([[maybe_unused]] Mission *pMission, [[maybe_unused]] PedInstance *pPed) {
}

bool LeaveVehicleAction::doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    Vehicle *v = pPed->inVehicle();
    if (v) {
        if (auto *pCar = dynamic_cast<GenericCar*>(v)) {
            if (pCar->getDriver() == pPed) {
                pCar->setDriver(nullptr);
            }
        }
        v->dropPassenger(pPed);
    }
    setSucceeded();
    return true;
}

DriveVehicleAction::DriveVehicleAction(GenericCar *pVehicle, const TilePoint &dest) :
    MovementAction(kActTypeUndefined, false, true) {
    pVehicle_ = pVehicle;
    dest_ = dest;
}

void DriveVehicleAction::doStart(Mission *pMission, PedInstance *pPed) {
    if (pVehicle_->isDead() || !pVehicle_->containsPed(pPed)) {
        setFailed();
    } else {
        if (pPed->isOurAgent()) {
            pVehicle_->setDriver(pPed, true);
        }
        if (!pVehicle_->initMovementToDestination(pMission, dest_)) {
            setFailed();
            LOG(Log::k_FLG_GAME, "DriveVehicleAction", "doStart", ("No path found for car %d to dest : %d, %d, %d", pVehicle_->id(), dest_.tx, dest_.ty, dest_.tz))
        }
    }
}

bool DriveVehicleAction::doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (pVehicle_ == nullptr || pVehicle_->isDead() || !pVehicle_->containsPed(pPed)) {
        setFailed();
        return true;
    }
    if (!pVehicle_->hasDestination() || pVehicle_->sameTile(dest_)) {
        setSucceeded();
    }
    return true;
}

DriveTrainAction::DriveTrainAction(TrainHead *pTrain, const TilePoint &dest) :
    MovementAction(kActTypeUndefined, false, true) {
    pTrain_ = pTrain;
    dest_ = dest;
}

void DriveTrainAction::doStart(Mission *pMission, PedInstance *pPed) {
    if (!pTrain_ || !pTrain_->containsPed(pPed) || pTrain_->health() <= 0) {
        setFailed();
        return;
    }

    TrainBody *pCheck = pTrain_->getNext();
    while (pCheck != nullptr) {
        if (pCheck->health() <= 0) {
            setFailed();
            return;
        }
        pCheck = pCheck->getNext();
    }

    if (!pTrain_->initMovementToDestination(pMission, dest_)) {
        setFailed();
        return;
    }
}

bool DriveTrainAction::doExecute([[maybe_unused]] uint32_t elapsed, Mission *pMission, [[maybe_unused]] PedInstance *pPed) {
    if (!pTrain_->hasDestination()) {
        setSucceeded();
    }

    return true;
}

void DriveTrainAction::dropAgents(const Mission &mission) {
    // find center of train
    // Train are always five elements long
    TrainBody *pBody = pTrain_->getNext()->getNext();
    TilePoint dropPos = pBody->position();

    // There should be passengers only in train body
    pBody = pTrain_->getNext();
    while (pBody->getType() != Vehicle::kVehicleTypeTrainHead) {
        pBody->dropAllPassengers(mission, dropPos);
        pBody = pBody->getNext();
    }
}

WaitAction::WaitAction(WaitEnum waitFor, uint32_t duration) :
MovementAction(kActTypeWait, true, true), waitTimer_(duration) {
    waitType_ = waitFor;
}

WaitAction::WaitAction(WaitEnum waitFor) :
MovementAction(kActTypeWait, true, true), waitTimer_(0) {
    waitType_ = waitFor;
}

void WaitAction::doStart([[maybe_unused]] Mission *pMission, [[maybe_unused]] PedInstance *pPed) {
    waitTimer_.reset();
}

bool WaitAction::doExecute(uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (waitType_ == kWaitTime) {
        if (waitTimer_.update(elapsed)) {
            setSucceeded();
            return true;
        }
    } else if (waitType_ == kWaitWeapon) {
        if (!pPed->isUsingWeapon()) {
            setSucceeded();
            return true;
        }
    }
    return false;
}

WaitBeforeShootingAction::WaitBeforeShootingAction(PedInstance *pPed) :
MovementAction(kActTypeWaitShoot, true), waitTimer_(2000), warned_(false) {
    pTarget_ = pPed;
}

void WaitBeforeShootingAction::doStart([[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (pTarget_->isDead()) {
        setFailed();
    } else {
        waitTimer_.reset();
        pPed->clearDestination();
        pPed->selectShootingWeaponWithAmmo();
        if (!warned_ && pPed->type() == PedInstance::kPedTypePolice && pTarget_->isOurAgent() && !pPed->isPersuaded()) {
            EventManager::fire<PoliceWarningEmittedEvent>(0);
            warned_ = true;
        }
    }
}

bool WaitBeforeShootingAction::doExecute(uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    // point toward target
    pPed->setDirectionTowardObject(*pTarget_);

    if (!warned_ && pPed->type() == PedInstance::kPedTypePolice && pTarget_->isOurAgent() && !pPed->isPersuaded()) {
        EventManager::fire<PoliceWarningEmittedEvent>(0);
        warned_ = true;
    }

    if (waitTimer_.update(elapsed)) {
        setSucceeded();
        return true;
    }

    return false;
}

FireWeaponAction::FireWeaponAction(PedInstance *pPed) :
MovementAction(kActTypeFire) {
    pTarget_ = pPed;
}

void FireWeaponAction::doStart(Mission *pMission, PedInstance *pPed) {
    if (!pTarget_ || pTarget_->isDead()) {
        setFailed();
    } else if (pTarget_->isEnergyShieldActivated()) {
        // Don't shoot if target has an energy shield
        setFailed();
    } else {
        if (pPed->selectedWeapon() == nullptr) {
            pPed->selectShootingWeaponWithAmmo();
        }
        WorldPoint pedPosW(pPed->position());
        pedPosW.z += 64;
        WorldPoint targetLocW(pTarget_->position());
        targetLocW.z += 64;
        double range = pPed->selectedWeapon() ? static_cast<double>(pPed->selectedWeapon()->range()) : 3584.0;
        if (pMission && pMission->checkBlockedByTile(pedPosW, &targetLocW, false, range) != 1) {
            // Target is blocked by wall/tile - do not shoot into walls!
            setFailed();
            return;
        }

        shootType_ = pPed->addActionShootAt(targetLocW);
        if (shootType_ == ShootAction::kShootActionNotAdded) {
            // failed to shoot because weapon has no ammo
            setFailed();
        } else if (shootType_ == ShootAction::kShootActionAutomaticShoot) {
            // limit the duration of an automatic shooting burst to a random time
            uint32_t shootingTime = kMinAutomaticShootingTime +
                    (rand() % (kMaxAutomaticShootingTime - kMinAutomaticShootingTime + 1));
            waitForTime(shootingTime);
        }
    }
}

bool FireWeaponAction::doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (!pPed->isUsingWeapon()) {
        setSucceeded();
    } else if (status_ == kActStatusWaitForTime && timeToWaitTimer_.update(elapsed)) {
        pPed->stopShooting();
    }

    return true;
}

HitAction::HitAction(DamageToInflict &d) :
        MovementAction(kActTypeHit, true) {
    damage_.aimedLocW = d.aimedLocW;
    damage_.dtype = d.dtype;
    damage_.dvalue = d.dvalue;
    damage_.d_owner = d.d_owner;
    damage_.originLocW = d.originLocW;
    damage_.pWeapon = d.pWeapon;

    targetState_ = kPedActionStateHit;
}

bool HitAction::execute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    if (status_ == kActStatusNotStarted) {
        setRunning();
        pPed->stopShooting();
        pPed->destroyUseWeaponAction();
        doStart(pMission, pPed);
        pPed->goToState(kPedActionStateHit);
        if (!isRunning()) {
            // not running means failed or waiting for next run
            return true;
        }
    }

    if (status_ == kActStatusWaitForAnim && animWaitTimer_.update(elapsed)) {
        setRunning();
    }

    if (isRunning()) {
        doExecute(elapsed, pMission, pPed);
    }

    return true;
}

FallDeadHitAction::FallDeadHitAction(DamageToInflict &d) : HitAction(d) {}

/*!
 *
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
bool FallDeadHitAction::doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (status_ == kActStatusRunning) {
        pPed->stopShooting();
        pPed->destroyUseWeaponAction();
        pPed->takeDamage(damage_);
        pPed->handleDeath(damage_);
        setSucceeded();
    }
    return true;
}

static bool isPedOverVoidSurface(Mission *pMission, PedInstance *pPed) {
    if (!pMission || !pPed) {
        return false;
    }
    const TilePoint &tp = pPed->position();
    if (tp.tz <= 0) {
        return true;
    }
    SurfaceType cur = pMission->surfaceAt(tp.tx, tp.ty, tp.tz);
    SurfaceType below = pMission->surfaceAt(tp.tx, tp.ty, tp.tz - 1);
    return cur == SurfaceType::kEmpty &&
           !SurfaceUtils::isFlatSurface(below) &&
           !SurfaceUtils::isStairs(below);
}

RecoilHitAction::RecoilHitAction(DamageToInflict &d) : HitAction(d) {}

/*!
 *
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
void RecoilHitAction::doStart([[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    pPed->stopShooting();
    pPed->destroyUseWeaponAction();
    pPed->goToState(kPedActionStateHit);
    if (!initialDamageApplied_) {
        initialDamageApplied_ = true;
        pPed->takeDamage(damage_);
    }
    pPed->setDirectionTowardPosition(damage_.originLocW);
    moveDirdesc_.clear();
    moveDirdesc_.bounce = false;
    moveDirdesc_.safe_walk = false;
    recoilDistLeft_ = 288;
    pPed->playHitAnimation();
    phase_ = kRecoilPhaseRecoil;
    waitForAnimation();
}

void RecoilHitAction::onAdditionalHit(const DamageToInflict &d, PedInstance *pPed) {
    if (phase_ == kRecoilPhaseDying || phase_ == kRecoilPhaseSinking) {
        return;
    }

    pPed->stopShooting();
    pPed->destroyUseWeaponAction();
    if (!pPed->isState(kPedActionStateHit)) {
        pPed->goToState(kPedActionStateHit);
    }
    if (!initialDamageApplied_) {
        initialDamageApplied_ = true;
        pPed->takeDamage(damage_);
    }
    DamageToInflict incoming = d;
    pPed->takeDamage(incoming);
    damage_ = d;
    moveDirdesc_.clear();
    moveDirdesc_.bounce = false;
    moveDirdesc_.safe_walk = false;
    recoilDistLeft_ = 288;
    pPed->setDirectionTowardPosition(damage_.originLocW);
    pPed->playHitAnimation();
    phase_ = kRecoilPhaseRecoil;
    waitForAnimation();
}

bool RecoilHitAction::execute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    if (status_ == kActStatusNotStarted) {
        doStart(pMission, pPed);
    }

    if (status_ == kActStatusWaitForAnim || status_ == kActStatusRunning) {
        if (phase_ == kRecoilPhaseRecoil && recoilDistLeft_ > 0) {
            WorldPoint pPos = pPed->position();
            int awayDx = pPos.x - damage_.originLocW.x;
            int awayDy = pPos.y - damage_.originLocW.y;
            int awayDir = 0;
            if (awayDx == 0 && awayDy == 0) {
                awayDir = (pPed->direction() + 128) & 0x00FF;
            } else {
                pPed->setDirection(awayDx, awayDy, &awayDir);
            }
            int stepDist = (int)(elapsed * 1000 / 1000.0);
            int moveDist = std::min(recoilDistLeft_, std::max(1, stepDist));
            int distParam = moveDist;
            bool stepOverVoid = false;
            pPed->moveToDir(pMission, elapsed, moveDirdesc_, awayDir, -1, -1, &distParam, true, 1000, true, &stepOverVoid);
            if (stepOverVoid) {
                overVoid_ = true;
            }
            recoilDistLeft_ -= distParam;
            if (distParam < moveDist) {
                // Hit a solid wall / obstacle: stop further recoil movement for this hit
                recoilDistLeft_ = 0;
            }
            pPed->setDirectionTowardPosition(damage_.originLocW);
            if (recoilDistLeft_ <= 0 && overVoid_) {
                if (isPedOverVoidSurface(pMission, pPed)) {
                    if (pPed->startVoidFall()) {
                        phase_ = kRecoilPhaseSinking;
                        waitForAnimation();
                        animWaitTimer_.reset(2500);
                    } else {
                        pPed->handleVoidFall();
                        setSucceeded();
                    }
                    return true;
                }
                overVoid_ = false;
            }
        } else if (phase_ == kRecoilPhaseSinking && animWaitTimer_.update(elapsed)) {
            setRunning();
        }
    }

    if (status_ == kActStatusRunning) {
        doExecute(elapsed, pMission, pPed);
    }

    return true;
}

/*!
 *
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
bool RecoilHitAction::doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (phase_ == kRecoilPhaseRecoil) {
        if (!initialDamageApplied_) {
            initialDamageApplied_ = true;
            pPed->takeDamage(damage_);
        }
        if (overVoid_ && isPedOverVoidSurface(pMission, pPed)) {
            recoilDistLeft_ = 0;
            if (pPed->startVoidFall()) {
                phase_ = kRecoilPhaseSinking;
                waitForAnimation();
                animWaitTimer_.reset(2500);
            } else {
                pPed->handleVoidFall();
                setSucceeded();
            }
        } else if (pPed->isDead()) {
            pPed->stopShooting();
            pPed->destroyUseWeaponAction();
            pPed->playDyingAnimation();
            phase_ = kRecoilPhaseDying;
            waitForAnimation();
        } else {
            pPed->leaveState(kPedActionStateHit);
            setSucceeded();
        }
    } else if (phase_ == kRecoilPhaseDying) {
        pPed->stopShooting();
        pPed->destroyUseWeaponAction();
        pPed->handleDeath(damage_);
        setSucceeded();
    } else if (phase_ == kRecoilPhaseSinking) {
        pPed->handleVoidFall();
        setSucceeded();
    }
    return true;
}

LaserHitAction::LaserHitAction(DamageToInflict &d) : HitAction(d) {}

/*!
 *
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
void LaserHitAction::doStart([[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    pPed->stopShooting();
    pPed->destroyUseWeaponAction();
    pPed->goToState(kPedActionStateHit);
    pPed->setDirectionTowardPosition(damage_.originLocW);
    pPed->takeDamage(damage_);
    pPed->playVaporizeAnimation();
    waitForAnimation();
}

/*!
 *
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
bool LaserHitAction::doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (isRunning()) {
        pPed->stopShooting();
        pPed->destroyUseWeaponAction();
        if (pPed->isDead()) {
            pPed->handleDeath(damage_);
        } else {
            pPed->leaveState(kPedActionStateHit);
        }
        setSucceeded();
    }
    return true;
}

WalkBurnHitAction::WalkBurnHitAction(DamageToInflict &d) :
    HitAction(d), burnTimer_(kTimeToWalkBurning), playedSecondScream_(false), burnTimeElapsed_(0), nonLethalBurn_(false) {
}

/*!
 *
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
void WalkBurnHitAction::doStart([[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    int hpBefore = pPed->health();
    pPed->takeDamage(damage_);
    if (pPed->isAlive()) {
        // Target survived burn/fire damage (e.g. Chest Mod V1-V3); brief singe recoil without burning to death
        nonLethalBurn_ = true;
        burnTimer_.setMax(150);
        pPed->goToState(kPedActionStateHit);
        pPed->playHitAnimation();
        return;
    }

    // Lethal burn: restore health during the 6-second burning walk so death finalizes at completion
    pPed->setHealth(hpBefore);
    nonLethalBurn_ = false;
    burnTimer_.setMax(kTimeToWalkBurning);
    pPed->goToState(kPedActionStateHit);
    pPed->deselectWeapon();
    moveDirdesc_.clear();
    walkedDist_ = 0;
    moveDirection_ = rand() % 256;
    pPed->setSpeedToMax();
    pPed->playWalkBurnAnimation();
    if (rs_eng::SoundManager::singletonPtr()) {
        g_SoundMgr.play(rs_eng::FLAMING_DEATH);
    }
    playedSecondScream_ = false;
    burnTimeElapsed_ = 0;
    phase_ = kBurnPhaseWalk;
}

/*!
 *
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
bool WalkBurnHitAction::doExecute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    if (nonLethalBurn_) {
        if (burnTimer_.update(elapsed)) {
            pPed->leaveState(kPedActionStateHit);
            setSucceeded();
        }
        return true;
    }

    switch (phase_) {
    case kBurnPhaseWalk:
    {
        int walkDistDiff = 0;
        pPed->moveToDir(pMission, elapsed, moveDirdesc_, moveDirection_, -1, -1, &walkDistDiff, true);
        walkedDist_ += walkDistDiff;

        burnTimeElapsed_ += elapsed;
        if (burnTimeElapsed_ >= kDeathScreamDelay && !playedSecondScream_) {
            if (rs_eng::SoundManager::singletonPtr()) {
                g_SoundMgr.play(rs_eng::FLAMING_DEATH);
            }
            playedSecondScream_ = true;
        }

        if (burnTimer_.update(elapsed)) {
            pPed->decreaseHealth(pPed->health());
            phase_ = kBurnPhaseDying;
            pPed->playDyingBurnAnimation();
            waitForAnimation();
        }
    }
        break;
    case kBurnPhaseDying:
        phase_ = kBurnPhaseDead;
        pPed->playSmokeBurnAnimation();
        waitForAnimation();
        break;
    case kBurnPhaseDead:
        pPed->handleDeath(damage_);
        setSucceeded();
        break;
    } 

    return true;
}

PersuadedHitAction::PersuadedHitAction(DamageToInflict &d) :
HitAction(d) {
    warnBehaviour_ = true;
}

/*!
 *
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
void PersuadedHitAction::doStart([[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    pPed->goToState(kPedActionStateHit);
    pPed->playPersuadedAnimation();
    if (rs_eng::SoundManager::singletonPtr()) {
        g_SoundMgr.play(rs_eng::PERSUADE);
    }
    PedInstance *pAgent = static_cast<PedInstance *>(damage_.d_owner);
    if (pAgent && !pPed->isPersuaded()) {
        pPed->handlePersuadedBy(pAgent);
    }
    waitForAnimation();
}

/*!
 *
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 */
bool PersuadedHitAction::doExecute([[maybe_unused]] uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (status_ == kActStatusRunning) {
        pPed->leaveState(kPedActionStateHit);
        setSucceeded();
    }
    return true;
}

ShootAction::ShootAction(const WorldPoint &aimedAt, WeaponInstance *pWeapon) :
    UseWeaponAction(kActTypeShoot, pWeapon) {
        aimedAt_ = aimedAt;
}

void ShootAction::setAimedAt(const WorldPoint &aimedAt) {
    aimedAt_ = aimedAt;
}

/*!
 * Execute the shoot action.
 * When action starts, it sets the firing state (and animation), fires
 * the weapon and then waits for the end of animation.
 * When animation is over, wait for weapon
 * to reload before finishing.
 * \param elapsed Time since last frame.
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 * \return true to redraw
 */
bool ShootAction::execute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    if (pPed == nullptr || !pPed->isAlive() || pPed->isDead() || pPed->isDying() || pPed->isState(kPedActionStateHit)) {
        stop();
        return false;
    }

    if (status_ == kActStatusNotStarted) {
        // TODO : stop moving only if shooting in different direction of movement
        if (pPed->currentAction() != nullptr && !pPed->currentAction()->isSuspended()) {
            pPed->currentAction()->suspend(pPed);
        }
        // Turn to target
        pPed->setDirectionTowardPosition(aimedAt_);
        // Shoot
        DamageToInflict dmg;
        fillDamageDesc(pPed, pWeapon_, dmg);
        pWeapon_->playSound();
        pWeapon_->fire(pMission, dmg, elapsed);
        // change state to firing
        pPed->goToState(kPedActionStateFiring);
        // waiting for animation to complete
        waitForAnimation();
        animWaitTimer_.reset(150);
    }

    if (status_ == kActStatusWaitForAnim && animWaitTimer_.update(elapsed)) {
        setRunning();
    }

    if (status_ == kActStatusRunning) {
        // Shooting animation is finished
        pPed->leaveState(kPedActionStateFiring);
        if (pPed->currentAction() && pPed->currentAction()->isSuspended()) {
            pPed->currentAction()->resume(pMission, pPed);
        }

        // The action is complete only after a certain laps of time to
        // simulate the fact that the weapon needs to be reloaded
        // and the shooter's reactivity to that
        waitForTime(pPed->getTimeBetweenShoots(pWeapon_));
    } else if (status_ == kActStatusWaitForTime) {
        if (timeToWaitTimer_.update(elapsed)) {
            // time is reached so action can finish
            setSucceeded();
        }
    }

    return true;
}

void ShootAction::stop() {
    if (!isFinished()) {
        PedInstance *pPed = pWeapon_ ? pWeapon_->owner() : nullptr;
        if (pPed) {
            pPed->leaveState(kPedActionStateFiring);
            if (pPed->currentAction() && pPed->currentAction()->isSuspended()) {
                if (rs_knl::MissionManager::singletonPtr() && g_missionCtrl.mission()) {
                    pPed->currentAction()->resume(g_missionCtrl.mission(), pPed);
                }
            }
        }
        // If the shot was already fired (status_ is waitForAnim, running, or waitForTime),
        // do not mark as succeeded early: transition to or maintain waitForTime so reload cooldown is enforced
        if (status_ == kActStatusWaitForAnim || status_ == kActStatusRunning) {
            int reloadWait = pPed ? pPed->getTimeBetweenShoots(pWeapon_) : (pWeapon_ ? pWeapon_->getClass()->reloadTime() : 0);
            waitForTime(reloadWait);
        } else if (status_ != kActStatusWaitForTime) {
            setSucceeded();
        }
    }
}

/*!
 * Fills the ShotAttributes structure with needed information.
 * \param targetLoc Where to shoot
 * \param dmg Structure to fill
 */
void ShootAction::fillDamageDesc(PedInstance *pShooter,
                                    WeaponInstance *pWeapon,
                                    DamageToInflict &dmg) {
    dmg.pWeapon = pWeapon;
    dmg.dtype = pWeapon->getClass()->dmgType();
    dmg.dvalue =  pWeapon->getClass()->damagePerShot();
    dmg.range = pWeapon->getClass()->range();
    dmg.d_owner = pShooter;
    dmg.aimedLocW = aimedAt_;
    dmg.originLocW.convertFromTilePoint(pShooter->position());
    // the weapon is located at half the size of the shooter
    dmg.originLocW.z += pShooter->sizeZ() >> 1;

    if (pWeapon->isInstanceOf(Weapon::Flamer)) {
        switch(pShooter->getDiscreteDirection()) {
        case 0:
            dmg.originLocW.y += 64;
            break;
        case 1:
            dmg.originLocW.x += 45;
            dmg.originLocW.y += 45;
            break;
        case 2:
            dmg.originLocW.x += 64;
            break;
        case 3:
            dmg.originLocW.x += 45;
            dmg.originLocW.y -= 45;
            break;
        case 4:
            dmg.originLocW.y -= 64;
            break;
        case 5:
            dmg.originLocW.x -= 45;
            dmg.originLocW.y -= 45;
            break;
        case 6:
            dmg.originLocW.x -= 64;
            break;
        case 7:
            dmg.originLocW.x -= 45;
            dmg.originLocW.y += 45;
            break;
        }
    }
}

AutomaticShootAction::AutomaticShootAction(const WorldPoint &aimedAt, WeaponInstance *pWeapon) :
        ShootAction(aimedAt, pWeapon),
        fireRateElapsed_(0)
{
}

bool AutomaticShootAction::execute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    if (pPed == nullptr || !pPed->isAlive() || pPed->isDead() || pPed->isDying() || pPed->isState(kPedActionStateHit)) {
        stop();
        return false;
    }

    bool firstTime = false;
    if (status_ == kActStatusNotStarted) {
        setRunning();
        // change state to firing
        pPed->goToState(kPedActionStateFiring);
        // If ped was moving, stop during shooting
        // TODO : stop moving only if shooting in different direction of movement
        if (pPed->hasDestination()) {
            pPed->currentAction()->suspend(pPed);
        }
        pWeapon_->setDirection(0);
        firstTime = true;
        fireRateElapsed_ = 0;
    }

    if (status_ == kActStatusRunning) {
        if (pPed->isDead() || pPed->isDying() || pPed->isState(kPedActionStateHit) || pWeapon_->ammoRemaining() == 0) {
            stop();
        } else {
            uint32_t fireRate = pWeapon_ ? static_cast<uint32_t>(std::max(0, pWeapon_->getClass()->fireRate())) : 0;
            bool shouldFire = firstTime;
            if (!firstTime) {
                fireRateElapsed_ += elapsed;
                if (fireRate == 0 || fireRateElapsed_ >= fireRate) {
                    shouldFire = true;
                    if (fireRate > 0) {
                        uint32_t rem = fireRateElapsed_ - fireRate;
                        fireRateElapsed_ = std::min(rem, fireRate - 1);
                    } else {
                        fireRateElapsed_ = 0;
                    }
                }
            }
            if (shouldFire) {
                pPed->setDirectionTowardPosition(aimedAt_);
                DamageToInflict dmg;
                fillDamageDesc(pPed, pWeapon_, dmg);
                if (pWeapon_->getClass()->impactsPerAmmo() <= 1) {
                    WorldPoint dispersedAimedLoc = aimedAt_;
                    pPed->adjustAimedPtWithRangeAndAccuracy(pWeapon_->getClass(), &dispersedAimedLoc);
                    dmg.aimedLocW = dispersedAimedLoc;
                } else {
                    dmg.aimedLocW = aimedAt_;
                }
                pWeapon_->playSound();
                pWeapon_->fire(pMission, dmg, elapsed);
            }
        }
    } else if (status_ == kActStatusWaitForTime) {
        if (timeToWaitTimer_.update(elapsed)) {
            // time is reached so action can finish
            setSucceeded();
        }
    }

    return true;
}

void AutomaticShootAction::stop() {
    if (!isFinished()) {
        PedInstance *pPed = pWeapon_ ? pWeapon_->owner() : nullptr;
        if (pPed) {
            pPed->leaveState(kPedActionStateFiring);
            if (pPed->currentAction() && pPed->currentAction()->isSuspended()) {
                if (rs_knl::MissionManager::singletonPtr() && g_missionCtrl.mission()) {
                    pPed->currentAction()->resume(g_missionCtrl.mission(), pPed);
                }
            }
        }
        setSucceeded();
    }
}

/*!
 * Execute the Use medikit action.
 * \param elapsed Time since last frame.
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 * \return true to redraw
 */
bool UseMedikitAction::execute(uint32_t elapsed, Mission *pMission, PedInstance *pPed) {
    bool update = false;
    if (status_ == kActStatusNotStarted) {
        // set time before completing action
        waitForTime(pPed->getTimeBetweenShoots(pWeapon_));

        if (!pWeapon_->isInstanceOf(Weapon::MediKit)) {
            setFailed();
        } else {
            pWeapon_->playSound();
            DamageToInflict dmg;
            dmg.d_owner = pPed;
            pWeapon_->fire(pMission, dmg, elapsed);
            update = true;
        }
    } else if (status_ == kActStatusWaitForTime) {
        if (timeToWaitTimer_.update(elapsed)) {
            // time is reached so action can finish
            setSucceeded();
            update = true;
        }
    }

    return update;
}

/*!
 * Execute the Use energy shield action.
 * Activates the shield on first call, then keeps ticking it while it stays
 * the ped's selected weapon. Ammo consumption is handled by the weapon
 * itself in WeaponInstance::doUpdateState().
 * \param elapsed Time since last frame.
 * \param pMission Mission data
 * \param pPed The ped executing the action.
 * \return true to redraw
 */
bool UseEnergyShieldAction::execute(uint32_t elapsed, [[maybe_unused]] Mission *pMission, PedInstance *pPed) {
    if (status_ == kActStatusNotStarted) {
        if (!pWeapon_->isInstanceOf(Weapon::EnergyShield) && !pWeapon_->isInstanceOf(Weapon::CloneShield)) {
            setFailed();
            return false;
        }
        status_ = kActStatusRunning;
        pWeapon_->activate();
        return true;
    }

    if (pPed->selectedWeapon() != pWeapon_) {
        // weapon has been switched away from: stop it immediately
        pWeapon_->deactivate();
        setSucceeded();
        return true;
    }

    pWeapon_->animate(elapsed);
    if (pWeapon_->ammoRemaining() == 0) {
        // no more ammo; weapon already deactivated itself in doUpdateState()
        setSucceeded();
    }

    return true;
}

void UseEnergyShieldAction::stop() {
    if (isRunning()) {
        pWeapon_->deactivate();
        setSucceeded();
    }
}

}