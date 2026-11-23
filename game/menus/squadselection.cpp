/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2012, 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#include <set>
#include "menus/squadselection.h"

#include "rs-kernel/model/squad.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/mgr/missionmanager.h"


/*!
 * Default constructor.
 */
SquadSelection::SquadSelection() {
    pSquad_ = NULL;
    selected_agents_ = 0;
    leader_ = 0;
}

/*!
 * Sets the squad and selects the first selectable agent.
 * Pre existing selection has been cleared.
 * \param pSquad The current squad.
 */
void SquadSelection::setSquad(rs_knl::Squad *pSquad) {
    clear();
    pSquad_ = pSquad;

    for (size_t i = rs_knl::Squad::kSlot1; i < rs_knl::Squad::kMaxSlot; i++) {
        // try to select agent. if true means agent has been selected
        if (selectAgent(i, false)) {
            break;
        }
    }
}

/*!
 * Clear current selection.
 */
void SquadSelection::clear() {
    selected_agents_ = 0;
}

/*!
 * Returns the number of selected agents
 * \return 0 if no agent is selected.
 */
size_t SquadSelection::size() {
    size_t agents = 0;

    for (size_t i = rs_knl::Squad::kSlot1; i < rs_knl::Squad::kMaxSlot; i++) {
        if (isAgentSelected(i)) {
            agents++;
        }
    }

    return agents;
}

/*!
 * Selects or deselects the agent on the given slot.
 * \param agentNo
 * \param addToGroup True if agent is added to the selection.
 * \return True if agent has been selected or deselected.
 */
bool SquadSelection::selectAgent(size_t agentNo, bool addToGroup) {
    if (isAgentSelectable(agentNo)) {
        // saves the current selection to check if it changes
        size_t oldSelection = selected_agents_;
        if (addToGroup) {
            // Add/remove to the group
            selected_agents_ ^= 1 << agentNo;
            // check if leader was deselected
            checkLeader(agentNo);
        } else {
            selected_agents_ = 1 << agentNo;
            leader_ = agentNo;
        }

        return (oldSelection != selected_agents_);
    }

    return false;
}

/*!
 * Deselects an agent. Called when an agent died.
 * \param p_ped The agent to deselect.
 */
void SquadSelection::deselectAgent(rs_knl::PedInstance *p_ped) {
    for (size_t i = rs_knl::Squad::kSlot1; i < rs_knl::Squad::kMaxSlot; i++) {
        if (pSquad_->member(i) == p_ped) {
            selected_agents_ &= ~(1 << i);
            // check if leader was deselected
            checkLeader(i);
            return;
        }
    }
}

/*!
 * Selects all agents.
 * \param b_selectAll True to select, false deselect
 */
void SquadSelection::selectAllAgents(bool b_selectAll) {
    if (b_selectAll) {
        // Select all agents
        for (size_t i = rs_knl::Squad::kSlot1; i < rs_knl::Squad::kMaxSlot; i++) {
            if (isAgentSelectable(i)) {
                selected_agents_ |= (1 << i);
            }
        }
    } else {
        // Deselect all except leader
        selected_agents_ = (1 << leader_);
    }
}

/*!
 * Returns true if agent is active and alive.
 */
bool SquadSelection::isAgentSelectable(size_t agentNo) {
    rs_knl::PedInstance *pPed = pSquad_->member(agentNo);
    return  pPed && pPed->isAlive();
}

/*!
 * checks if leader has been deselected then finds
 * a new leader.
 */
void SquadSelection::checkLeader(size_t agentNo) {
    if (agentNo == leader_) {
        // The leader has been deselected
        // find another leader among the remaining selection
        for (size_t i = rs_knl::Squad::kSlot1; i < rs_knl::Squad::kMaxSlot; i++) {
            if (isAgentSelectable(i) && leader_ != i) {
                leader_ = i;
                break;
            }
        }
    }
}

/*!
 * Deselects the leader selected weapon and for the other agents, it depends
 * on the leader's weapon.
 */
void SquadSelection::deselectWeaponOfSameCategory(rs_knl::Weapon *pWeaponFromLeader) {
    bool categoryShooting = pWeaponFromLeader->canShoot();
    for (SquadSelection::Iterator it = begin(); it != end(); ++it) {
        rs_knl::PedInstance *pAgent = *it;
        rs_knl::WeaponInstance *pWeaponToDeselect = pAgent->selectedWeapon();
        if (pWeaponToDeselect != NULL) {
            if (pAgent == leader() ||
                (categoryShooting && pWeaponToDeselect->getClass()->canShoot()) ||
                (pWeaponToDeselect->isInstanceOf(pWeaponFromLeader->getType()))) {
                pAgent->stopUsingWeapon();
                pAgent->deselectWeapon();
            }
        }
    }
}

/*!
 * Selects the given weapon for the leader and updates weapon selection
 * for other selected agents.
 * \param weapon_idx The index in the leader inventory of the weapon to select.
 * \param apply_to_all In case of Medikit, all selected agents must use one.
 */
void SquadSelection::selectWeaponFromLeader(uint8_t weapon_idx, bool applySelectionToAll) {
    rs_knl::PedInstance *pLeader = leader();
    rs_knl::WeaponInstance *pLeaderWeapon = pLeader->weapon(weapon_idx);

    for (SquadSelection::Iterator it = begin(); it != end(); ++it)
    {
        rs_knl::PedInstance *ped = *it;
        ped->stopUsingWeapon();
        if (pLeader == ped) {
            // Forces selection of the weapon for the leader
            pLeader->selectWeapon(weapon_idx);
        } else if (pLeaderWeapon->canShoot()) {
            ped->selectShootingWeaponWithSameTypeFirst(pLeaderWeapon);
        } else if (applySelectionToAll) {
            ped->selectMedikitOrShield(pLeaderWeapon->getClass()->getType());
        }
    } // end for
}

/*!
 * Choose the first agent who has free space in his inventory and send
 * him pickup weapon.
 * \param pWeapon The weapon to pickup
 * \param addAction True to add the action at the end of the list of action,
 * false to set as the only action.
 */
void SquadSelection::pickupWeapon(rs_knl::WeaponInstance *pWeapon, bool addAction) {
    for (SquadSelection::Iterator it = begin(); it != end(); ++it)
    {
        rs_knl::PedInstance *pAgent = *it;
        // Agent has space in inventory
        if (pAgent->numWeapons() < rs_knl::WeaponHolder::kMaxHoldedWeapons) {
            rs_knl::MovementAction * pActions = pAgent->createActionPickup(pWeapon);
            pAgent->addMovementAction(pActions, addAction);

            break;
        }
    }
}

/*!
 * All selected agents that are not in a vehicle, follows the given pedestrian.
 * This action replaces all actions in the agents list.
 * \param pPed The ped to follow
 */
void SquadSelection::followPed(rs_knl::PedInstance *pPed) {
    for (SquadSelection::Iterator it = begin(); it != end(); ++it)
    {
        rs_knl::PedInstance *pAgent = *it;
        if (!pAgent->inVehicle()) { // Agent must not be in a vehicle
            pAgent->addActionFollowPed(pPed);
        }
    }
}

/*!
 * Selected agents enter or leave the given vehicle.
 * - if he is in the vehicle, the vehicle stops and he gets out. It only works
 *    with cars : Trains automatically drop their passengers.
 * - else he gets in the given vehicle.
 * \param pVehicle The vehicle
 * \param addAction True to add the action at the end of the list of action,
 * false to set as the only action.
 */
void SquadSelection::enterOrLeaveVehicle(rs_knl::Vehicle *pVehicle, bool addAction) {
    for (SquadSelection::Iterator it = begin(); it != end(); ++it)
    {
        rs_knl::PedInstance *pAgent = *it;

        if (pVehicle->isCar() && pAgent->inVehicle() == pVehicle) {
            // First stop the car
            if (pVehicle->speed() != 0) {
                pVehicle->clearDestination();
                if (auto *pDriver = pVehicle->getDriver()) {
                    pDriver->destroyAllActions();
                }
            }

            // then drop passenger
            pVehicle->dropPassenger(pAgent);
            pAgent->dropPersuadedFromCar(pVehicle);
        } else {
            // Agent is out
            rs_knl::MovementAction *pAction =
                pAgent->createActionEnterVehicle(pVehicle);
            pAgent->addMovementAction(pAction, addAction);
            pAgent->informPersuadedToEnterVehicle(pVehicle);
        }
    }
}

/*!
 * Tells every agents in the selection to go to the given point.
 * Agents in a drivable vehicle will use vehicle and other will walk.
 * \param mapPt The destination point
 * \param addAction True to add the action at the end of the list of action,
 * false to set as the only action.
 */
void SquadSelection::moveTo(const rs_knl::TilePoint &mapPt, bool addAction) {
    std::set<rs_knl::GenericCar *> handledCars;
    size_t i = 0;
    for (SquadSelection::Iterator it = begin(); it != end(); ++it, i++) {
        rs_knl::PedInstance *pAgent = *it;
        rs_knl::Vehicle *pVehicle = pAgent->inVehicle();
        if (pVehicle) {
            if (pVehicle->isCar()) {
                rs_knl::GenericCar *pCar = dynamic_cast<rs_knl::GenericCar *>(pVehicle);
                if (pCar && handledCars.find(pCar) == handledCars.end()) {
                    handledCars.insert(pCar);

                    // Find if the car already has a driver that is in our squad selection
                    rs_knl::PedInstance *pDriver = pCar->getDriver();
                    bool driverInSelection = false;
                    if (pDriver) {
                        for (SquadSelection::Iterator selIt = begin(); selIt != end(); ++selIt) {
                            if (*selIt == pDriver) {
                                driverInSelection = true;
                                break;
                            }
                        }
                    }

                    if (!driverInSelection) {
                        pCar->setDriver(pAgent, true);
                        pDriver = pAgent;
                    }

                    rs_knl::TilePoint finalDest = mapPt;
                    pAgent->map()->adjustClickOnRoad(finalDest);
                    pDriver->addActionDriveVehicle(pCar, finalDest, addAction);
                }
            } else {
                // Train (TrainHead or TrainBody)
                rs_knl::Mission *pMission = g_missionCtrl.mission();
                if (!pVehicle->isTrainMoving(pMission)) {
                    // Train is stopped at a station: offboard agent and persuaded NPCs
                    pAgent->dropPersuadedFromCar(pVehicle);
                    pVehicle->dropPassenger(pAgent);

                    rs_knl::TilePoint tmpPosT = mapPt;
                    if (size() > 1) {
                        pSquad_->getPositionInSquadFormation(i, &tmpPosT);
                    }
                    pAgent->addActionWalk(tmpPosT, addAction);
                }
                // If moving: exiting is prevented while train is in motion
            }
        } else {
            rs_knl::TilePoint tmpPosT = mapPt;

            if (size() > 1) {
                pSquad_->getPositionInSquadFormation(i, &tmpPosT);
            }

            pAgent->addActionWalk(tmpPosT, addAction);
        }
    } // end of for
}

/*!
 * Each selected agent will use his weapon and shoot at the given target.
 * Agent will shoot only if he's armed and ready to shoot.
 * \param aimedLocW Where the agent must shoot
 */
void SquadSelection::shootAt(const rs_knl::WorldPoint &aimedLocW) {
    for (SquadSelection::Iterator it = begin(); it != end(); ++it) {
        rs_knl::PedInstance *pAgent = *it;
        pAgent->addActionShootAt(aimedLocW);
    }
}


/**
 * Return true if the given object is in the line of fire of at least
 * one selected agent.
 * \param pMission Mission*
 * \param pTarget ShootableMapObject*
 * \return bool
 */
bool SquadSelection::isTargetInRange(rs_knl::Mission *pMission, rs_knl::ShootableMapObject *pTarget) {
    if (!pTarget) {
        return false;
    }

    if (pTarget->is(rs_knl::MapObject::kNaturePed)) {
        rs_knl::PedInstance *pPed = static_cast<rs_knl::PedInstance *>(pTarget);
        if (pPed->isOurAgent() || pPed->isPersuaded()) {
            return false;
        }
    }

    for (SquadSelection::Iterator it = begin(); it != end(); ++it) {
        if ((*it)->isArmed() && (*it)->selectedWeapon()->canShoot()) {
            rs_knl::WorldPoint shooterPosW((*it)->position());
            shooterPosW.z += (*it)->sizeZ() >> 1;
            rs_knl::WeaponInstance *pWeapon = (*it)->selectedWeapon();

            rs_knl::Mission::BlockerCriteria crits;
            crits.maxr = pWeapon->range();
            crits.pOrigin = (*it);
            uint8_t blockRes = pMission->checkIfBlockersInShootingLine(shooterPosW, crits, &pTarget);

            if (blockRes == 1) {
                return true;
            }
        }
    }

    return false;
}
