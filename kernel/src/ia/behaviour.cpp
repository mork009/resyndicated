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
 *   Copyright (C) 2014, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-kernel/ia/behaviour.h"

#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/squad.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/ia/actions.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "rs-utils/io/file.h"

namespace rs_knl {
namespace {
/*!
 * Compute perception-scaled detection range matching the original DOS Syndicate formula:
 * range * (amount - dependency) / (max - dependency).
 * Falls back to neutralRange when amount <= dependency (e.g. default neutral test IPA).
 */
double calcScaledPerceptionRange(PedInstance *pPed, int maxDosRange, int neutralRange) {
    if (!pPed) {
        return static_cast<double>(neutralRange);
    }
    int amt = pPed->perception().amount();
    int dep = pPed->perception().dependency();
    if (amt > dep && dep < 100) {
        double ratio = static_cast<double>(amt - dep) / static_cast<double>(100 - dep);
        return static_cast<double>(maxDosRange) * ratio;
    }
    return static_cast<double>(neutralRange > 0 ? neutralRange : pPed->sightRange());
}

/*!
 * Variable Police disengagement cooldown (ms) after an engaged target holsters their weapon.
 * Scales inversely with Adrenaline (matching DOS MAIN.EXE 0x311d8 / 0x30028 timer behavior)
 * plus per-encounter variation so cops may still pursue and shoot briefly before disengaging.
 */
int calcPoliceDisengageCooldownMs(PedInstance *pPed) {
    float adrMult = pPed ? pPed->adrenaline().getMultiplier() : 1.0f;
    int baseMs = static_cast<int>(std::round(2400.0f - (adrMult - 0.5f) * 700.0f));
    int jitterMs = (std::rand() % 901) - 450;
    return std::clamp(baseMs + jitterMs, 750, 2600);
}
} // namespace
//*************************************
// Constant definition
//*************************************
const int CommonAgentBehaviourComponent::kRegeratesHealthStep = 1;
const int PanicComponent::kScoutDistance = 1500;
const int PanicComponent::kDistanceToRun = 3000;
const double PersuadedBehaviourComponent::kMaxRangeForSearchingWeapon = 768.0;
const int PoliceBehaviourComponent::kPoliceScoutDistance = 1500;
const int PoliceBehaviourComponent::kPolicePendingTime = 1500;
const int PlayerHostileBehaviourComponent::kEnemyScoutDistance = 1500;

Behaviour::~Behaviour() {
    destroyComponents();
}

void Behaviour::destroyComponents() {
    while(compLst_.size() != 0) {
        BehaviourComponent *pComp = compLst_.front();
        compLst_.pop_front();
        delete pComp;
    }
}

void Behaviour::handleBehaviourEvent(BehaviourEventType evtType, void *pCtxt) {
    if (pThisPed_ && pThisPed_->inVehicle() != nullptr && evtType != kBehvEvtEjectedFromVehicle) {
        return;
    }
    for (std::list < BehaviourComponent * >::iterator it = compLst_.begin();
            it != compLst_.end(); it++) {
        BehaviourEvent event {pThisPed_, evtType, pCtxt};
        (*it)->handleBehaviourEvent(event);
    }
}

void Behaviour::addComponent(BehaviourComponent *pComp) {
    compLst_.push_back(pComp);
}

/*!
 * Destroy existing components and set given one as new one
 */
void Behaviour::replaceAllcomponentsBy(BehaviourComponent *pComp) {
    destroyComponents();
    addComponent(pComp);
}

bool Behaviour::isInPanic() const {
    for (auto* comp : compLst_) {
        PanicComponent* pComp = dynamic_cast<PanicComponent*>(comp);
        if (pComp && pComp->isInPanic()) {
            return true;
        }
    }
    return false;
}

bool Behaviour::hasHostileEngaged() const {
    for (auto* comp : compLst_) {
        PlayerHostileBehaviourComponent* pHost = dynamic_cast<PlayerHostileBehaviourComponent*>(comp);
        if (pHost && pHost->status() == PlayerHostileBehaviourComponent::kHostileStatusFollowAndShoot) {
            return true;
        }
    }
    return false;
}

void Behaviour::engageHostileTarget(PedInstance *pTarget) {
    for (auto* comp : compLst_) {
        PlayerHostileBehaviourComponent* pHost = dynamic_cast<PlayerHostileBehaviourComponent*>(comp);
        if (pHost && pTarget) {
            pHost->followAndShootTarget(pThisPed_, pTarget);
        }
    }
}

/*!
 * Run the execute method  of each component listed in the behaviour.
 * Component must be enabled.
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 */
void Behaviour::execute(uint32_t elapsed, Mission *pMission) {
    if (pThisPed_->isDead() || !pThisPed_->isAlive() || pThisPed_->isDying() || pThisPed_->isState(kPedActionStateHit)) {
        return;
    }
    if (pThisPed_->inVehicle() != nullptr) {
        return;
    }

    for (std::list < BehaviourComponent * >::iterator it = compLst_.begin();
            it != compLst_.end(); it++) {
        BehaviourComponent *pComp = *it;
        if (pComp->isEnabled()) {
            Behaviour::BehaviourParam param {elapsed, pMission, pThisPed_};
            pComp->execute(param);
        }
    }
}

CommonAgentBehaviourComponent::CommonAgentBehaviourComponent(PedInstance *pPed):
        BehaviourComponent(),
        healthTimer_(pPed->getHealthRegenerationPeriod()),
        pAutoTarget_(nullptr),
        reactionDelayRemaining_(0),
        inCombatEngaged_(false) {
    doRegenerates_ = false;
}

/*!
 *
 * \param elapsed Time elapsed since last frame
 * \param pMission Mission data
 * \param pPed The owner of the behaviour
 */
void CommonAgentBehaviourComponent::execute(const Behaviour::BehaviourParam &param) {
    if (!param.pPed || !param.pPed->isAlive() || param.pPed->isDead() || param.pPed->isDying()) {
        pAutoTarget_ = nullptr;
        reactionDelayRemaining_ = 0;
        inCombatEngaged_ = false;
        return;
    }

    // 1. Health Regeneration (Chest V1/V2/V3 fixed period above threshold; 800ms when Adrenaline < 4%)
    uint16_t period = param.pPed->getHealthRegenerationPeriod();
    if (period > 0) {
        if (!doRegenerates_ && param.pPed->health() < param.pPed->maxHealth()) {
            doRegenerates_ = true;
        }
        if (doRegenerates_) {
            // In original DOS Syndicate (0x2fae7..0x2fb65), when Adrenaline < 10/255 (< 4%),
            // the regeneration tick counter wraps mod 8, healing all Chest tiers (V1-V3) at 800ms.
            // Above that threshold, each Chest tier heals at its fixed period independent of Adrenaline.
            uint16_t effectivePeriod = (param.pPed->adrenaline().amount() < 4) ? 800 : period;
            healthTimer_.setMax(effectivePeriod);
            if (healthTimer_.update(param.elapsed)) {
                if (param.pPed->increaseHealth(kRegeratesHealthStep)) {
                    doRegenerates_ = false;
                }
            }
        }
    } else {
        doRegenerates_ = false;
    }

    // 2. Intelligence: Tactical Autonomy (Only active when Intelligence > 55%)
    uint8_t inteliAmount = param.pPed->intelligence().amount();
    if (inteliAmount <= 55 || param.pPed->isCloneShieldActivated()) {
        // Intelligence <= 55%: No autonomous reaction / pure player manual control
        if (pAutoTarget_ != nullptr) {
            param.pPed->stopShooting();
            pAutoTarget_ = nullptr;
        }
        reactionDelayRemaining_ = 0;
        inCombatEngaged_ = false;
        return;
    }

    if (!param.pMission) {
        return;
    }

    // Check currently active autonomous shooting target
    if (pAutoTarget_ != nullptr) {
        bool targetDead = (!pAutoTarget_->isAlive() || pAutoTarget_->isDead() || pAutoTarget_->isDying());
        bool targetRecoiling = pAutoTarget_->isState(kPedActionStateHit);
        bool outOfReach = false;
        WeaponInstance *curWpn = param.pPed->selectedWeapon();
        if (curWpn) {
            double dist = param.pPed->distanceToPosition(pAutoTarget_->position());
            if (dist > static_cast<double>(curWpn->range())) {
                outOfReach = true;
            }
        } else {
            outOfReach = true;
        }

        if (targetDead || targetRecoiling || outOfReach) {
            // Target is dead, dying, recoiling from a hit, or out of reach:
            // Stop firing at this target immediately!
            param.pPed->stopShooting();
            pAutoTarget_ = nullptr;
        } else if (param.pPed->isUsingWeapon()) {
            // Target is alive, within reach, and NOT recoiling: continue aiming and firing
            WorldPoint curTargetLocW;
            curTargetLocW.convertFromTilePoint(pAutoTarget_->position());
            curTargetLocW.z += pAutoTarget_->sizeZ() >> 1;
            param.pPed->updateShootingTarget(curTargetLocW);
        }
    }

    // Threat Scanning:
    // Only detect actual hostile threats (enemy agents or NPCs with active weapons unholstered).
    struct ThreatInfo {
        double dist;
        PedInstance *pPed;
    };
    std::vector<ThreatInfo> threats;
    double threatRange = 1600.0 * static_cast<double>(param.pPed->perception().getMultiplier());

    for (size_t i = 0; i < param.pMission->numPeds(); ++i) {
        PedInstance *pOther = param.pMission->ped(i);
        if (!pOther || !pOther->isAlive() || pOther->isDead() || pOther->isDying() || pOther == param.pPed) {
            continue;
        }
        if (param.pPed->isFriendWith(pOther) || param.pPed->isFriendlyFireProtected(pOther)) {
            continue;
        }

        bool isHostile = false;
        bool otherDisguised = pOther->isCloneShieldActivated() && param.pPed->type() != PedInstance::kPedTypeAgent;
        if (pOther->type() == PedInstance::kPedTypeAgent && !otherDisguised) {
            // Rival syndicate agents are always active threats
            isHostile = true;
        } else if (pOther->isAnyWeaponSelected() && pOther->selectedWeapon()->canShoot()) {
            // Police, guards, criminals or other peds with an unholstered active weapon ready to fire
            isHostile = true;
        }

        if (!isHostile) {
            continue;
        }

        double dist = param.pPed->distanceToPosition(pOther->position());
        if (dist <= threatRange) {
            threats.push_back({dist, pOther});
        }
    }

    // Auto-Wield: Only draw a weapon if there are active threats in range
    if (!threats.empty()) {
        WeaponInstance *curWpn = param.pPed->selectedWeapon();
        bool needsWield = (curWpn == nullptr || curWpn->ammoRemaining() <= 0 || !curWpn->canShoot());
        if (!needsWield && curWpn) {
            Weapon::WeaponType wt = curWpn->type();
            if (wt == Weapon::MediKit || wt == Weapon::Scanner || wt == Weapon::TimeBomb ||
                wt == Weapon::AccessCard || wt == Weapon::EnergyShield || wt == Weapon::Persuadatron ||
                wt == Weapon::Unknown) {
                needsWield = true;
            }
        }
        if (needsWield) {
            WeaponInstance *bestWpn = param.pPed->getBestDamagingWeaponWithAmmo();
            if (bestWpn && bestWpn != curWpn) {
                param.pPed->selectWeapon(bestWpn);
            }
        }
    }

    // Auto-Defense Firing (Intelligence > 55%)
    WeaponInstance *wpn = param.pPed->selectedWeapon();
    if (wpn && wpn->canShoot() && wpn->ammoRemaining() > 0 && param.pPed->canAddUseWeaponAction()) {
        struct ReachableThreat {
            double dist;
            bool isRecoiling;
            PedInstance *pPed;
            WorldPoint targetLocW;
        };
        std::vector<ReachableThreat> reachableThreats;

        WorldPoint originLocW;
        originLocW.convertFromTilePoint(param.pPed->position());
        originLocW.z += param.pPed->sizeZ() >> 1;

        Mission::BlockerCriteria crits;
        crits.setBlocker = true;
        crits.maxr = wpn->range();
        crits.pOrigin = param.pPed;

        for (auto &t : threats) {
            if (t.dist > static_cast<double>(wpn->range())) {
                continue;
            }
            WorldPoint targetLocW;
            targetLocW.convertFromTilePoint(t.pPed->position());
            targetLocW.z += t.pPed->sizeZ() >> 1;

            if (param.pMission->map() != nullptr) {
                ShootableMapObject *pTargetHit = nullptr;
                WorldPoint impactPosW = targetLocW;
                param.pMission->checkIfBlockersInShootingLine(originLocW, crits, &pTargetHit, &impactPosW);

                if (pTargetHit == t.pPed || (pTargetHit == nullptr && param.pPed->distanceToPosition(impactPosW) >= t.dist - 64.0)) {
                    bool recoiling = t.pPed->isState(kPedActionStateHit);
                    reachableThreats.push_back({t.dist, recoiling, t.pPed, targetLocW});
                }
            } else {
                bool recoiling = t.pPed->isState(kPedActionStateHit);
                reachableThreats.push_back({t.dist, recoiling, t.pPed, targetLocW});
            }
        }

        if (!reachableThreats.empty()) {
            // Find closest non-recoiling threat
            ReachableThreat *pChosen = nullptr;
            double bestDist = 1e9;
            for (auto &rt : reachableThreats) {
                if (!rt.isRecoiling && rt.dist < bestDist) {
                    bestDist = rt.dist;
                    pChosen = &rt;
                }
            }

            if (pChosen != nullptr) {
                PedInstance *chosenTarget = pChosen->pPed;
                WorldPoint chosenTargetLocW = pChosen->targetLocW;

                if (!inCombatEngaged_) {
                    // Initial entry into combat: calculate reaction delay
                    // At 60% Intelligence: 3000ms delay; at 100% Intelligence: 0ms delay
                    double factor = (100.0 - static_cast<double>(inteliAmount)) / 40.0;
                    if (factor < 0.0) factor = 0.0;
                    if (factor > 1.0) factor = 1.0;
                    double baseDelay = 3000.0 * factor;
                    double brainFactor = 1.0;
                    Mod *pBrainMod = param.pPed->slot(Mod::MOD_BRAIN);
                    if (pBrainMod) {
                        brainFactor = 1.0 - 0.10 * (pBrainMod->getVersion() + 1);
                        if (brainFactor < 0.3) brainFactor = 0.3;
                    }
                    reactionDelayRemaining_ = static_cast<int>(baseDelay * brainFactor);
                    inCombatEngaged_ = true;
                }

                if (param.pPed->canAddUseWeaponAction()) {
                    if (reactionDelayRemaining_ > 0) {
                        // Face the target while reacting / acquiring
                        param.pPed->setDirectionTowardPosition(chosenTargetLocW);
                        reactionDelayRemaining_ -= static_cast<int>(param.elapsed);
                    } else {
                        // Instant fire / immediate switch to non-recoiling target
                        pAutoTarget_ = chosenTarget;
                        param.pPed->addActionShootAt(chosenTargetLocW);
                    }
                } else {
                    // While reloading weapon, face the chosen target
                    param.pPed->setDirectionTowardPosition(chosenTargetLocW);
                }
            } else {
                // All reachable threats are currently recoiling: hold fire and face closest threat
                std::sort(reachableThreats.begin(), reachableThreats.end(),
                          [](const ReachableThreat &a, const ReachableThreat &b) {
                              return a.dist < b.dist;
                          });
                param.pPed->setDirectionTowardPosition(reachableThreats.front().targetLocW);
                if (pAutoTarget_ != nullptr) {
                    param.pPed->stopShooting();
                    pAutoTarget_ = nullptr;
                }
            }
        } else {
            // No reachable threats: make sure shooting is stopped and combat disengages
            if (pAutoTarget_ != nullptr) {
                param.pPed->stopShooting();
                pAutoTarget_ = nullptr;
            }
            reactionDelayRemaining_ = 0;
            inCombatEngaged_ = false;
        }
    }

    // Weapon Cycling on Ammo Depletion
    if (param.pPed->isAnyWeaponSelected() && param.pPed->selectedWeapon()->ammoRemaining() == 0) {
        if (pAutoTarget_ != nullptr) {
            param.pPed->stopShooting();
            pAutoTarget_ = nullptr;
        }
        WeaponInstance *nextWpn = param.pPed->getBestDamagingWeaponWithAmmo();
        if (nextWpn) {
            param.pPed->selectWeapon(nextWpn);
        } else {
            param.pPed->deselectWeapon();
        }
    }
}

void CommonAgentBehaviourComponent::handleBehaviourEvent(const Behaviour::BehaviourEvent &event) {
    switch(event.evtType) {
    case Behaviour::kBehvEvtHit:
        if (event.pPed->hasMinimumVersionOfMod(Mod::MOD_CHEST, Mod::MOD_V1)) {
            doRegenerates_ = true;
        }
        break;
    default:
        break;
    }
}

PersuaderBehaviourComponent::PersuaderBehaviourComponent():
        BehaviourComponent() {
    doUsePersuadotron_ = false;
    persuadotronRange_ = g_weaponMgr.getWeapon(Weapon::Persuadatron)->range();
}

void PersuaderBehaviourComponent::execute(const Behaviour::BehaviourParam &param) {
    // Check if Agent has selected his Persuadotron
    if (doUsePersuadotron_) {
        int range = persuadotronRange_;
        Mod *pBrainMod = param.pPed->slot(Mod::MOD_BRAIN);
        if (pBrainMod) {
            double brainMult = 1.0 + 0.5 * (pBrainMod->getVersion() + 1);
            range = static_cast<int>(persuadotronRange_ * brainMult);
        }
        // iterate through all peds except our agents
        for (size_t i = param.pMission->getSquad()->size(); i < param.pMission->numPeds(); i++) {
            PedInstance *pOtherPed = param.pMission->ped(i);
            if (param.pPed->canPersuade(pOtherPed, range)) {
                DamageToInflict dmg;
                dmg.dtype = kDmgTypePersuasion;
                dmg.d_owner = param.pPed;
                pOtherPed->insertHitAction(dmg);
            }
        }
    }
}

void PersuaderBehaviourComponent::handleBehaviourEvent(const Behaviour::BehaviourEvent &event) {
    switch(event.evtType) {
    case Behaviour::kBehvEvtPersuadotronActivated:
        doUsePersuadotron_ = true;
        break;
    case Behaviour::kBehvEvtPersuadotronDeactivated:
        doUsePersuadotron_ = false;
        break;
    default:
        break;
    }
}

PersuadedBehaviourComponent::PersuadedBehaviourComponent():
        BehaviourComponent(), checkWeaponTimer_(500) {
    status_ = kPersuadStatusWaitForHitAction;
}

void PersuadedBehaviourComponent::execute(const Behaviour::BehaviourParam &param) {
    if (status_ == kPersuadStatusWaitForHitAction) {
        if (param.pPed->currentAction() == nullptr) {
            status_ = kPersuadStatusInitializing;
        }
    }

    if (status_ == kPersuadStatusInitializing ||
        (status_ == kPersuadStatusFollow && param.pPed->currentAction() == nullptr)) {
        param.pPed->destroyAllActions(true);
        // set follow owner as new default action
        FollowAction *pAction = new FollowAction(param.pPed->owner());
        param.pPed->addToDefaultActions(pAction);
        param.pPed->addToDefaultActions(new ResetScriptedAction(Action::kActionDefault));
        param.pPed->addMovementAction(pAction, false);
        status_ = kPersuadStatusFollow;
    }

    // Check if ped needs a weapon:
    // 1) Not wielding a weapon (selectedWeapon is null or has no shooting weapon with ammo)
    // 2) Selected weapon has 0 ammo or cannot shoot
    bool needsWeapon = false;
    bool ownerArmed = (param.pPed->owner() && param.pPed->owner()->isAlive() && param.pPed->owner()->isArmed());

    if (param.pPed->selectedWeapon() == nullptr) {
        bool hasWeaponWithAmmo = false;
        for (size_t i = 0; i < param.pPed->numWeapons(); ++i) {
            WeaponInstance *wi = param.pPed->weapon(i);
            if (wi && wi->canShoot() && wi->ammoRemaining() > 0) {
                hasWeaponWithAmmo = true;
                break;
            }
        }
        if (!hasWeaponWithAmmo) {
            needsWeapon = true;
        } else if (ownerArmed) {
            param.pPed->selectShootingWeaponWithAmmo();
        }
    } else if (param.pPed->selectedWeapon()->ammoRemaining() == 0 || !param.pPed->selectedWeapon()->canShoot()) {
        needsWeapon = true;
    } else if (!ownerArmed) {
        param.pPed->deselectWeapon();
    }

    if (status_ == kPersuadStatusTakeWeapon) {
        if (param.pPed->currentAction() == nullptr) {
            status_ = kPersuadStatusFollow;
        }
    }

    if (needsWeapon && (status_ == kPersuadStatusFollow || status_ == kPersuadStatusLookForWeapon)) {
        if (checkWeaponTimer_.update(param.elapsed)) {
            WeaponInstance *pWeapon = findWeaponWithAmmo(param.pMission, param.pPed);
            if (pWeapon) {
                // If holding an empty weapon, drop it
                if (param.pPed->selectedWeapon() != nullptr && param.pPed->selectedWeapon()->ammoRemaining() == 0) {
                    param.pPed->dropWeapon(0);
                }

                // a weapon is found
                // initiate alternative actions : go to weapon and take it
                status_ = kPersuadStatusTakeWeapon;
                param.pPed->destroyAltActions();
                MovementAction *pActions = param.pPed->createActionPickup(pWeapon);
                // set a warning after picking up weapon so we know we can select it
                pActions->next()->setWarnBehaviour(true);
                // add a reset action to automatically go back to follow owner after picking up weapon
                pActions->next()->link(
                    new ResetScriptedAction(Action::kActionDefault));
                param.pPed->addToAltActions(pActions);
                // execute alternative actions
                param.pPed->setCurrentActionWithSource(Action::kActionAlt);
            }
        }
    }
}

void PersuadedBehaviourComponent::handleBehaviourEvent(const Behaviour::BehaviourEvent &event) {
    if (event.evtType == Behaviour::kBehvEvtWeaponOut) {
        PedInstance *pPedSource = static_cast<PedInstance *> (event.pCtxt);
        if (pPedSource == event.pPed->owner()) {
            // the ped who is armed is our owner so select weapon or look for one
            event.pPed->selectShootingWeaponWithAmmo();
            if (!event.pPed->isArmed()) {
                // ped has no weapon -> start looking for some
                status_ = kPersuadStatusLookForWeapon;
            }
        }
    } else if (event.evtType == Behaviour::kBehvEvtWeaponCleared) {
        PedInstance *pPedSource = static_cast<PedInstance *> (event.pCtxt);
        if (pPedSource == event.pPed->owner()) {
            // the ped who cleared his weapon is our owner so deselect weapon or
            // stop searching for one
            event.pPed->deselectWeapon();
            if (status_ == kPersuadStatusLookForWeapon) {
                status_ = kPersuadStatusFollow;
            }
        }
    } else if (event.evtType == Behaviour::kBehvEvtActionEnded) {
        if (status_ == kPersuadStatusWaitForHitAction) {
            status_ = kPersuadStatusInitializing;
        } else {
            Action::ActionType *pType = static_cast<Action::ActionType *> (event.pCtxt);
            if (*pType == Action::kActTypePickUp) {
                if (event.pPed->owner() && event.pPed->owner()->isAlive() && event.pPed->owner()->isArmed()) {
                    event.pPed->selectShootingWeaponWithAmmo();
                }
                // weapon found so back to normal
                status_ = kPersuadStatusFollow;
            } else if (*pType == Action::kActTypeDrop) {
                // weapon dropped so look for another
                status_ = kPersuadStatusLookForWeapon;
            }
        }
    } else if (event.evtType == Behaviour::kBehvEvtEnterVehicle) {
        Vehicle *pVehicle = static_cast<Vehicle *> (event.pCtxt);
        MovementAction *pAction =
                event.pPed->createActionEnterVehicle(pVehicle);
        event.pPed->addMovementAction(pAction, false);
        status_ = kPersuadStatusFollow;
    }
}

/*!
 * Look for weapon on the ground with ammo within 3 tiles (768 units).
 * The closest weapon within range will be returned.
 * \param pMission Mission data
 * \param pPed The ped searching for the weapon
 * \return NULL if no weapon is found.
 */
WeaponInstance * PersuadedBehaviourComponent::findWeaponWithAmmo(Mission *pMission, PedInstance *pPed) {
    if (!pMission || !pPed) return nullptr;

    WeaponInstance *pWeaponFound = nullptr;
    double currentDistance = kMaxRangeForSearchingWeapon;

    size_t numweapons = pMission->numWeaponsOnGround();
    for (size_t i = 0; i < numweapons; ++i) {
        WeaponInstance *w = pMission->weaponOnGround(i);
        if (w && !w->hasOwner() && !w->isDead() && w->canShoot() && w->ammoRemaining() > 0) {
            // Check vertical tile difference (within 1 z-level)
            if (std::abs(pPed->position().tz - w->position().tz) <= 1) {
                double dist = pPed->distanceToPosition(WorldPoint(w->position()));
                if (dist <= kMaxRangeForSearchingWeapon && dist < currentDistance) {
                    pWeaponFound = w;
                    currentDistance = dist;
                }
            }
        }
    }
    return pWeaponFound;
}

void PersuadedBehaviourComponent::changeTargetWeaponInAltActions(WeaponInstance *pWeapon, PedInstance *pPed) {
    MovementAction *pAction = pPed->altAction();
    while (pAction != NULL) {
        if (pAction->type() == Action::kActTypeWalk) {
            WalkAction *pWalk = static_cast<WalkAction *> (pAction);
            pWalk->setDestination(pWeapon);
        } else if (pAction->type() == Action::kActTypePickUp) {
            PickupWeaponAction *pWalk = static_cast<PickupWeaponAction *> (pAction);
            pWalk->setWeapon(pWeapon);
        }
        pAction = pAction->next();
    }
}

static bool isDirectionWalkable(Mission *pMission, const WorldPoint &pos, int dir) {
    if (!pMission || !pMission->mdpoints_) return true;
    int tx = pos.x / 256;
    int ty = pos.y / 256;
    int tz = pos.z / 128;
    if (tx < 0 || ty < 0 || tz < 0 || tx >= pMission->mmax_x_ || ty >= pMission->mmax_y_ || tz >= pMission->mmax_z_) {
        return false;
    }
    int idx = tx + ty * pMission->mmax_x_ + tz * pMission->mmax_m_xy;
    const FloodNode *fn = &pMission->mdpoints_[idx];
    if (fn->flags == kNonWalkable) {
        return false;
    }
    int d8 = ((dir + 16) & 0x00FF) / 32;
    static const uint8_t bitmasks[8] = {
        0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80
    };
    uint8_t mask = bitmasks[d8 % 8];
    bool currentTileAllows = (fn->dirsSame & mask) != 0 || (fn->dirsAbove & mask) != 0 || (fn->dirsBelow & mask) != 0;
    if (!currentTileAllows) {
        return false;
    }

    // Check adjacent target tile in candidate direction
    static const int dxTable[8] = { 0,  1, 1,  1,  0, -1, -1, -1 };
    static const int dyTable[8] = { 1,  1, 0, -1, -1, -1,  0,  1 };
    int ntx = tx + dxTable[d8 % 8];
    int nty = ty + dyTable[d8 % 8];
    if (ntx < 0 || nty < 0 || ntx >= pMission->mmax_x_ || nty >= pMission->mmax_y_) {
        return false;
    }
    int targetZ = tz;
    if ((fn->dirsAbove & mask) != 0 && tz + 1 < pMission->mmax_z_) {
        targetZ = tz + 1;
    } else if ((fn->dirsBelow & mask) != 0 && tz > 0) {
        targetZ = tz - 1;
    }
    int nidx = ntx + nty * pMission->mmax_x_ + targetZ * pMission->mmax_m_xy;
    if (pMission->mdpoints_[nidx].flags == kNonWalkable) {
        return false;
    }

    return true;
}

PanicComponent::PanicComponent():
        BehaviourComponent(), scoutTimer_(500) {
    backFromPanic_ = false;
    status_ = kPanicStatusCalm;
    // this component will be activated by event to
    // lower CPU consumption
    setEnabled(false);
}

void PanicComponent::calmDown(PedInstance *pPed) {
    status_ = kPanicStatusCalm;
    pArmedPed_ = nullptr;
    backFromPanic_ = true;
    scoutTimer_.setToMax();

    if (pPed) {
        pPed->cancelAltActions();
    }
}

void PanicComponent::execute(const Behaviour::BehaviourParam &param) {
    if (param.pPed->isImmunedToPanic()) {
        if (status_ == kPanicStatusInPanic) {
            calmDown(param.pPed);
        }
        return;
    }

    if (scoutTimer_.update(param.elapsed)) {
        PedInstance *pArmed = findNearbyArmedPed(param.pMission, param.pPed);
        if (!pArmed && status_ == kPanicStatusInPanic && pArmedPed_ != nullptr &&
            pArmedPed_->isAlive() && !pArmedPed_->isDead() && !pArmedPed_->isDying() &&
            pArmedPed_->isArmed() && param.pPed->isCloseTo(pArmedPed_, kDistanceToRun)) {
            // Hysteresis: keep fleeing until clear of kDistanceToRun or threat holsters/dies
            pArmed = pArmedPed_;
        }
        if (pArmed) {
            pArmedPed_ = pArmed;
            WorldPoint thisPos(param.pPed->position());
            WorldPoint armedPos(pArmedPed_->position());

            int toArmedX = armedPos.x - thisPos.x;
            int toArmedY = armedPos.y - thisPos.y;

            int curDir = param.pPed->direction();
            double cvx = sin((curDir / 128.0) * rs_utl::kPI);
            double cvy = cos((curDir / 128.0) * rs_utl::kPI);
            double dotToArmed = cvx * toArmedX + cvy * toArmedY;

            if (status_ == kPanicStatusCalm || param.pPed->altAction() == NULL || dotToArmed > 0) {
                runAway(param.pPed, param.pMission);
                status_ = kPanicStatusInPanic;
            }
        } else if (status_ == kPanicStatusInPanic) {
            // Threat is dead, holstered, or beyond flee distance: calm down
            calmDown(param.pPed);
        }
    }
}

void PanicComponent::handleBehaviourEvent(const Behaviour::BehaviourEvent &event) {
    if (event.pPed->isImmunedToPanic()) {
        return;
    }

    switch(event.evtType) {
    case Behaviour::kBehvEvtHit:
        if (!isEnabled()) {
            setEnabled(true);
        }
        {
            Mission *pMission = (rs_knl::MissionManager::singletonPtr() ? g_missionCtrl.mission() : nullptr);
            if (pMission) {
                pArmedPed_ = findNearbyArmedPed(pMission, event.pPed);
                if (!pArmedPed_ && event.pCtxt != nullptr) {
                    const DamageToInflict *pDmg = static_cast<const DamageToInflict *>(event.pCtxt);
                    PedInstance *pAttacker = pDmg->d_owner ? dynamic_cast<PedInstance *>(pDmg->d_owner) : nullptr;
                    if (pAttacker && pAttacker != event.pPed && pAttacker->isAlive() && !pAttacker->isDead() && !pAttacker->isDying()) {
                        pArmedPed_ = pAttacker;
                    }
                }
                if (pArmedPed_) {
                    runAway(event.pPed, pMission);
                    status_ = kPanicStatusInPanic;
                }
            }
        }
        break;
    case Behaviour::kBehvEvtWeaponOut:
        // Someone has shown a gun, so activate panic behaviour for peds
        if (!isEnabled()) {
            setEnabled(true);
            status_ = kPanicStatusCalm;
        }
        break;
    case Behaviour::kBehvEvtWeaponCleared:
        {
            Mission *pMission = (rs_knl::MissionManager::singletonPtr() ? g_missionCtrl.mission() : nullptr);
            if (!pMission || pMission->numArmedPeds() == 0) {
                calmDown(event.pPed);
                setEnabled(false);
            }
        }
        break;
    case Behaviour::kBehvEvtActionEnded:
        {
            Mission *pMission = (rs_knl::MissionManager::singletonPtr() ? g_missionCtrl.mission() : nullptr);
            if (pMission) {
                pArmedPed_ = findNearbyArmedPed(pMission, event.pPed);
            } else {
                pArmedPed_ = nullptr;
            }
            if (pArmedPed_) {
                runAway(event.pPed, pMission);
                status_ = kPanicStatusInPanic;
            } else {
                calmDown(event.pPed);
            }
        }
        break;
    default:
        break;
    }
}

/*!
 * Return the closest armed ped that is close to the given ped.
 * \param pMission
 * \param pPed
 * \return NULL if no ped is found
 */
PedInstance * PanicComponent::findNearbyArmedPed(Mission *pMission, PedInstance *pPed) {
    if (!pMission) {
        return NULL;
    }
    PedInstance *pClosest = NULL;
    double closestDist = 0;

    int neutralRange = pPed->sightRange() > 0 ? pPed->sightRange() : kScoutDistance;
    int scoutRange = static_cast<int>(std::round(calcScaledPerceptionRange(pPed, 2560, neutralRange)));
    for (size_t i = 0; i < pMission->numArmedPeds(); i++) {
        PedInstance *pOtherPed = pMission->armedPedAtIndex(i);
        if (pOtherPed && pOtherPed->isAlive() && !pOtherPed->isDead() && !pOtherPed->isDying()
            && pPed != pOtherPed && pPed->isCloseTo(pOtherPed, scoutRange)) {
            double dist = pPed->distanceToPosition(pOtherPed->position());
            if (pClosest == NULL || dist < closestDist) {
                pClosest = pOtherPed;
                closestDist = dist;
            }
        }
    }
    return pClosest;
}

/*!
 * Makes the ped run in the opposite way of the armed ped.
 * \param pPed The panicking ped
 * \param pMission Active mission
 */
void  PanicComponent::runAway(PedInstance *pPed, Mission *pMission) {
    if (!pArmedPed_) {
        return;
    }

    WorldPoint thisPos(pPed->position());
    WorldPoint armedPos(pArmedPed_->position());
    if (!pMission) {
        pMission = (rs_knl::MissionManager::singletonPtr() ? g_missionCtrl.mission() : nullptr);
    }

    int toArmedX = armedPos.x - thisPos.x;
    int toArmedY = armedPos.y - thisPos.y;

    int awayDx = thisPos.x - armedPos.x;
    int awayDy = thisPos.y - armedPos.y;
    if (awayDx == 0 && awayDy == 0) {
        awayDx = (rand() % 3) - 1;
        awayDy = (rand() % 3) - 1;
    }

    int idealDir = 0;
    pPed->setDirection(awayDx, awayDy, &idealDir);

    static const int offsets[5] = { 0, 32, -32, 64, -64 };
    int chosenDir = -1;

    for (int i = 0; i < 5; ++i) {
        int candDir = (idealDir + offsets[i] + 256) & 0x00FF;
        double cvx = sin((candDir / 128.0) * rs_utl::kPI);
        double cvy = cos((candDir / 128.0) * rs_utl::kPI);
        double cDot = cvx * toArmedX + cvy * toArmedY;

        if (cDot <= 0 && isDirectionWalkable(pMission, thisPos, candDir)) {
            chosenDir = candDir;
            break;
        }
    }

    if (chosenDir != -1) {
        pPed->setDirection(chosenDir);

        if (pPed->altAction() == NULL) {
            WalkToDirectionAction *pAction = new WalkToDirectionAction();
            pAction->setMaxDistanceToWalk(kDistanceToRun);
            pAction->setWarnBehaviour(true);
            pPed->addToAltActions(pAction);
            pPed->addToAltActions(new ResetScriptedAction(Action::kActionDefault));
        }
        pPed->setCurrentActionWithSource(Action::kActionAlt);
    } else {
        // Blocked in all candidate directions away from the armed ped:
        // Stop walking into the obstacle and cancel panic run action
        pPed->setDirection(idealDir);
        pPed->stop();
        pPed->cancelAltActions();
    }
}

PoliceBehaviourComponent::PoliceBehaviourComponent():
        BehaviourComponent(), scoutTimer_(200), disengageActive_(false), disengageRemainingMs_(0) {
    status_ = kPoliceStatusDefault;
    pTarget_ = NULL;
}

void PoliceBehaviourComponent::execute(const Behaviour::BehaviourParam &param) {
    if ((status_ == kPoliceStatusAlert || (status_ == kPoliceStatusDefault && param.pMission && param.pMission->numArmedPeds() != 0)) && scoutTimer_.update(param.elapsed)) {
        findAndEngageNewTarget(param.pMission, param.pPed);
    } else if (status_ == kPoliceStatusFollowAndShootTarget && (pTarget_ == nullptr || pTarget_->isDead() || pTarget_->isDying())) {
        disengageActive_ = false;
        disengageRemainingMs_ = 0;
        status_ = kPoliceStatusPendingEndFollow;
        pTarget_ = nullptr;
        param.pPed->stopShooting();
        param.pPed->clearDestination();
        WaitAction *pWait = new WaitAction(WaitAction::kWaitWeapon, kPolicePendingTime);
        pWait->setWarnBehaviour(true);
        param.pPed->addMovementAction(pWait, false);
    } else if (status_ == kPoliceStatusFollowAndShootTarget && disengageActive_) {
        if (pTarget_->selectedWeapon() != nullptr && pTarget_->selectedWeapon()->canShoot()) {
            // Target re-armed before cooldown finished: stay engaged
            disengageActive_ = false;
            disengageRemainingMs_ = 0;
        } else {
            disengageRemainingMs_ -= static_cast<int>(param.elapsed);
            if (disengageRemainingMs_ <= 0) {
                disengageActive_ = false;
                disengageRemainingMs_ = 0;
                status_ = kPoliceStatusPendingEndFollow;
                pTarget_ = nullptr;
                param.pPed->stopShooting();
                param.pPed->clearDestination();
                WaitAction *pWait = new WaitAction(WaitAction::kWaitWeapon, kPolicePendingTime);
                pWait->setWarnBehaviour(true);
                param.pPed->addMovementAction(pWait, false);
            }
        }
    } else if (status_ == kPoliceStatusCheckReengageOrDefault) {
        // check if there is a nearby enemy
        bool foundNewTarget = findAndEngageNewTarget(param.pMission, param.pPed);
        if (!foundNewTarget && !param.pPed->isCurrentActionFromSource(Action::kActionDefault)) {
            // there is no one around so go back to patrol if it's not already the case
            param.pPed->deselectWeapon();
            param.pPed->setCurrentActionWithSource(Action::kActionDefault);
            if (param.pMission && param.pMission->numArmedPeds() != 0) {
                // There are still some armed peds so keep on alert
                status_ = kPoliceStatusAlert;
            } else {
                status_ = kPoliceStatusDefault;
            }
        }
    }
}

void PoliceBehaviourComponent::handleBehaviourEvent(const Behaviour::BehaviourEvent &event) {
    switch(event.evtType) {
    case Behaviour::kBehvEvtHit:
        {
            const DamageToInflict *pDmg = static_cast<const DamageToInflict *>(event.pCtxt);
            PedInstance *pAttacker = (pDmg && pDmg->d_owner) ? dynamic_cast<PedInstance *>(pDmg->d_owner) : nullptr;
            if (pAttacker && pAttacker->isAlive() && !pAttacker->isDead() && pAttacker != event.pPed) {
                followAndShootTarget(event.pPed, pAttacker);
            } else if (status_ == kPoliceStatusDefault || status_ == kPoliceStatusCheckReengageOrDefault) {
                status_ = kPoliceStatusAlert;
            }
        }
        break;
    case Behaviour::kBehvEvtEjectedFromVehicle:
        handleEjectionFromVehicle(event.pPed, event.pCtxt);
        break;
    case Behaviour::kBehvEvtWeaponOut:
        if (status_ == kPoliceStatusFollowAndShootTarget && pTarget_ == event.pCtxt) {
            disengageActive_ = false;
            disengageRemainingMs_ = 0;
        } else if ((status_ == kPoliceStatusDefault || status_ == kPoliceStatusCheckReengageOrDefault) && !event.pPed->inVehicle()) {
            // When someone gets his weapon out, police is on alert
            status_ = kPoliceStatusAlert;
        }
        break;
    case Behaviour::kBehvEvtWeaponCleared:
        // Target has holstered/dropped his weapon (or died)
        if (status_ == kPoliceStatusFollowAndShootTarget && pTarget_ == event.pCtxt) {
            if (pTarget_->isDead() || pTarget_->isDying()) {
                disengageActive_ = false;
                disengageRemainingMs_ = 0;
                status_ = kPoliceStatusPendingEndFollow;
                pTarget_ = nullptr;
                event.pPed->stopShooting();

                WaitAction *pWait = new WaitAction(WaitAction::kWaitWeapon, kPolicePendingTime);
                pWait->setWarnBehaviour(true);
                event.pPed->addMovementAction(pWait, false);
            } else if (!disengageActive_) {
                // Keep pursuing/shooting during variable cooldown before disengaging
                disengageActive_ = true;
                disengageRemainingMs_ = calcPoliceDisengageCooldownMs(event.pPed);
            }
        } else if (status_ == kPoliceStatusAlert) {
            status_ = kPoliceStatusCheckReengageOrDefault;
        }
        break;
    case Behaviour::kBehvEvtActionEnded:
        {
            Action::ActionType *pType = static_cast<Action::ActionType *> (event.pCtxt);
            if (*pType == Action::kActTypeWait) {
                // We are at the end of waiting period so check if we need to engage right now
                // of if we can go back on patrol
                status_ = kPoliceStatusCheckReengageOrDefault;
            } else {
                status_ = kPoliceStatusAlert;
            }
        }

        break;
    default:
        break;
    }
}

void PoliceBehaviourComponent::handleEjectionFromVehicle(PedInstance *pPed, void *pCtxt) {
    pPed->destroyAllActions(true);
    pPed->addToDefaultActions(new WalkToDirectionAction());
    PedInstance *pShooter = static_cast<PedInstance *>(pCtxt);
    pPed->setDirectionTowardObject(*pShooter);
    // Add a walk action just to get away from the vehicle so that the ped can shoot
    // without being blocked by the vehicle
    WalkToDirectionAction *outOfCarAction = new WalkToDirectionAction();
    outOfCarAction->setMaxDistanceToWalk(20);
    outOfCarAction->setWarnBehaviour(true);
    pPed->addMovementAction(outOfCarAction, true);
    status_ = kPoliceStatusOutOfVehicle;
}

bool PoliceBehaviourComponent::findAndEngageNewTarget(Mission *pMission, PedInstance *pPed) {
    PedInstance *pArmedGuy = findArmedPedNotPolice(pMission, pPed);
    if (pArmedGuy != NULL) {
        followAndShootTarget(pPed, pArmedGuy);
    }
    return pArmedGuy != NULL;
}

/*!
 * Return a ped that has his weapon out and is not a police man and is close to this policeman.
 */
PedInstance * PoliceBehaviourComponent::findArmedPedNotPolice(Mission *pMission, PedInstance *pPed) {
    if (!pMission) {
        return NULL;
    }
    int neutralRange = pPed->sightRange() > 0 ? pPed->sightRange() : kPoliceScoutDistance;
    int scoutRange = static_cast<int>(std::round(calcScaledPerceptionRange(pPed, 2560, neutralRange)));
    for (size_t i = 0; i < pMission->numArmedPeds(); i++) {
        PedInstance *pOtherPed = pMission->armedPedAtIndex(i);
        if (pPed != pOtherPed && pOtherPed->isAlive() && !pOtherPed->isDead() && !pOtherPed->isDying() &&
            pOtherPed->type() != PedInstance::kPedTypePolice && pPed->isCloseTo(pOtherPed, scoutRange)) {
            return pOtherPed;
        }
    }
    return NULL;
}

void PoliceBehaviourComponent::followAndShootTarget(PedInstance *pPed, PedInstance *pArmedGuy) {
    pTarget_ = pArmedGuy;
    disengageActive_ = false;
    disengageRemainingMs_ = 0;
    status_ = kPoliceStatusFollowAndShootTarget;

    // Set new actions
    if (pPed->altAction() == NULL) { // the first time
        WaitBeforeShootingAction *pWarnAction = new WaitBeforeShootingAction(pArmedGuy);
        pPed->addToAltActions(pWarnAction);
        FollowToShootAction* pFollowAction = new FollowToShootAction(pArmedGuy);
        pPed->addToAltActions(pFollowAction);
        pPed->addToAltActions(new FireWeaponAction(pArmedGuy));
        pPed->addToAltActions(new ResetScriptedAction(Action::kActionAlt));
    } else { // just update the target in the existing chain of actions
        MovementAction *pAction = pPed->altAction();
        while (pAction != NULL) {
            switch(pAction->type()) {
            case Action::kActTypeWaitShoot:
                {
                WaitBeforeShootingAction *pAct = dynamic_cast<WaitBeforeShootingAction *>(pAction);
                pAct->setTarget(pArmedGuy);
                }
                break;
            case Action::kActTypeFollowToShoot:
                {
                FollowToShootAction *pAct = dynamic_cast<FollowToShootAction *>(pAction);
                pAct->setTarget(pArmedGuy);
                }
                break;
            case Action::kActTypeFire:
                {
                FireWeaponAction *pAct = dynamic_cast<FireWeaponAction *>(pAction);
                pAct->setTarget(pArmedGuy);
                }
                break;
            default:
                break;
            }
            pAction = pAction->next();
        }
    }
    pPed->setCurrentActionWithSource(Action::kActionAlt);
}

PlayerHostileBehaviourComponent::PlayerHostileBehaviourComponent():
        BehaviourComponent() {
    status_ = kHostileStatusDefault;
}

void PlayerHostileBehaviourComponent::initFromTactic(Mission *pMission, int tacticIdx) {
    if (!pMission || tacticIdx < 0 || tacticIdx >= 128) return;
    const auto &tactics = pMission->tactics();

    int effectiveIdx = tacticIdx;
    if (effectiveIdx > 0) {
        std::vector<bool> visited(128, false);
        while (effectiveIdx > 0 && effectiveIdx < 128 && !visited[effectiveIdx]) {
            visited[effectiveIdx] = true;
            const auto &e = tactics.entries[effectiveIdx];
            if (e.mode > 0 && !(e.sub_filter & LevelData::kAITacticFlagDisabled)) {
                break;
            }
            effectiveIdx = rs_utl::READ_LE_UINT16(e.next_tactic);
        }
    }
    if (effectiveIdx < 0 || effectiveIdx >= 128) return;
    const auto &entry = tactics.entries[effectiveIdx];
    if (entry.mode == 0 || (entry.sub_filter & LevelData::kAITacticFlagDisabled)) return;

    bool isInitialSetup = (currentTacticIndex_ < 0);
    currentTacticIndex_ = effectiveIdx;
    tacticMode_ = entry.mode;
    tacticParam1_ = rs_utl::READ_LE_UINT16(entry.param1);
    tacticParam2_ = rs_utl::READ_LE_UINT16(entry.param2);
    tacticParam3_ = rs_utl::READ_LE_UINT16(entry.param3);
    tacticFlags_ = entry.sub_filter;

    // Mission start reaction latency (3.0s delay before initial tactical movement)
    if (isInitialSetup && startupDelayTicks_ == 0 && tacticMode_ != LevelData::kAITacticModeDelayTimer) {
        startupDelayTicks_ = 3000;
    } else if (tacticMode_ == LevelData::kAITacticModeDelayTimer) {
        startupDelayTicks_ = 0;
    }

    if (entry.faction_id != LevelData::kAIFactionWildcard) {
        factionId_ = entry.faction_id;
    }

    // Set targetFactionId_:
    // In Mode 2 (Rival Sweep) and Mode 3 (Objective Sweep), param1 specifies the target faction ID (1..7).
    // If param1 is 0 or targets Player, targetFactionId_ is LevelData::kAIFactionPlayer (0).
    if (tacticMode_ == LevelData::kAITacticModeRivalSweep ||
        tacticMode_ == LevelData::kAITacticModeObjectiveSweep) {
        if (tacticParam1_ > 0 && tacticParam1_ <= 7) {
            targetFactionId_ = static_cast<uint8_t>(tacticParam1_);
        } else if (entry.sub_mode > 0 && entry.sub_mode <= 7) {
            targetFactionId_ = entry.sub_mode;
        } else {
            targetFactionId_ = LevelData::kAIFactionPlayer;
        }
    } else if (tacticMode_ == LevelData::kAITacticModeProximityTripwire) {
        // Inspect downstream chain for the target of the subsequent tactic
        targetFactionId_ = LevelData::kAIFactionPlayer;
        uint16_t nextIdx = rs_utl::READ_LE_UINT16(entry.next_tactic);
        std::vector<bool> nextVisited(128, false);
        while (nextIdx > 0 && nextIdx < 128 && !nextVisited[nextIdx]) {
            nextVisited[nextIdx] = true;
            const auto &nextEntry = tactics.entries[nextIdx];
            if (nextEntry.mode == LevelData::kAITacticModeRivalSweep ||
                nextEntry.mode == LevelData::kAITacticModeObjectiveSweep) {
                uint16_t p1 = rs_utl::READ_LE_UINT16(nextEntry.param1);
                if (p1 > 0 && p1 <= 7) {
                    targetFactionId_ = static_cast<uint8_t>(p1);
                } else if (nextEntry.sub_mode > 0 && nextEntry.sub_mode <= 7) {
                    targetFactionId_ = nextEntry.sub_mode;
                }
                break;
            }
            nextIdx = rs_utl::READ_LE_UINT16(nextEntry.next_tactic);
        }
    } else {
        targetFactionId_ = LevelData::kAIFactionPlayer;
    }

    switch (tacticMode_) {
    case LevelData::kAITacticModeSeekDestroy:
    case LevelData::kAITacticModeRivalSweep:
        isHunter_ = true;
        break;
    case LevelData::kAITacticModeStationaryGuard:
    case LevelData::kAITacticModeWeaponAmbush:
        isHunter_ = false;
        break;
    case LevelData::kAITacticModeProximityTripwire:
        isHunter_ = false;
        tacticTimerTicks_ = static_cast<int32_t>(tacticParam1_ > 0 ? tacticParam1_ : 10) * 1350;
        break;
    case LevelData::kAITacticModeDelayTimer:
        isHunter_ = false;
        tacticTimerTicks_ = static_cast<int32_t>(tacticParam1_) * 100;
        startupDelayTicks_ = 0;
        break;
    case LevelData::kAITacticModeObjectiveSweep:
        // Bullfrog self-sweep inhibit check (syndicat_main 0x14724):
        // If an Objective Sweep entry targets its own faction (param1 == faction_id),
        // it cannot sweep itself and remains an unactivated sentry holding ground.
        // Otherwise Mode 3 (0x13dc2) sweeps targetFactionId_ (0=Player, 1..7=Rival)
        // unconditionally regardless of mission objective type.
        isHunter_ = (tacticParam1_ != factionId_);
        break;
    default:
        break;
    }
}

void PlayerHostileBehaviourComponent::advanceTactic(Mission *pMission) {
    if (!pMission || currentTacticIndex_ < 0 || currentTacticIndex_ >= 128) return;
    const auto &tactics = pMission->tactics();
    uint16_t nextIdx = rs_utl::READ_LE_UINT16(tactics.entries[currentTacticIndex_].next_tactic);

    // Follow chain past any disabled nodes
    std::vector<bool> visited(128, false);
    while (nextIdx > 0 && nextIdx < 128 && !visited[nextIdx]) {
        visited[nextIdx] = true;
        const auto &nextEntry = tactics.entries[nextIdx];
        if (nextEntry.mode > 0 && !(nextEntry.sub_filter & LevelData::kAITacticFlagDisabled)) {
            break;
        }
        nextIdx = rs_utl::READ_LE_UINT16(nextEntry.next_tactic);
    }

    if (nextIdx < 128) {
        // In Bullfrog mission data, tactic 0 is the active wildcard combat sweep node.
        // Advancing to 0 (when next_tactic == 0) activates the squad to combat sweep the player squad.
        if (nextIdx != currentTacticIndex_ || currentTacticIndex_ != 0) {
            const auto &nextEntry = tactics.entries[nextIdx];
            if (nextEntry.mode > 0 && !(nextEntry.sub_filter & LevelData::kAITacticFlagDisabled)) {
                tripwireCooldownTicks_ = 12000;
                pMission->updateSquadTactic(factionId_, nextIdx);
            }
        }
    }
}

void PlayerHostileBehaviourComponent::execute(const Behaviour::BehaviourParam &param) {
    if (startupDelayTicks_ > 0) {
        startupDelayTicks_ -= static_cast<int32_t>(param.elapsed);
        if (startupDelayTicks_ > 0) return;
    }
    if (tripwireCooldownTicks_ > 0) {
        tripwireCooldownTicks_ -= static_cast<int32_t>(param.elapsed);
    }

    // Mode 10: Delay Timer
    if (tacticMode_ == LevelData::kAITacticModeDelayTimer) {
        if (param.pPed && (param.pPed->hasDestination() || param.pPed->currentAction() != nullptr)) {
            param.pPed->clearDestination();
            param.pPed->destroyAllActions(true);
            param.pPed->setSpeed(0);
        }
        tacticTimerTicks_ -= static_cast<int32_t>(param.elapsed);
        if (tacticTimerTicks_ <= 0) {
            advanceTactic(param.pMission);
        }
        return;
    }

    // Mode 7: Proximity Tripwire
    if (tacticMode_ == LevelData::kAITacticModeProximityTripwire) {
        bool timerExpired = false;
        if (tacticTimerTicks_ > 0) {
            uint16_t nextIdx = 0;
            if (param.pMission && currentTacticIndex_ >= 0 && currentTacticIndex_ < 128) {
                nextIdx = rs_utl::READ_LE_UINT16(param.pMission->tactics().entries[currentTacticIndex_].next_tactic);
            }
            // Chained tripwires (nextIdx > 0, e.g. Mission 9) count down stage timer
            // Terminal tripwires (nextIdx == 0, e.g. Mission 2) do not timeout; they trigger on proximity
            if (nextIdx > 0 && nextIdx < 128) {
                tacticTimerTicks_ -= static_cast<int32_t>(param.elapsed);
                if (tacticTimerTicks_ <= 0) {
                    timerExpired = true;
                }
            }
        }

        PedInstance *pTrigger = nullptr;
        PedInstance *pAnchor = nullptr;
        if (param.pMission) {
            // Mode 7 threshold: if param2 is set, use param2 as tiles; if param1 <= 30, use param1 as tiles;
            // otherwise (param1 is stage timer 50, 100, 200), default to 12 tiles alert perimeter (~3072 units)
            uint32_t threshold = 12 * 256;
            if (tacticParam2_ > 0) {
                threshold = static_cast<uint32_t>(tacticParam2_) * 256;
            } else if (tacticParam1_ > 0 && tacticParam1_ <= 30) {
                threshold = static_cast<uint32_t>(tacticParam1_) * 256;
            }

            // Each sentry in the faction acts as an alert post against player agents.
            // For inter-faction rival AI encounters (e.g. Mission 9), anchor to the squad base to preserve
            // cross-city sweep pacing without premature perimeter breaches.
            pAnchor = param.pPed;
            if (targetFactionId_ > 0) {
                PedInstance *pSquadLeader = nullptr;
                for (size_t i = 0; i < param.pMission->numPeds(); ++i) {
                    PedInstance *p = param.pMission->ped(i);
                    if (p && p->id() == (factionId_ * 8) && p->isAlive() && !p->isDead()) {
                        pSquadLeader = p;
                        break;
                    }
                }
                if (pSquadLeader) {
                    pAnchor = pSquadLeader;
                }
            }

            if (targetFactionId_ == LevelData::kAIFactionPlayer) {
                if (param.pMission->getSquad()) {
                    for (size_t i = 0; i < param.pMission->getSquad()->size(); ++i) {
                        PedInstance *pPlayerPed = param.pMission->getSquad()->member(i);
                        if (pPlayerPed && pPlayerPed->isAlive() && !pPlayerPed->isDead()) {
                            if (pAnchor->isCloseTo(pPlayerPed, threshold)) {
                                pTrigger = pPlayerPed;
                                break;
                            }
                        }
                    }
                }
            } else if (targetFactionId_ > 0) {
                for (size_t i = 0; i < param.pMission->numPeds(); ++i) {
                    PedInstance *pOther = param.pMission->ped(i);
                    if (pOther && pOther->type() == PedInstance::kPedTypeAgent &&
                        pOther->factionId() == targetFactionId_ && pOther->isAlive() && !pOther->isDead()) {
                        if (pAnchor->isCloseTo(pOther, threshold)) {
                            pTrigger = pOther;
                            break;
                        }
                    }
                }
            }
        }
        if (pTrigger || timerExpired) {
            advanceTactic(param.pMission);
            return;
        }
        return;
    }

    // Mode 4: Stationary Guard
    if (tacticMode_ == LevelData::kAITacticModeStationaryGuard) {
        if (status_ == kHostileStatusDefault) {
            PedInstance *pArmedGuy = findTargetPed(param.pMission, param.pPed);
            if (pArmedGuy != nullptr) {
                if (!(tacticFlags_ & LevelData::kAITacticFlagArmedRequired) || pArmedGuy->isArmed()) {
                    status_ = kHostileStatusFollowAndShoot;
                    followAndShootTarget(param.pPed, pArmedGuy);
                }
            }
            return;
        }
    }

    if (status_ == kHostileStatusDefault) {
        if (param.pPed->type() == PedInstance::kPedTypeAgent && !isHunter_) {
            // Sentry enemy agents (non-hunters) do not hunt distant targets across the map.
            // If in Weapon Ambush mode (Mode 5), they engage when target enters lethal weapon range & LoS.
            if (tacticMode_ == LevelData::kAITacticModeWeaponAmbush) {
                PedInstance *pArmedGuy = findTargetPed(param.pMission, param.pPed);
                if (pArmedGuy != nullptr) {
                    if (!(tacticFlags_ & LevelData::kAITacticFlagArmedRequired) || pArmedGuy->isArmed()) {
                        status_ = kHostileStatusFollowAndShoot;
                        followAndShootTarget(param.pPed, pArmedGuy);
                        if (currentTacticIndex_ > 0) {
                            advanceTactic(param.pMission);
                        }
                    }
                }
                return;
            }


        }

        // For scripted guards on a waypoint route, do not break route unless approached in self-defense proximity
        if (param.pPed->hasScript() && param.pPed->currentAction() != nullptr && !isHunter_) {
            PedInstance *pArmedGuy = findTargetPed(param.pMission, param.pPed);
            if (pArmedGuy != nullptr && param.pPed->isCloseTo(pArmedGuy, 768)) {
                status_ = kHostileStatusFollowAndShoot;
                followAndShootTarget(param.pPed, pArmedGuy);
            }
            return;
        }

        // In this mode, ped (Hunter agent or unscripted guard) is looking for an enemy
        PedInstance *pArmedGuy = findTargetPed(param.pMission, param.pPed);
        if (pArmedGuy != NULL) {
            status_ = kHostileStatusFollowAndShoot;
            followAndShootTarget(param.pPed, pArmedGuy);
            if (tacticMode_ == LevelData::kAITacticModeWeaponAmbush && currentTacticIndex_ > 0) {
                advanceTactic(param.pMission);
            }
        }
    } else if (status_ == kHostileStatusFollowAndShoot) {
        bool targetDisguised = pTarget_->isCloneShieldActivated() && param.pPed->type() != PedInstance::kPedTypeAgent;
        if (!pTarget_ || pTarget_->isDead() || pTarget_->isDying() || targetDisguised) {
            status_ = kHostileStatusPendingEndFollow;
            pTarget_ = NULL;
            param.pPed->stopShooting();
            param.pPed->clearDestination();
            if (param.pPed->type() == PedInstance::kPedTypeAgent) {
                param.pPed->deselectWeapon();
            }

            // Check if current target faction has any alive members remaining; if not, advance tactic
            if (targetFactionId_ > 0 && param.pMission) {
                bool factionAlive = false;
                for (size_t i = 0; i < param.pMission->numPeds(); ++i) {
                    PedInstance *pOther = param.pMission->ped(i);
                    if (pOther && pOther->type() == PedInstance::kPedTypeAgent &&
                        pOther->factionId() == targetFactionId_ && pOther->isAlive() && !pOther->isDead()) {
                        factionAlive = true;
                        break;
                    }
                }
                if (!factionAlive) {
                    advanceTactic(param.pMission);
                }
            }

            // just wait a few time before engaging another target or simply
            // continue with default behavior
            WaitAction *pWait = new WaitAction(WaitAction::kWaitWeapon);
            pWait->setWarnBehaviour(true);
            param.pPed->addMovementAction(pWait, false);
        } else if (!isHunter_) {
            double scoutDist = 3200.0 * param.pPed->perception().getMultiplier();
            // If target has escaped beyond pursuit distance (1.5x scout distance)
            if (!param.pPed->isCloseTo(pTarget_, static_cast<int>(scoutDist * 1.5))) {
                pTarget_ = NULL;
                param.pPed->stopShooting();
                param.pPed->clearDestination();
                param.pPed->deselectWeapon();
                param.pPed->setCurrentActionWithSource(Action::kActionDefault);
                status_ = kHostileStatusDefault;
            }
        }
    } else if (status_ == kHostileStatusCheckForDefault) {
        // check if there is a nearby enemy
        PedInstance *pArmedGuy = findTargetPed(param.pMission, param.pPed);
        if (pArmedGuy != NULL) {
            status_ = kHostileStatusFollowAndShoot;
            followAndShootTarget(param.pPed, pArmedGuy);
        } else {
            param.pPed->deselectWeapon();
            param.pPed->setCurrentActionWithSource(Action::kActionDefault);
            status_ = kHostileStatusDefault;
        }
    }
}

void PlayerHostileBehaviourComponent::handleBehaviourEvent(const Behaviour::BehaviourEvent &event) {
    if (event.evtType == Behaviour::kBehvEvtActionEnded) {
        // We are at the end of waiting period so check if we need to engage right now
        // of if we can go back to default
        status_ = kHostileStatusCheckForDefault;
    } else if (event.evtType == Behaviour::kBehvEvtHit) {
        const DamageToInflict *pDmg = static_cast<const DamageToInflict *>(event.pCtxt);
        PedInstance *pAttacker = (pDmg && pDmg->d_owner) ? dynamic_cast<PedInstance *>(pDmg->d_owner) : nullptr;
        if (pAttacker && pAttacker->isAlive() && !pAttacker->isDead() && pAttacker != event.pPed) {
            if (status_ == kHostileStatusDefault || status_ == kHostileStatusCheckForDefault) {
                status_ = kHostileStatusFollowAndShoot;
                followAndShootTarget(event.pPed, pAttacker);
            }
            if (event.pPed && event.pPed->selectedWeapon() == nullptr) {
                event.pPed->selectShootingWeaponWithAmmo();
            }
        }
    }
}

PedInstance * PlayerHostileBehaviourComponent::findTargetPed(Mission *pMission, PedInstance *pPed) {
    if (!pMission || !pPed) return nullptr;

    double scoutDist = 1e9;
    if (!isHunter_) {
        WeaponInstance *pCurWpn = pPed->selectedWeapon();
        if (!pCurWpn) pCurWpn = pPed->getBestRankedWeaponWithAmmo();
        if (pPed->type() == PedInstance::kPedTypeGuard) {
            int neutralGuardRange = pPed->sightRange() > 0 ? pPed->sightRange() : kEnemyScoutDistance;
            scoutDist = calcScaledPerceptionRange(pPed, 2048, neutralGuardRange);
        } else {
            int baseRange = pCurWpn ? pCurWpn->range() : pPed->sightRange();
            scoutDist = std::min(2560.0, calcScaledPerceptionRange(pPed, baseRange, baseRange > 0 ? baseRange : kEnemyScoutDistance));
        }
    }
    PedInstance *pClosest = nullptr;
    double closestDist = scoutDist;

    auto evaluateTarget = [&](PedInstance *pCand) {
        if (!pCand || !pCand->isAlive() || pCand->isDead() || pCand == pPed) return;
        bool targetDisguised = pCand->isCloneShieldActivated() && pPed->type() != PedInstance::kPedTypeAgent;
        if (targetDisguised) return;

        WorldPoint pedW(pPed->position());
        WorldPoint candW(pCand->position());
        double dx = (double)(pedW.x - candW.x);
        double dy = (double)(pedW.y - candW.y);
        double d = sqrt(dx * dx + dy * dy);

        // If mode is weapon ambush, candidate must be within weapon lethal range
        if (tacticMode_ == LevelData::kAITacticModeWeaponAmbush) {
            WeaponInstance *pWpn = pPed->selectedWeapon();
            if (!pWpn) pWpn = pPed->getBestRankedWeaponWithAmmo();
            int wpnRange = pWpn ? pWpn->range() : 1280;
            if (d > (double)wpnRange) return;
        }

        if (d <= closestDist) {
            if (!isHunter_ && d > 512.0) {
                WorldPoint pedEyeW = pedW;
                pedEyeW.z += 64;
                WorldPoint candTargetW = candW;
                candTargetW.z += 64;
                if (pMission->checkBlockedByTile(pedEyeW, &candTargetW, false, scoutDist) != 1) {
                    return; // Blocked by wall
                }
            }
            closestDist = d;
            pClosest = pCand;
        }
    };

    auto isSameSyndicate = [](int f1, int f2) {
        return (f1 == f2);
    };

    if (targetFactionId_ == LevelData::kAIFactionPlayer) {
        if (pMission->getSquad()) {
            for (size_t i = 0; i < pMission->getSquad()->size(); i++) {
                evaluateTarget(pMission->getSquad()->member(i));
            }
        }
    } else {
        // Target assigned rival faction
        // When advancing towards a rival faction across the map, target the squad leader anchor (targetFactionId_ * 8)
        // while beyond effective weapon range, so the squad advances cohesively toward the enemy base without
        // scattering or attacking isolated sentries on the outskirts.
        if (targetFactionId_ != LevelData::kAIFactionWildcard) {
            int leaderId = targetFactionId_ * 8;
            PedInstance *pLeader = nullptr;
            for (size_t i = 0; i < pMission->numPeds(); ++i) {
                PedInstance *p = pMission->ped(i);
                if (p && p->id() == leaderId && p->isAlive() && !p->isDead()) {
                    pLeader = p;
                    break;
                }
            }
            if (!pLeader) {
                for (size_t i = 0; i < pMission->numPeds(); ++i) {
                    PedInstance *p = pMission->ped(i);
                    if (p && p->type() == PedInstance::kPedTypeAgent &&
                        p->factionId() == targetFactionId_ && p->isAlive() && !p->isDead()) {
                        pLeader = p;
                        break;
                    }
                }
            }
            if (pLeader) {
                WeaponInstance *pWpn = pPed->selectedWeapon();
                if (!pWpn) pWpn = pPed->getBestRankedWeaponWithAmmo();
                int wpnRange = pWpn ? pWpn->range() : 1280;
                double leaderDist = pPed->distanceToPosition(pLeader->position());
                if (leaderDist > static_cast<double>(wpnRange + 256)) {
                    evaluateTarget(pLeader);
                }
            }
        }

        if (!pClosest) {
            for (size_t i = 0; i < pMission->numPeds(); i++) {
                PedInstance *pCand = pMission->ped(i);
                if (!pCand || pCand->type() != PedInstance::kPedTypeAgent || pCand->isOurAgent()) continue;
                if (targetFactionId_ == LevelData::kAIFactionWildcard) {
                    if (!isSameSyndicate(pCand->factionId(), factionId_)) {
                        evaluateTarget(pCand);
                    }
                } else if (pCand->factionId() == targetFactionId_) {
                    evaluateTarget(pCand);
                }
            }
        }


    }

    return pClosest;
}

void PlayerHostileBehaviourComponent::followAndShootTarget(PedInstance *pPed, PedInstance *pArmedGuy) {
    pTarget_ = pArmedGuy;
    status_ = kHostileStatusFollowAndShoot;

    if (pPed && pPed->type() == PedInstance::kPedTypeAgent) {
        pPed->setIPAAmount(IPAStim::Adrenaline, 100);
    }
    if (pPed) {
        if (pPed->type() == PedInstance::kPedTypeAgent) {
            // For enemy agents, only wield if already within weapon range + margin (1 tile / 256 units)
            WeaponInstance *pWeapon = pPed->selectedWeapon();
            if (!pWeapon) {
                pWeapon = pPed->getBestRankedWeaponWithAmmo();
            }
            int wpnRange = pWeapon ? pWeapon->range() : 1280;
            int wieldThreshold = wpnRange + 256;
            if (pPed->isCloseTo(pArmedGuy, wieldThreshold)) {
                pPed->selectShootingWeaponWithAmmo();
            } else {
                pPed->deselectWeapon();
            }
        } else {
            pPed->selectShootingWeaponWithAmmo();
        }
    }

    // Set new actions
    if (pPed->altAction() == NULL) { // the first time
        if (!isHunter_) {
            WaitBeforeShootingAction *pWarnAction = new WaitBeforeShootingAction(pArmedGuy);
            pPed->addToAltActions(pWarnAction);
        }
        FollowToShootAction* pFollowAction = new FollowToShootAction(pArmedGuy);
        pPed->addToAltActions(pFollowAction);
        pPed->addToAltActions(new FireWeaponAction(pArmedGuy));
        pPed->addToAltActions(new ResetScriptedAction(Action::kActionAlt));
    } else { // just update the target in the existing chain of actions
        MovementAction *pAction = pPed->altAction();
        while (pAction != NULL) {
            switch(pAction->type()) {
            case Action::kActTypeWaitShoot:
                {
                WaitBeforeShootingAction *pAct = dynamic_cast<WaitBeforeShootingAction *>(pAction);
                pAct->setTarget(pArmedGuy);
                }
                break;
            case Action::kActTypeFollowToShoot:
                {
                FollowToShootAction *pAct = dynamic_cast<FollowToShootAction *>(pAction);
                pAct->setTarget(pArmedGuy);
                }
                break;
            case Action::kActTypeFire:
                {
                FireWeaponAction *pAct = dynamic_cast<FireWeaponAction *>(pAction);
                pAct->setTarget(pArmedGuy);
                }
                break;
            default:
                break;
            }
            pAction = pAction->next();
        }
    }
    pPed->setCurrentActionWithSource(Action::kActionAlt);
}

}
