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

#include "mission_validator.h"

#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/static.h"
#include "rs-kernel/model/map.h"
#include "rs-kernel/model/objectivedesc.h"
#include "rs-engine/gfx/tile.h"

namespace rs_win_editor {

namespace {

std::string getPedTypeString(rs_knl::PedInstance *p) {
    if (!p) return "Unknown Ped";
    if (p->isOurAgent()) {
        return std::format("Player's Agent #{}", p->id() + 1);
    }
    switch (p->type()) {
        case rs_knl::PedInstance::kPedTypeCivilian: return std::format("Civilian #{:03}", p->id());
        case rs_knl::PedInstance::kPedTypeAgent:    return std::format("Enemy Agent #{:03}", p->id());
        case rs_knl::PedInstance::kPedTypeGuard:    return std::format("Guard #{:03}", p->id());
        case rs_knl::PedInstance::kPedTypePolice:   return std::format("Police #{:03}", p->id());
        case rs_knl::PedInstance::kPedTypeCriminal: return std::format("Criminal #{:03}", p->id());
        default: return std::format("Ped #{:03}", p->id());
    }
}

std::string getVehicleTypeString(rs_knl::Vehicle *v, size_t idx) {
    if (!v) return std::format("Vehicle #{:02}", idx);
    if (!v->isCar()) {
        return std::format("Train Car #{:02}", idx);
    }
    return std::format("Car #{:02}", idx);
}

std::string getStaticTypeString(rs_knl::Static *s, size_t idx) {
    if (!s) return std::format("Static #{:03}", idx);
    switch (s->type()) {
        case rs_knl::Static::smt_Door:           return std::format("Door #{:03}", idx);
        case rs_knl::Static::smt_LargeDoor:      return std::format("Large Door #{:03}", idx);
        case rs_knl::Static::smt_Tree:           return std::format("Tree #{:03}", idx);
        case rs_knl::Static::smt_Semaphore:      return std::format("Semaphore #{:03}", idx);
        case rs_knl::Static::smt_Advertisement:  return std::format("Ad Sign #{:03}", idx);
        case rs_knl::Static::smt_NeonSign:       return std::format("Neon Sign #{:03}", idx);
        case rs_knl::Static::smt_Window:         return std::format("Window #{:03}", idx);
        case rs_knl::Static::smt_AnimatedWindow: return std::format("Anim Window #{:03}", idx);
        default: return std::format("Static #{:03}", idx);
    }
}

//! Returns true if there is at least one non-empty tile in the column at (tx, ty) at or below maxCheckZ (or z=1 if maxCheckZ is 0).
bool hasTerrainBeneath(rs_knl::Map *pMap, int tx, int ty, int maxCheckZ) {
    if (!pMap) return false;
    if (tx < 0 || tx >= pMap->maxTx() || ty < 0 || ty >= pMap->maxTy()) return false;
    int limitZ = std::min(std::max(maxCheckZ, 1), pMap->maxTz() - 1);
    for (int z = limitZ; z >= 0; --z) {
        rs_eng::Tile *t = pMap->getTileAt(tx, ty, z);
        if (t && t->id() != 0) {
            return true;
        }
    }
    return false;
}

} // anonymous namespace

ValidationReport MissionValidator::validate(rs_knl::Mission *pMission) {
    ValidationReport report;
    if (!pMission || !pMission->map()) {
        report.addError("Mission", rs_knl::TilePoint(0, 0, 0), "No active mission or map loaded.");
        return report;
    }

    if (!pMission->mdpoints_) {
        pMission->buildNavigationGraph();
    }

    rs_knl::Map *pMap = pMission->map();
    int maxTx = pMap->maxTx();
    int maxTy = pMap->maxTy();
    int maxTz = pMap->maxTz();

    // -------------------------------------------------------------------------
    // 1. Player's Agents (Agent Slots 0..3)
    // -------------------------------------------------------------------------
    int squadAgentCount = 0;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        rs_knl::PedInstance *p = pMission->ped(i);
        if (p && p->isOurAgent()) {
            squadAgentCount++;
            int tx = p->tileX();
            int ty = p->tileY();
            int tz = p->tileZ();
            std::string agentDesc = std::format("Player's Agent #{}", p->id() + 1);

            // Bounds check
            if (tx < 0 || tx >= maxTx || ty < 0 || ty >= maxTy || tz < 0 || tz >= maxTz) {
                report.addError(agentDesc, p->position(),
                    std::format("Spawn position ({}, {}, {}) is outside map boundaries [0..{}, 0..{}, 0..{}].",
                        tx, ty, tz, maxTx - 1, maxTy - 1, maxTz - 1));
                continue;
            }

            // Supporting terrain check
            if (!hasTerrainBeneath(pMap, tx, ty, tz)) {
                report.addError(agentDesc, p->position(),
                    std::format("Spawn position ({}, {}, {}) is floating over empty void space with no terrain beneath.",
                        tx, ty, tz));
            }

            // Solid tile overlap check: Squad agent must not be spawned inside a solid wall
            rs_eng::Tile *tAt = pMap->getTileAt(tx, ty, tz);
            if (tAt && tAt->id() != 0 && tAt->type() == rs_eng::Tile::kWall && !pMission->isTileWalkable(tx, ty, tz)) {
                report.addError(agentDesc, p->position(),
                    std::format("Spawn position ({}, {}, {}) is embedded inside solid wall tile #{}.",
                        tx, ty, tz, tAt->id()));
            }

            // Walkability check: Squad agents must be able to move from their spawn position
            if (!pMission->isTileWalkable(tx, ty, tz)) {
                report.addError(agentDesc, p->position(),
                    std::format("Spawn position ({}, {}, {}) is not walkable. Squad will not be able to navigate.",
                        tx, ty, tz));
            }
        }
    }

    if (squadAgentCount != 4) {
        report.addError("Player Squad", rs_knl::TilePoint(0, 0, 0),
            std::format("Mission has {} Player's Agents placed! Exactly 4 player agents must be placed on the map.", squadAgentCount));
    }

    // -------------------------------------------------------------------------
    // 2. All Other Pedestrians (Civilians, Enemy Agents, Guards, Police, Criminals)
    // -------------------------------------------------------------------------
    if (pMission->numPeds() > 256) {
        report.addError("Pedestrian Capacity", rs_knl::TilePoint(0, 0, 0),
            std::format("Total pedestrians ({}) exceeds the maximum limit of 256 supported by Syndicate binary mission data.",
                pMission->numPeds()));
    }

    std::unordered_map<uint16_t, rs_knl::PedInstance*> seenPedIds;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        rs_knl::PedInstance *p = pMission->ped(i);
        if (!p) continue;
        auto it = seenPedIds.find(p->id());
        if (it != seenPedIds.end()) {
            report.addError(getPedTypeString(p), p->position(),
                std::format("Pedestrian slot #{:03} is duplicated! Conflicts with another pedestrian at (TX:{}, TY:{}, TZ:{}). Every pedestrian must have a unique slot index (0..255).",
                    p->id(), it->second->tileX(), it->second->tileY(), it->second->tileZ()));
        } else {
            seenPedIds[p->id()] = p;
        }
        if (p->id() >= 256) {
            report.addError(getPedTypeString(p), p->position(),
                std::format("Pedestrian slot #{:03} is out of bounds [0..255].", p->id()));
        }
    }
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        rs_knl::PedInstance *p = pMission->ped(i);
        if (!p || p->isOurAgent()) continue;

        // Skip passengers or drivers currently inside vehicles
        if (p->inVehicle() != nullptr) continue;

        // Skip dead corpses
        if (p->isDead()) continue;

        // Skip inactive reserve slots at (0, 0)
        if (p->tileX() == 0 && p->tileY() == 0) continue;

        int tx = p->tileX();
        int ty = p->tileY();
        int tz = p->tileZ();
        std::string desc = getPedTypeString(p);

        // Bounds check
        if (tx < 0 || tx >= maxTx || ty < 0 || ty >= maxTy || tz < 0 || tz >= maxTz) {
            report.addError(desc, p->position(),
                std::format("Position ({}, {}, {}) is outside map boundaries [0..{}, 0..{}, 0..{}].",
                    tx, ty, tz, maxTx - 1, maxTy - 1, maxTz - 1));
            continue;
        }

        // Supporting terrain check: Must have terrain in the map column
        if (!hasTerrainBeneath(pMap, tx, ty, tz)) {
            report.addWarning(desc, p->position(),
                std::format("Position ({}, {}, {}) is floating over empty void space with no terrain beneath.",
                    tx, ty, tz));
        }
    }

    // Check Enemy Agent Faction capacities and designated slot ranges
    std::unordered_map<int, int> factionCounts;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        rs_knl::PedInstance *p = pMission->ped(i);
        if (!p || p->isOurAgent()) continue;
        if (p->type() == rs_knl::PedInstance::kPedTypeAgent) {
            int fac = p->factionId();
            factionCounts[fac]++;
            if (fac >= 1 && fac <= 7) {
                int expectedMin = fac * 8;
                int expectedMax = fac * 8 + 7;
                if (p->id() < expectedMin || p->id() > expectedMax) {
                    report.addWarning(getPedTypeString(p), p->position(),
                        std::format("Agent slot #{:03} is outside designated range [{}..{}] for Faction {}.",
                            p->id(), expectedMin, expectedMax, fac));
                }
            }
        }
    }
    for (const auto &[facId, count] : factionCounts) {
        if (count > 8) {
            report.addError(std::format("Faction {}", facId), rs_knl::TilePoint(0, 0, 0),
                std::format("Faction {} has {} agents, which exceeds the maximum limit of 8 agents per faction supported by Syndicate mission data.", facId, count));
        }
    }

    // -------------------------------------------------------------------------
    // 3. Vehicles (Cars and Trains)
    // -------------------------------------------------------------------------
    if (pMission->numVehicles() > 64) {
        report.addError("Vehicle Capacity", rs_knl::TilePoint(0, 0, 0),
            std::format("Total vehicles ({}) exceeds the maximum limit of 64 supported by Syndicate mission data.",
                pMission->numVehicles()));
    }
    for (size_t vi = 0; vi < pMission->numVehicles(); ++vi) {
        rs_knl::Vehicle *v = pMission->vehicle(vi);
        if (!v) continue;

        int tx = v->tileX();
        int ty = v->tileY();
        int tz = v->tileZ();
        std::string desc = getVehicleTypeString(v, vi);

        // Bounds check
        if (tx < 0 || tx >= maxTx || ty < 0 || ty >= maxTy || tz < 0 || tz >= maxTz) {
            report.addError(desc, v->position(),
                std::format("Position ({}, {}, {}) is outside map boundaries [0..{}, 0..{}, 0..{}].",
                    tx, ty, tz, maxTx - 1, maxTy - 1, maxTz - 1));
            continue;
        }

        // Supporting terrain check
        if (!hasTerrainBeneath(pMap, tx, ty, tz)) {
            report.addError(desc, v->position(),
                std::format("Position ({}, {}, {}) is floating over empty void space with no terrain beneath.",
                    tx, ty, tz));
        }

        // Wall collision check: Vehicle should not be embedded inside a solid wall
        rs_eng::Tile *tAt = pMap->getTileAt(tx, ty, tz);
        if (tAt && tAt->id() != 0 && tAt->type() == rs_eng::Tile::kWall) {
            report.addWarning(desc, v->position(),
                std::format("Position ({}, {}, {}) is embedded inside solid wall tile #{}.",
                    tx, ty, tz, tAt->id()));
        }

        // Destroyed vehicle with assigned driver check
        if (v->health() <= 0 && v->getDriver() != nullptr) {
            report.addWarning(desc, v->position(),
                std::format("Vehicle #{:03} is destroyed but has an assigned driver (Ped #{:03}).",
                    v->id(), v->getDriver()->id()));
        }
    }

    // -------------------------------------------------------------------------
    // 4. Static Map Objects
    // -------------------------------------------------------------------------
    if (pMission->numStatics() > 400) {
        report.addError("Static Objects Capacity", rs_knl::TilePoint(0, 0, 0),
            std::format("Total static objects ({}) exceeds the maximum limit of 400 supported by Syndicate mission data.",
                pMission->numStatics()));
    }
    for (size_t si = 0; si < pMission->numStatics(); ++si) {
        rs_knl::Static *s = pMission->statics(si);
        if (!s) continue;

        int tx = s->tileX();
        int ty = s->tileY();
        int tz = s->tileZ();
        std::string desc = getStaticTypeString(s, si);

        // Bounds check
        if (tx < 0 || tx >= maxTx || ty < 0 || ty >= maxTy || tz < 0 || tz >= maxTz) {
            report.addError(desc, s->position(),
                std::format("Position ({}, {}, {}) is outside map boundaries [0..{}, 0..{}, 0..{}].",
                    tx, ty, tz, maxTx - 1, maxTy - 1, maxTz - 1));
            continue;
        }

        // Must have some map structure in the column
        if (!hasTerrainBeneath(pMap, tx, ty, maxTz - 1)) {
            report.addWarning(desc, s->position(),
                std::format("Position ({}, {}, {}) is placed over empty void with no map geometry.",
                    tx, ty, tz));
        }
    }

    // -------------------------------------------------------------------------
    // 5. Mission Objectives
    // -------------------------------------------------------------------------
    if (pMission->numObjectives() > 6) {
        report.addError("Objectives Capacity", rs_knl::TilePoint(0, 0, 0),
            std::format("Total objectives ({}) exceeds the maximum limit of 6 supported by Syndicate mission data.",
                pMission->numObjectives()));
    }
    if (pMission->numObjectives() == 0) {
        report.addWarning("Objectives", rs_knl::TilePoint(0, 0, 0),
            "Mission has no objectives configured. Players will have no victory conditions.");
    }

    for (size_t oi = 0; oi < pMission->numObjectives(); ++oi) {
        auto *obj = pMission->objective(oi);
        if (!obj) continue;

        int objNum = static_cast<int>(oi + 1);

        if (auto *oa = dynamic_cast<rs_knl::ObjAssassinate*>(obj)) {
            if (!oa->target()) {
                report.addError(std::format("Objective #{} (Assassinate)", objNum), rs_knl::TilePoint(0, 0, 0),
                    "Target pedestrian was deleted or does not exist in the mission.");
            }
        } else if (auto *op = dynamic_cast<rs_knl::ObjPersuade*>(obj)) {
            if (!op->target()) {
                report.addError(std::format("Objective #{} (Persuade)", objNum), rs_knl::TilePoint(0, 0, 0),
                    "Target pedestrian was deleted or does not exist in the mission.");
            }
        } else if (auto *opr = dynamic_cast<rs_knl::ObjProtect*>(obj)) {
            if (!opr->target()) {
                report.addError(std::format("Objective #{} (Protect)", objNum), rs_knl::TilePoint(0, 0, 0),
                    "Target pedestrian was deleted or does not exist in the mission.");
            }
        } else if (auto *odv = dynamic_cast<rs_knl::ObjDestroyVehicle*>(obj)) {
            if (!odv->target()) {
                report.addError(std::format("Objective #{} (Destroy Vehicle)", objNum), rs_knl::TilePoint(0, 0, 0),
                    "Target vehicle was deleted or does not exist in the mission.");
            }
        } else if (auto *ouv = dynamic_cast<rs_knl::ObjUseVehicle*>(obj)) {
            if (!ouv->target()) {
                report.addError(std::format("Objective #{} (Use Vehicle)", objNum), rs_knl::TilePoint(0, 0, 0),
                    "Target vehicle was deleted or does not exist in the mission.");
            }
        } else if (auto *oev = dynamic_cast<rs_knl::ObjEvacuate*>(obj)) {
            int ex = oev->location().x / 256;
            int ey = oev->location().y / 256;
            int ez = oev->location().z / 128;
            if (ex < 0 || ex >= maxTx || ey < 0 || ey >= maxTy || ez < 0 || ez >= maxTz) {
                report.addError(std::format("Objective #{} (Evacuate)", objNum), rs_knl::TilePoint(ex, ey, ez),
                    std::format("Evacuation zone center ({}, {}, {}) is outside map boundaries.", ex, ey, ez));
            }
        }
    }

    // -------------------------------------------------------------------------
    // 6. Pedestrian Route Steps and Vehicle Dependencies
    // -------------------------------------------------------------------------
    for (size_t pi = 0; pi < pMission->numPeds(); ++pi) {
        rs_knl::PedInstance *p = pMission->ped(pi);
        if (!p) continue;

        const auto &steps = p->scriptSteps();
        std::string pedDesc = getPedTypeString(p);
        bool inVehicle = (p->inVehicle() != nullptr);

        for (size_t si = 0; si < steps.size(); ++si) {
            const auto &st = steps[si];
            if (st.type == LevelData::kScenarioTypeUseVehicle) {
                if (st.targetEntityId < 0 || static_cast<size_t>(st.targetEntityId) >= pMission->numVehicles()) {
                    report.addError(pedDesc, p->position(),
                        std::format("Route Step #{:02}: References invalid or deleted Vehicle #{}.",
                            si + 1, st.targetEntityId));
                } else {
                    rs_knl::Vehicle *v = pMission->vehicle(st.targetEntityId);
                    if (v && v->health() <= 0) {
                        report.addWarning(pedDesc, p->position(),
                            std::format("Route Step #{:02}: Targets Vehicle #{:03} which is currently destroyed.",
                                si + 1, v->id()));
                    }
                    inVehicle = true;
                }
            } else if (st.type == LevelData::kScenarioTypeLeaveVehicle) {
                inVehicle = false;
            }
        }
    }

    return report;
}

} // namespace rs_win_editor
