/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
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

#include "editor_commands.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/model/leveldata.h"

#include <algorithm>
#include <format>

namespace rs_win_editor {

// --- PaintTileCommand ---

PaintTileCommand::PaintTileCommand(rs_knl::Map *pMap, int tx, int ty, int tz, uint8_t oldTile, uint8_t newTile)
    : pMap_(pMap), tx_(tx), ty_(ty), tz_(tz), oldTile_(oldTile), newTile_(newTile) {}

bool PaintTileCommand::execute() {
    if (!pMap_) return false;
    return pMap_->setTileAt(tx_, ty_, tz_, newTile_);
}

bool PaintTileCommand::undo() {
    if (!pMap_) return false;
    return pMap_->setTileAt(tx_, ty_, tz_, oldTile_);
}

std::string PaintTileCommand::description() const {
    return std::format("Paint tile at ({}, {}, {}) to {}", tx_, ty_, tz_, newTile_);
}

// --- BoxFillTilesCommand ---

BoxFillTilesCommand::BoxFillTilesCommand(rs_knl::Map *pMap, int x1, int y1, int x2, int y2, int tz, uint8_t newTile)
    : pMap_(pMap),
      minX_(std::min(x1, x2)),
      minY_(std::min(y1, y2)),
      maxX_(std::max(x1, x2)),
      maxY_(std::max(y1, y2)),
      tz_(tz),
      newTile_(newTile) {
    if (pMap_) {
        for (int y = minY_; y <= maxY_; ++y) {
            for (int x = minX_; x <= maxX_; ++x) {
                int tileId = pMap_->getTileIdAt(x, y, tz_);
                previousTiles_.push_back({x, y, tz_, static_cast<uint8_t>(tileId >= 0 ? tileId : 0)});
            }
        }
    }
}

bool BoxFillTilesCommand::execute() {
    if (!pMap_) return false;
    for (int y = minY_; y <= maxY_; ++y) {
        for (int x = minX_; x <= maxX_; ++x) {
            pMap_->setTileAt(x, y, tz_, newTile_);
        }
    }
    return true;
}

bool BoxFillTilesCommand::undo() {
    if (!pMap_) return false;
    for (const auto &prev : previousTiles_) {
        pMap_->setTileAt(prev.tx, prev.ty, prev.tz, prev.tileId);
    }
    return true;
}

std::string BoxFillTilesCommand::description() const {
    return std::format("Box fill ({},{})-({},{}) at Z={} with tile {}", minX_, minY_, maxX_, maxY_, tz_, newTile_);
}

// --- MoveTileCommand ---

MoveTileCommand::MoveTileCommand(rs_knl::Map *pMap, int srcX, int srcY, int srcZ, int dstX, int dstY, int dstZ, bool copyOnly)
    : pMap_(pMap), srcX_(srcX), srcY_(srcY), srcZ_(srcZ), dstX_(dstX), dstY_(dstY), dstZ_(dstZ), copyOnly_(copyOnly) {
    if (pMap_) {
        int sId = pMap_->getTileIdAt(srcX_, srcY_, srcZ_);
        srcTileId_ = static_cast<uint8_t>(sId >= 0 ? sId : 0);
        int dId = pMap_->getTileIdAt(dstX_, dstY_, dstZ_);
        dstOldTileId_ = static_cast<uint8_t>(dId >= 0 ? dId : 0);
    }
}

bool MoveTileCommand::execute() {
    if (!pMap_) return false;
    if (!copyOnly_) {
        pMap_->setTileAt(srcX_, srcY_, srcZ_, 0);
    }
    pMap_->setTileAt(dstX_, dstY_, dstZ_, srcTileId_);
    return true;
}

bool MoveTileCommand::undo() {
    if (!pMap_) return false;
    pMap_->setTileAt(dstX_, dstY_, dstZ_, dstOldTileId_);
    if (!copyOnly_) {
        pMap_->setTileAt(srcX_, srcY_, srcZ_, srcTileId_);
    }
    return true;
}

std::string MoveTileCommand::description() const {
    if (copyOnly_) {
        return std::format("Copy tile from ({}, {}, {}) to ({}, {}, {})", srcX_, srcY_, srcZ_, dstX_, dstY_, dstZ_);
    }
    return std::format("Move tile from ({}, {}, {}) to ({}, {}, {})", srcX_, srcY_, srcZ_, dstX_, dstY_, dstZ_);
}

// --- SpawnPedCommand ---

SpawnPedCommand::SpawnPedCommand(rs_knl::Mission *pMission, rs_knl::PedInstance::PedType type,
                                 const rs_knl::TilePoint &pos, uint8_t orientation, uint16_t baseAnim,
                                 bool cyberwareOverridden, int cyberwareTier,
                                 uint8_t movementMode, int factionId,
                                 bool isOurAgent, int squadSlot)
    : pMission_(pMission), type_(type), pos_(pos), orientation_(orientation), baseAnim_(baseAnim),
      cyberwareOverridden_(cyberwareOverridden), cyberwareTier_(cyberwareTier),
      movementMode_(movementMode), factionId_(factionId),
      isOurAgent_(isOurAgent), squadSlot_(squadSlot) {}

bool SpawnPedCommand::execute() {
    if (!pMission_) return false;
    if (isOurAgent_ && squadSlot_ >= 0 && squadSlot_ < 4) {
        pSpawnedPed_ = pMission_->spawnPlayerAgent(squadSlot_, pos_, orientation_);
    } else {
        pSpawnedPed_ = pMission_->spawnPed(type_, pos_, orientation_, baseAnim_, factionId_);
        if (pSpawnedPed_) {
            pSpawnedPed_->setInitialLevelState(movementMode_);
            if (pSpawnedPed_->scriptSteps().empty()) {
                pSpawnedPed_->destroyAllActions(true);
                if (movementMode_ & LevelData::kPeopleStateWalking) {
                    pSpawnedPed_->addToDefaultActions(new rs_knl::WalkToDirectionAction());
                    pSpawnedPed_->addMovementAction(pSpawnedPed_->defaultAction(), false);
                }
            }

            if (pSpawnedPed_->type() == rs_knl::PedInstance::kPedTypeAgent && !pSpawnedPed_->isOurAgent()) {
                pSpawnedPed_->setCyberwareOverridden(cyberwareOverridden_);
                if (cyberwareOverridden_) {
                    pSpawnedPed_->applyCyberwareTier(cyberwareTier_);
                } else if (pMission_) {
                    pSpawnedPed_->applyCyberwareTier(pMission_->techLevel());
                }
                if (factionId_ >= 0) {
                    pSpawnedPed_->setFactionId(factionId_);
                }
            }
        }
    }
    return pSpawnedPed_ != nullptr;
}

bool SpawnPedCommand::undo() {
    if (!pMission_ || !pSpawnedPed_) return false;
    if (isOurAgent_ && squadSlot_ >= 0 && squadSlot_ < 4) {
        pMission_->setSquadSpawnPoint(squadSlot_, rs_knl::TilePoint(-1, -1, 0), 0);
    }
    bool res = pMission_->removePed(pSpawnedPed_);
    pSpawnedPed_ = nullptr;
    return res;
}

std::string SpawnPedCommand::description() const {
    if (isOurAgent_) {
        return std::format("Spawn Player's Agent #{} at ({}, {}, {})", squadSlot_ + 1, pos_.tx, pos_.ty, pos_.tz);
    }
    return std::format("Spawn actor at ({}, {}, {})", pos_.tx, pos_.ty, pos_.tz);
}

// --- DeletePedCommand ---

DeletePedCommand::DeletePedCommand(rs_knl::Mission *pMission, rs_knl::PedInstance *pPed)
    : pMission_(pMission), pPedRef_(pPed) {
    if (pPed) {
        type_ = pPed->type();
        isOurAgent_ = pPed->isOurAgent();
        squadSlot_ = pPed->id();
        pos_ = rs_knl::TilePoint(pPed->tileX(), pPed->tileY(), pPed->tileZ());
        orientation_ = static_cast<uint8_t>(pPed->direction());
        health_ = pPed->health();
        baseAnim_ = pPed->baseAnim();
        scriptSteps_ = pPed->scriptSteps();
        for (size_t i = 0; i < pPed->numWeapons(); ++i) {
            rs_knl::WeaponInstance *w = pPed->weapon(i);
            if (w) {
                weapons_.push_back(w->type());
            }
        }
    }
}

bool DeletePedCommand::execute() {
    if (!pMission_ || !pPedRef_) return false;
    if (isOurAgent_ && squadSlot_ < 4) {
        pMission_->setSquadSpawnPoint(squadSlot_, rs_knl::TilePoint(-1, -1, 0), 0);
    }
    return pMission_->removePed(pPedRef_);
}

bool DeletePedCommand::undo() {
    if (!pMission_) return false;
    if (isOurAgent_) {
        pPedRef_ = pMission_->spawnPlayerAgent(squadSlot_, pos_, orientation_);
    } else {
        pPedRef_ = pMission_->spawnPed(type_, pos_, orientation_, baseAnim_);
    }
    if (!pPedRef_) return false;
    pPedRef_->setHealth(health_);
    pPedRef_->setScriptSteps(scriptSteps_);
    rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPedRef_);
    for (auto wType : weapons_) {
        rs_knl::Weapon *pDef = g_weaponMgr.getWeapon(wType);
        if (pDef) {
            rs_knl::WeaponInstance *wi = rs_knl::WeaponInstance::createInstance(pDef);
            if (wi) {
                wi->setMap(pMission_->map());
                pPedRef_->addWeapon(wi);
                wi->setOwner(pPedRef_);
            }
        }
    }
    return true;
}

std::string DeletePedCommand::description() const {
    if (isOurAgent_) {
        return std::format("Delete Player's Agent #{} at ({}, {}, {})", squadSlot_ + 1, pos_.tx, pos_.ty, pos_.tz);
    }
    return std::format("Delete actor at ({}, {}, {})", pos_.tx, pos_.ty, pos_.tz);
}

// --- MoveEntityCommand ---

MoveEntityCommand::MoveEntityCommand(rs_knl::MapObject *pObj, const rs_knl::TilePoint &oldPos, const rs_knl::TilePoint &newPos, rs_knl::Mission *pMission)
    : pObj_(pObj), oldPos_(oldPos), newPos_(newPos), pMission_(pMission) {}

bool MoveEntityCommand::execute() {
    if (!pObj_) return false;
    pObj_->setPosition(newPos_.tx, newPos_.ty, newPos_.tz, newPos_.ox, newPos_.oy, newPos_.oz);
    if (pMission_) {
        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pObj_)) {
            if (pPed->isOurAgent() && pPed->id() < 4) {
                pMission_->setSquadSpawnPoint(pPed->id(), newPos_, static_cast<uint8_t>(pPed->direction()));
            }
            if (!pPed->scriptSteps().empty()) {
                auto steps = pPed->scriptSteps();
                steps[0].targetPos = newPos_;
                pPed->setScriptSteps(steps);
                rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
            }
        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pObj_)) {
            if (auto *pDriver = pVeh->getDriver()) {
                pDriver->setPosition(newPos_);
                if (!pDriver->scriptSteps().empty()) {
                    auto steps = pDriver->scriptSteps();
                    steps[0].targetPos = newPos_;
                    pDriver->setScriptSteps(steps);
                    rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pDriver);
                }
            }
        }
    }
    return true;
}

bool MoveEntityCommand::undo() {
    if (!pObj_) return false;
    pObj_->setPosition(oldPos_.tx, oldPos_.ty, oldPos_.tz, oldPos_.ox, oldPos_.oy, oldPos_.oz);
    if (pMission_) {
        if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pObj_)) {
            if (pPed->isOurAgent() && pPed->id() < 4) {
                pMission_->setSquadSpawnPoint(pPed->id(), oldPos_, static_cast<uint8_t>(pPed->direction()));
            }
            if (!pPed->scriptSteps().empty()) {
                auto steps = pPed->scriptSteps();
                steps[0].targetPos = oldPos_;
                pPed->setScriptSteps(steps);
                rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed);
            }
        } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pObj_)) {
            if (auto *pDriver = pVeh->getDriver()) {
                pDriver->setPosition(oldPos_);
                if (!pDriver->scriptSteps().empty()) {
                    auto steps = pDriver->scriptSteps();
                    steps[0].targetPos = oldPos_;
                    pDriver->setScriptSteps(steps);
                    rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pDriver);
                }
            }
        }
    }
    return true;
}

std::string MoveEntityCommand::description() const {
    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pObj_)) {
        if (pPed->isOurAgent()) {
            return std::format("Move Player's Agent #{} from ({}, {}) to ({}, {})", pPed->id() + 1, oldPos_.tx, oldPos_.ty, newPos_.tx, newPos_.ty);
        }
    }
    return std::format("Move entity from ({}, {}) to ({}, {})", oldPos_.tx, oldPos_.ty, newPos_.tx, newPos_.ty);
}

// --- SpawnVehicleCommand ---

SpawnVehicleCommand::SpawnVehicleCommand(rs_knl::Mission *pMission, uint8_t type,
                                         const rs_knl::TilePoint &pos, uint8_t orientation,
                                         int health, int driverType, uint16_t driverBaseAnim,
                                         uint16_t baseAnim)
    : pMission_(pMission), type_(type), pos_(pos), orientation_(orientation),
      health_(health), driverType_(driverType), driverBaseAnim_(driverBaseAnim), baseAnim_(baseAnim) {}

bool SpawnVehicleCommand::execute() {
    if (!pMission_) return false;
    size_t countBefore = pMission_->numVehicles();
    pSpawnedVehicle_ = pMission_->spawnVehicle(type_, pos_, orientation_, baseAnim_);
    if (!pSpawnedVehicle_) return false;

    size_t countAfter = pMission_->numVehicles();
    spawnedVehicles_.clear();
    for (size_t i = countBefore; i < countAfter; ++i) {
        rs_knl::Vehicle *v = pMission_->vehicle(i);
        if (v) {
            spawnedVehicles_.push_back(v);
            if (health_ > 0) {
                v->setStartHealth(health_);
                v->setHealth(health_);
            } else if (health_ <= 0 && health_ != 0) {
                v->setStartHealth(0);
                v->setHealth(-1);
                v->playAnimation(v->burntAnimation());
            }
        }
    }

    if (driverType_ > 0) {
        if (pSpawnedVehicle_ && (pSpawnedVehicle_->isCar() || pSpawnedVehicle_->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead)) {
            pSpawnedDriver_ = pMission_->spawnPed(
                static_cast<rs_knl::PedInstance::PedType>(driverType_),
                pos_, orientation_, driverBaseAnim_);
            if (pSpawnedDriver_) {
                pSpawnedVehicle_->setDriver(pSpawnedDriver_);
                pSpawnedDriver_->setPosition(pSpawnedVehicle_->position());
            }
        }
    }

    return true;
}

bool SpawnVehicleCommand::undo() {
    if (!pMission_) return false;
    if (pSpawnedDriver_) {
        pMission_->removePed(pSpawnedDriver_);
        pSpawnedDriver_ = nullptr;
    }
    bool res = true;
    for (auto it = spawnedVehicles_.rbegin(); it != spawnedVehicles_.rend(); ++it) {
        if (*it) {
            if (!pMission_->removeVehicle(*it)) res = false;
        }
    }
    spawnedVehicles_.clear();
    pSpawnedVehicle_ = nullptr;
    return res;
}

std::string SpawnVehicleCommand::description() const {
    if (type_ == 0xFE) {
        return std::format("Spawn train group at ({}, {}, {})", pos_.tx, pos_.ty, pos_.tz);
    }
    return std::format("Spawn vehicle at ({}, {}, {})", pos_.tx, pos_.ty, pos_.tz);
}

// --- DeleteVehicleCommand ---

DeleteVehicleCommand::DeleteVehicleCommand(rs_knl::Mission *pMission, rs_knl::Vehicle *pVehicle)
    : pMission_(pMission), pVehicleRef_(pVehicle) {
    if (pVehicle) {
        type_ = pVehicle->getType();
        pos_ = rs_knl::TilePoint(pVehicle->tileX(), pVehicle->tileY(), pVehicle->tileZ());
        orientation_ = static_cast<uint8_t>(pVehicle->direction());
        health_ = pVehicle->health();
        baseAnim_ = pVehicle->getDrawAnimation();
        origDriver_ = pVehicle->getDriver();
    }
}

bool DeleteVehicleCommand::execute() {
    if (!pMission_ || !pVehicleRef_) return false;

    // Find the current index of the vehicle in pMission_->vehicles_
    origVehicleIndex_ = -1;
    for (size_t i = 0; i < pMission_->numVehicles(); ++i) {
        if (pMission_->vehicle(i) == pVehicleRef_) {
            origVehicleIndex_ = static_cast<int>(i);
            break;
        }
    }

    affectedPedRoutes_.clear();

    // 1. Handle Assigned Driver of this vehicle
    origDriver_ = pVehicleRef_->getDriver();
    if (origDriver_) {
        affectedPedRoutes_.push_back({ origDriver_, origDriver_->scriptSteps() });
        origDriver_->clearScriptSteps();
        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, origDriver_);
        pVehicleRef_->setDriver(nullptr);
    }

    // 2. Handle all pedestrians with route steps dependent on this vehicle (UseVehicle -> LeaveVehicle)
    if (origVehicleIndex_ >= 0) {
        for (size_t pi = 0; pi < pMission_->numPeds(); ++pi) {
            rs_knl::PedInstance *p = pMission_->ped(pi);
            if (!p || p == origDriver_) continue;

            const auto &steps = p->scriptSteps();
            bool hasRef = false;
            for (const auto &st : steps) {
                if (st.type == LevelData::kScenarioTypeUseVehicle && st.targetEntityId >= origVehicleIndex_) {
                    hasRef = true;
                    break;
                }
            }
            if (!hasRef) continue;

            affectedPedRoutes_.push_back({ p, steps });

            std::vector<rs_knl::ScriptStep> newSteps;
            bool inDeletedVehicle = false;

            for (size_t si = 0; si < steps.size(); ++si) {
                const auto &st = steps[si];

                if (st.type == LevelData::kScenarioTypeUseVehicle) {
                    if (st.targetEntityId == origVehicleIndex_) {
                        inDeletedVehicle = true;
                        continue;
                    } else if (st.targetEntityId > origVehicleIndex_) {
                        auto adjusted = st;
                        adjusted.targetEntityId--;
                        newSteps.push_back(adjusted);
                        inDeletedVehicle = false;
                        continue;
                    } else {
                        newSteps.push_back(st);
                        inDeletedVehicle = false;
                        continue;
                    }
                }

                if (inDeletedVehicle) {
                    if (st.type == LevelData::kScenarioTypeLeaveVehicle) {
                        inDeletedVehicle = false;
                    }
                    continue;
                }

                newSteps.push_back(st);
            }

            // Remove orphaned loop reset if no actionable waypoints remain
            bool hasRealActions = false;
            for (const auto &st : newSteps) {
                if (st.type != LevelData::kScenarioTypeReset) {
                    hasRealActions = true;
                    break;
                }
            }
            if (!hasRealActions) {
                newSteps.clear();
            }

            p->setScriptSteps(newSteps);
            rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, p);
        }
    }

    return pMission_->removeVehicle(pVehicleRef_);
}

bool DeleteVehicleCommand::undo() {
    if (!pMission_) return false;
    pVehicleRef_ = pMission_->spawnVehicle(type_, pos_, orientation_, baseAnim_);
    if (!pVehicleRef_) return false;

    pVehicleRef_->setHealth(health_);
    if (health_ <= 0 && health_ != 0) {
        pVehicleRef_->setStartHealth(0);
        pVehicleRef_->setHealth(-1);
        pVehicleRef_->playAnimation(pVehicleRef_->burntAnimation());
    } else {
        pVehicleRef_->setStartHealth(health_);
        pVehicleRef_->playAnimation(pVehicleRef_->regularAnimation());
    }

    // Restore vehicle back to its original slot index
    if (origVehicleIndex_ >= 0 && origVehicleIndex_ < static_cast<int>(pMission_->numVehicles())) {
        size_t currentIdx = pMission_->numVehicles() - 1;
        if (currentIdx != static_cast<size_t>(origVehicleIndex_)) {
            pMission_->moveVehicleToIndex(currentIdx, static_cast<size_t>(origVehicleIndex_));
        }
    }

    // Reattach driver
    if (origDriver_) {
        pVehicleRef_->setDriver(origDriver_, true);
        origDriver_->setPosition(pVehicleRef_->position());
    }

    // Restore all affected pedestrian routes
    for (auto &pair : affectedPedRoutes_) {
        if (pair.first) {
            pair.first->setScriptSteps(pair.second);
            rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pair.first);
        }
    }

    return true;
}

std::string DeleteVehicleCommand::description() const {
    return std::format("Delete vehicle at ({}, {}, {})", pos_.tx, pos_.ty, pos_.tz);
}

// --- SpawnStaticCommand ---

SpawnStaticCommand::SpawnStaticCommand(rs_knl::Mission *pMission, uint16_t staticId, const rs_knl::TilePoint &pos, uint16_t baseAnim, uint8_t orientation)
    : pMission_(pMission), staticId_(staticId), pos_(pos), baseAnim_(baseAnim), orientation_(orientation) {}

bool SpawnStaticCommand::execute() {
    if (!pMission_) return false;
    pSpawnedStatic_ = pMission_->spawnStatic(staticId_, pos_, baseAnim_, orientation_);
    return pSpawnedStatic_ != nullptr;
}

bool SpawnStaticCommand::undo() {
    if (!pMission_ || !pSpawnedStatic_) return false;
    bool res = pMission_->removeStatic(pSpawnedStatic_);
    pSpawnedStatic_ = nullptr;
    return res;
}

std::string SpawnStaticCommand::description() const {
    return std::format("Spawn static prop at ({}, {}, {})", pos_.tx, pos_.ty, pos_.tz);
}

// --- DeleteStaticCommand ---

DeleteStaticCommand::DeleteStaticCommand(rs_knl::Mission *pMission, rs_knl::Static *pStatic)
    : pMission_(pMission), pStaticRef_(pStatic) {
    if (pStatic) {
        staticId_ = pStatic->subType();
        baseAnim_ = pStatic->getDrawAnimation();
        orientation_ = static_cast<uint8_t>(pStatic->orientation() * 64);
        pos_ = rs_knl::TilePoint(pStatic->tileX(), pStatic->tileY(), pStatic->tileZ());
    }
}

bool DeleteStaticCommand::execute() {
    if (!pMission_ || !pStaticRef_) return false;
    return pMission_->removeStatic(pStaticRef_);
}

bool DeleteStaticCommand::undo() {
    if (!pMission_) return false;
    pStaticRef_ = pMission_->spawnStatic(staticId_, pos_, baseAnim_, orientation_);
    return pStaticRef_ != nullptr;
}

std::string DeleteStaticCommand::description() const {
    return std::format("Delete static prop at ({}, {}, {})", pos_.tx, pos_.ty, pos_.tz);
}

// --- SpawnWeaponCommand ---

SpawnWeaponCommand::SpawnWeaponCommand(rs_knl::Mission *pMission, rs_knl::Weapon::WeaponType wType, const rs_knl::TilePoint &pos)
    : pMission_(pMission), wType_(wType), pos_(pos) {}

bool SpawnWeaponCommand::execute() {
    if (!pMission_) return false;
    pSpawnedWeapon_ = pMission_->spawnGroundWeapon(wType_, pos_);
    return pSpawnedWeapon_ != nullptr;
}

bool SpawnWeaponCommand::undo() {
    if (!pMission_ || !pSpawnedWeapon_) return false;
    bool res = pMission_->removeGroundWeapon(pSpawnedWeapon_);
    pSpawnedWeapon_ = nullptr;
    return res;
}

std::string SpawnWeaponCommand::description() const {
    return std::format("Spawn weapon drop at ({}, {}, {})", pos_.tx, pos_.ty, pos_.tz);
}

// --- DeleteWeaponCommand ---

DeleteWeaponCommand::DeleteWeaponCommand(rs_knl::Mission *pMission, rs_knl::WeaponInstance *pWeapon)
    : pMission_(pMission), pWeaponRef_(pWeapon) {
    if (pWeapon) {
        wType_ = pWeapon->type();
        pos_ = rs_knl::TilePoint(pWeapon->tileX(), pWeapon->tileY(), pWeapon->tileZ());
    }
}

bool DeleteWeaponCommand::execute() {
    if (!pMission_ || !pWeaponRef_) return false;
    return pMission_->removeGroundWeapon(pWeaponRef_);
}

bool DeleteWeaponCommand::undo() {
    if (!pMission_) return false;
    pWeaponRef_ = pMission_->spawnGroundWeapon(wType_, pos_);
    return pWeaponRef_ != nullptr;
}

std::string DeleteWeaponCommand::description() const {
    return std::format("Delete weapon at ({}, {}, {})", pos_.tx, pos_.ty, pos_.tz);
}

// --- ModifyPedRouteCommand ---

ModifyPedRouteCommand::ModifyPedRouteCommand(rs_knl::Mission *pMission, rs_knl::PedInstance *pPed,
                                             const std::vector<rs_knl::ScriptStep> &oldSteps,
                                             const std::vector<rs_knl::ScriptStep> &newSteps,
                                             bool oldLoop, bool newLoop)
    : pMission_(pMission), pPed_(pPed),
      oldLoop_(oldLoop), newLoop_(newLoop) {
    oldSteps_.reserve(oldSteps.size());
    for (const auto &st : oldSteps) {
        if (st.type != LevelData::kScenarioTypeReset) {
            oldSteps_.push_back(st);
        }
    }
    newSteps_.reserve(newSteps.size());
    for (const auto &st : newSteps) {
        if (st.type != LevelData::kScenarioTypeReset) {
            newSteps_.push_back(st);
        }
    }
}

bool ModifyPedRouteCommand::execute() {
    if (!pPed_) return false;
    pPed_->setScriptSteps(newSteps_);
    pPed_->setScriptLooping(newLoop_);
    if (!newSteps_.empty() && !oldSteps_.empty() &&
        (newSteps_[0].targetPos.tx != oldSteps_[0].targetPos.tx ||
         newSteps_[0].targetPos.ty != oldSteps_[0].targetPos.ty ||
         newSteps_[0].targetPos.tz != oldSteps_[0].targetPos.tz)) {
        pPed_->setPosition(newSteps_[0].targetPos);
        if (pMission_ && pPed_->inVehicle()) {
            pPed_->inVehicle()->setPosition(newSteps_[0].targetPos);
        }
    }
    if (pMission_) {
        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed_);
    }
    return true;
}

bool ModifyPedRouteCommand::undo() {
    if (!pPed_) return false;
    pPed_->setScriptSteps(oldSteps_);
    pPed_->setScriptLooping(oldLoop_);
    if (!newSteps_.empty() && !oldSteps_.empty() &&
        (newSteps_[0].targetPos.tx != oldSteps_[0].targetPos.tx ||
         newSteps_[0].targetPos.ty != oldSteps_[0].targetPos.ty ||
         newSteps_[0].targetPos.tz != oldSteps_[0].targetPos.tz)) {
        pPed_->setPosition(oldSteps_[0].targetPos);
        if (pMission_ && pPed_->inVehicle()) {
            pPed_->inVehicle()->setPosition(oldSteps_[0].targetPos);
        }
    }
    if (pMission_) {
        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPed_);
    }
    return true;
}

std::string ModifyPedRouteCommand::description() const {
    int pedId = pPed_ ? pPed_->id() : 0;
    if (oldSteps_.empty() && !newSteps_.empty()) {
        return std::format("Create Route for Ped #{} ({} steps)", pedId, newSteps_.size());
    } else if (!oldSteps_.empty() && newSteps_.empty()) {
        return std::format("Delete Route for Ped #{}", pedId);
    }
    return std::format("Modify Route for Ped #{} ({} steps)", pedId, newSteps_.size());
}

// --- ModifyEntityAttributesCommand ---

ModifyEntityAttributesCommand::ModifyEntityAttributesCommand(
    rs_knl::Mission *pMission, rs_knl::ShootableMapObject *pObj,
    const Attrs &oldAttrs, const Attrs &newAttrs)
    : pMission_(pMission), pObj_(pObj), oldAttrs_(oldAttrs), newAttrs_(newAttrs) {}

bool ModifyEntityAttributesCommand::execute() {
    if (!pObj_) return false;
    apply(newAttrs_, false);
    return true;
}

bool ModifyEntityAttributesCommand::undo() {
    if (!pObj_) return false;
    apply(oldAttrs_, true);
    return true;
}

void ModifyEntityAttributesCommand::apply(const Attrs &attrs, bool isUndo) {
    if (!pObj_) return;

    if (auto *pPed = dynamic_cast<rs_knl::PedInstance*>(pObj_)) {
        pPed->setHealth(attrs.health);
        pPed->setDirection(attrs.direction);
        if (attrs.health <= 0) {
            pPed->playDeadAnimation();
        } else {
            pPed->playStandOrWalkAnimation();
        }

        pPed->setInitialLevelState(attrs.movementMode);
        if (pPed->type() == rs_knl::PedInstance::kPedTypeAgent && !pPed->isOurAgent()) {
            pPed->setCyberwareOverridden(attrs.cyberwareOverridden);
            if (attrs.cyberwareOverridden) {
                pPed->applyCyberwareTier(attrs.cyberwareTier);
            } else if (pMission_) {
                pPed->applyCyberwareTier(pMission_->techLevel());
            }
            if (attrs.factionId >= 1 && attrs.factionId <= 7 && attrs.factionId != pPed->factionId()) {
                if (pMission_) {
                    pMission_->allocateFactionSlot(attrs.factionId, pPed);
                }
            }
            pPed->setFactionId(attrs.factionId);
        }

        if (pMission_ && pPed->scriptSteps().empty()) {
            pPed->destroyAllActions(true);
            if (attrs.movementMode & LevelData::kPeopleStateWalking) {
                pPed->addToDefaultActions(new rs_knl::WalkToDirectionAction());
                pPed->addMovementAction(pPed->defaultAction(), false);
            }
        }

        if (pPed->isOurAgent() && pMission_) {
            size_t slot = (pPed->id() < 4) ? pPed->id() : 0;
            pMission_->setSquadSpawnPoint(slot, pPed->position(), attrs.direction);
        }

        // Sync weapons
        while (pPed->numWeapons() > 0) {
            rs_knl::WeaponInstance *w = pPed->removeWeaponAtIndex(0);
            delete w;
        }
        for (auto wt : attrs.weapons) {
            rs_knl::Weapon *pDef = g_weaponMgr.getWeapon(wt);
            if (pDef) {
                rs_knl::WeaponInstance *wi = rs_knl::WeaponInstance::createInstance(pDef);
                if (wi) {
                    wi->setOwner(pPed);
                    pPed->addWeapon(wi);
                }
            }
        }
    } else if (auto *pVeh = dynamic_cast<rs_knl::Vehicle*>(pObj_)) {
        pVeh->setHealth(attrs.health);
        pVeh->setDirection(attrs.direction);
        if (attrs.health <= 0) {
            pVeh->playAnimation(pVeh->burntAnimation());
        } else {
            pVeh->playAnimation(pVeh->regularAnimation());
        }

        if (pVeh->isCar() || pVeh->getType() == rs_knl::Vehicle::kVehicleTypeTrainHead) {
            auto findExitPos = [&]() -> rs_knl::TilePoint {
                rs_knl::TilePoint exitPos(pVeh->tileX() + 1, pVeh->tileY(), pVeh->tileZ(), 128, 128, 0);
                static const int dirs[8][2] = {
                    { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 },
                    { 1, 1 }, { -1, -1 }, { 1, -1 }, { -1, 1 }
                };
                for (int d = 0; d < 8; ++d) {
                    int tx = pVeh->tileX() + dirs[d][0];
                    int ty = pVeh->tileY() + dirs[d][1];
                    if (pMission_ && pMission_->isTileWalkable(tx, ty, pVeh->tileZ())) {
                        exitPos = rs_knl::TilePoint(tx, ty, pVeh->tileZ(), 128, 128, 0);
                        break;
                    }
                }
                return exitPos;
            };

            if (!isUndo) {
                rs_knl::PedInstance *oldDriver = pVeh->getDriver();
                pPreviousDriver_ = oldDriver;
                if (oldDriver) {
                    oldDriverRoute_ = oldDriver->scriptSteps();
                }

                if (attrs.pDriver == nullptr && attrs.driverChoice != 1) {
                    if (oldDriver) {
                        oldDriverExitPos_ = findExitPos();
                        pVeh->setDriver(nullptr);
                        oldDriver->setPosition(oldDriverExitPos_);
                        oldDriver->clearScriptSteps();
                        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, oldDriver);
                    }
                } else if (attrs.driverChoice == 1 && attrs.pDriver == nullptr) {
                    if (oldDriver && oldDriver != pSpawnedDriver_) {
                        oldDriverExitPos_ = findExitPos();
                        oldDriver->setPosition(oldDriverExitPos_);
                        oldDriver->clearScriptSteps();
                        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, oldDriver);
                    }
                    if (!pSpawnedDriver_ && pMission_) {
                        pSpawnedDriver_ = pMission_->spawnPed(
                            rs_knl::PedInstance::kPedTypeCivilian, pVeh->position(), static_cast<uint8_t>(pVeh->direction()));
                    }
                    if (pSpawnedDriver_) {
                        pVeh->setDriver(pSpawnedDriver_, true);
                        if (!oldDriverRoute_.empty() && pSpawnedDriver_->scriptSteps().empty()) {
                            pSpawnedDriver_->setScriptSteps(oldDriverRoute_);
                            rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pSpawnedDriver_);
                        }
                    }
                } else if (attrs.pDriver != nullptr || attrs.driverChoice >= 2) {
                    if (oldDriver && oldDriver != attrs.pDriver) {
                        oldDriverExitPos_ = findExitPos();
                        oldDriver->setPosition(oldDriverExitPos_);
                        oldDriver->clearScriptSteps();
                        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, oldDriver);
                    }
                    if (attrs.pDriver) {
                        newDriverPreviousRoute_ = attrs.pDriver->scriptSteps();
                        pVeh->setDriver(attrs.pDriver, true);
                        attrs.pDriver->setPosition(pVeh->position());
                        if (!oldDriverRoute_.empty() && newDriverPreviousRoute_.empty()) {
                            attrs.pDriver->setScriptSteps(oldDriverRoute_);
                            rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, attrs.pDriver);
                        }
                    }
                }
            } else {
                // Undoing back to oldAttrs
                if (pSpawnedDriver_) {
                    pVeh->setDriver(nullptr);
                    if (pMission_) pMission_->removePed(pSpawnedDriver_);
                    pSpawnedDriver_ = nullptr;
                }
                if (newAttrs_.pDriver && newAttrs_.pDriver != pPreviousDriver_) {
                    newAttrs_.pDriver->setScriptSteps(newDriverPreviousRoute_);
                    rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, newAttrs_.pDriver);
                }
                if (attrs.pDriver) {
                    pVeh->setDriver(attrs.pDriver, true);
                    attrs.pDriver->setPosition(pVeh->position());
                    attrs.pDriver->setScriptSteps(oldDriverRoute_);
                    rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, attrs.pDriver);
                } else {
                    pVeh->setDriver(nullptr);
                    if (newAttrs_.pDriver && newAttrs_.pDriver != pSpawnedDriver_) {
                        newAttrs_.pDriver->setPosition(oldDriverExitPos_);
                    }
                    if (pPreviousDriver_) {
                        pPreviousDriver_->setScriptSteps(oldDriverRoute_);
                        rs_knl::MissionManager::buildScriptedActionsForPed(pMission_, pPreviousDriver_);
                    }
                }
            }
        }
    } else if (auto *pStatic = dynamic_cast<rs_knl::Static*>(pObj_)) {
        pStatic->setHealth(attrs.health);
        pStatic->setOrientation(attrs.direction);
    }
}

std::string ModifyEntityAttributesCommand::description() const {
    if (dynamic_cast<rs_knl::PedInstance*>(pObj_)) return "Modify Pedestrian Attributes";
    if (dynamic_cast<rs_knl::Vehicle*>(pObj_)) return "Modify Vehicle Attributes";
    if (dynamic_cast<rs_knl::Static*>(pObj_)) return "Modify Static Attributes";
    return "Modify Entity Attributes";
}

// --- AddObjectiveCommand ---

AddObjectiveCommand::AddObjectiveCommand(rs_knl::Mission *pMission, const ObjectiveDialogData &data)
    : pMission_(pMission), data_(data) {}

bool AddObjectiveCommand::execute() {
    if (!pMission_) return false;
    rs_knl::ObjectiveDesc *obj = ObjectiveDialog::createObjectiveFromData(pMission_, data_);
    if (!obj) return false;
    pMission_->addObjective(obj);
    return true;
}

bool AddObjectiveCommand::undo() {
    if (!pMission_ || pMission_->numObjectives() == 0) return false;
    pMission_->removeObjective(pMission_->numObjectives() - 1);
    return true;
}

std::string AddObjectiveCommand::description() const {
    return "Add Objective";
}

// --- EditObjectiveCommand ---

EditObjectiveCommand::EditObjectiveCommand(rs_knl::Mission *pMission, size_t index,
                                           const ObjectiveDialogData &oldData,
                                           const ObjectiveDialogData &newData)
    : pMission_(pMission), index_(index), oldData_(oldData), newData_(newData) {}

bool EditObjectiveCommand::execute() {
    if (!pMission_ || index_ >= pMission_->numObjectives()) return false;
    rs_knl::ObjectiveDesc *newObj = ObjectiveDialog::createObjectiveFromData(pMission_, newData_);
    if (!newObj) return false;
    pMission_->replaceObjective(index_, newObj);
    return true;
}

bool EditObjectiveCommand::undo() {
    if (!pMission_ || index_ >= pMission_->numObjectives()) return false;
    rs_knl::ObjectiveDesc *oldObj = ObjectiveDialog::createObjectiveFromData(pMission_, oldData_);
    if (!oldObj) return false;
    pMission_->replaceObjective(index_, oldObj);
    return true;
}

std::string EditObjectiveCommand::description() const {
    return "Edit Objective";
}

// --- DeleteObjectiveCommand ---

DeleteObjectiveCommand::DeleteObjectiveCommand(rs_knl::Mission *pMission, size_t index, const ObjectiveDialogData &oldData)
    : pMission_(pMission), index_(index), oldData_(oldData) {}

bool DeleteObjectiveCommand::execute() {
    if (!pMission_ || index_ >= pMission_->numObjectives()) return false;
    pMission_->removeObjective(index_);
    return true;
}

bool DeleteObjectiveCommand::undo() {
    if (!pMission_) return false;
    rs_knl::ObjectiveDesc *oldObj = ObjectiveDialog::createObjectiveFromData(pMission_, oldData_);
    if (!oldObj) return false;
    pMission_->insertObjective(index_, oldObj);
    return true;
}

std::string DeleteObjectiveCommand::description() const {
    return "Delete Objective";
}

// --- ClearObjectivesCommand ---

ClearObjectivesCommand::ClearObjectivesCommand(rs_knl::Mission *pMission, const std::vector<ObjectiveDialogData> &oldDataList)
    : pMission_(pMission), oldDataList_(oldDataList) {}

bool ClearObjectivesCommand::execute() {
    if (!pMission_) return false;
    pMission_->clearObjectives();
    return true;
}

bool ClearObjectivesCommand::undo() {
    if (!pMission_) return false;
    pMission_->clearObjectives();
    for (const auto &d : oldDataList_) {
        rs_knl::ObjectiveDesc *obj = ObjectiveDialog::createObjectiveFromData(pMission_, d);
        if (obj) {
            pMission_->addObjective(obj);
        }
    }
    return true;
}

std::string ClearObjectivesCommand::description() const {
    return "Clear Objectives";
}

// --- EditAITacticsCommand ---

EditAITacticsCommand::EditAITacticsCommand(
    rs_knl::Mission *pMission,
    const LevelData::AITacticsBlock &oldTactics,
    const LevelData::AITacticsBlock &newTactics)
    : pMission_(pMission), oldTactics_(oldTactics), newTactics_(newTactics) {}

bool EditAITacticsCommand::execute() {
    if (!pMission_) return false;
    pMission_->tactics() = newTactics_;
    return true;
}

bool EditAITacticsCommand::undo() {
    if (!pMission_) return false;
    pMission_->tactics() = oldTactics_;
    return true;
}

std::string EditAITacticsCommand::description() const {
    return "Edit AI Tactics";
}

// --- EditorCommandStack ---

EditorCommandStack::EditorCommandStack(size_t maxHistory) : maxHistory_(maxHistory) {}

bool EditorCommandStack::pushAndExecute(std::unique_ptr<IEditorCommand> cmd) {
    if (!cmd) return false;
    if (cmd->execute()) {
        redoStack_.clear();
        undoStack_.push_back(std::move(cmd));
        if (undoStack_.size() > maxHistory_) {
            undoStack_.erase(undoStack_.begin());
        }
        return true;
    }
    return false;
}

bool EditorCommandStack::undo() {
    if (undoStack_.empty()) return false;
    auto cmd = std::move(undoStack_.back());
    undoStack_.pop_back();
    if (cmd->undo()) {
        redoStack_.push_back(std::move(cmd));
        return true;
    }
    return false;
}

bool EditorCommandStack::redo() {
    if (redoStack_.empty()) return false;
    auto cmd = std::move(redoStack_.back());
    redoStack_.pop_back();
    if (cmd->execute()) {
        undoStack_.push_back(std::move(cmd));
        return true;
    }
    return false;
}

void EditorCommandStack::clear() {
    undoStack_.clear();
    redoStack_.clear();
}

std::string EditorCommandStack::getUndoDescription() const {
    if (undoStack_.empty()) return "";
    return undoStack_.back()->description();
}

std::string EditorCommandStack::getRedoDescription() const {
    if (redoStack_.empty()) return "";
    return redoStack_.back()->description();
}

} // namespace rs_win_editor
