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

#include "rs-utils/common.h"
#include <cmath>
#include "rs-utils/log/log.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/events/event.h"
#include "rs-engine/system/system.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/squad.h"
#include "rs-kernel/model/shot.h"
#include "rs-kernel/mgr/agentmanager.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/mgr/pedmanager.h"
#include "rs-kernel/ia/behaviour.h"
#include "rs-kernel/model/leveldata.h"
#include "rs-kernel/mgr/modmanager.h"
#include "rs-engine/config.h"
#include "rs-kernel/ia/actions.h"

namespace rs_knl {

//*************************************
// Constant definition
//*************************************
const int PedInstance::kAgentMaxHealth = 16;
const int PedInstance::kDefaultShootReactionTime = 200;
const uint32_t PedInstance::kPlayerGroupId = 1;
const int PedInstance::kAgentMaxSpeedWithOverweight = 64;
const int PedInstance::kTimeToSmokeBurn = 10000;
const uint32_t PedInstance::pd_smUndefined = 0;

PedInstance::PedInstance(uint16_t anId, Map *pMap, PedType pedType, bool isOur, int maxSpeed) :
    ShootableMovableMapObject(anId, pMap, MapObject::kNaturePed, maxSpeed),
    type_(pedType),
    state_(kPedActionStateStanding),
    desc_state_(PedInstance::pd_smUndefined),
    hostile_desc_(PedInstance::pd_smUndefined),
    obj_group_def_(PedInstance::og_dmUndefined),
    old_obj_group_def_(PedInstance::og_dmUndefined),
    obj_group_id_(0), old_obj_group_id_(0),
    sight_range_(0), in_vehicle_(NULL),
    owner_(NULL),
    adrenaline_(IPAStim::Adrenaline),
    perception_(IPAStim::Perception),
    intelligence_(IPAStim::Intelligence)
{
    unblockPath();
    is_our_ = isOur;

    tm_before_check_ = 1000;
    base_mod_acc_ = 0.1;

    behaviour_.setOwner(this);
    currentAction_ = NULL;
    defaultAction_ = NULL;
    altAction_ = NULL;
    pUseWeaponAction_ = NULL;
    panicImmuned_ = false;
    totalPersuasionPoints_ = 0;
    pSelectedWeaponBeforeMedikit_ = NULL;
    trainBoardingCooldown_ = 0;
}

PedInstance::~PedInstance() {
    destroyAllActions();
    destroyUseWeaponAction();
}

/*!
 * A ped cannot panic only if:
 * - he has a scripted scenario (matching original DOS Syndicate check at 0x0002a6bc: [ped + 0x24] != 0)
 * - he's in a vehicle or actively boarding one (matching original DOS Syndicate Status 5 & 6)
 * - he's persuaded
 * - he's manually flagged as panic immuned
 * @return True means he can't panic
 */
bool PedInstance::isImmunedToPanic() { 
    return panicImmuned_ || hasScript() || isInVehicle() || isEnteringVehicle() || isPersuaded();
}

/*!
 * Returns true if ped is actively boarding a vehicle or moving towards one to enter it.
 * Matches original DOS Syndicate Action Status 5 (USE_VEHICLE).
 */
bool PedInstance::isEnteringVehicle() const {
    if (in_vehicle_ != nullptr) {
        return false;
    }
    const MovementAction *act = (currentAction_ != nullptr) ? currentAction_ : defaultAction_;
    if (act == nullptr) {
        return false;
    }
    // Directly executing EnterVehicleAction
    if (dynamic_cast<const EnterVehicleAction *>(act) != nullptr) {
        return true;
    }
    // Or walking directly towards a vehicle to enter it (WalkAction linked to EnterVehicleAction)
    if (const WalkAction *walk = dynamic_cast<const WalkAction *>(act)) {
        if (walk->next() != nullptr && dynamic_cast<const EnterVehicleAction *>(walk->next()) != nullptr) {
            return true;
        }
    }
    return false;
}

bool PedInstance::isInPanic() {
    return behaviour_.isInPanic();
}

bool PedInstance::isScriptLooping() const {
    if (scriptSteps_.empty()) return false;
    return scriptSteps_.back().type == LevelData::kScenarioTypeReset;
}

void PedInstance::setScriptLooping(bool loop) {
    if (loop) {
        if (!isScriptLooping()) {
            ScriptStep step;
            step.type = LevelData::kScenarioTypeReset;
            scriptSteps_.push_back(step);
        }
    } else {
        if (isScriptLooping()) {
            scriptSteps_.pop_back();
        }
    }
}

void PedInstance::synchDrawnAnimWithActionState(void) {
    if (!isAlive()) {
        return;
    }
    switch (state_) {
        case kPedActionStateStanding:
        case kPedActionStateWalking:
            playStandOrWalkAnimation();
            break;
        case kPedActionStateStandingFiring:
            playStandAndShootAnimation();
            break;
        case kPedActionStateWalkingFiring:
            playWalkAndShootAnimation();
            break;
        default:
            // Hit/PickUp/PutDown animations are triggered explicitly by their action.
            break;
    }
}

/*!
 * Set the given state as the new state.
 * Update corresponding animation.
 * @param as new state
 * @param forceSync Force a sync of animation even if state did not change
 */
void PedInstance::goToState(PedActionState as, bool forceSync) {
    if (!isAlive()) {
        return;
    }
    PedActionState prevState = state_;
    switch(as) {
        case kPedActionStateStanding:
            state_ = kPedActionStateStanding;
            break;
        case kPedActionStateWalking:
            state_ = (state_ == kPedActionStateStandingFiring) ?
                kPedActionStateWalkingFiring : kPedActionStateWalking;
            break;
        case kPedActionStateHit:
            state_ = kPedActionStateHit;
            break;
        case kPedActionStateFiring:
            state_ = (state_ == kPedActionStateWalking) ?
                kPedActionStateWalkingFiring : kPedActionStateStandingFiring;
            break;
        case kPedActionStatePickUp:
            state_ = kPedActionStatePickUp;
            break;
        case kPedActionStatePutDown:
            state_ = kPedActionStatePutDown;
            break;
        case kPedActionStateStandingFiring:
        case kPedActionStateWalkingFiring:
            // Not valid targets: reached only through kPedActionStateFiring.
            break;
    }

    if(prevState != state_ || forceSync) {
        if (currentAction_ && currentAction_->isWaitingForAnimation() && currentAction_->targetState() != state_) {
            currentAction_->setRunning();
        }
        if (pUseWeaponAction_ && pUseWeaponAction_->isWaitingForAnimation()) {
            pUseWeaponAction_->setRunning();
        }
        synchDrawnAnimWithActionState();
    }
}

/*!
 * Leaves the given state.
 * Update corresponding animation.
 * \param as new state
 */
void PedInstance::leaveState(PedActionState as) {
    if (!isAlive()) {
        return;
    }
    PedActionState prevState = state_;
    switch(as) {
        case kPedActionStateStanding:
            break;
        case kPedActionStateWalking:
            state_ = (state_ == kPedActionStateWalkingFiring) ?
                kPedActionStateStandingFiring : kPedActionStateStanding;
            break;
        case kPedActionStateHit:
            state_ = kPedActionStateStanding;
            break;
        case kPedActionStateFiring:
            state_ = (state_ == kPedActionStateWalkingFiring) ?
                kPedActionStateWalking : kPedActionStateStanding;
            break;
        case kPedActionStatePickUp:
        case kPedActionStatePutDown:
            state_ = kPedActionStateStanding;
            break;
        default:
            state_ = kPedActionStateStanding;
    }

    if (prevState != state_) {
        if (currentAction_ && currentAction_->isWaitingForAnimation() && currentAction_->targetState() != state_) {
            currentAction_->setRunning();
        }
        if (pUseWeaponAction_ && pUseWeaponAction_->isWaitingForAnimation()) {
            pUseWeaponAction_->setRunning();
        }
        synchDrawnAnimWithActionState();
    }
}

/*!
 * @brief Return true if the ped is currently in the given state.
 * @param aState The state to test.
 * @return True if the ped's current state is exactly aState.
 */
bool PedInstance::isState(PedActionState aState) {
    return state_ == aState;
}

void PedInstance::setAnimations(uint16_t baseSpriteAnimationId) {
    baseAnim_ = baseSpriteAnimationId;
    animations_.standAnimations[Weapon::CloneShield_Anim] =
            animationPlayer_->addAnimation(1537);
    animations_.walkAnimations[Weapon::CloneShield_Anim] =
            animationPlayer_->addAnimation(1537 + 8, rs_eng::kAnimationModeLoop);
    animations_.standAnimations[Weapon::Unarmed_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId);
    
    if (baseSpriteAnimationId == 1) { // Animations for agents
        animations_.standAnimations[Weapon::EnergyShield_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 16, rs_eng::kAnimationModeLoop);
        animations_.standAnimations[Weapon::Pistol_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Uzi_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Shotgun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Gauss_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Minigun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 32);
        animations_.standAnimations[Weapon::Laser_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Flamer_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 40, rs_eng::kAnimationModeLoop);
        animations_.standAnimations[Weapon::LongRange_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 48);
        
        animations_.walkAnimations[Weapon::Unarmed_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 8, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::EnergyShield_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 56, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Pistol_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Uzi_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Shotgun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Gauss_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Minigun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 72, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Laser_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Flamer_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 80, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::LongRange_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 88, rs_eng::kAnimationModeLoop);

        // I think 97 was the original pistol shoot anim, but it wasn't sexy enough.
        animations_.standShootAnimations[Weapon::Pistol_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.standShootAnimations[Weapon::Uzi_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104, rs_eng::kAnimationModeLoop);
        animations_.standShootAnimations[Weapon::Shotgun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.standShootAnimations[Weapon::Gauss_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.standShootAnimations[Weapon::Minigun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 112, rs_eng::kAnimationModeLoop);
        animations_.standShootAnimations[Weapon::Laser_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 120);
        animations_.standShootAnimations[Weapon::Flamer_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 128, rs_eng::kAnimationModeLoop);
        animations_.standShootAnimations[Weapon::LongRange_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 136);
        
        animations_.walkShootAnimations[Weapon::Pistol_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 152);
        animations_.walkShootAnimations[Weapon::Uzi_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 152);
        animations_.walkShootAnimations[Weapon::Shotgun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.walkShootAnimations[Weapon::Gauss_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 152);
        animations_.walkShootAnimations[Weapon::Minigun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 160);
        animations_.walkShootAnimations[Weapon::Laser_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 168);
        animations_.walkShootAnimations[Weapon::Flamer_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 176);
        animations_.walkShootAnimations[Weapon::LongRange_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 184);
    } else {
        // NOTE: peds other then agents have pistol like animations for
        // all weapons
        animations_.standAnimations[Weapon::EnergyShield_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId, rs_eng::kAnimationModeLoop);
        animations_.standAnimations[Weapon::Pistol_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Uzi_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Shotgun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Gauss_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Minigun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Laser_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        animations_.standAnimations[Weapon::Flamer_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24, rs_eng::kAnimationModeLoop);
        animations_.standAnimations[Weapon::LongRange_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24);
        
        animations_.walkAnimations[Weapon::Unarmed_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 8, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::EnergyShield_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 24, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Pistol_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Uzi_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Shotgun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Gauss_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Minigun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Laser_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::Flamer_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);
        animations_.walkAnimations[Weapon::LongRange_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 64, rs_eng::kAnimationModeLoop);

        animations_.standShootAnimations[Weapon::Pistol_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.standShootAnimations[Weapon::Uzi_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104, rs_eng::kAnimationModeLoop);
        animations_.standShootAnimations[Weapon::Shotgun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.standShootAnimations[Weapon::Gauss_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.standShootAnimations[Weapon::Minigun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104, rs_eng::kAnimationModeLoop);
        animations_.standShootAnimations[Weapon::Laser_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.standShootAnimations[Weapon::Flamer_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104, rs_eng::kAnimationModeLoop);
        animations_.standShootAnimations[Weapon::LongRange_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        
        animations_.walkShootAnimations[Weapon::Pistol_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.walkShootAnimations[Weapon::Uzi_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.walkShootAnimations[Weapon::Shotgun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.walkShootAnimations[Weapon::Gauss_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.walkShootAnimations[Weapon::Minigun_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.walkShootAnimations[Weapon::Laser_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.walkShootAnimations[Weapon::Flamer_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
        animations_.walkShootAnimations[Weapon::LongRange_Anim]  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 104);
    }

    animations_.pickAnimation  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 192);
    animations_.hitAnimation  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 193,
                                            rs_eng::kAnimationModeSingle, 6);
    animations_.vaporizeAnimation  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 197,
                                            rs_eng::kAnimationModeSingle, 6);
    animations_.sinkAnimation =
            animationPlayer_->addAnimation(baseSpriteAnimationId + 201,
                                            rs_eng::kAnimationModeSingle, 6);
    animations_.dyingAnimation  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 205);
    animations_.deadAgentAnimation  = 
            animationPlayer_->addAnimation(205, rs_eng::kAnimationModeSingle, 2);
    animations_.deadAnimation  = 
            animationPlayer_->addAnimation(baseSpriteAnimationId + 206,
                                            rs_eng::kAnimationModeSingle, 2);
    animations_.walkBurnAnimation  =  animationPlayer_->addAnimation(209, rs_eng::kAnimationModeLoop);
    animations_.dyingBurnAnimation  = 
            animationPlayer_->addAnimation(210, rs_eng::kAnimationModeSingle, 6);
    animations_.smokeBurnAnimation  = 
            animationPlayer_->addAnimation(211, rs_eng::kAnimationModeLoop, 8, kTimeToSmokeBurn);
    animations_.deadBurnAnimation  = 
            animationPlayer_->addAnimation(828, rs_eng::kAnimationModeSingle, 2);

    animations_.persuadedAnimation = animationPlayer_->addAnimation(236, rs_eng::kAnimationModeLoop, 12, 1500);
    animations_.standShootAnimations[Weapon::Unarmed_Anim] = animations_.standAnimations[Weapon::Unarmed_Anim];
    animations_.walkShootAnimations[Weapon::Unarmed_Anim] = animations_.walkAnimations[Weapon::Unarmed_Anim];
    animations_.standShootAnimations[Weapon::CloneShield_Anim] = animations_.standAnimations[Weapon::CloneShield_Anim];
    animations_.walkShootAnimations[Weapon::CloneShield_Anim] = animations_.walkAnimations[Weapon::CloneShield_Anim];

    /*
    TODO : should we use these animations?
    pedanim->setSinkAnim(201 + baseAnim);
    // 203 - die, 204 - dead, agent only
    pedanim->setDieAgentAnim(203 + baseAnim);
    // when this burning should be used?
    pedanim->setStandBurnAnim(208);
    */
}

void PedInstance::doUpdateState(uint32_t elapsed) {
    if (trainBoardingCooldown_ > 0) {
        if ((uint32_t)trainBoardingCooldown_ > elapsed) {
            trainBoardingCooldown_ -= elapsed;
        } else {
            trainBoardingCooldown_ = 0;
        }
    }

    Mission *mission = rs_knl::MissionManager::singletonPtr() ? g_missionCtrl.mission() : nullptr;
    // Execute current behaviour
    if (mission) {
        behaviour_.execute(elapsed, mission);
    }

    // Execute any active action
    executeAction(elapsed, mission);

    // Check for car avoidance when ped is on foot and alive
    if (mission && isAlive() && inVehicle() == nullptr && !isDead() && !isDying() && !isState(kPedActionStateHit)) {
        checkAndAvoidMovingVehicles(mission, elapsed);
    }

    // cannot shoot if ped is doing something exclusive, or is dead/dying/in hit state
    if (isAlive() && !isDead() && !isDying() && !isState(kPedActionStateHit)) {
        if (currentAction_ == NULL || !currentAction_->isExclusive()) {
            executeUseWeaponAction(elapsed, mission);
        }
    }
}

bool PedInstance::checkAndAvoidMovingVehicles(Mission *pMission, uint32_t elapsed) {
    if (!pMission || inVehicle() != nullptr || !isAlive() || isDead() || isDying()) {
        isEvadingVehicle_ = false;
        wasEvadingVehicle_ = false;
        return false;
    }

    // If this is a player agent actively executing a player-ordered walk command, do not interrupt
    if (isOurAgent() && currentAction_ != nullptr && currentAction_->source() == Action::kActionNotScripted) {
        isEvadingVehicle_ = false;
        if (wasEvadingVehicle_) {
            wasEvadingVehicle_ = false;
            playStandOrWalkAnimation();
        }
        return false;
    }

    int px = pos_.tx * 256 + pos_.ox;
    int py = pos_.ty * 256 + pos_.oy;
    int pz = pos_.tz * 128 + pos_.oz;

    for (size_t i = 0; i < pMission->numVehicles(); ++i) {
        Vehicle *v = pMission->vehicle(i);
        // An active hazard is a vehicle that is moving OR has an active driving route (even if temporarily yielding/stopped)
        if (!v || v->isDead() || (!v->isMoving() && !v->hasDestination())) {
            continue;
        }

        int cz = v->position().tz * 128 + v->position().oz;
        if (std::abs(pz - cz) > 128) {
            continue;
        }

        int cx = v->position().tx * 256 + v->position().ox;
        int cy = v->position().ty * 256 + v->position().oy;

        double pdx = px - cx;
        double pdy = py - cy;
        double distSq = pdx * pdx + pdy * pdy;
        double maxDetect = static_cast<double>(kPedCarAvoidanceDetectionDist + 64);
        if (distSq > maxDetect * maxDetect) {
            continue;
        }

        // Calculate car forward direction vector (vx, vy)
        double vx = 0.0;
        double vy = 0.0;
        if (v->hasDestination() && !v->destPath().empty()) {
            auto it = v->destPath().begin();
            if (v->destPath().size() > 1) {
                int diffx = it->tx * 256 + it->ox - cx;
                int diffy = it->ty * 256 + it->oy - cy;
                int halfSize = std::max(v->sizeX(), v->sizeY());
                if (diffx * diffx + diffy * diffy < halfSize * halfSize) {
                    ++it;
                }
            }
            int vdx = it->tx * 256 + it->ox - cx;
            int vdy = it->ty * 256 + it->oy - cy;
            double pathLen = sqrt(static_cast<double>(vdx * vdx + vdy * vdy));
            if (pathLen > 0.001) {
                vx = vdx / pathLen;
                vy = vdy / pathLen;
            }
        }
        if (vx == 0.0 && vy == 0.0) {
            double angleRad = (v->direction() / 128.0) * rs_utl::kPI;
            vx = sin(angleRad);
            vy = cos(angleRad);
        }

        // Longitudinal projection s: distance ahead of car
        double s = pdx * vx + pdy * vy;
        if (s < -48.0 || s > static_cast<double>(kPedCarAvoidanceDetectionDist)) {
            continue;
        }

        // Lateral projection l: signed distance from car centerline
        double l = pdx * vy - pdy * vx;
        if (std::abs(l) > static_cast<double>(kPedCarAvoidanceCorridorWidth)) {
            continue;
        }

        // Hazard confirmed! Calculate lateral evasion normal vector (nx, ny)
        double nx = 0.0;
        double ny = 0.0;
        if (l >= 0.0) {
            // Ped is to the right of car center -> dodge right
            nx = vy;
            ny = -vx;
        } else {
            // Ped is to the left of car center -> dodge left
            nx = -vy;
            ny = vx;
        }

        // Blend in a forward-escape component if close to front bumper
        if (s < 96.0) {
            nx += 0.35 * vx;
            ny += 0.35 * vy;
        }

        double nlen = sqrt(nx * nx + ny * ny);
        if (nlen > 0.001) {
            nx /= nlen;
            ny /= nlen;
        } else {
            nx = vy;
            ny = -vx;
        }

        int pedSpeed = std::max(160, maxSpeed());
        int step = std::max(6, (pedSpeed * static_cast<int>(elapsed)) / 1000);

        // Try primary evasion direction
        int dodgeDx = static_cast<int>(round(nx * step));
        int dodgeDy = static_cast<int>(round(ny * step));

        int targetX = px + dodgeDx;
        int targetY = py + dodgeDy;
        int targetTileX = targetX / 256;
        int targetTileY = targetY / 256;

        bool canDodgePrimary = false;
        if (targetTileX >= 0 && targetTileY >= 0) {
            if (!pMission->map()) {
                canDodgePrimary = true;
            } else if (targetTileX < pMission->mmax_x_ && targetTileY < pMission->mmax_y_) {
                canDodgePrimary = pMission->isTileWalkable(targetTileX, targetTileY, pos_.tz);
            }
        }

        if (canDodgePrimary) {
            clearDestination();
            setDirection(dodgeDx, dodgeDy, &dir_);
            addOffsetToPosition(dodgeDx, dodgeDy);
            isEvadingVehicle_ = true;
            wasEvadingVehicle_ = true;
            playStandOrWalkAnimation();
            return true;
        }

        // Primary direction blocked (e.g. wall / obstacle): try opposite side
        int oppDx = -dodgeDx;
        int oppDy = -dodgeDy;
        int oppTargetX = px + oppDx;
        int oppTargetY = py + oppDy;
        int oppTileX = oppTargetX / 256;
        int oppTileY = oppTargetY / 256;

        bool canDodgeOpp = false;
        if (oppTileX >= 0 && oppTileY >= 0) {
            if (!pMission->map()) {
                canDodgeOpp = true;
            } else if (oppTileX < pMission->mmax_x_ && oppTileY < pMission->mmax_y_) {
                canDodgeOpp = pMission->isTileWalkable(oppTileX, oppTileY, pos_.tz);
            }
        }

        if (canDodgeOpp) {
            clearDestination();
            setDirection(oppDx, oppDy, &dir_);
            addOffsetToPosition(oppDx, oppDy);
            isEvadingVehicle_ = true;
            wasEvadingVehicle_ = true;
            playStandOrWalkAnimation();
            return true;
        }
    }

    isEvadingVehicle_ = false;
    if (wasEvadingVehicle_) {
        wasEvadingVehicle_ = false;
        setSpeedToMax();
        playStandOrWalkAnimation();
    }
    return false;
}

void PedInstance::handleAnimationEnded() {
    if (animationPlayer_) {
        if (animationPlayer_->isCurrentAnimation(animations_.dyingBurnAnimation)) {
            playSmokeBurnAnimation();
            return;
        } else if (animationPlayer_->isCurrentAnimation(animations_.smokeBurnAnimation)) {
            playDeadBurnAnimation();
            return;
        }
    }

    // An action was waiting for the animation to finish
    if (currentAction_ && currentAction_->isWaitingForAnimation()) {
        // so continue action
        currentAction_->setRunning();
    }
    if (pUseWeaponAction_ && pUseWeaponAction_->isWaitingForAnimation()) {
        // so continue action
        pUseWeaponAction_->setRunning();
    }
}

void PedInstance::handleSelectedWeaponHasNoAmmo() {
    // when weapon is empty persuaded will drop weapon
    if (isPersuaded()) {
        // we should be able to suspend as by default it should be a follow action
        currentAction_->suspend(this);
        PutdownWeaponAction *pDrop = new PutdownWeaponAction(0);
        pDrop->setWarnBehaviour(true);
        //
        pDrop->link(currentAction_);
        currentAction_ = pDrop;
    } else {
        // first deselect empty weapon
        WeaponInstance *pDeselectedWeapon = deselectWeapon();
        // selection was a shooting weapon so replace with the same type
        // or something else
        if (pDeselectedWeapon->canShoot()) {
            selectShootingWeaponWithSameTypeFirst(pDeselectedWeapon);
        } else if (pDeselectedWeapon->isInstanceOf(Weapon::EnergyShield)) {
            // Use another energy shield
            selectMedikitOrShield(Weapon::EnergyShield);
        } else if (pDeselectedWeapon->isInstanceOf(Weapon::CloneShield)) {
            selectMedikitOrShield(Weapon::CloneShield);
        } else if (pDeselectedWeapon->isInstanceOf(Weapon::MediKit)) {
            if (pSelectedWeaponBeforeMedikit_ != NULL) {
                selectWeapon(pSelectedWeaponBeforeMedikit_);
                pSelectedWeaponBeforeMedikit_ = NULL;
            }
        }
    }
}

/*!
 * Return true if :
 * - is not doing something that prevents him from using weapon
 * - is not already using a weapon
 * - weapon is usable (ie a shooting weapon or a medikit)
 */
bool PedInstance::canAddUseWeaponAction(WeaponInstance *pWeapon) {
    if (!isAlive() || isDead() || isDying() || isState(kPedActionStateHit)) {
        return false;
    }

    if (currentAction_ != NULL && currentAction_->isExclusive()) {
        return false;
    }

    if (pUseWeaponAction_ != NULL) {
        if (pUseWeaponAction_->isFinished()) {
            destroyUseWeaponAction();
        } else {
            return false;
        }
    }

    WeaponInstance *pWi = pWeapon != NULL ? pWeapon : selectedWeapon();
    if (!pWi) return false;
    if (pWi->isInstanceOf(Weapon::AirStrike)) return true;
    return pWi->usesAmmo() && pWi->ammoRemaining() > 0;
}

/*!
 * Terminate the current action of using weapon.
 */
void PedInstance::stopUsingWeapon() {
    if (isUsingWeapon()) {
        pUseWeaponAction_->stop();
    }
}

/*!
 * Terminate the current action of using weapon that shoots.
 */
void PedInstance::stopShooting() {
    if (isUsingWeapon() && pUseWeaponAction_->type() == Action::kActTypeShoot) {
        // stop shooting in case of automatic shooting
        WeaponInstance *pWpn = pUseWeaponAction_->weapon();
        if (pWpn && pWpn->getClass()->isAutomatic()) {
            pUseWeaponAction_->stop();
        }
    }
    for (PedInstance *pPers : persuadedSet_) {
        if (pPers && pPers->isAlive()) {
            pPers->stopShooting();
        }
    }
}

/*!
 * Update the ped's shooting target.
 * \param aimedPt New target position
 */
void PedInstance::updateShootingTarget(const WorldPoint &aimedPt) {
    if (pUseWeaponAction_->type() == Action::kActTypeShoot) {
        ShootAction *pShoot = dynamic_cast<ShootAction *>(pUseWeaponAction_);
        pShoot->setAimedAt(aimedPt);
    }
}

/*!
 * Returns the mean time between two shoots.
 * When a ped has shot, it takes time to shoot again : time to reload
 * the weapon + ped's reactivity time (influenced by IPA and Mods)
 * \param pWeapon The weapon used to shoot
 * \return Time to wait
 */
int PedInstance::getTimeBetweenShoots(WeaponInstance *pWeapon) {
    // TODO : Add IPA and mods influence
    return kDefaultShootReactionTime +
            pWeapon->getClass()->reloadTime();
}

/*!
 * Forces an agent to commit suicide.
 * If he's equiped with the good version of Mod Chest, he will
 * explode causing damage on nearby Peds and all his weapons will
 * be destroyed.
 * Else he dies alone leaving his weapons on the ground.
 */
void PedInstance::commitSuicide() {
    if (hasMinimumVersionOfMod(Mod::MOD_CHEST, Mod::MOD_V2)) {
        // Having a chest v2 makes agent explode
        if (rs_knl::MissionManager::singletonPtr() && g_missionCtrl.mission()) {
            Explosion::createExplosion(g_missionCtrl.mission(), this, 512.0, 16);
        }
    } else {
        // else he just shoot himself
        DamageToInflict dit;
        dit.dtype = kDmgTypeBullet;
        dit.d_owner = this;
        // force damage value to agent health so he's killed at once
        dit.dvalue = PedInstance::kAgentMaxHealth;

        handleHit(dit);
    }
}

/*!
 * Energy Shield is activated when it is selected.
 * @return true if energy shield is activated
 */
bool PedInstance::isEnergyShieldActivated() {
    WeaponInstance *pWeapon = selectedWeapon();

    return pWeapon && pWeapon->isInstanceOf(Weapon::EnergyShield);
}

bool PedInstance::isCloneShieldActivated() const {
    const WeaponInstance *pWeapon = const_cast<PedInstance *>(this)->selectedWeapon();
    return pWeapon && pWeapon->isInstanceOf(Weapon::CloneShield);
}

void PedInstance::showPath(int scrollX, int scrollY, rs_eng::RSColor color) {
    if (dest_path_.empty() || !pMap_) {
        return;
    }

    auto toScreen = [&](const TilePoint &pt) -> Point2D {
        int screen_w = (pMap_->maxTx() + (pt.tx - pt.ty)) * (rs_eng::Tile::kTileWidth / 2);
        int coord_h = ((pMap_->maxTz() + pt.tx + pt.ty) - pt.tz) * (rs_eng::Tile::kTileHeight / 3);
        Point2D basePos = {
            screen_w - scrollX + rs_eng::Tile::kTileWidth / 2,
            coord_h - scrollY + rs_eng::Tile::kTileHeight / 3 * 2
        };
        Point2D posWithOffs;
        posWithOffs.x = (((pt.ox - pt.oy) * (rs_eng::Tile::kTileWidth / 2)) / 256) + basePos.x;
        posWithOffs.y = (((pt.ox + pt.oy) * (rs_eng::Tile::kTileHeight / 3)) / 256) + basePos.y;
        posWithOffs.y -= (pt.oz * (rs_eng::Tile::kTileHeight / 3)) / 128;
        return posWithOffs;
    };

    Point2D p1 = toScreen(pos_);

    for (const auto &d : dest_path_) {
        Point2D p2 = toScreen(d);

        // Draw thickened path line directly connecting waypoints
        g_System.drawLine(p1, p2, color);
        g_System.drawLine({ p1.x + 1, p1.y }, { p2.x + 1, p2.y }, color);
        g_System.drawLine({ p1.x - 1, p1.y }, { p2.x - 1, p2.y }, color);
        g_System.drawLine({ p1.x, p1.y + 1 }, { p2.x, p2.y + 1 }, color);
        g_System.drawLine({ p1.x, p1.y - 1 }, { p2.x, p2.y - 1 }, color);

        // Draw waypoint node marker (5x5 filled box)
        g_System.drawFillRect({ p2.x - 2, p2.y - 2 }, 5, 5, color);

        p1 = p2;
    }
}

/*!
 * Play a stand or walk animation whether the ped is moving or not and depending
 * on the selected weapon. 
 */
void PedInstance::playStandOrWalkAnimation() {
    if (!isAlive() || !animationPlayer_) {
        return;
    }
    if (currentAction_ && currentAction_->isWaitingForAnimation()) {
        return;
    }
    Weapon::WeaponAnimIndex weapon_idx =
        selectedWeapon() ? selectedWeapon()->index() : Weapon::Unarmed_Anim;
    if (isMoving() || isEvadingVehicle_) {
        animationPlayer_->play(animations_.walkAnimations[weapon_idx]);
    } else {
        animationPlayer_->play(animations_.standAnimations[weapon_idx]);
    }
}

void PedInstance::playStandAndShootAnimation() {
    if (!isAlive() || !animationPlayer_) {
        return;
    }
    Weapon::WeaponAnimIndex weapon_idx =
        selectedWeapon() ? selectedWeapon()->index() : Weapon::Unarmed_Anim;
    animationPlayer_->play(animations_.standShootAnimations[weapon_idx]);
}

void PedInstance::playWalkAndShootAnimation() {
    if (!isAlive() || !animationPlayer_) {
        return;
    }
    Weapon::WeaponAnimIndex weapon_idx =
        selectedWeapon() ? selectedWeapon()->index() : Weapon::Unarmed_Anim;
    animationPlayer_->play(animations_.walkShootAnimations[weapon_idx]);
}

void PedInstance::playPickupOrDropAnimation() {
    if (animationPlayer_) {
        animationPlayer_->play(animations_.pickAnimation);
    }
}

void PedInstance::playHitAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_) {
        animationPlayer_->play(animations_.hitAnimation);
    }
}

void PedInstance::playDyingAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_) {
        animationPlayer_->play(animations_.dyingAnimation);
    }
}

void PedInstance::playDeadAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_) {
        animationPlayer_->play(animations_.deadAnimation);
    }
}

void PedInstance::playVaporizeAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_) {
        animationPlayer_->play(animations_.vaporizeAnimation);
    }
}

void PedInstance::playSinkAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_ && animations_.sinkAnimation != PedAnimations::kUndefinedAnim) {
        animationPlayer_->play(animations_.sinkAnimation);
    }
}

void PedInstance::playDeadAgentAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_) {
        animationPlayer_->play(animations_.deadAgentAnimation);
    }
}

void PedInstance::playWalkBurnAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_) {
        animationPlayer_->play(animations_.walkBurnAnimation);
    }
}

void PedInstance::playDyingBurnAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_) {
        animationPlayer_->play(animations_.dyingBurnAnimation);
    }
}

void PedInstance::playSmokeBurnAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_) {
        animationPlayer_->play(animations_.smokeBurnAnimation);
    }
}

void PedInstance::playDeadBurnAnimation() {
    stopShooting();
    destroyUseWeaponAction();
    if (animationPlayer_) {
        animationPlayer_->play(animations_.deadBurnAnimation);
    }
}

void PedInstance::playPersuadedAnimation() {
    if (animationPlayer_) {
        animationPlayer_->play(animations_.persuadedAnimation);
    }
}

void PedInstance::draw(const Point2D &screenPos) {
    Point2D posWithOffs = addOffs(screenPos);

    // ensure on map
    int minX = (std::abs(rs_eng::g_mapZoom - 1.0) > 0.01) ? -64 : 110;
    if (posWithOffs.x < minX || posWithOffs.y < -64 || !isDrawable())
        return;

    Weapon::WeaponAnimIndex weapon_idx =
        selectedWeapon() ? selectedWeapon()->index() : Weapon::Unarmed_Anim;

    int fId = isCloneShieldActivated() ? -1 : factionId();
    if (animationPlayer_->isCurrentAnimation(animations_.standAnimations[weapon_idx])) {
        animationPlayer_->draw(posWithOffs, getDiscreteDirection(), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.walkAnimations[weapon_idx])) {
        animationPlayer_->draw(posWithOffs, getDiscreteDirection(), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.standShootAnimations[weapon_idx])) {
        animationPlayer_->draw(posWithOffs, getDiscreteDirection(), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.walkShootAnimations[weapon_idx])) {
        animationPlayer_->draw(posWithOffs, getDiscreteDirection(), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.hitAnimation)) {
        // There are only 4 animations for hit
        animationPlayer_->draw(posWithOffs, getDiscreteDirection(4), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.vaporizeAnimation)) {
        // There are only 4 animations for vaporized
        animationPlayer_->draw(posWithOffs, getDiscreteDirection(4), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.pickAnimation)) {
        animationPlayer_->draw(posWithOffs, 0, fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.persuadedAnimation)) {
        animationPlayer_->draw(posWithOffs, 0, fId);
    } else {
        animationPlayer_->draw(posWithOffs, 0, fId);
    }
}

uint16_t PedInstance::getDrawAnimation() const {
    uint16_t anim = 0;
    if (animationPlayer_ && animationPlayer_->currentFramesAnimation() != 0) {
        anim = animationPlayer_->currentFramesAnimation();
    } else if (baseAnim_ != 0) {
        anim = baseAnim_;
    } else {
        anim = rs_knl::PedManager::getDefaultBaseAnim(type_);
    }

    if (animationPlayer_) {
        WeaponInstance *selW = const_cast<PedInstance*>(this)->selectedWeapon();
        Weapon::WeaponAnimIndex weapon_idx = selW ? selW->index() : Weapon::Unarmed_Anim;

        if (animationPlayer_->isCurrentAnimation(animations_.standAnimations[weapon_idx]) ||
            animationPlayer_->isCurrentAnimation(animations_.walkAnimations[weapon_idx]) ||
            animationPlayer_->isCurrentAnimation(animations_.standShootAnimations[weapon_idx]) ||
            animationPlayer_->isCurrentAnimation(animations_.walkShootAnimations[weapon_idx])) {
            return anim + getDiscreteDirection(8);
        } else if (animationPlayer_->isCurrentAnimation(animations_.hitAnimation) ||
                   animationPlayer_->isCurrentAnimation(animations_.vaporizeAnimation)) {
            return anim + getDiscreteDirection(4);
        } else if (animationPlayer_->isCurrentAnimation(animations_.pickAnimation) ||
                   animationPlayer_->isCurrentAnimation(animations_.persuadedAnimation) ||
                   animationPlayer_->isCurrentAnimation(animations_.sinkAnimation) ||
                   animationPlayer_->isCurrentAnimation(animations_.deadAnimation) ||
                   animationPlayer_->isCurrentAnimation(animations_.deadAgentAnimation) ||
                   animationPlayer_->isCurrentAnimation(animations_.deadBurnAnimation)) {
            return anim;
        }
    }
    if (!isAlive()) {
        return anim;
    }
    return anim + getDiscreteDirection(8);
}

int PedInstance::getDrawFrame() const {
    if (animationPlayer_) {
        return animationPlayer_->currentFrame();
    }
    return 0;
}


void PedInstance::drawSelectorAnim(const Point2D &screenPos) {

    Weapon::WeaponAnimIndex weapon_idx =
        selectedWeapon() ? selectedWeapon()->index() : Weapon::Unarmed_Anim;

    int fId = isCloneShieldActivated() ? -1 : factionId();
    if (animationPlayer_->isCurrentAnimation(animations_.standAnimations[weapon_idx])) {
        animationPlayer_->draw(screenPos, getDiscreteDirection(), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.walkAnimations[weapon_idx])) {
        animationPlayer_->draw(screenPos, getDiscreteDirection(), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.standShootAnimations[weapon_idx])) {
        animationPlayer_->draw(screenPos, getDiscreteDirection(), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.walkShootAnimations[weapon_idx])) {
        animationPlayer_->draw(screenPos, getDiscreteDirection(), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.hitAnimation)) {
        // There are only 4 animations for hit
        animationPlayer_->draw(screenPos, getDiscreteDirection(4), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.vaporizeAnimation)) {
        // There are only 4 animations for vaporized
        animationPlayer_->draw(screenPos, getDiscreteDirection(4), fId);
    } else if (animationPlayer_->isCurrentAnimation(animations_.pickAnimation)) {
        animationPlayer_->draw(screenPos, 0, fId);
    } else {
        animationPlayer_->draw(screenPos, 0, fId);
    }
}

bool PedInstance::inSightRange(MapObject *t) {

    return this->isCloseTo(t, sight_range_);
}

/*!
 * Called before a weapon is selected to check if weapon can be selected.
 * \param wi The weapon to select
 */
bool PedInstance::canSelectWeapon(WeaponInstance *pNewWeapon) {
    if (pNewWeapon->isInstanceOf(Weapon::MediKit)) {
        // we cas use medikit only if ped is hurt
        return health() != startHealth() &&
            canAddUseWeaponAction(pNewWeapon);
    } else if (pNewWeapon->isInstanceOf(Weapon::EnergyShield) || pNewWeapon->isInstanceOf(Weapon::CloneShield)) {
        return canAddUseWeaponAction(pNewWeapon);
    }

    return WeaponHolder::canSelectWeapon(pNewWeapon);
}

/*!
 * Called when a weapon has been deselected.
 * \param wi The deselected weapon
 */
void PedInstance::handleWeaponDeselected(WeaponInstance * wi) {
    if (wi->isInstanceOf(Weapon::AccessCard)) {
        rmEmulatedGroupDef(4, og_dmPolice);
    }

    if (wi->isInstanceOf(Weapon::Persuadatron)) {
        behaviour_.handleBehaviourEvent(Behaviour::kBehvEvtPersuadotronDeactivated);
    } else if (wi->canShoot() && (type_ != kPedTypePolice || isPersuaded())) {
        // don't warn if ped is police to limit calls
        EventManager::fire<ShootingWeaponSelectedEvent>(this, false);
    }
    // Update animation
    playStandOrWalkAnimation();
}

/*!
 * Called when a weapon has been selected.
 * \param wi The selected weapon
 * \param previousWeapon The previous selected weapon (can be null if no weapon was selected)
 */
void PedInstance::handleWeaponSelected(WeaponInstance * wi, WeaponInstance * previousWeapon) {
    switch(wi->getClass()->getType()) {
    case Weapon::CloneShield:
        if (animationPlayer_) {
            static const uint16_t kDisguiseBases[6] = { 626, 833, 1744, 1330, 1537, 1123 };
            uint16_t disguise = kDisguiseBases[rand() % 6];
            animations_.standAnimations[Weapon::CloneShield_Anim] = animationPlayer_->addAnimation(disguise);
            animations_.walkAnimations[Weapon::CloneShield_Anim] = animationPlayer_->addAnimation(disguise + 8, rs_eng::kAnimationModeLoop);
            animations_.standShootAnimations[Weapon::CloneShield_Anim] = animations_.standAnimations[Weapon::CloneShield_Anim];
            animations_.walkShootAnimations[Weapon::CloneShield_Anim] = animations_.walkAnimations[Weapon::CloneShield_Anim];
        }
        addActionUseEnergyShield(wi);
        playStandOrWalkAnimation();
        break;
    case Weapon::EnergyShield:
        addActionUseEnergyShield(wi);
        playStandOrWalkAnimation();
        break;
    case Weapon::AccessCard:
        addEmulatedGroupDef(4, og_dmPolice);
        break;
    case Weapon::MediKit:
        if (previousWeapon != NULL &&
            previousWeapon->getClass()->canShoot() &&
            previousWeapon->ammoRemaining() > 0) {
            pSelectedWeaponBeforeMedikit_ = previousWeapon;
        }
        addActionUseMedikit(wi);
        break;
    case Weapon::Persuadatron:
        behaviour_.handleBehaviourEvent(Behaviour::kBehvEvtPersuadotronActivated);
        break;
    default:
        if (wi->canShoot()) {
            playStandOrWalkAnimation();
        }
        break;
    }

    if (type_ != kPedTypePolice || isPersuaded()) {
        if (selectedWeapon()->canShoot()) {
            // alert that the ped is holding a shooting weapon
            EventManager::fire<ShootingWeaponSelectedEvent>(this, true);
        }
    }
}

/*!
 * Drops the weapon at given index on the ground.
 * \param index Index of weapon in the agent inventory.
 * \return the instance of dropped weapon
 */
WeaponInstance * PedInstance::dropWeapon(uint8_t index) {
    if (isWeaponSelected(weapon(index))) {
        stopUsingWeapon();
    }
    WeaponInstance *pWeapon = removeWeaponAtIndex(index);

    if(pWeapon) {
        pWeapon->setMap(map());
        pWeapon->setDrawable(true);
        pWeapon->setPosition(pos_);
        if (rs_knl::MissionManager::singletonPtr() && g_missionCtrl.mission()) {
            g_missionCtrl.mission()->addWeaponToGround(pWeapon);
        }
    }

    return pWeapon;
}

/*!
 * Drop all the ped's weapons on the ground around him.
 * \return void
 *
 */
void PedInstance::dropAllWeapons() {
    while (weapons_.size()) {
        WeaponInstance *w = dropWeapon(0);

        // randomizing location for drop
        int ox = rand() % 256;
        int oy = rand() % 256;
        w->setPosition(pos_.tx, pos_.ty, pos_.tz, ox, oy);
        w->adjustPositionForSlope();
    }
}

bool PedInstance::pickupWeapon() {
    return state_ == kPedActionStatePickUp;
}

Vehicle *PedInstance::inVehicle() const {
    return in_vehicle_;
}

void PedInstance::putInVehicle(Vehicle * pVehicle)
{
    setDrawable(false);
    in_vehicle_ = pVehicle;
}

void PedInstance::leaveVehicle() {
    setDrawable(true);
    TilePoint targetPos = in_vehicle_ ? in_vehicle_->position() : position();
    Mission *pMission = rs_knl::MissionManager::singletonPtr() ? g_missionCtrl.mission() : nullptr;
    if (pMission && !pMission->isTileWalkable(targetPos.tx, targetPos.ty, targetPos.tz)) {
        // 1. Check direct vertical layers (e.g. sidewalk at Z+1 directly above the road)
        bool found = false;
        for (int dz = 1; dz <= 3; ++dz) {
            if (pMission->isTileWalkable(targetPos.tx, targetPos.ty, targetPos.tz + dz)) {
                targetPos.tz += dz;
                found = true;
                break;
            }
            if (targetPos.tz - dz >= 0 && pMission->isTileWalkable(targetPos.tx, targetPos.ty, targetPos.tz - dz)) {
                targetPos.tz -= dz;
                found = true;
                break;
            }
        }
        // 2. Check nearby tiles in radius 1..3
        if (!found) {
            int bestDistSq = 999999;
            TilePoint bestPt = targetPos;
            static const int dxy[8][2] = {
                { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
                { 1, 1 }, { -1, 1 }, { 1, -1 }, { -1, -1 }
            };
            for (int r = 1; r <= 3 && !found; ++r) {
                for (int i = 0; i < 8; ++i) {
                    int nx = targetPos.tx + dxy[i][0] * r;
                    int ny = targetPos.ty + dxy[i][1] * r;
                    for (int dz = -2; dz <= 2; ++dz) {
                        int nz = targetPos.tz + dz;
                        if (pMission->isTileWalkable(nx, ny, nz)) {
                            int dx = (nx * 256 + 128) - (targetPos.tx * 256 + targetPos.ox);
                            int dy = (ny * 256 + 128) - (targetPos.ty * 256 + targetPos.oy);
                            int d2 = dx * dx + dy * dy;
                            if (d2 < bestDistSq) {
                                bestDistSq = d2;
                                bestPt = TilePoint(nx, ny, nz, 128, 128, 0);
                                found = true;
                            }
                        }
                    }
                }
            }
            if (found) {
                targetPos = bestPt;
            }
        }
    }
    setPosition(targetPos);
    in_vehicle_ = nullptr;
}

bool PedInstance::isFollowing() {
    if (currentAction_) {
        Action::ActionType type = currentAction_->type();
        return type == Action::ActionType::kActTypeFollow ||
            type == Action::ActionType::kActTypeFollowToShoot;
    }

    return false;
}

/*!
 * Return the damage after applying reduction of Mod protection.
 * @param damage Damage description
 * @return True if Ped has died due to damage received
 */
bool PedInstance::takeDamage(DamageToInflict &damage) {
    int finalDmg = damage.dvalue;
    Mod *pMod = slots_[Mod::MOD_CHEST];
    int chestVer = pMod ? pMod->getVersion() : -1;

    switch (damage.dtype) {
        case kDmgTypePersuasion:
            finalDmg = damage.dvalue;
            break;
        case kDmgTypeBullet:
            if (pMod) {
                int denom = 1;
                if (chestVer == Mod::MOD_V1) denom = 2;      // 50% hit chance
                else if (chestVer == Mod::MOD_V2) denom = 4; // 25% hit chance
                else if (chestVer == Mod::MOD_V3) denom = 6; // 16.7% hit chance

                if (denom > 1) {
                    int unit = (damage.pWeapon && damage.pWeapon->getClass())
                                   ? damage.pWeapon->getClass()->damagePerShot()
                                   : 0;
                    if (unit > 0 && damage.dvalue >= unit && (damage.dvalue % unit) == 0) {
                        int numHits = damage.dvalue / unit;
                        finalDmg = 0;
                        for (int i = 0; i < numHits; ++i) {
                            if ((std::rand() % denom) == 0) {
                                finalDmg += unit;
                            }
                        }
                    } else {
                        finalDmg = ((std::rand() % denom) == 0) ? damage.dvalue : 0;
                    }
                }
            }
            break;
        case kDmgTypeLaser:
        case kDmgTypeExplosion:
        case kDmgTypeCollision:
            if (pMod) {
                if (chestVer == Mod::MOD_V1) {
                    finalDmg = std::max(damage.dvalue, start_health_);
                } else if (chestVer == Mod::MOD_V2) {
                    finalDmg = 8;
                } else if (chestVer == Mod::MOD_V3) {
                    finalDmg = 4;
                }
            }
            break;
        case kDmgTypeBurn:
            if (!pMod) {
                finalDmg = std::max(damage.dvalue, start_health_);
            } else if (chestVer == Mod::MOD_V1) {
                finalDmg = 8;
            } else if (chestVer == Mod::MOD_V2) {
                finalDmg = 4;
            } else if (chestVer == Mod::MOD_V3) {
                finalDmg = 2;
            }
            break;
        default:
            break;
    }

    decreaseHealth(finalDmg);
    return isDead();
}

/*!
 * Method called when object is hit by a weapon shot.
 * \param d Damage description
 */
void PedInstance::handleHit(DamageToInflict &d) {
    stopShooting();
    destroyUseWeaponAction();

    if (isAlive() && (d.dtype & (kDmgTypeBullet | kDmgTypeCollision))) {
        if (RecoilHitAction *pRecoil = dynamic_cast<RecoilHitAction *>(currentAction_)) {
            pRecoil->onAdditionalHit(d, this);
            return;
        }
    }

    if (canTakeAction(rs_knl::Action::kActTypeHit)) {
        insertHitAction(d);

        // Alert behaviour
        behaviour_.handleBehaviourEvent(Behaviour::kBehvEvtHit, &d);
    }
}

void PedInstance::finalizeDeathState() {
    stopShooting();
    destroyUseWeaponAction();
    clearDestination();

    if (rs_knl::MissionManager::singletonPtr() && g_missionCtrl.mission()) {
        updatePersuadedRelations(g_missionCtrl.mission()->getSquad());
        g_missionCtrl.mission()->removeArmedPed(this);
        for (size_t i = 0; i < g_missionCtrl.mission()->numPeds(); i++) {
            g_missionCtrl.mission()->ped(i)->behaviour().handleBehaviourEvent(rs_knl::Behaviour::kBehvEvtWeaponCleared, this);
        }
    }

    // send an event to alert agent died
    if (isOurAgent()) {
        EventManager::fire<AgentDiedEvent>(this);
    }
}

bool PedInstance::startVoidFall() {
    stopShooting();
    destroyUseWeaponAction();
    clearDestination();
    setHealth(0);
    if (animationPlayer_ && animations_.sinkAnimation != PedAnimations::kUndefinedAnim) {
        animationPlayer_->play(animations_.sinkAnimation);
        return true;
    }
    return false;
}

void PedInstance::handleVoidFall() {
    leaveState(kPedActionStateHit);
    setHealth(0);
    setDrawable(false);
    finalizeDeathState();
}

/*!
 * This method do all things when a ped dies
 * @param damage The damage that caused the death 
 */
void PedInstance::handleDeath(const DamageToInflict &damage) {
    stopShooting();
    destroyUseWeaponAction();
    clearDestination();
    
    switch (damage.dtype) {
        case kDmgTypeBullet:
        case kDmgTypeCollision:
            dropAllWeapons();
            if (animationPlayer_) {
                if (animationPlayer_->isCurrentAnimation(animations_.dyingAnimation)) {
                    playDeadAnimation();
                } else if (!animationPlayer_->isCurrentAnimation(animations_.deadAnimation)) {
                    playDyingAnimation();
                }
            } else {
                playDeadAnimation();
            }
            break;
        case kDmgTypeLaser:
            setDrawable(false);
            if (is_our_) {
                playDyingBurnAnimation();
            }
            break;
        case kDmgTypeExplosion:
        case kDmgTypeBurn:
            // weapons are dropped only if ped had enough protection or don't die by suicide
            if (damage.nonIncendiary || (hasMinimumVersionOfMod(Mod::MOD_CHEST, Mod::MOD_V2) && damage.d_owner != this)) {
                dropAllWeapons();
                if (animationPlayer_) {
                    playDeadAnimation();
                }
            } else {
                playDeadBurnAnimation();
            }
            break;
        default:
            RSERR(Log::k_FLG_GAME, "PedInstance", "handleDeath", ("Unhandled damage type: %d\n", damage.dtype))
    }

    finalizeDeathState();
}

bool PedInstance::isHitByBullet() {
    if (isState(kPedActionStateHit)) {
        return dynamic_cast<RecoilHitAction *>(currentAction_) != nullptr;
    }

    return false;
}

bool PedInstance::isVaporizing() const {
    if (animationPlayer_ && animations_.vaporizeAnimation != PedAnimations::kUndefinedAnim) {
        if (animationPlayer_->isCurrentAnimation(animations_.vaporizeAnimation) && animationPlayer_->isPlaying()) {
            return true;
        }
    }
    if (currentAction_ && !currentAction_->isFinished() && currentAction_->type() == Action::kActTypeHit) {
        if (dynamic_cast<const LaserHitAction *>(currentAction_) != nullptr) {
            return true;
        }
    }
    return false;
}

bool PedInstance::isSinking() const {
    if (animationPlayer_ && animations_.sinkAnimation != PedAnimations::kUndefinedAnim) {
        if (animationPlayer_->isCurrentAnimation(animations_.sinkAnimation) && animationPlayer_->isPlaying()) {
            return true;
        }
    }
    if (currentAction_ && !currentAction_->isFinished() && currentAction_->type() == Action::kActTypeHit) {
        if (const auto *pRecoil = dynamic_cast<const RecoilHitAction *>(currentAction_)) {
            if (pRecoil->isSinking()) {
                return true;
            }
        }
    }
    return false;
}

bool PedInstance::isDying() const {
    if (animationPlayer_) {
        if (animationPlayer_->isCurrentAnimation(animations_.dyingAnimation) ||
            animationPlayer_->isCurrentAnimation(animations_.dyingBurnAnimation) ||
            animationPlayer_->isCurrentAnimation(animations_.smokeBurnAnimation) ||
            animationPlayer_->isCurrentAnimation(animations_.deadBurnAnimation) ||
            animationPlayer_->isCurrentAnimation(animations_.deadAnimation) ||
            animationPlayer_->isCurrentAnimation(animations_.deadAgentAnimation) ||
            animationPlayer_->isCurrentAnimation(animations_.vaporizeAnimation) ||
            animationPlayer_->isCurrentAnimation(animations_.sinkAnimation)) {
            return true;
        }
    }
    return health_ <= 0;
}

void PedInstance::addEnemyGroupDef(uint32_t eg_id, uint32_t eg_def) {
    enemy_group_defs_.add(eg_id, eg_def);
}

void PedInstance::rmEnemyGroupDef(uint32_t eg_id, uint32_t eg_def) {
    enemy_group_defs_.rm(eg_id, eg_def);
}

bool PedInstance::isInEnemyGroupDef(uint32_t eg_id, uint32_t eg_def) {
    return enemy_group_defs_.isIn(eg_id, eg_def);
}

void PedInstance::addEmulatedGroupDef(uint32_t eg_id, uint32_t eg_def) {
    emulated_group_defs_.add(eg_id, eg_def);
}
void PedInstance::rmEmulatedGroupDef(uint32_t eg_id, uint32_t eg_def) {
    emulated_group_defs_.rm(eg_id, eg_def);
}

bool PedInstance::isInEmulatedGroupDef(uint32_t eg_id, uint32_t eg_def) {
    return emulated_group_defs_.isIn(eg_id, eg_def);
}

bool PedInstance::isInEmulatedGroupDef(PedInstance::Mmuu32_t &r_egd,
        bool id_only)
{
    if (id_only) {
        return emulated_group_defs_.isIn_KeyOnly(r_egd);
    }
    return emulated_group_defs_.isIn_All(r_egd);
}

/*!
 * Returns true if the given object is considered hostile by this Ped.
 * If object is a Vehicle, check if it contains hostiles inside.
 * If it is another Ped, check if he is a friend or in a opposite group
 * \param obj The object whom hostility is being evaluated
 * \param hostile_desc_alt
 * \return true if object is considered hostile
 */
bool PedInstance::isHostileTo(ShootableMapObject *obj,
    unsigned int hostile_desc_alt)
{
    bool isHostile = false;

    if (obj->nature() == MapObject::kNatureVehicle) {
        Vehicle *pVehicle = static_cast<Vehicle *>(obj);
        isHostile = pVehicle->containsHostilesForPed(
            this, hostile_desc_alt);
    } else if (obj->nature() == MapObject::kNaturePed) {
        PedInstance *pPed = static_cast<PedInstance *>(obj);
        if (pPed->isCloneShieldActivated() && type() != kPedTypeAgent) {
            return false;
        }
        if (!isFriendWith(pPed)) {
            // Ped is not a declared friend, check its group
            if ((pPed)->emulatedGroupDefsEmpty()) {
                isHostile =
                    isInEnemyGroupDef(pPed->objGroupID(), pPed->objGroupDef());
            } else {
                isHostile = pPed->isInEmulatedGroupDef(enemy_group_defs_);
            }
            if (!isHostile) {
                if (hostile_desc_alt == PedInstance::pd_smUndefined)
                    hostile_desc_alt = hostile_desc_;
                isHostile = (pPed->descStateMasks() & hostile_desc_alt) != 0;
            }
        }
    }

    return isHostile;
}

/*!
 * Friend can be neutral to be sure that object is hostile use
 * isHostileTo and check hostiles_found_(isInHostilesFound)
 * \param p
 * \return True if other ped is considered a friend.
 */
bool PedInstance::isFriendWith(PedInstance *p) {
    // Search ped in friends
    if (friends_found_.find(p) != friends_found_.end())
        return true;
    if (p->isInEmulatedGroupDef(obj_group_id_, obj_group_def_)) {
        if (obj_group_def_ == og_dmPolice
            && !isPersuaded())
        {
            friends_not_seen_.insert(p);
        }
        return true;
    }
    if (friend_group_defs_.find(p->objGroupDef()) != friend_group_defs_.end())
        return true;
    return (p->objGroupID() == obj_group_id_);
}

bool PedInstance::isFriendlyFireProtected(const PedInstance *pOther) const {
    if (!pOther) return false;
    if (this == pOther) return true;
    if ((isOurAgent() || isPersuaded()) && (pOther->isOurAgent() || pOther->isPersuaded())) {
        return true;
    }
    return obj_group_id_ == pOther->objGroupID();
}

void PedInstance::verifyHostilesFound(Mission *m) {
    std::vector <ShootableMapObject *> rm_set;
    WorldPoint cur_xyz(pos_);
    rs_knl::Mission::BlockerCriteria crits;
    crits.maxr = sight_range_;

    WeaponInstance *wi = selectedWeapon();
    if (wi && wi->canShoot() && wi->range() > crits.maxr)
        crits.maxr = wi->range();

    
    // removing destroyed, friends, objects out of shot/sight range
    for (Msmod_t::iterator it = hostiles_found_.begin();
        it != hostiles_found_.end(); ++it)
    {
        ShootableMapObject *smo = it->first;
        double distTo = 0;
        if (smo->isDead() || (smo->nature() == MapObject::kNaturePed
            && isFriendWith((PedInstance *)(smo)))
            || (smo->nature() == MapObject::kNatureVehicle
            && ((Vehicle *)smo)->containsHostilesForPed(this, hostile_desc_))
            || (m->checkIfBlockersInShootingLine(cur_xyz, crits, &smo, nullptr, &distTo) != 1))
        {
            rm_set.push_back(smo);
        }
    }
    while (!rm_set.empty()) {
        hostiles_found_.erase(hostiles_found_.find(rm_set.back()));
        rm_set.pop_back();
    }
}

/*!
 * Initiate the amount, depend and effect level for a given type of IPA.
 * Levels passed to this method should come from original values and will be transformed into pourcentage.
 * @param type The type of IPA to init for this ped
 * @param amount The amount level
 * @param depend The depend level
 * @param effect The effect level
 */
void PedInstance::initAllLevelsForIPAType(IPAStim::IPAType type, uint8_t amount, uint8_t depend, uint8_t effect) {
    switch (type)
    {
    case IPAStim::IPAType::Adrenaline:
        adrenaline_.setLevels256(amount, depend, effect);
        break;
    case IPAStim::IPAType::Perception:
        perception_.setLevels256(amount, depend, effect);
        break;
    case IPAStim::IPAType::Intelligence:
        intelligence_.setLevels256(amount, depend, effect);
        break;
    default:
        break;
    }
}


/*!
 * Set amount for given IPA.
 * @param ipaType The type of IPA
 * @param percentage The amount expressed as a percentage
 */
void PedInstance::setIPAAmount(IPAStim::IPAType ipaType, uint8_t percentage) {
    switch(ipaType) {
        case IPAStim::Adrenaline:
            adrenaline_.setAmount(percentage);
            break;
        case IPAStim::Perception:
            perception_.setAmount(percentage);
            break;
        case IPAStim::Intelligence:
            intelligence_.setAmount(percentage);
            break;
    }
}

/*!
 * Movement speed calculated from base speed, mods, weight of inventory,
 * ipa, etc.
 */
int PedInstance::applySpeedModifier(int speed) {
    if (isInPanic()) {
        return 256;
    }

    float speed_new = static_cast<float>(speed);

    // Match original DOS MAIN.EXE (0x2d8c8 / 0x30180): apply agent inventory weight penalty
    // to base speed first, then apply Adrenaline / Leg mod multipliers afterward so Adrenaline
    // (including hunter chases) still accelerates heavily armed agents.
    if (type_ == PedInstance::kPedTypeAgent) {
        int weight_max = getMaxWeight();
        int weight_inv = getInventoryWeight();

        if (weight_inv > weight_max) {
            int over = weight_inv - weight_max;
            if (over >= 1000) {
                speed_new = kAgentMaxSpeedWithOverweight;
            } else {
                speed_new = speed_new * static_cast<float>(1000 - over) / 1000.0f;
                if (speed > kAgentMaxSpeedWithOverweight && speed_new < kAgentMaxSpeedWithOverweight) {
                    speed_new = kAgentMaxSpeedWithOverweight;
                }
            }
        }
    }

    if (obj_group_def_ == PedInstance::og_dmAgent) {
        float adrMult = adrenaline_.getMultiplier();
        if (!isOurAgent() && isHunter()) {
            adrMult = 2.0f;
        }
        if (adrMult > 1.0f) {
            float boostMult = getLegModAdrenalineBoostMultiplier();
            speed_new *= (1.0f + (adrMult - 1.0f) * boostMult);
        } else {
            speed_new *= adrMult;
        }
    } else {
        speed_new *= getSpeedMultiplier();
    }

    if (isPersuaded() && owner_ != nullptr) {
        speed_new *= owner_->getPersuadedSpeedMultiplier();
    }

    return static_cast<int>(speed_new);
}

/*!
 * @brief Returns the speed multiplier this ped gives to the peds it has persuaded.
 * When Adrenaline is boosted, persuaded peds receive a 1.30x boost factor
 * (scaling up to 2.30x at max Adrenaline).
 * @return 1 + (Adrenaline multiplier - 1) * 1.30 for boosted agents, 1 for neutral
 */
float PedInstance::getPersuadedSpeedMultiplier() const
{
    if (obj_group_def_ == PedInstance::og_dmAgent)
    {
        float adr = adrenaline_.getMultiplier();
        if (adr > 1.0f) {
            return 1.0f + (adr - 1.0f) * 1.30f;
        } else if (adr < 1.0f) {
            return 1.0f + (adr - 1.0f) * 0.5f;
        }
    }

    return 1.0f;
}

/*!
 * Adds a little imprecision to the aimed point. Precision depends
 * on the weapon used, the ped's mods and IPA levels.
 * \param pWeaponClass The type of weapon used to shoot
 * \param aimedPt Where the player has clicked on the map. This point
 * will be updated to reflect the influence of precision.
 */
void PedInstance::adjustAimedPtWithRangeAndAccuracy(Weapon *pWeaponClass, WorldPoint *pAimedLocW) {
    if (!pWeaponClass || !pAimedLocW) return;

    // 1- Adjust Range
    WorldPoint originLocW(pos_);
    if (pMap_) {
        if (originLocW.z > (pMap_->maxTz() - 1) * 128)
            return;

        if (pAimedLocW->z > (pMap_->maxTz() - 1) * 128)
            return;
    }

    double dx = static_cast<double>(pAimedLocW->x - originLocW.x);
    double dy = static_cast<double>(pAimedLocW->y - originLocW.y);
    double d2D = sqrt(dx * dx + dy * dy);

    if (d2D <= 0.0)
        return;

    double maxr = (double) pWeaponClass->range();
    if (d2D > maxr) {
        // weapon's range is less than the distance to aimed point
        // so compute new aimed point that is clipped by the range
        double dist_k = maxr / d2D;
        pAimedLocW->x = originLocW.x + (int)(dx * dist_k);
        pAimedLocW->y = originLocW.y + (int)(dy * dist_k);
        pAimedLocW->z = originLocW.z + (int)((pAimedLocW->z - originLocW.z) * dist_k);
        dx *= dist_k;
        dy *= dist_k;
        d2D = maxr;
    }

    // 2- Adjust Accuracy and Angular Dispersion
    double accuracy = 0.5;
    getAccuracy(accuracy);

    // Compute effective spread angle (degrees) and depth (world units) directly from weapons.dat
    double effectiveSpreadAngleDeg = pWeaponClass->effectiveSpreadAngle(accuracy);
    double effectiveSpreadDepth = pWeaponClass->effectiveSpreadDepth(accuracy);

    double spreadAngleRad = (effectiveSpreadAngleDeg * rs_utl::kPI) / 180.0;

    if (spreadAngleRad > 0.0001 || effectiveSpreadDepth > 0.0001) {
        // Unit direction vector in the horizontal plane
        double ux = dx / d2D;
        double uy = dy / d2D;

        // Perpendicular (lateral) unit vector in the horizontal plane
        double perpX = -uy;
        double perpY = ux;

        // Random lateral deflection angle in [-spreadAngleRad, +spreadAngleRad]
        double randLateral = (static_cast<double>(rand() % 2001) - 1000.0) / 1000.0; // [-1.0, +1.0]
        double theta = randLateral * spreadAngleRad;
        double lateralOffset = d2D * tan(theta);

        // Random front-to-back longitudinal deflection in [-effectiveSpreadDepth, +effectiveSpreadDepth]
        double randLongitudinal = (static_cast<double>(rand() % 2001) - 1000.0) / 1000.0; // [-1.0, +1.0]
        double longitudinalOffset = randLongitudinal * effectiveSpreadDepth;

        pAimedLocW->x += static_cast<int>(round(lateralOffset * perpX + longitudinalOffset * ux));
        pAimedLocW->y += static_cast<int>(round(lateralOffset * perpY + longitudinalOffset * uy));
    }
}

void PedInstance::getAccuracy(double &base_acc)
{
    if (obj_group_def_ == PedInstance::og_dmAgent)
    {
        double pNorm = static_cast<double>(perception_.amount()) / 100.0;
        if (pNorm < 0.0) pNorm = 0.0;
        if (pNorm > 1.0) pNorm = 1.0;

        // Base unmodded accuracy scales from 0.10 (at 0% Perception) to 0.85 (at 100% Perception)
        double acc = 0.10 + 0.75 * pNorm;

        // Cybernetic modifications add additional accuracy bonuses
        double modBonus = 0.0;
        Mod *pMod = slots_[Mod::MOD_EYES];
        if (pMod) {
            modBonus += 0.05 * (pMod->getVersion() + 1); // +0.05 (V1), +0.10 (V2), +0.15 (V3)
        }
        pMod = slots_[Mod::MOD_BRAIN];
        if (pMod) {
            modBonus += 0.01 * (pMod->getVersion() + 1);
        }
        pMod = slots_[Mod::MOD_ARMS];
        if (pMod) {
            modBonus += 0.01 * (pMod->getVersion() + 1);
        }
        pMod = slots_[Mod::MOD_HEART];
        if (pMod) {
            modBonus += 0.01 * (pMod->getVersion() + 1);
        }
        pMod = slots_[Mod::MOD_LEGS];
        if (pMod) {
            modBonus += 0.01 * (pMod->getVersion() + 1);
        }

        acc += modBonus;

        // High adrenaline adds slight jitter penalty
        if (adrenaline_.getMultiplier() > 1.0f) {
            acc -= 0.05 * (adrenaline_.getMultiplier() - 1.0f);
        }

        if (acc < 0.05) acc = 0.05;
        if (acc > 0.98) acc = 0.98;

        base_acc = acc;
    } else {
        base_mod_acc_ = (base_mod_acc_ <= 0.0) ? 0.5 : base_mod_acc_;
        double acc = base_mod_acc_ * (perception_.getMultiplier() / 2.0f);
        if (acc < 0.05) acc = 0.05;
        if (acc > 0.98) acc = 0.98;
        base_acc = acc;
    }
}

bool PedInstance::hasAccessCard()
{
    WeaponInstance * wi = selectedWeapon();
    Mod *pMod = slots_[Mod::MOD_BRAIN];
    return wi && pMod && wi->isInstanceOf(Weapon::AccessCard)
        && pMod->getVersion() == Mod::MOD_V3 ? true : false;
}

/*!
 * Returns the number of points an agent must have to persuade
 * a ped of given type. Civilians or criminals are always persuaded.
 * \param aType The type of the ped to persuade.
 */
uint16_t PedInstance::getRequiredPointsToPersuade(PedType aType) {
    Mod *pMod = slots_[Mod::MOD_BRAIN];
    int baseCost = PedManager::getStats(aType).persuasion_cost;

    if (aType == kPedTypeCivilian || aType == kPedTypeCriminal) {
        return 0;
    }

    uint16_t points = static_cast<uint16_t>(baseCost);
    if (aType == kPedTypeGuard) {
        if (!pMod) {
            points = static_cast<uint16_t>(baseCost * 2);
        } else if (pMod->getVersion() == Mod::MOD_V1) {
            points = static_cast<uint16_t>(baseCost);
        } else {
            points = 1;
        }
    } else if (aType == kPedTypePolice) {
        if (!pMod) {
            points = static_cast<uint16_t>(baseCost * 2);
        } else if (pMod->getVersion() == Mod::MOD_V1) {
            points = static_cast<uint16_t>(baseCost);
        } else if (pMod->getVersion() == Mod::MOD_V2) {
            points = 3;
        } else if (pMod->getVersion() == Mod::MOD_V3) {
            points = static_cast<uint16_t>(baseCost / 2);
        }
    } else if (aType == kPedTypeAgent) {
        if (!pMod) {
            points = static_cast<uint16_t>(baseCost * 4);
        } else if (pMod->getVersion() == Mod::MOD_V1) {
            points = static_cast<uint16_t>(baseCost * 2);
        } else if (pMod->getVersion() == Mod::MOD_V2) {
            points = 11;
        } else if (pMod->getVersion() == Mod::MOD_V3) {
            points = static_cast<uint16_t>(baseCost);
        }
    }

    return points;
}

/*!
 * Return true if this agent can persuade the given ped.
 * A ped can be persuaded if he's not already persuaded or
 * if its persuasion points are less or equal than the agent
 * total persuasion points.
 * \param pOtherPed Ped to persuade.
 * \param persuadotronRange Distance under which a ped can be persuaded
 */
bool PedInstance::canPersuade(PedInstance *pOtherPed, const int persuadotronRange) {
    Action *pAction = pOtherPed->currentAction();
    if (pAction != NULL && pAction->type() == Action::kActTypeHit) {
        // cannot persuade a ped if he's currently being hit
        return false;
    }

    if (!pOtherPed->isPersuaded() && pOtherPed->isAlive() &&
            isCloseTo(pOtherPed, persuadotronRange)) {
        uint16_t points = getRequiredPointsToPersuade(pOtherPed->type());
        return points <= totalPersuasionPoints_;
    }

    return false;
}

/*!
 * Called when an agent tries to persuad this ped.
 * \param pAgent Agent trying to persuad
 */
void PedInstance::handlePersuadedBy(PedInstance *pAgent) {
    pAgent->addPersuaded(this);
    setObjGroupID(pAgent->objGroupID());
    owner_ = pAgent;
    setPanicImmuned();

    behaviour_.replaceAllcomponentsBy(new PersuadedBehaviourComponent());

    if (pAgent->isArmed()) {
        selectShootingWeaponWithAmmo();
    } else if (isAnyWeaponSelected()) {
        deselectWeapon();
    }

    /////////////////// Check if still useful ////////////
    pAgent->cpyEnemyDefs(enemy_group_defs_);
    friends_found_.clear();
    hostiles_found_.clear();
    hostile_desc_ = pAgent->hostileDesc();
    //////////////////////////////////////////////////////
}

/*!
 * Adds given ped to the list of persuaded peds by this agent.
 * Increments the persuasion points of this ped depending on the type
 * of persuaded ped.
 * \param p Persuaded ped
 */
void PedInstance::addPersuaded(PedInstance *p) {
    persuadedSet_.insert(p);
    totalPersuasionPoints_ += PedManager::getStats(p->type()).persuasion_cost;
}

/*!
 * Removes the given ped p from the list of persuaded peds
 * @param p The PedInstance to remove
 */
void PedInstance::removePersuaded(PedInstance *p) {
    std::set <PedInstance *>::iterator it =  persuadedSet_.find(p);
    if (it != persuadedSet_.end())
        persuadedSet_.erase(it);
}

/*!
 * After a ped died (agent or other), updates the relation between the owner
 * and the peds he has persuaded.
 * If dead ped is a persuaded, just removed him from the list of his owner.
 * If dead ped is our agent, transfer all his persuaded to another living agent.
 * \param pSquad List of available agents
 */
void PedInstance::updatePersuadedRelations(Squad *pSquad) {
    if (isPersuaded()) {
        owner_->removePersuaded(this);
        owner_ = NULL;
    } else if (isOurAgent()) {
        // our agent is dead, assign all persuaded to another living agent
        for (uint8_t i = 0; i < Squad::kMaxSlot; ++i) {
            PedInstance *pAgent = pSquad->member(i);
            if (pAgent && pAgent->isAlive()) {
                while(!persuadedSet_.empty()) {
                    std::set <PedInstance *>::iterator it = persuadedSet_.begin();
                    PedInstance *pPed = *it;
                    persuadedSet_.erase(it);
                    pPed->setNewOwner(pAgent);
                }
                break;
            }
        }
    }
}

void PedInstance::informPersuadedToEnterVehicle(Vehicle *pVehicle) {
    ;
    for (std::set <PedInstance *>::iterator it = persuadedSet_.begin();
        it != persuadedSet_.end(); it++) {
            (*it)->behaviour().handleBehaviourEvent(Behaviour::kBehvEvtEnterVehicle, pVehicle);
    }
}

void PedInstance::dropPersuadedFromCar(Vehicle *pCar) {
    for (std::set <PedInstance *>::iterator it = persuadedSet_.begin();
        it != persuadedSet_.end(); it++) {
            pCar->dropPassenger(*it);
            // default action is to follow owner
            (*it)->setCurrentActionWithSource(Action::kActionDefault);
    }
}

uint16_t PedInstance::modsBitmaskFromTier(int tier) {
    switch (tier) {
        case 1: return 0x0AAA;
        case 2: return 0x1554;
        case 3: return 0x1FFE;
        case 0:
        default: return 0x0000;
    }
}

int PedInstance::tierFromModsBitmask(uint16_t mask) {
    mask &= 0x7FFF;
    if (mask == 0x0AAA) return 1;
    if (mask == 0x1554) return 2;
    if (mask == 0x1FFE) return 3;
    return 0;
}

uint16_t PedInstance::getModsInfoBitmask() const {
    uint16_t mask = 0;
    auto encodeMod = [&](Mod::EModType type, int bit_shift) {
        Mod *pMod = slots_[type];
        if (pMod) {
            int ver = static_cast<int>(pMod->getVersion()) + 1; // 1=V1, 2=V2, 3=V3
            mask |= static_cast<uint16_t>((ver & 3) << bit_shift);
        }
    };
    encodeMod(Mod::MOD_LEGS, 1);
    encodeMod(Mod::MOD_ARMS, 3);
    encodeMod(Mod::MOD_CHEST, 5);
    encodeMod(Mod::MOD_HEART, 7);
    encodeMod(Mod::MOD_EYES, 9);
    encodeMod(Mod::MOD_BRAIN, 11);
    return mask;
}

void PedInstance::applyCyberwareTier(int tier) {
    clearSlots();
    if (tier < 0) tier = 0;
    if (tier > 3) tier = 3;
    cyberwareTier_ = tier;
    if (tier == 0) {
        return;
    }
    Mod::EModVersion ver = static_cast<Mod::EModVersion>(tier - 1);
    addMod(g_modMgr.getMod(Mod::MOD_LEGS, ver));
    addMod(g_modMgr.getMod(Mod::MOD_ARMS, ver));
    addMod(g_modMgr.getMod(Mod::MOD_CHEST, ver));
    addMod(g_modMgr.getMod(Mod::MOD_HEART, ver));
    addMod(g_modMgr.getMod(Mod::MOD_EYES, ver));
    addMod(g_modMgr.getMod(Mod::MOD_BRAIN, ver));
}

bool PedInstance::isHunter() const {
    auto *pHos = behaviour_.getComponent<PlayerHostileBehaviourComponent>();
    if (pHos) return pHos->isHunter();
    return isHunter_;
}

void PedInstance::setHunter(bool hunter) {
    isHunter_ = hunter;
    auto *pHos = behaviour_.getComponent<PlayerHostileBehaviourComponent>();
    if (pHos) pHos->setHunter(hunter);
}

}