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
 *   Copyright (C) 2010, 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-kernel/model/mission.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <random>
#include <algorithm>
#include <cmath>

#include "tracy/Tracy.hpp"

#include "rs-utils/log/log.h"
#include "rs-engine/events/event.h"
#include "rs-engine/gfx/tile.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-kernel/model/shot.h"
#include "rs-kernel/model/objectivedesc.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/train.h"
#include "rs-kernel/model/squad.h"
#include "rs-kernel/mgr/pedmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/mgr/vehiclemanager.h"
#include "rs-kernel/ia/behaviour.h"
#include "rs-kernel/model/ped.h"

namespace {
    // Générateur de nombres aléatoires thread-safe
    thread_local std::mt19937 rng(std::random_device{}());
}

namespace rs_knl {

const uint8_t Mission::kBMaskBlockerTargetInRange = 0x01;
const uint8_t Mission::kBMaskBlockerTargetOutOfMap = 0x20;
const uint8_t Mission::kBMaskBlockerTargetObjectUpdated = 0x02;
const uint8_t Mission::kBMaskBlockerTargetPosUpdated = 0x04;
const uint8_t Mission::kBMaskBlockerBlockedByTile = 0x10;

/*!
 * Initialize the statistics.
 * \param nbAgents Number of agents for the mission
 */
void MissionStats::init(size_t nbAgents) {
    nbAgents_ = nbAgents;
    missionDuration_ = 0;
    nbAgentCaptured_ = 0;
    enemyKilled_ = 0;
    criminalKilled_ = 0;
    civilKilled_ = 0;
    policeKilled_ = 0;
    guardKilled_ = 0;
    convinced_ = 0;
    nbOfShots_ = 0;
    nbOfHits_ = 0;
}

/*!
 * Mission constructor
 * @param map_infos 
 * @param pMap 
 */
Mission::Mission() :
        cur_objective_(0),
        p_map_(nullptr), p_minimap_(nullptr),
        squad_(std::make_unique<Squad>()) {
    status_ = kMissionStatusRunning;
    mtsurfaces_ = NULL;
    mdpoints_ = NULL;
    mdpoints_cp_ = NULL;
}

Mission::~Mission()
{
    for (unsigned int i = 0; i < vehicles_.size(); i++)
        delete vehicles_[i];
    for (unsigned int i = 0; i < peds_.size(); i++)
        delete peds_[i];
    for (unsigned int i = 0; i < weaponsOnGround_.size(); i++)
        delete weaponsOnGround_[i];
    
    sfx_objects_.clear();
    projectileShots_.clear();
    statics_.clear();

    for (unsigned int i = 0; i < objectives_.size(); i++)
        delete objectives_[i];
    
    armedPedsVec_.clear();
    escapedPeds_.clear();
    
    clrNavigationGraph();

    if (p_minimap_) {
        delete p_minimap_;
    }
}

/*!
 * Sets the given map for the mission.
 * If p_map is not null, creates a minimap from it.
 * @param map_infos 
 * @param pMap The map to set.
 * @return True if everything is ok
 */
bool Mission::init(Map *pMap) {
    if (!pMap) {
        RSERR(Log::k_FLG_GAME, "Mission", "init", ("Map is null"));
        return false;
    }

    p_map_ = pMap;
    p_map_->mapDimensions(&mmax_x_, &mmax_y_, &mmax_z_);
    mmax_m_xy = mmax_x_ * mmax_y_;

    if (p_minimap_) {
        delete p_minimap_;
    }
    // TODO : change with new init method
    p_minimap_ = new MiniMap();

    hasViewport_ = false;

    return p_minimap_->init(p_map_);
}

/*!
 * Add the given Static to the list of objects to animate
 * @param aStatic 
 */
void Mission::addStatic(std::unique_ptr<Static> aStatic) { 
    statics_.push_back(std::move(aStatic));
}

/*!
 * Adds the given ProjectileShot to the list of animated shots.
 * @param shot The projectile to add
 */
void Mission::addProjectileShot(std::unique_ptr<ProjectileShot> shot) {
    projectileShots_.push_back(std::move(shot));
}

/*!
 * Add an new instance of SfxObject.
 * If object is visible, start its animation.
 * @param so The SfxObject to manage
 */
void Mission::addSfxObject(std::unique_ptr<SFXObject> so) {
    if (so->isDrawable()) {
        so->playMainAnimation();
    }
    sfx_objects_.push_back(std::move(so));
}

void Mission::setViewport(const Point2D &origin, int width, int height, int marginX, int marginY) {
    viewportOriginPt_ = origin;
    viewportWidth_ = width;
    viewportHeight_ = height;
    viewportMarginX_ = marginX;
    viewportMarginY_ = marginY;
    hasViewport_ = true;
}

void Mission::clearViewport() {
    hasViewport_ = false;
}

bool Mission::isObjectInView(const MapObject *pObj) const {
    return isObjectInView(pObj, viewportMarginX_, viewportMarginY_);
}

bool Mission::isObjectInView(const MapObject *pObj, int marginX, int marginY) const {
    if (!hasViewport_ || !pObj || !p_map_) {
        return true;
    }
    Point2D objectViewport;
    p_map_->tileToScreenPoint(pObj->position(), &objectViewport);

    return objectViewport.x >= (viewportOriginPt_.x - marginX) &&
           objectViewport.x <= (viewportOriginPt_.x + viewportWidth_ + marginX) &&
           objectViewport.y >= (viewportOriginPt_.y - marginY) &&
           objectViewport.y <= (viewportOriginPt_.y + viewportHeight_ + marginY);
}

/*!
 * Removes given ped from the list of armed peds.
 * \param pPed The ped to remove
 */
void Mission::removeArmedPed(PedInstance *pPed) {
    std::erase_if(armedPedsVec_, [pPed](PedInstance* ped) {
        return ped == pPed;
    });
}

/*!
 * Sets the given message with the current objective label.
 */
void Mission::objectiveMsg(std::string& msg) {
    if (cur_objective_ < objectives_.size()) {
        msg = objectives_[cur_objective_]->msg;
    } else {
        msg = "";
    }
}

/**
 * Iterates over the given weapon list and identifies the two weapons
 * with the highest rank values.
 *
 * @param weapons List of available weapons to select from.
 * @return A pair of indices {bestWeaponIndex, secondBestWeaponIndex}.
 *         If no weapons are available, indices are set to -1.
 */
std::pair<int, int> Mission::findTopTwoWeapons(const std::vector<Weapon*>& weapons) {
    int indexBest = -1, indexSecond = -1;
    int rankBest = -1, rankSecond = -1;

    for (size_t i = 0; i < weapons.size(); ++i) {
        int weaponRank = weapons[i]->rank();
        if (weaponRank > rankBest) {
            rankSecond = rankBest;
            indexSecond = indexBest;
            rankBest = weaponRank;
            indexBest = static_cast<int>(i);
        } else if (weaponRank > rankSecond) {
            rankSecond = weaponRank;
            indexSecond = static_cast<int>(i);
        }
    }
    return {indexBest, indexSecond};
}

/**
 * For each enemy agent without any weapon, this function assigns either the best-ranked
 * or second-best-ranked weapon (randomly with a small chance) and optionally a time bomb
 * if one is available.
 *
 * @param weapons List of available weapons.
 * @param bomb Pointer to a bomb weapon (can be nullptr if not available).
 */
void Mission::assignWeaponsToEnemyAgents(const std::vector<Weapon*>& weapons, Weapon* bomb) {
    std::uniform_int_distribution<int> distribution(0, 255);

    auto [indexBest, indexSecond] = findTopTwoWeapons(weapons);

    for (size_t i = squad_->size(); i < peds_.size(); ++i) {
        auto* ped = peds_[i];
        if (ped->objGroupDef() == PedInstance::og_dmAgent && ped->numWeapons() == 0) {
            int indexToGive = indexBest;

            if (indexSecond != -1 && distribution(rng) > 200) {
                indexToGive = indexSecond;
            }

            if (indexToGive != -1 && static_cast<size_t>(indexToGive) < weapons.size()) {
                auto* weaponInstance = WeaponInstance::createInstance(weapons[indexToGive]);
                ped->addWeapon(weaponInstance);
                weaponInstance->setOwner(ped);
            }

            if (bomb) {
                auto* bombInstance = WeaponInstance::createInstance(bomb);
                ped->addWeapon(bombInstance);
                bombInstance->setOwner(ped);
            }
        }
    }
}

/**
 * Initializes mission statistics, prepares available weapons,
 * and assigns weapons to enemy agents.
 *
 * @param weaponMgr WeaponManager instance providing access to available weapons.
 */
void Mission::start(WeaponManager& weaponMgr) {
    // TODO: consider weight of weapons when adding?
    // TODO: check whether enemy agents weapons are equal to best two

    LOG(Log::k_FLG_GAME, "Mission", "start()", ("Start mission"));
    // Reset mission statistics
    stats_.init(squad_->size());

    cur_objective_ = 0;
    escapedPeds_.clear();

    // Get available weapons
    std::vector<Weapon*> availableWeapons;
    weaponMgr.getAvailable(kDmgTypeBullet, availableWeapons);

    // see if bomb is available
    Weapon* bomb = weaponMgr.getAvailable(Weapon::TimeBomb);

    // Assign best weapons to enemy agents
    assignWeaponsToEnemyAgents(availableWeapons, bomb);
}


/*!
 * Run animate method for each managed object.
 * @param elapsed 
 * @param diff 
 */
void Mission::windDeltaPerSubstep(double &dx, double &dy) const {
    // Matches original DOS Syndicate wind table (sin/cos on 256-step compass)
    double rad = static_cast<double>(windAngle_) * (2.0 * rs_utl::kPI / 256.0);
    dx = static_cast<double>(windSpeed_) * std::sin(rad);
    dy = static_cast<double>(windSpeed_) * std::cos(rad);
}

void Mission::handleTick(uint32_t elapsed, uint32_t diff) {
    ZoneScoped;

    // Update ambient wind every 50ms DOS tick: angle += (rand() % 9) - 2
    windTickAccumMs_ += diff;
    while (windTickAccumMs_ >= 50) {
        windTickAccumMs_ -= 50;
        windAngle_ = static_cast<uint8_t>((static_cast<int>(windAngle_) + (std::rand() % 9) - 2) & 0xFF);
    }

    buildDynamicSpatialGrid();

    double windX = 0.0, windY = 0.0;
    windDeltaPerSubstep(windX, windY);
    double smokeWindScale = static_cast<double>(diff) / 50.0;

    for (auto it = sfx_objects_.begin(); it != sfx_objects_.end(); ) {
        auto& sfx = *it;

        if (diff > 0 && sfx->type() == SFXObject::sfxt_ExplosionBall) {
            WorldPoint p = sfx->position();
            p.x += static_cast<int>(std::lround(windX * smokeWindScale));
            p.y += static_cast<int>(std::lround(windY * smokeWindScale));
            sfx->setPosition(p);
        }

        sfx->animate(diff);

        if (sfx->sfxLifeOver()) {
            it = sfx_objects_.erase(it);
        } else {
            ++it;
        }
    }

    for (rs_knl::PedInstance *pPed : peds_) {
        pPed->animate(diff);
    }

    for (rs_knl::Vehicle *pVehicle : vehicles_) {
        pVehicle->animate(diff);
    }

    for (rs_knl::WeaponInstance *pWeapon : weaponsOnGround_) {
        pWeapon->animate(diff);
    }

    for (auto & pStatics : statics_) {
        pStatics->animate(diff);
    }

    for (auto it = projectileShots_.begin(); it != projectileShots_.end(); ) {
        auto& projectileShot = *it;

        projectileShot->animate(diff, this);

        if (projectileShot->isLifeOver()) {
            // TODO Delete object 
            it = projectileShots_.erase(it);
        } else {
            ++it;
        }
    }
}

/*!
 * Checks if objectives are completed or failed and updates
 * mission status.
 */
void Mission::checkObjectives() {
    // We only check the current objective
    if (cur_objective_ < objectives_.size()) {
        ObjectiveDesc * pObj = objectives_[cur_objective_];

        // If it's the first time the objective is checked,
        // declares it started
        if (pObj->status == kNotStarted) {
            LOG(Log::k_FLG_GAME, "Mission", "checkObjectives()", ("Start objective : %d", cur_objective_));
            // An objective has just started, warn all listeners
            pObj->start();
        }

        // Checks if the objective is completed
        pObj->evaluate(this);

        if (pObj->isTerminated()) {
            if (pObj->status == kFailed) {
                endWithStatus(kMissionStatusFailed);
            } else {
                // Objective is completed -> go to next one
                cur_objective_++;
                if (cur_objective_ >= objectives_.size()) {
                    // the last objective has been completed : mission succeeded
                    endWithStatus(kMissionStatusCompleted);
                }
            }
        }
    }
}


/*!
 * 
 * @param pPed 
 * @return 
 */
bool Mission::hasEscaped(rs_knl::PedInstance *pPed) {
    auto it = find(escapedPeds_.begin(), escapedPeds_.end(), pPed);
    return it != escapedPeds_.end();
}

/*!
 * @brief 
 * @param pPed 
 */
void Mission::escape(rs_knl::PedInstance *pPed) {
    escapedPeds_.push_back(pPed);
}

/*!
 * Ends the mission with the given status.
 * \param status The ending status
 */
void Mission::endWithStatus(Status status) {
    status_ = status;
    EventManager::fire<MissionEndedEvent>(status);

    updateStats();
}


/** \brief
 *
 * \return void
 *
 */
void Mission::updateStats() {
    LOG(Log::k_FLG_GAME, "Mission", "updateStats()", ("calculate statistics for mission"));
    for (size_t i = squad_->size(); i < peds_.size(); i++) {
        PedInstance *p = peds_[i];
        // TODO: influence country happiness with number of killed overall
        // civilians+police, more killed less happy
        // TODO: add money per every persuaded non-agent ped
        if (p->isDead()) {
            switch (p->type()) {
                case PedInstance::kPedTypeAgent:
                    stats_.incrEnemyKilled();
                    break;
                case PedInstance::kPedTypeCriminal:
                    stats_.incrCriminalKilled();
                    break;
                case PedInstance::kPedTypeCivilian:
                    stats_.incrCivilKilled();
                    break;
                case PedInstance::kPedTypeGuard:
                    stats_.incrGuardKilled();
                    break;
                case PedInstance::kPedTypePolice:
                    stats_.incrPoliceKilled();
                    break;
            }
        } else if (p->isPersuaded()) {
            if (p->objGroupDef() == PedInstance::og_dmAgent) {
                stats_.incrAgentCaptured();
            } else {
                stats_.incrConvinced();
            }
        }
    }
}

/*!
 * Add the given weapon to the list of weapons on the ground.
 * Start the animation of the weapon
 * @param pWeapon Weapon to drop on ground
 */
void Mission::addWeaponToGround(WeaponInstance * pWeapon)
{
    for (unsigned int i = 0; i < weaponsOnGround_.size(); i++) {
        // TODO : check if == operator is used correctly (see  == in WeaponInstance)
        if (weaponsOnGround_[i] == pWeapon)
            return;
    }
    weaponsOnGround_.push_back(pWeapon);
    pWeapon->playOnGroundAnimation();
}

/*!
 * Remove the weapon from the list of weapons on the ground.
 * @param pWeapon 
 */
void Mission::removeWeaponOnGround(WeaponInstance *pWeapon) {
    for (unsigned int i = 0; i < weaponsOnGround_.size(); i++) {
        if (weaponsOnGround_[i] == pWeapon) {
            weaponsOnGround_.erase(weaponsOnGround_.begin() + i);
        }
    }
    pWeapon->resetAnimation();
}

/*!
 * This function iterates over the objects of the requested type (pedestrians or vehicles) starting
 * from the provided search index and looks for an object located at the specified tile coordinates.
 * If a matching object is found, it is returned and the search index is updated
 * so that subsequent calls can continue searching from the next object.
 *
 * @param tilex X coordinate of the tile.
 * @param tiley Y coordinate of the tile.
 * @param tilez Z level (altitude) of the tile.
 * @param nature The nature/type of the object to search for (pedestrian or vehicle).
 * @param searchIndex [in,out] Pointer to the current search index; updated if a matching object is found.
 * @return Pointer to the found object, or nullptr if no matching object is found.
 *
 * @note
 * - Dead pedestrians are included in the search to prevent logic issues (e.g., doors closing over a corpse).
 * - If an undefined nature is provided, an error is logged and the function returns nullptr.
 *
 * @see MapObject::ObjectNature
 */
MapObject * Mission::findObjectWithNatureAtPos(int tilex, int tiley, int tilez,
                            MapObject::ObjectNature nature, size_t *searchIndex) {

    const TilePoint position{tilex, tiley, tilez};

    // lambda to search for an object in a generic container that has same position
    auto findIn = [&](auto& container) -> MapObject* {
        for (size_t i = *searchIndex; i < container.size(); ++i) {
            if (container[i]->sameTile(position)) {
                *searchIndex = i + 1;
                return container[i];
            }
        }
        return nullptr;
    };

    switch (nature) {
        case MapObject::kNaturePed:
            return findIn(peds_);
        case MapObject::kNatureVehicle:
            return findIn(vehicles_);
        default:
            RSERR(Log::k_FLG_GAME, "Mission", "findObjectWithNatureAtPos", 
                    ("Undefined nature %i\n", static_cast<int>(nature)));
            return nullptr;
    }
}

/*!
 * Constructs the navigation data for the entire 3-D tile map in three phases:
 *
 * **Phase 1 – Surface initialisation** (`initSurface`)\n
 * Allocates the `mdpoints_` and `mtsurfaces_` arrays and copies the raw
 * walkdata from the map into `mtsurfaces_`, then patches tiles that overlap
 * large doors so they are treated as passable.
 *
 * **Phase 2 – Flood fill** (`floodFillFromSeed` / `classifyTile`)\n
 * For each live pedestrian whose starting tile has not yet been classified, a
 * flood fill is seeded from that tile.  For every visited tile the fill records:
 * - whether the tile is walkable (`kWalkable`) or not (`kNonWalkable`),
 *   and whether it is safe ground (not a road or railway) via `kSafeWalk`;
 * - reachable horizontal neighbours at the same level (`FloodNode::dirsSame`),
 *   one level up (`dirsAbove`), and one level down (`dirsBelow`), each as an 8-direction
 *   bitmask.
 *
 * Tiles whose surface type redirects traversal (type `0x00`/`0x10` fall back to
 * the tile below; types `0x11`/`0x12` are treated as the tile above) are
 * resolved before classification.  Tiles never reachable from any pedestrian
 * start position remain `kNone`.
 *
 * **Phase 3 – Static spatial grid**\n
 * Populates `staticSpatialGrid_` with all static map objects for
 * grid-accelerated collision and line-of-sight queries.  Done only once because
 * statics never move.
 *
 * @return true (always; kept for historical compatibility).
 */
bool Mission::buildNavigationGraph() {
    LOG(Log::k_FLG_GAME, "Mission", "buildNavigationGraph", ("Starting build of navigation graph"));

    clrNavigationGraph();
    int gridSize = mmax_x_ * mmax_y_ * mmax_z_;

    mtsurfaces_ = new SurfaceType[gridSize];
    mdpoints_ = new FloodNode[gridSize];
    mdpoints_cp_ = new FloodNode[gridSize];
    
    buildSurfaces();

    for (PedInstance *pPed : peds_) {
        if (!pPed) continue;
        if (pPed->isDead()) continue;
        if (pPed->tileZ() >= mmax_z_ || pPed->tileZ() < 0) {
            // TODO : check on all maps those peds correct position
            LOG(Log::k_FLG_GAME, "Mission", "buildNavigationGraph", ("!! Ped %d has tz (%d) superior to maxtz %d", pPed->id(), pPed->tileZ(), mmax_z_));
            pPed->setTileZ(mmax_z_ - 1);
            continue;
        }
        if (pPed->tileX() < 0 || pPed->tileX() >= mmax_x_ || pPed->tileY() < 0 || pPed->tileY() >= mmax_y_) {
            continue;
        }

        int pIdx = getTileIndex(pPed->position());
        if (pIdx >= 0 && pIdx < gridSize && mdpoints_[pIdx].flags == kNone) {
            floodFillFromSeed(pPed->position());
        }
    }

    // Build static spatial grid (statics never move so this is done only once).
    staticSpatialGrid_.assign(gridSize, {});
    for (const auto& s : statics_) {
        if (s) {
            insertIntoSpatialGrid(staticSpatialGrid_, s.get());
            if (s->type() == Static::smt_LargeDoor) {
                static_cast<LargeDoor *>(s.get())->initNavigationBlocker(this);
            }
        }
    }

    LOG(Log::k_FLG_GAME, "Mission", "buildNavigationGraph", ("End of build of navigation graph"));

    return true;
}

void Mission::clrNavigationGraph() {

    for (const auto& s : statics_) {
        if (s && s->type() == Static::smt_LargeDoor) {
            static_cast<LargeDoor *>(s.get())->initNavigationBlocker(nullptr);
        }
    }

    if(mtsurfaces_ != NULL) {
        delete[] mtsurfaces_;
        mtsurfaces_ = NULL;
    }
    if(mdpoints_ != NULL) {
        delete[] mdpoints_;
        mdpoints_ = NULL;
    }
    if(mdpoints_cp_ != NULL) {
        delete[] mdpoints_cp_;
        mdpoints_cp_ = NULL;
    }
    staticSpatialGrid_.clear();
    dynamicSpatialGrid_.clear();
}

void Mission::buildSurfaces() {
    int mmax_m_all = mmax_x_ * mmax_y_ * mmax_z_;
    
    for (int ix = 0; ix < mmax_x_; ++ix) {
        for (int iy = 0; iy < mmax_y_; ++iy) {
            for (int iz = 0; iz < mmax_z_; ++iz) {
                int sIdx = ix + iy * mmax_x_ + iz * mmax_m_xy;
                if (sIdx >= 0 && sIdx < mmax_m_all) {
                    mtsurfaces_[sIdx] =
                        p_map_->getSurfaceTypeForTile(ix, iy, iz);
                }
            }
        }
    }

    // to make surfaces where large doors are located walkable
    for (const auto & s : statics_) {
        if (s && s->type() == Static::smt_LargeDoor) {
            if (s->tileX() < 0 || s->tileX() >= mmax_x_ || s->tileY() < 0 || s->tileY() >= mmax_y_ || s->tileZ() < 0 || s->tileZ() >= mmax_z_) {
                continue;
            }
            int indx = getTileIndex(s->position());
            if (indx >= 0 && indx < mmax_m_all) {
                mtsurfaces_[indx] = SurfaceType::kEmpty;
                if (s->orientation() == Static::kStaticOrientationNS) {
                    if (indx - 1 >= 0)
                        mtsurfaces_[indx - 1] = SurfaceType::kEmpty;
                    if (indx + 1 < mmax_m_all)
                        mtsurfaces_[indx + 1] = SurfaceType::kEmpty;
                } else if (s->orientation() == Static::kStaticOrientationEW) {
                    if (indx - mmax_x_ >= 0)
                        mtsurfaces_[indx - mmax_x_] = SurfaceType::kEmpty;
                    if (indx + mmax_x_ < mmax_m_all)
                        mtsurfaces_[indx + mmax_x_] = SurfaceType::kEmpty;
                }
            }
        }
    }
}

/*!
 * Called if the tile at seedPt is not already classified.
 * Then it seeds the flood fill queue, iterates over reachable tiles,
 * resolves surface redirections (0x00/0x10 → lower level, 0x11/0x12 →
 * upper level), and calls classifyTile() for each resolved node.
 * @param seedPt Starting point for the flooding
 * @note Surface type `0x0D` is not handled correctly (see in-source TODO).
 */
void Mission::floodFillFromSeed(const TilePoint &seedPt) {
    int mmax_m_all = mmax_x_ * mmax_y_ * mmax_z_;
    if (seedPt.tx < 0 || seedPt.tx >= mmax_x_ || seedPt.ty < 0 || seedPt.ty >= mmax_y_ || seedPt.tz < 0 || seedPt.tz >= mmax_z_) {
        return;
    }
    int seedIdx = getTileIndex(seedPt);
    if (seedIdx < 0 || seedIdx >= mmax_m_all) return;
    std::vector<WorldPoint> vtodefine;
    mdpoints_[seedIdx].flags = kPending;
    WorldPoint seedWPt;
    seedWPt.x = seedPt.tx;
    seedWPt.y = seedPt.ty;
    seedWPt.z = seedPt.tz;
    vtodefine.push_back(seedWPt);

    // TODO: tiles walkdata type 0x0D are quiet special, and they
    // are not handled correctly, these correction and adjustings
    // can create additional speed drain, as such I didn't
    // implemented them as needed. To make it possible a patch
    // required to walkdata and a lot of changes which I don't
    // want to do.
    // 0x10 appear above walking tile where train stops

    do {
        WorldPoint stodef = vtodefine.back();
        vtodefine.pop_back();
        if (stodef.x < 0 || stodef.x >= mmax_x_ || stodef.y < 0 || stodef.y >= mmax_y_ || stodef.z < 0 || stodef.z >= mmax_z_) {
            continue;
        }
        int x = stodef.x;
        int y = stodef.y * mmax_x_;
        int z = stodef.z * mmax_m_xy;
        int curIdx = x + y + z;
        if (curIdx < 0 || curIdx >= mmax_m_all) continue;
        SurfaceType this_s = mtsurfaces_[curIdx];
        SurfaceType upper_s = SurfaceType::kEmpty;
        FloodNode *pNode = &(mdpoints_[curIdx]);
        int zm = z - mmax_m_xy;
        // if current is an Empty tile we will use lower tile
        // to define it
        if (this_s == SurfaceType::kEmpty) {
            if (zm < 0) {
                pNode->flags = kNonWalkable;
                continue;
            }
            z = zm;
            zm -= mmax_m_xy;
            upper_s = this_s;
            curIdx = x + y + z;
            if (curIdx < 0 || curIdx >= mmax_m_all) {
                pNode->flags = kNonWalkable;
                continue;
            }
            this_s = mtsurfaces_[curIdx];
            if (!SurfaceUtils::isTraversable(this_s, upper_s))
                continue;
        } else if (this_s == SurfaceType::kTrainPlatformNS || this_s == SurfaceType::kTrainPlatformEW) {
            int zp_tmp = z + mmax_m_xy;
            if (zp_tmp < mmax_m_all && (x + y + zp_tmp) < mmax_m_all) {
                // we are defining tile above current
                pNode = &(mdpoints_[x + y + zp_tmp]);
            } else
                pNode->flags = kNonWalkable;
        }
        int xm = x - 1;
        int ym = y - mmax_x_;
        int xp = x + 1;
        int yp = y + mmax_x_;
        int zp = z + mmax_m_xy;
        if (zp < mmax_m_all && (x + y + zp) < mmax_m_all) {
            upper_s = mtsurfaces_[x + y + zp];
            if(!SurfaceUtils::isTraversable(this_s, upper_s)) {
                pNode->flags = kNonWalkable;
                continue;
            }
        } else {
            pNode->flags = kNonWalkable;
            continue;
        }
        classifyTile(x, y, z,
                     xm, xp, ym, yp, zm, zp,
                     this_s, mmax_m_all, pNode, vtodefine);
    } while (vtodefine.size());
}

void Mission::enqueueIfUndefined(FloodNode *fp, int x, int y, int z,
                                  std::vector<WorldPoint>& queue) {
    if (!fp) return;
    int mmax_m_all = mmax_x_ * mmax_y_ * mmax_z_;
    if (x < 0 || x >= mmax_x_ || y < 0 || y >= mmax_m_xy || z < 0 || z >= mmax_m_all) return;
    int idx = x + y + z;
    if (idx < 0 || idx >= mmax_m_all) return;
    if (fp->flags == kNone) {
        fp->flags = kPending;
        WorldPoint pt;
        pt.x = x;
        pt.y = y / mmax_x_;
        pt.z = z / mmax_m_xy;
        queue.push_back(pt);
    }
}

/**
 * Sets cfp->flags and the direction bitmasks (dirsSame/dirsAbove/dirsBelow) for
 * the tile at (x, y, z) and enqueues any reachable neighbours.
 * All coordinate parameters use the stride-multiplied convention:
 * y = tyRaw*mmax_x_, z = tzRaw*mmax_m_xy.
 * @param x     stride-multiplied x (raw tile index)
 * @param y     stride-multiplied y
 * @param z     stride-multiplied z
 * @param xm    x - 1
 * @param xp    x + 1
 * @param ym    y - mmax_x_
 * @param yp    y + mmax_x_
 * @param zm    z - mmax_m_xy
 * @param zp    z + mmax_m_xy
 * @param this_s surface type of the current tile (already resolved)
 * @param mmax_m_all total number of tiles (mmax_x_ * mmax_y_ * mmax_z_)
 * @param cfp   flood-point descriptor to fill in (output)
 * @param vtodefine flood-fill queue (modified in place)
 */
void Mission::classifyTile(int x, int y, int z,
                           int xm, int xp, int ym, int yp, int zm, int zp,
                           SurfaceType this_s, int mmax_m_all,
                           FloodNode* cfp, std::vector<WorldPoint>& vtodefine) {
    SurfaceType upper_s = SurfaceType::kEmpty;
    FloodNode* nxtfp = nullptr;
    unsigned char sdirm = 0x00;
    unsigned char sdirh = 0x00;
    unsigned char sdirl = 0x00;
    unsigned char sdirmr = 0x00;

    switch (this_s) {
        case SurfaceType::kSlopeSN:
            cfp->flags = kWalkable;
            cfp->flags |= kSafeWalk;
            if (zm >= 0) {
                mdpoints_[x + y + zm].flags = kNonWalkable;
                if (yp < mmax_m_xy) {
                    this_s = mtsurfaces_[x + yp + zm];
                    upper_s = mtsurfaces_[x + yp + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        sdirm |= 0x01;
                        nxtfp = &(mdpoints_[x + yp + z]);
                        enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                    } else if (this_s == SurfaceType::kSlopeSN) {
                        nxtfp = &(mdpoints_[x + yp + zm]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x01;
                            nxtfp = &(mdpoints_[x + yp + zm]);
                            enqueueIfUndefined(nxtfp, x, yp, zm, vtodefine);
                        } else
                            nxtfp->flags = kNonWalkable;
                    }
                }
                if (xm >= 0) {
                    this_s = mtsurfaces_[xm + y + zm];
                    upper_s = mtsurfaces_[xm + y + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[xm + y + z]);
                        sdirm |= 0x40;
                        enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                    } else if (SurfaceUtils::isStairs(this_s)) {
                        nxtfp = &(mdpoints_[xm + y + zm]);
                        enqueueIfUndefined(nxtfp, xm, y, zm, vtodefine);
                    }
                }
                if (xp < mmax_x_) {
                    this_s = mtsurfaces_[xp + y + zm];
                    upper_s = mtsurfaces_[xp + y + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[xp + y + z]);
                        sdirm |= 0x04;
                        enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                    } else if (SurfaceUtils::isStairs(this_s)) {
                        nxtfp = &(mdpoints_[xp + y + zm]);
                        enqueueIfUndefined(nxtfp, xp, y, zm, vtodefine);
                    }
                }
            }

            if (ym >= 0) {
                nxtfp = &(mdpoints_[x + ym + zp]);
                this_s = mtsurfaces_[x + ym + z];
                upper_s = mtsurfaces_[x + ym + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    sdirh |= 0x10;
                    enqueueIfUndefined(nxtfp, x, ym, zp, vtodefine);
                } else if(upper_s == SurfaceType::kSlopeSN && (zp + mmax_m_xy) < mmax_m_all) {
                    if(SurfaceUtils::isTraversable(upper_s, mtsurfaces_[
                        x + ym + (zp + mmax_m_xy)]))
                    {
                        sdirh |= 0x10;
                        enqueueIfUndefined(nxtfp, x, ym, zp, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }

            if (xm >= 0) {
                this_s = mtsurfaces_[xm + y + z];
                upper_s = mtsurfaces_[xm + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    nxtfp = &(mdpoints_[xm + y + zp]);
                    sdirh |= 0x40;
                    enqueueIfUndefined(nxtfp, xm, y, zp, vtodefine);
                } else if (this_s == SurfaceType::kSlopeSN) {
                    nxtfp = &(mdpoints_[xm + y + z]);
                    if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                        sdirm |= 0x40;
                        enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }

            if (xp < mmax_x_) {
                this_s = mtsurfaces_[xp + y + z];
                upper_s = mtsurfaces_[xp + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    nxtfp = &(mdpoints_[xp + y + zp]);
                    sdirh |= 0x04;
                    enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                } else if (this_s == SurfaceType::kSlopeSN) {
                    nxtfp = &(mdpoints_[xp + y + z]);
                    if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                        sdirm |= 0x04;
                        enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }
            cfp->dirsSame = sdirm;
            cfp->dirsAbove = sdirh;
            cfp->dirsBelow = sdirl;

            break;
        case SurfaceType::kSlopeNS:
            cfp->flags = kWalkable;
            cfp->flags |= kSafeWalk;
            if (zm >= 0) {
                mdpoints_[x + y + zm].flags = kNonWalkable;
                if (ym >= 0) {
                    this_s = mtsurfaces_[x + ym + zm];
                    upper_s = mtsurfaces_[x + ym + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[x + ym + z]);
                        sdirm |= 0x10;
                        enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                    } else if (this_s == SurfaceType::kSlopeNS) {
                        nxtfp = &(mdpoints_[x + ym + zm]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x10;
                            enqueueIfUndefined(nxtfp, x, ym, zm, vtodefine);
                        } else
                            nxtfp->flags = kNonWalkable;
                    }
                }
                if (xm >= 0) {
                    this_s = mtsurfaces_[xm + y + zm];
                    upper_s = mtsurfaces_[xm + y + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[xm + y + z]);
                        sdirm |= 0x40;
                        enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                    } else if (SurfaceUtils::isStairs(this_s)) {
                        nxtfp = &(mdpoints_[xm + y + zm]);
                        enqueueIfUndefined(nxtfp, xm, y, zm, vtodefine);
                    }
                }
                if (xp < mmax_x_) {
                    this_s = mtsurfaces_[xp + y + zm];
                    upper_s = mtsurfaces_[xp + y + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[xp + y + z]);
                        sdirm |= 0x04;
                        enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                    } else if (SurfaceUtils::isStairs(this_s)) {
                        nxtfp = &(mdpoints_[xp + y + zm]);
                        enqueueIfUndefined(nxtfp, xp, y, zm, vtodefine);
                    }
                }
            }

            if (yp < mmax_m_xy) {
                nxtfp = &(mdpoints_[x + yp + zp]);
                this_s = mtsurfaces_[x + yp + z];
                upper_s = mtsurfaces_[x + yp + zp];
                if(SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    sdirh |= 0x01;
                    enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                } else if(upper_s == SurfaceType::kSlopeNS && (zp + mmax_m_xy) < mmax_m_all) {
                    if(SurfaceUtils::isTraversable(upper_s,  mtsurfaces_[
                        x + yp + (zp + mmax_m_xy)]))
                    {
                        sdirh |= 0x01;
                        enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }

            if (xm >= 0) {
                this_s = mtsurfaces_[xm + y + z];
                upper_s = mtsurfaces_[xm + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    nxtfp = &(mdpoints_[xm + y + zp]);
                    sdirh |= 0x40;
                    enqueueIfUndefined(nxtfp, xm, y, zp, vtodefine);
                } else if (this_s == SurfaceType::kSlopeNS) {
                    nxtfp = &(mdpoints_[xm + y + z]);
                    if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                        sdirm |= 0x40;
                        enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }

            if (xp < mmax_x_) {
                this_s = mtsurfaces_[xp + y + z];
                upper_s = mtsurfaces_[xp + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    nxtfp = &(mdpoints_[xp + y + zp]);
                    sdirh |= 0x04;
                    enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                } else if (this_s == SurfaceType::kSlopeNS) {
                    nxtfp = &(mdpoints_[xp + y + z]);
                    if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                        sdirm |= 0x04;
                        enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }
            cfp->dirsSame = sdirm;
            cfp->dirsAbove = sdirh;
            cfp->dirsBelow = sdirl;

            break;
        case SurfaceType::kSlopeEW:
            cfp->flags = kWalkable;
            cfp->flags |= kSafeWalk;
            if (zm >= 0) {
                mdpoints_[x + y + zm].flags = kNonWalkable;
                if (xm >= 0) {
                    this_s = mtsurfaces_[xm + y + zm];
                    upper_s = mtsurfaces_[xm + y + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[xm + y + z]);
                        sdirm |= 0x40;
                        enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                    } else if (this_s == SurfaceType::kSlopeEW) {
                        nxtfp = &(mdpoints_[xm + y + zm]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x40;
                            enqueueIfUndefined(nxtfp, xm, y, zm, vtodefine);
                        } else
                            nxtfp->flags = kNonWalkable;
                    }
                }
                if (ym >= 0) {
                    this_s = mtsurfaces_[x + ym + zm];
                    upper_s = mtsurfaces_[x + ym + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[x + ym + z]);
                        sdirm |= 0x10;
                        enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                    } else if (SurfaceUtils::isStairs(this_s)) {
                        nxtfp = &(mdpoints_[x + ym + zm]);
                        enqueueIfUndefined(nxtfp, x, ym, zm, vtodefine);
                    }
                }
                if (yp < mmax_m_xy) {
                    this_s = mtsurfaces_[x + yp + zm];
                    upper_s = mtsurfaces_[x + yp + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[x + yp + z]);
                        sdirm |= 0x01;
                        enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                    } else if (SurfaceUtils::isStairs(this_s)) {
                        nxtfp = &(mdpoints_[x + yp + zm]);
                        enqueueIfUndefined(nxtfp, x, yp, zm, vtodefine);
                    }
                }
            }

            if (xp < mmax_x_) {
                nxtfp = &(mdpoints_[xp + y + zp]);
                this_s = mtsurfaces_[xp + y + z];
                upper_s = mtsurfaces_[xp + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    sdirh |= 0x04;
                    enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                } else if(upper_s == SurfaceType::kSlopeEW && (zp + mmax_m_xy) < mmax_m_all) {
                    if(SurfaceUtils::isTraversable(upper_s,
                        mtsurfaces_[xp + y + (zp + mmax_m_xy)]))
                    {
                        sdirh |= 0x04;
                        enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }

            if (ym >= 0) {
                this_s = mtsurfaces_[x + ym + z];
                upper_s = mtsurfaces_[x + ym + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    nxtfp = &(mdpoints_[x + ym + zp]);
                    sdirh |= 0x10;
                    enqueueIfUndefined(nxtfp, x, ym, zp, vtodefine);
                } else if (this_s == SurfaceType::kSlopeEW) {
                    nxtfp = &(mdpoints_[x + ym + z]);
                    if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                        sdirm |= 0x10;
                        enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }

            if (yp < mmax_m_xy) {
                this_s = mtsurfaces_[x + yp + z];
                upper_s = mtsurfaces_[x + yp + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    nxtfp = &(mdpoints_[x + yp + zp]);
                    sdirh |= 0x01;
                    enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                } else if (this_s == SurfaceType::kSlopeEW) {
                    nxtfp = &(mdpoints_[x + yp + z]);
                    if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                        sdirm |= 0x01;
                        enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }
            cfp->dirsSame = sdirm;
            cfp->dirsAbove = sdirh;
            cfp->dirsBelow = sdirl;

            break;
        case SurfaceType::kSlopeWE:
            cfp->flags = kWalkable;
            cfp->flags |= kSafeWalk;
            if (zm >= 0) {
                mdpoints_[x + y + zm].flags = kNonWalkable;
                if (xp < mmax_x_) {
                    this_s = mtsurfaces_[xp + y + zm];
                    upper_s = mtsurfaces_[xp + y + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[xp + y + z]);
                        sdirm |= 0x04;
                        enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                    } else if (this_s == SurfaceType::kSlopeWE) {
                        nxtfp = &(mdpoints_[xp + y + zm]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x04;
                            enqueueIfUndefined(nxtfp, xp, y, zm, vtodefine);
                        } else
                            nxtfp->flags = kNonWalkable;
                    }
                }
                if (ym >= 0) {
                    this_s = mtsurfaces_[x + ym + zm];
                    upper_s = mtsurfaces_[x + ym + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[x + ym + z]);
                        sdirm |= 0x10;
                        enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                    } else if (SurfaceUtils::isStairs(this_s)) {
                        nxtfp = &(mdpoints_[x + ym + zm]);
                        enqueueIfUndefined(nxtfp, x, ym, zm, vtodefine);
                    }
                }
                if (yp < mmax_m_xy) {
                    this_s = mtsurfaces_[x + yp + zm];
                    upper_s = mtsurfaces_[x + yp + z];
                    if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                        nxtfp = &(mdpoints_[x + yp + z]);
                        sdirm |= 0x01;
                        enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                    } else if (SurfaceUtils::isStairs(this_s)) {
                        nxtfp = &(mdpoints_[x + yp + zm]);
                        enqueueIfUndefined(nxtfp, x, yp, zm, vtodefine);
                    }
                }
            }

            if (xm >= 0) {
                nxtfp = &(mdpoints_[xm + y + zp]);
                this_s = mtsurfaces_[xm + y + z];
                upper_s = mtsurfaces_[xm + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    sdirh |= 0x40;
                    enqueueIfUndefined(nxtfp, xm, y, zp, vtodefine);
                } else if(upper_s == SurfaceType::kSlopeWE && (zp + mmax_m_xy) < mmax_m_all) {
                    if(SurfaceUtils::isTraversable(upper_s, mtsurfaces_[
                        xm + y + (zp + mmax_m_xy)]))
                    {
                        sdirh |= 0x40;
                        enqueueIfUndefined(nxtfp, xm, y, zp, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }

            if (ym >= 0) {
                this_s = mtsurfaces_[x + ym + z];
                upper_s = mtsurfaces_[x + ym + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    nxtfp = &(mdpoints_[x + ym + zp]);
                    sdirh |= 0x10;
                    enqueueIfUndefined(nxtfp, x, ym, zp, vtodefine);
                } else if (this_s == SurfaceType::kSlopeWE) {
                    nxtfp = &(mdpoints_[x + ym + z]);
                    if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                        sdirm |= 0x10;
                        enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }

            if (yp < mmax_m_xy) {
                this_s = mtsurfaces_[x + yp + z];
                upper_s = mtsurfaces_[x + yp + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s)) {
                    nxtfp = &(mdpoints_[x + yp + zp]);
                    sdirh |= 0x01;
                    enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                } else if (this_s == SurfaceType::kSlopeWE) {
                    nxtfp = &(mdpoints_[x + yp + z]);
                    if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                        sdirm |= 0x01;
                        enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                    } else
                        nxtfp->flags = kNonWalkable;
                }
            }
            cfp->dirsSame = sdirm;
            cfp->dirsAbove = sdirh;
            cfp->dirsBelow = sdirl;

            break;
        case SurfaceType::kFloor:
        case SurfaceType::kRoad:
            cfp->flags = kWalkable;
            if (this_s != SurfaceType::kRoad) {
                cfp->flags |= kSafeWalk;
            }
            if (xm >= 0) {
                this_s = mtsurfaces_[xm + y + z];
                upper_s = mtsurfaces_[xm + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x20 | 0x40 | 0x80);
                    nxtfp = &(mdpoints_[xm + y + zp]);
                    enqueueIfUndefined(nxtfp, xm, y, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x20 | 0x80);
                    if (this_s == SurfaceType::kSlopeSN || this_s == SurfaceType::kSlopeNS
                        || this_s == SurfaceType::kSlopeEW)
                    {
                        sdirl |= 0x40;
                    }
                    nxtfp = &(mdpoints_[xm + y + z]);
                    enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                } else {
                    sdirmr |= (0x20 | 0x80);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeSN || upper_s == SurfaceType::kSlopeNS || upper_s == SurfaceType::kSlopeWE
                        || upper_s == SurfaceType::kTrainPlatformEW)) {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[xm + y + (zp + mmax_m_xy)]))
                        {
                            if (upper_s == SurfaceType::kTrainPlatformEW)
                                sdirh |= 0x40;
                            else
                                sdirm |= 0x40;
                            nxtfp = &(mdpoints_[xm + y + zp]);
                            enqueueIfUndefined(nxtfp, xm, y, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x20 | 0x80);

            if (xp < mmax_x_) {
                this_s = mtsurfaces_[xp + y + z];
                upper_s = mtsurfaces_[xp + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x02 | 0x04 | 0x08);
                    nxtfp = &(mdpoints_[xp + y + zp]);
                    enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x02 | 0x08);
                    if (this_s == SurfaceType::kSlopeSN || this_s == SurfaceType::kSlopeNS
                        || this_s == SurfaceType::kSlopeWE)
                    {
                        sdirl |= 0x04;
                    }
                    nxtfp = &(mdpoints_[xp + y + z]);
                    enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                } else {
                    sdirmr |= (0x02 | 0x08);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeSN || upper_s == SurfaceType::kSlopeNS
                        || upper_s == SurfaceType::kSlopeEW || upper_s == SurfaceType::kTrainPlatformNS))
                    {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[xp + y + (zp + mmax_m_xy)]))
                        {
                            if (upper_s == SurfaceType::kTrainPlatformNS)
                                sdirh |= 0x04;
                            else
                                sdirm |= 0x04;
                            nxtfp = &(mdpoints_[xp + y + zp]);
                            enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x02 | 0x08);

            if(ym >= 0) {
                this_s = mtsurfaces_[x + ym + z];
                upper_s = mtsurfaces_[x + ym + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x08 | 0x10 | 0x20);
                    nxtfp = &(mdpoints_[x + ym + zp]);
                    enqueueIfUndefined(nxtfp, x, ym, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x08 | 0x20);
                    if (this_s == SurfaceType::kSlopeNS || this_s == SurfaceType::kSlopeEW || this_s == SurfaceType::kSlopeWE){
                        sdirl |= 0x10;
                    }
                    nxtfp = &(mdpoints_[x + ym + z]);
                    enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                } else {
                    sdirmr |= (0x08 | 0x20);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeSN || upper_s == SurfaceType::kSlopeEW
                        || upper_s == SurfaceType::kSlopeWE || upper_s == SurfaceType::kTrainPlatformNS))
                    {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[x + ym + (zp + mmax_m_xy)]))
                        {
                            if (upper_s == SurfaceType::kTrainPlatformNS)
                                sdirh |= 0x10;
                            else
                                sdirm |= 0x10;
                            nxtfp = &(mdpoints_[x + ym + zp]);
                            enqueueIfUndefined(nxtfp, x, ym, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x08 | 0x20);

            if (yp < mmax_m_xy) {
                this_s = mtsurfaces_[x + yp + z];
                upper_s = mtsurfaces_[x + yp + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x80 | 0x01 | 0x02);
                    nxtfp = &(mdpoints_[x + yp + zp]);
                    enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x80 | 0x02);
                    if (this_s == SurfaceType::kSlopeSN || this_s == SurfaceType::kSlopeEW
                        || this_s == SurfaceType::kSlopeWE)
                    {
                        sdirl |= 0x01;
                    }
                    nxtfp = &(mdpoints_[x + yp + z]);
                    enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                } else {
                    sdirmr |= (0x80 | 0x02);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeNS || upper_s == SurfaceType::kSlopeEW
                        || upper_s == SurfaceType::kSlopeWE || upper_s == SurfaceType::kTrainPlatformEW))
                    {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[x + yp + (zp + mmax_m_xy)]))
                        {
                            if (upper_s == SurfaceType::kTrainPlatformEW)
                                sdirh |= 0x01;
                            else
                                sdirm |= 0x01;
                            nxtfp = &(mdpoints_[x + yp + zp]);
                            enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x80 | 0x02);
            sdirm &= (0xFF ^ sdirmr);

            // edges

            if (xm >= 0) {
                if (ym >= 0 && (sdirm & 0x20) != 0) {
                    nxtfp = &(mdpoints_[xm + ym + zp]);
                    this_s = mtsurfaces_[xm + ym + z];
                    upper_s = mtsurfaces_[xm + ym + zp];
                    if (!(SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s,
                        upper_s)))
                    {
                        sdirm &= (0xFF ^ 0x20);
                    } else {
                        enqueueIfUndefined(nxtfp, xm, ym, zp, vtodefine);
                    }
                }

                if (yp < mmax_m_xy && (sdirm & 0x80) != 0) {
                    nxtfp = &(mdpoints_[xm + yp + zp]);
                    this_s = mtsurfaces_[xm + yp + z];
                    upper_s = mtsurfaces_[xm + yp + zp];
                    if (!(SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s,
                        upper_s)))
                    {
                        sdirm &= (0xFF ^ 0x80);
                    } else {
                        enqueueIfUndefined(nxtfp, xm, yp, zp, vtodefine);
                    }
                }
            }

            if (xp < mmax_x_) {
                if (ym >= 0 && (sdirm & 0x08) != 0) {
                    nxtfp = &(mdpoints_[xp + ym + zp]);
                    this_s = mtsurfaces_[xp + ym + z];
                    upper_s = mtsurfaces_[xp + ym + zp];
                    if (!(SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s,
                        upper_s)))
                    {
                        sdirm &= (0xFF ^ 0x08);
                    } else {
                        enqueueIfUndefined(nxtfp, xp, ym, zp, vtodefine);
                    }
                }

                if (yp < mmax_m_xy && (sdirm & 0x02) != 0) {
                    nxtfp = &(mdpoints_[xp + yp + zp]);
                    this_s = mtsurfaces_[xp + yp + z];
                    upper_s = mtsurfaces_[xp + yp + zp];
                    if (!(SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s,
                        upper_s)))
                    {
                        sdirm &= (0xFF ^ 0x02);
                    } else {
                        enqueueIfUndefined(nxtfp, xp, yp, zp, vtodefine);
                    }
                }
            }
            cfp->dirsSame = sdirm;
            cfp->dirsAbove = sdirh;
            cfp->dirsBelow = sdirl;

            break;
        case SurfaceType::kNonWalkable:
        case SurfaceType::kHandrailLight:
        case SurfaceType::kEmpty:
            cfp->flags = kNonWalkable;
            break;
        case SurfaceType::kTrainPlatformNS:
            cfp->flags = kWalkable;
            cfp->flags |= kSafeWalk;
            if (zm >= 0) {
                mdpoints_[x + y + zm].flags = kNonWalkable;
                if (xm >= 0) {
                    this_s = mtsurfaces_[xm + y + zm];
                    upper_s = mtsurfaces_[xm + y + z];
                    if (SurfaceUtils::isFlatSurface(this_s)) {
                        nxtfp = &(mdpoints_[xm + y + z]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x40;
                            enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                        }
                    } else if (SurfaceUtils::isStairs(upper_s) && upper_s != SurfaceType::kSlopeWE) {
                        nxtfp = &(mdpoints_[xm + y + z]);
                        this_s = upper_s;
                        upper_s = mtsurfaces_[xm + y + zp];
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x40;
                            enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                        }
                    }
                }
                if (ym >= 0) {
                    this_s = mtsurfaces_[x + ym + zm];
                    upper_s = mtsurfaces_[x + ym + z];
                    if (SurfaceUtils::isFlatSurface(this_s)) {
                        nxtfp = &(mdpoints_[x + ym + z]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x10;
                            enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                        }
                    } else if (SurfaceUtils::isStairs(upper_s) && upper_s != SurfaceType::kSlopeSN) {
                        nxtfp = &(mdpoints_[x + ym + z]);
                        this_s = upper_s;
                        upper_s = mtsurfaces_[x + ym + zp];
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x10;
                            enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                        }
                    }
                }
                if (yp < mmax_m_xy) {
                    this_s = mtsurfaces_[x + yp + zm];
                    upper_s = mtsurfaces_[x + yp + z];
                    if (SurfaceUtils::isFlatSurface(this_s)) {
                        nxtfp = &(mdpoints_[x + yp + z]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x01;
                            enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                        }
                    } else if (SurfaceUtils::isStairs(upper_s) && upper_s != SurfaceType::kSlopeNS) {
                        nxtfp = &(mdpoints_[x + yp + z]);
                        this_s = upper_s;
                        upper_s = mtsurfaces_[x + yp + zp];
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x01;
                            enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                        }
                    }
                }
            }

            if (xp < mmax_x_) {
                this_s = mtsurfaces_[xp + y + z];
                upper_s = mtsurfaces_[xp + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x02 | 0x04 | 0x08);
                    nxtfp = &(mdpoints_[xp + y + zp]);
                    enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x02 | 0x08);
                    if (this_s == SurfaceType::kSlopeSN || this_s == SurfaceType::kSlopeNS || this_s == SurfaceType::kSlopeWE){
                        sdirl |= 0x04;
                    }
                    nxtfp = &(mdpoints_[xp + y + z]);
                    enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                } else {
                    sdirmr |= (0x02 | 0x08);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeSN || upper_s == SurfaceType::kSlopeNS
                        || upper_s == SurfaceType::kSlopeEW))
                    {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[xp + y + (zp + mmax_m_xy)]))
                        {
                            sdirm |= 0x04;
                            nxtfp = &(mdpoints_[xp + y + zp]);
                            enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x02 | 0x08);

            if(ym >= 0) {
                this_s = mtsurfaces_[x + ym + z];
                upper_s = mtsurfaces_[x + ym + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x08 | 0x10);
                    nxtfp = &(mdpoints_[x + ym + zp]);
                    enqueueIfUndefined(nxtfp, x, ym, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x08 | 0x20);
                    if (this_s == SurfaceType::kSlopeNS || this_s == SurfaceType::kSlopeEW || this_s == SurfaceType::kSlopeWE) {
                        sdirl |= 0x10;
                    }
                    nxtfp = &(mdpoints_[x + ym + z]);
                    enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                } else {
                    sdirmr |= (0x08 | 0x20);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeSN || upper_s == SurfaceType::kSlopeEW || upper_s == SurfaceType::kSlopeWE)) {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[x + ym + (zp + mmax_m_xy)]))
                        {
                            sdirm |= 0x10;
                            nxtfp = &(mdpoints_[x + ym + zp]);
                            enqueueIfUndefined(nxtfp, x, ym, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x08);

            if (yp < mmax_m_xy) {
                this_s = mtsurfaces_[x + yp + z];
                upper_s = mtsurfaces_[x + yp + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x01 | 0x02);
                    nxtfp = &(mdpoints_[x + yp + zp]);
                    enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x80 | 0x02);
                    if (this_s == SurfaceType::kSlopeSN || this_s == SurfaceType::kSlopeEW || this_s == SurfaceType::kSlopeWE) {
                        sdirl |= 0x01;
                    }
                    nxtfp = &(mdpoints_[x + yp + z]);
                    enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                } else {
                    sdirmr |= (0x80 | 0x02);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeNS || upper_s == SurfaceType::kSlopeEW
                        || upper_s == SurfaceType::kSlopeWE))
                    {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[x + yp + (zp + mmax_m_xy)]))
                        {
                            sdirm |= 0x01;
                            nxtfp = &(mdpoints_[x + yp + zp]);
                            enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x80 | 0x02);
            sdirm &= (0xFF ^ sdirmr);

            // edges
            if (xp < mmax_x_) {
                if (ym >= 0 && (sdirm & 0x08) != 0) {
                    nxtfp = &(mdpoints_[xp + ym + zp]);
                    this_s = mtsurfaces_[xp + ym + z];
                    upper_s = mtsurfaces_[xp + ym + zp];
                    if (!(SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s,
                        upper_s)))
                    {
                        sdirm &= (0xFF ^ 0x08);
                    } else {
                        enqueueIfUndefined(nxtfp, xp, ym, zp, vtodefine);
                    }
                }

                if (yp < mmax_m_xy && (sdirm & 0x02) != 0) {
                    nxtfp = &(mdpoints_[xp + yp + zp]);
                    this_s = mtsurfaces_[xp + yp + z];
                    upper_s = mtsurfaces_[xp + yp + zp];
                    if (!(SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s,
                        upper_s)))
                    {
                        sdirm &= (0xFF ^ 0x02);
                    } else {
                        enqueueIfUndefined(nxtfp, xp, yp, zp, vtodefine);
                    }
                }
            }
            cfp->dirsSame = sdirm;
            cfp->dirsAbove = sdirh;
            cfp->dirsBelow = sdirl;

            break;
        case SurfaceType::kTrainPlatformEW:
            cfp->flags = kWalkable;
            cfp->flags |= kSafeWalk;
            if (zm >= 0) {
                mdpoints_[x + y + zm].flags = kNonWalkable;
                if (ym >= 0) {
                    this_s = mtsurfaces_[x + ym + zm];
                    upper_s = mtsurfaces_[x + ym + z];
                    if (SurfaceUtils::isFlatSurface(this_s)) {
                        nxtfp = &(mdpoints_[x + ym + z]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x10;
                            enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                        }
                    } else if (SurfaceUtils::isStairs(upper_s) && upper_s != SurfaceType::kSlopeSN) {
                        nxtfp = &(mdpoints_[x + ym + z]);
                        this_s = upper_s;
                        upper_s = mtsurfaces_[x + ym + zp];
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x10;
                            enqueueIfUndefined(nxtfp, x, ym, z, vtodefine);
                        }
                    }
                }
                if (xm >= 0) {
                    this_s = mtsurfaces_[xm + y + zm];
                    upper_s = mtsurfaces_[xm + y + z];
                    if (SurfaceUtils::isFlatSurface(this_s)) {
                        nxtfp = &(mdpoints_[xm + y + z]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x40;
                            enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                        }
                    } else if (SurfaceUtils::isStairs(upper_s) && upper_s != SurfaceType::kSlopeWE) {
                        nxtfp = &(mdpoints_[xm + y + z]);
                        this_s = upper_s;
                        upper_s = mtsurfaces_[xm + y + zp];
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x40;
                            enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                        }
                    }
                }
                if (xp < mmax_x_) {
                    this_s = mtsurfaces_[xp + y + zm];
                    upper_s = mtsurfaces_[xp + y + z];
                    if (SurfaceUtils::isFlatSurface(this_s)) {
                        nxtfp = &(mdpoints_[xp + y + z]);
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x04;
                            enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                        }
                    } else if (SurfaceUtils::isStairs(upper_s) && upper_s != SurfaceType::kSlopeEW) {
                        nxtfp = &(mdpoints_[xp + y + z]);
                        this_s = upper_s;
                        upper_s = mtsurfaces_[xp + y + zp];
                        if (SurfaceUtils::isTraversable(this_s, upper_s)) {
                            sdirl |= 0x04;
                            enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                        }
                    }
                }
            }

            if (xm >=0) {
                this_s = mtsurfaces_[xm + y + z];
                upper_s = mtsurfaces_[xm + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x40 | 0x80);
                    nxtfp = &(mdpoints_[xm + y + zp]);
                    enqueueIfUndefined(nxtfp, xm, y, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x20 | 0x80);
                    if (this_s == SurfaceType::kSlopeSN || this_s == SurfaceType::kSlopeNS || this_s == SurfaceType::kSlopeEW){
                        sdirl |= 0x40;
                    }
                    nxtfp = &(mdpoints_[xm + y + z]);
                    enqueueIfUndefined(nxtfp, xm, y, z, vtodefine);
                } else {
                    sdirmr |= (0x20 | 0x80);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeSN || upper_s == SurfaceType::kSlopeNS
                        || upper_s == SurfaceType::kSlopeWE))
                    {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[xm + y + (zp + mmax_m_xy)]))
                        {
                            sdirm |= 0x40;
                            nxtfp = &(mdpoints_[xm + y + zp]);
                            enqueueIfUndefined(nxtfp, xm, y, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x20 | 0x80);

            if (xp < mmax_x_) {
                this_s = mtsurfaces_[xp + y + z];
                upper_s = mtsurfaces_[xp + y + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x02 | 0x04);
                    nxtfp = &(mdpoints_[xp + y + zp]);
                    enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x02 | 0x08);
                    if (this_s == SurfaceType::kSlopeSN || this_s == SurfaceType::kSlopeNS
                        || this_s == SurfaceType::kSlopeWE)
                    {
                        sdirl |= 0x04;
                    }
                    nxtfp = &(mdpoints_[xp + y + z]);
                    enqueueIfUndefined(nxtfp, xp, y, z, vtodefine);
                } else {
                    sdirmr |= (0x02 | 0x08);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeSN || upper_s == SurfaceType::kSlopeNS
                        || upper_s == SurfaceType::kSlopeEW))
                    {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[xp + y + (zp + mmax_m_xy)]))
                        {
                            sdirm |= 0x04;
                            nxtfp = &(mdpoints_[xp + y + zp]);
                            enqueueIfUndefined(nxtfp, xp, y, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x02 | 0x08);

            if (yp < mmax_m_xy) {
                this_s = mtsurfaces_[x + yp + z];
                upper_s = mtsurfaces_[x + yp + zp];
                if (SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s, upper_s))
                {
                    sdirm |= (0x80 | 0x01 | 0x02);
                    nxtfp = &(mdpoints_[x + yp + zp]);
                    enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                } else if (SurfaceUtils::isStairs(this_s) && SurfaceUtils::isTraversable(this_s,
                    upper_s))
                {
                    sdirmr |= (0x80 | 0x02);
                    if (this_s == SurfaceType::kSlopeSN || this_s == SurfaceType::kSlopeEW || this_s == SurfaceType::kSlopeWE) {
                        sdirl |= 0x01;
                    }
                    nxtfp = &(mdpoints_[x + yp + z]);
                    enqueueIfUndefined(nxtfp, x, yp, z, vtodefine);
                } else {
                    sdirmr |= (0x80 | 0x02);
                    if ((zp + mmax_m_xy) < mmax_m_all
                        && (upper_s == SurfaceType::kSlopeNS || upper_s == SurfaceType::kSlopeEW
                        || upper_s == SurfaceType::kSlopeWE))
                    {
                        if (SurfaceUtils::isTraversable(upper_s,
                            mtsurfaces_[x + yp + (zp + mmax_m_xy)]))
                        {
                            sdirm |= 0x01;
                            nxtfp = &(mdpoints_[x + yp + zp]);
                            enqueueIfUndefined(nxtfp, x, yp, zp, vtodefine);
                        }
                    }
                }
            } else
                sdirmr |= (0x80 | 0x02);
            sdirm &= (0xFF ^ sdirmr);

            // edges
            if (yp < mmax_m_xy) {
                if (xm >= 0 && (sdirm & 0x80) != 0) {
                    nxtfp = &(mdpoints_[xm + yp + zp]);
                    this_s = mtsurfaces_[xm + yp + z];
                    upper_s = mtsurfaces_[xm + yp + zp];
                    if (!(SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s,
                        upper_s)))
                    {
                        sdirm &= (0xFF ^ 0x80);
                    } else {
                        enqueueIfUndefined(nxtfp, xm, yp, zp, vtodefine);
                    }
                }
                if (xp < mmax_x_ && (sdirm & 0x02) != 0) {
                    nxtfp = &(mdpoints_[xp + yp + zp]);
                    this_s = mtsurfaces_[xp + yp + z];
                    upper_s = mtsurfaces_[xp + yp + zp];
                    if (!(SurfaceUtils::isFlatSurface(this_s) && SurfaceUtils::isTraversable(this_s,
                        upper_s)))
                    {
                        sdirm &= (0xFF ^ 0x02);
                    } else {
                        enqueueIfUndefined(nxtfp, xp, yp, zp, vtodefine);
                    }
                }
            }

            cfp->dirsSame = sdirm;
            cfp->dirsAbove = sdirh;
            cfp->dirsBelow = sdirl;

            break;
        case SurfaceType::kUnknown:
            LOG(Log::k_FLG_GAME, "Mission", "classifyTile()", ("Unknown surface"));
            break;
    }
}

bool Mission::findReachableTileFromBase(TilePoint &basePt) {
    for (int z = mmax_z_ - 1; z >= 0; z--) {
        TilePoint candidate = p_map_->projectToZLevel(basePt, z);
        
        if (!p_map_->isWithinMapBounds(candidate)) {
            if (candidate.tz < 0 || candidate.tx < 0 || candidate.ty < 0) {
                break; // Out of bounds below/behind, stop searching
            }
            continue; // Out of bounds above/ahead, try next Z level
        }
        
        int tileIndex = getTileIndex(candidate);
        
        // Try to find reachable position at current Z level
        if (isTileReachable(tileIndex)) {
            TilePoint adjusted = adjustPositionForSurface(candidate, tileIndex);
            if (adjusted.isValid()) {
                basePt = adjusted;
                return true;
            }
        }
        
        // Try to find walkable position on slope at Z-1 level
        if (z > 0) {
            TilePoint onLowerSlope = tryProjectOntoLowerSlope(candidate);
            if (onLowerSlope.isValid()) {
                basePt = onLowerSlope;
                return true;
            }
        }
    }
    
    return false;
}

/**
 * Adjusts the position within a tile based on its surface type (flat or sloped).
 * Returns an invalid TilePoint if the position cannot be adjusted to be walkable.
 */
TilePoint Mission::adjustPositionForSurface(const TilePoint &point, int tileIndex) {
    int mmax_m_all = mmax_x_ * mmax_y_ * mmax_z_;
    if (!mtsurfaces_ || tileIndex < 0 || tileIndex >= mmax_m_all) return point;
    SurfaceType surfaceType = mtsurfaces_[tileIndex];
    
    switch (surfaceType) {
        case SurfaceType::kSlopeSN:
            return adjustForSlopeSN(point);
        case SurfaceType::kSlopeNS:
            return adjustForSlopeNS(point);
        case SurfaceType::kSlopeEW:
            return adjustForSlopeEW(point);
        case SurfaceType::kSlopeWE:
            return adjustForSlopeWE(point);
        default:
            // Flat surface or other walkable type - position is valid as-is
            return point;
    }
}

/**
 * Adjusts position for South-to-North slope (rises northward).
 */
TilePoint Mission::adjustForSlopeSN(const TilePoint &point) {
    int adjustedY = ((point.oy + 128) * 2) / 3;
    int adjustedX = point.ox + 128 - adjustedY / 2;
    
    if (adjustedX < 256) {
        // Position fits within current tile
        return TilePoint(point.tx, point.ty, point.tz, adjustedX, adjustedY);
    }
    
    // Position overflows to adjacent tile in +X direction
    return tryAdjacentTile(point, +1, 0, adjustedX - 256, adjustedY, 
                          SurfaceType::kSlopeSN);
}

/**
 * Adjusts position for North-to-South slope (rises southward).
 */
TilePoint Mission::adjustForSlopeNS(const TilePoint &point) {
    if (point.oy >= 128) {
        // TODO: Southern half not implemented
        return TilePoint::invalid();
    }
    
    int adjustedY = point.oy * 2;
    int adjustedX = point.ox + point.oy;
    
    if (adjustedX < 256) {
        // Position fits within current tile
        return TilePoint(point.tx, point.ty, point.tz, adjustedX, adjustedY);
    }
    
    // Position overflows to adjacent tile in +X direction
    return tryAdjacentTile(point, +1, 0, adjustedX - 256, adjustedY, 
                          SurfaceType::kSlopeNS);
}

/**
 * Adjusts position for East-to-West slope (rises westward).
 */
TilePoint Mission::adjustForSlopeEW(const TilePoint &point) {
    if (point.ox >= 128) {
        // TODO: Eastern half not implemented
        return TilePoint::invalid();
    }
    
    int adjustedX = point.ox * 2;
    int adjustedY = point.ox + point.oy;
    
    if (adjustedY < 256) {
        // Position fits within current tile
        return TilePoint(point.tx, point.ty, point.tz, adjustedX, adjustedY);
    }
    
    // Position overflows to adjacent tile in +Y direction
    return tryAdjacentTile(point, 0, +1, adjustedX, adjustedY - 256, 
                          SurfaceType::kSlopeEW);
}

/**
 * Adjusts position for West-to-East slope (rises eastward).
 */
TilePoint Mission::adjustForSlopeWE(const TilePoint &point) {
    int adjustedX = ((point.ox + 128) * 2) / 3;
    int adjustedY = point.oy + 128 - adjustedX / 2;
    
    if (adjustedY < 256) {
        // Position fits within current tile
        return TilePoint(point.tx, point.ty, point.tz, adjustedX, adjustedY);
    }
    
    // Position overflows to adjacent tile in +Y direction
    return tryAdjacentTile(point, 0, +1, adjustedX, adjustedY - 256, 
                          SurfaceType::kSlopeWE);
}

/**
 * Tries to continue slope adjustment onto an adjacent tile.
 */
TilePoint Mission::tryAdjacentTile(const TilePoint &point, int deltaX, int deltaY,
                                   int newOffsetX, int newOffsetY, 
                                   SurfaceType expectedSurfaceType) {
    int adjacentTx = point.tx + deltaX;
    int adjacentTy = point.ty + deltaY;
    
    // Check if adjacent tile is within bounds
    if (adjacentTx < 0 || adjacentTx >= mmax_x_ || 
        adjacentTy < 0 || adjacentTy >= mmax_y_ ||
        point.tz < 0 || point.tz >= mmax_z_ || !mtsurfaces_) {
        return TilePoint::invalid();
    }
    
    int adjacentIndex = adjacentTx + adjacentTy * mmax_x_ + point.tz * mmax_m_xy;
    
    // Check if adjacent tile is reachable and has the same slope type
    if (isTileReachable(adjacentIndex) && 
        mtsurfaces_[adjacentIndex] == expectedSurfaceType) {
        return TilePoint(adjacentTx, adjacentTy, point.tz, newOffsetX, newOffsetY);
    }
    
    return TilePoint::invalid();
}

/**
 * Attempts to project the point onto a slope at Z-1 level.
 * This handles cases where the character is "above" a sloped surface.
 */
TilePoint Mission::tryProjectOntoLowerSlope(const TilePoint &point) {
    int lowerZ = point.tz - 1;
    if (point.tx < 0 || point.tx >= mmax_x_ ||
        point.ty < 0 || point.ty >= mmax_y_ ||
        lowerZ < 0 || lowerZ >= mmax_z_ || !mtsurfaces_) {
        return TilePoint::invalid();
    }
    
    int lowerIndex = point.tx + point.ty * mmax_x_ + lowerZ * mmax_m_xy;
    
    if (!isTileReachable(lowerIndex)) {
        return TilePoint::invalid();
    }
    
    SurfaceType surfaceType = mtsurfaces_[lowerIndex];
    
    switch (surfaceType) {
        case SurfaceType::kSlopeSN:
            return projectOntoSlopeSN(point, lowerZ);
        case SurfaceType::kSlopeNS:
            return projectOntoSlopeNS(point, lowerZ);
        case SurfaceType::kSlopeEW:
            return projectOntoSlopeEW(point, lowerZ);
        case SurfaceType::kSlopeWE:
            return projectOntoSlopeWE(point, lowerZ);
        default:
            return TilePoint::invalid();
    }
}

/**
 * Projects point onto South-to-North slope at lower Z level.
 */
TilePoint Mission::projectOntoSlopeSN(const TilePoint &point, int lowerZ) {
    int projectedY = (point.oy * 2) / 3;
    int projectedX = point.ox - projectedY / 2;
    
    if (projectedX >= 0) {
        return TilePoint(point.tx, point.ty, lowerZ, projectedX, projectedY);
    }
    
    return TilePoint::invalid();
}

/**
 * Projects point onto North-to-South slope at lower Z level.
 */
TilePoint Mission::projectOntoSlopeNS(const TilePoint &point, int lowerZ) {
    int projectedY = (point.oy - 128) * 2;
    int projectedX = (point.ox + projectedY / 2) - 128;
    
    if (projectedY >= 0 && projectedX >= 0 && projectedX < 256) {
        return TilePoint(point.tx, point.ty, lowerZ, projectedX, projectedY);
    }
    
    return TilePoint::invalid();
}

/**
 * Projects point onto East-to-West slope at lower Z level.
 */
TilePoint Mission::projectOntoSlopeEW(const TilePoint &point, int lowerZ) {
    int projectedX = (point.ox - 128) * 2;
    int projectedY = (point.oy + projectedX / 2) - 128;
    
    if (projectedX >= 0 && projectedY >= 0 && projectedY < 256) {
        return TilePoint(point.tx, point.ty, lowerZ, projectedX, projectedY);
    }
    
    return TilePoint::invalid();
}

/**
 * Projects point onto West-to-East slope at lower Z level.
 */
TilePoint Mission::projectOntoSlopeWE(const TilePoint &point, int lowerZ) {
    int projectedX = (point.ox * 2) / 3;
    int projectedY = point.oy - projectedX / 2;
    
    if (projectedY >= 0) {
        return TilePoint(point.tx, point.ty, lowerZ, projectedX, projectedY);
    }
    
    return TilePoint::invalid();
}

bool Mission::getWalkableClosestByZ(TilePoint &mtp) {
    // NOTE: using z from mtp as start to find closest z
    int inc_z = mtp.tz;
    int dec_z = mtp.tz;
    bool found = false;

    do {
        if (inc_z < mmax_z_) {
            if ((mdpoints_[mtp.tx + mtp.ty * mmax_x_
                + inc_z * mmax_m_xy].flags & kWalkable) == kWalkable)
            {
                mtp.tz = inc_z;
                found = true;
                break;
            }
            inc_z++;
        }
        if (dec_z >= 0) {
            if ((mdpoints_[mtp.tx + mtp.ty * mmax_x_
                + dec_z * mmax_m_xy].flags & kWalkable) == kWalkable)
            {
                mtp.tz = dec_z;
                found = true;
                break;
            }
            dec_z--;
        }
    } while (inc_z < mmax_z_ || dec_z >= 0);

    return found;
}

/*!
 * Inserts obj into every cell of grid whose tile overlaps the object's bounding box.
 * Each object is in exactly the cells it occupies (1 cell for small objects, up to a
 * few cells for large vehicles/statics).
 */
void Mission::insertIntoSpatialGrid(std::vector<std::vector<MapObject*>>& grid, MapObject* obj) {
    int wx = obj->tileX() * 256 + obj->offX();
    int wy = obj->tileY() * 256 + obj->offY();
    int wz = obj->tileZ() * 128 + obj->offZ();

    int tx_min = std::max(0, (wx - obj->sizeX()) / 256);
    int tx_max = std::min(mmax_x_ - 1, (wx + obj->sizeX()) / 256);
    int ty_min = std::max(0, (wy - obj->sizeY()) / 256);
    int ty_max = std::min(mmax_y_ - 1, (wy + obj->sizeY()) / 256);
    int tz_min = std::max(0, wz / 128);
    int tz_max = std::min(mmax_z_ - 1, (wz + obj->sizeZ()) / 128);

    for (int tz = tz_min; tz <= tz_max; ++tz) {
        for (int ty = ty_min; ty <= ty_max; ++ty) {
            for (int tx = tx_min; tx <= tx_max; ++tx) {
                grid[tx + ty * mmax_x_ + tz * mmax_m_xy].push_back(obj);
            }
        }
    }
}

/*!
 * Rebuilds the dynamic spatial grid from the current positions of all mobile entities.
 * Called once per tick, before any entity animation, so the grid reflects positions
 * from the end of the previous tick (positional error < 1 tick, negligible).
 */
void Mission::buildDynamicSpatialGrid() {
    dynamicSpatialGrid_.assign(mmax_x_ * mmax_y_ * mmax_z_, {});

    for (PedInstance* pPed : peds_) {
        if (pPed->isAlive() && !pPed->isInVehicle()) {
            insertIntoSpatialGrid(dynamicSpatialGrid_, pPed);
        }
    }
    for (Vehicle* pVehicle : vehicles_) {
        insertIntoSpatialGrid(dynamicSpatialGrid_, pVehicle);
    }
    for (WeaponInstance* pWeapon : weaponsOnGround_) {
        if (!pWeapon->hasOwner()) {
            insertIntoSpatialGrid(dynamicSpatialGrid_, pWeapon);
        }
    }
}

/*!
 * @brief Searches for the closest object blocking the ray between two world points.
 *
 * Uses a spatial grid to test only objects present in the tiles the ray traverses,
 * rather than scanning all entities linearly. The stepping method mirrors
 * checkBlockedByTile() (8 world-unit steps).
 *
 * Excluded from the search:
 * - \p pOrigin itself (the shooter)
 * - the vehicle \p pOrigin is currently riding, if any
 * - Static objects whose isExcludedFromBlockers() returns true (e.g. open doors)
 * - Dead peds
 *
 * If a blocker is found, \p pStartPt and \p pEndPt are updated to the intersection
 * points with the blocker's bounding box, and \p dist is set to the distance from
 * the original \p pStartPt to that intersection.
 *
 * @param pStartPt  Origin of the ray (world coordinates). Updated to the blocker
 *                  entry point if a blocker is found.
 * @param pEndPt    End of the ray (world coordinates). Updated to the blocker
 *                  exit point if a blocker is found.
 * @param dist      Distance from \p pStartPt to \p pEndPt. Updated to the distance
 *                  to the closest blocker if one is found.
 * @param pOrigin   The entity that initiated the ray (shooter). May be nullptr.
 * @return Pointer to the closest blocking MapObject, or nullptr if none was found.
 */
MapObject * Mission::checkBlockedByObject(WorldPoint * pStartPt, WorldPoint * pEndPt,
        double *dist, const ShootableMapObject *pOrigin) {
    double inc_xyz[3];
    inc_xyz[0] = (pEndPt->x - pStartPt->x) / (*dist);
    inc_xyz[1] = (pEndPt->y - pStartPt->y) / (*dist);
    inc_xyz[2] = (pEndPt->z - pStartPt->z) / (*dist);
    WorldPoint copyStartPt = *pStartPt;
    WorldPoint copyEndPt = *pEndPt;
    WorldPoint blockStartPt;
    WorldPoint blockEndPt;
    double closest = *dist;
    MapObject *pBlocker = NULL;

    // if shooter is a Ped shooting from a vehicle, skip that vehicle
    Vehicle *pShooterVehicle = NULL;
    if (pOrigin && pOrigin->is(MapObject::kNaturePed)) {
        const PedInstance *pPed = static_cast<const PedInstance *>(pOrigin);
        pShooterVehicle = pPed->inVehicle(); // can be null
    }

    // Test one object against the ray and update the closest blocker if hit.
    auto testObject = [&](MapObject* obj) {
        if (static_cast<const MapObject*>(obj) == static_cast<const MapObject*>(pOrigin)) return;
        if (obj == pShooterVehicle) return;
        // Peds may have died since the grid was built this tick
        if (obj->is(MapObject::kNaturePed)) {
            PedInstance *pPed = static_cast<PedInstance*>(obj);
            if (pPed->isDead()) return;
            if (pOrigin && pOrigin->is(MapObject::kNaturePed)) {
                const PedInstance *pShooter = static_cast<const PedInstance*>(pOrigin);
                if (pShooter->isFriendlyFireProtected(pPed)) {
                    // no friendly fire
                    return;
                }
            }
        }
        if (obj->is(MapObject::kNatureVehicle)) {
            Vehicle *pVeh = static_cast<Vehicle*>(obj);
            if (pVeh->health() <= 0) return;
        }
        if (obj->isBlocker(&copyStartPt, &copyEndPt, inc_xyz)) {
            int cx = pStartPt->x - copyStartPt.x;
            int cy = pStartPt->y - copyStartPt.y;
            int cz = pStartPt->z - copyStartPt.z;
            double dist_blocker = sqrt((double)(cx * cx + cy * cy + cz * cz));
            if (closest == -1 || dist_blocker < closest) {
                closest = dist_blocker;
                pBlocker = obj;
                blockStartPt = copyStartPt;
                blockEndPt = copyEndPt;
            }
            copyStartPt = *pStartPt;
            copyEndPt = *pEndPt;
        }
    };

    // Test all objects present in a single grid cell.
    auto testCell = [&](int idx) {
        if (idx >= 0) {
            size_t cellIdx = static_cast<size_t>(idx);
            if (cellIdx < staticSpatialGrid_.size()) {
                for (MapObject* obj : staticSpatialGrid_[cellIdx]) testObject(obj);
            }
            if (cellIdx < dynamicSpatialGrid_.size()) {
                for (MapObject* obj : dynamicSpatialGrid_[cellIdx]) testObject(obj);
            }
        }
    };

    // Step along the ray (same stepping method as checkBlockedByTile) and visit
    // each tile the ray crosses, testing only the objects present in that tile.
    double sx = (double)pStartPt->x;
    double sy = (double)pStartPt->y;
    double sz = (double)pStartPt->z;
    const double incrX = inc_xyz[0] * 8.0;
    const double incrY = inc_xyz[1] * 8.0;
    const double incrZ = inc_xyz[2] * 8.0;
    const double dist_dec = 8.0;
    double dist_close = *dist;
    int oldtx = -1, oldty = -1, oldtz = -1;

    while (dist_close > dist_dec) {
        int ntx = (int)sx / 256;
        int nty = (int)sy / 256;
        int ntz = (int)sz / 128;
        if (ntx != oldtx || nty != oldty || ntz != oldtz) {
            if (ntx >= 0 && ntx < mmax_x_ && nty >= 0 && nty < mmax_y_
                    && ntz >= 0 && ntz < mmax_z_) {
                testCell(ntx + nty * mmax_x_ + ntz * mmax_m_xy);
            }
            oldtx = ntx; oldty = nty; oldtz = ntz;
        }
        sx += incrX;
        sy += incrY;
        sz += incrZ;
        dist_close -= dist_dec;
    }
    // Also test the end tile (may be missed by the stepping loop)
    {
        int ntx = pEndPt->x / 256, nty = pEndPt->y / 256, ntz = pEndPt->z / 128;
        if (ntx >= 0 && ntx < mmax_x_ && nty >= 0 && nty < mmax_y_ && ntz >= 0 && ntz < mmax_z_
                && (ntx != oldtx || nty != oldty || ntz != oldtz)) {
            testCell(ntx + nty * mmax_x_ + ntz * mmax_m_xy);
        }
    }

    if (pBlocker != NULL) {
        *pStartPt = blockStartPt;
        *pEndPt = blockEndPt;
        *dist = closest;
    }

    return pBlocker;
}

/*!
 * Verify that the path from originPosW to pTargetPosW is not blocked by a tile.
 * If such a tile exists, pTargetPosW is updated with the position of the blocking tile.
 * \param originPosW Path starting point
 * \param pTargetPosW Path end point
 * \param updateLoc Set to true to update pTargetPosW when blocking tile is found
 * \param distanceMax Maximum distance we cannot cross. If distanceMax is
 *   reached before pTargetPosW, then path is stopped.
 * \param pInitialDistance This is the distance between origin and initial target position
 * \return a bitfield indicating the type of result:
 *      - bit 0(kBMaskBlockerTargetInRange) : target in range
 *      - bit 3(8) : distanceMax is reached
 *      - bit 4(kBMaskBlockerBlockedByTile): blocker tile, "pTargetLoc" is set
 *      - bit 5(kBMaskBlockerTargetOutOfMap): out of visible reach
 */
uint8_t Mission::checkBlockedByTile(const WorldPoint & originPosW, WorldPoint *pTargetPosW,
                                  bool updateLoc, double distanceMax, double *pInitialDistance) {
    // TODO: some objects mid point is higher then map z
    assert(distanceMax >= 0);

    int cx = originPosW.x;
    int cy = originPosW.y;
    int cz = originPosW.z;
    if (cz > (mmax_z_ - 1) * 128)
        return kBMaskBlockerTargetOutOfMap;

    // This variable will store the target location as it may moves if
    // a tile blocks the path.
    WorldPoint tmpTargetWLoc = *pTargetPosW;

    if (tmpTargetWLoc.z > (mmax_z_ - 1) * 128)
        return kBMaskBlockerTargetOutOfMap;

    // This is the distance between the origin and the target
    double distanceToTarget = 0;
    distanceToTarget = sqrt((double)((tmpTargetWLoc.x - cx) * (tmpTargetWLoc.x - cx) + (tmpTargetWLoc.y - cy) * (tmpTargetWLoc.y - cy)
        + (tmpTargetWLoc.z - cz) * (tmpTargetWLoc.z - cz)));
    uint8_t block_mask = kBMaskBlockerTargetInRange;

    if (pInitialDistance)
        *pInitialDistance = distanceToTarget;
    if (distanceToTarget == 0)
        return block_mask;

    double sx = (double) cx;
    double sy = (double) cy;
    double sz = (double) cz;

    if (distanceToTarget >= distanceMax) {
        // the distance we have to cross (distanceToTarget) is higher than the maximum
        // distance we are allowed to cross (distanceMax)

        // update target position according to distanceMax
        double dist_k = (double)distanceMax / distanceToTarget;
        tmpTargetWLoc.x = cx + (int)((tmpTargetWLoc.x - cx) * dist_k);
        tmpTargetWLoc.y = cy + (int)((tmpTargetWLoc.y - cy) * dist_k);
        tmpTargetWLoc.z = cz + (int)((tmpTargetWLoc.z - cz) * dist_k);
        // set mask to indicate distanceMax is reached
        block_mask = 8;
        if (updateLoc) {
            *pTargetPosW = tmpTargetWLoc;
        }
        distanceToTarget = distanceMax;
    }

    // NOTE: these values are less then 1.
    // If they are incremented, time required to check range will be shorter but less precise check,
    // If decremented longer but more precise.
    // Increment is (n * 8)
    double incrX = ((tmpTargetWLoc.x - cx) * 8) / distanceToTarget;
    double incrY = ((tmpTargetWLoc.y - cy) * 8) / distanceToTarget;
    double incrZ = ((tmpTargetWLoc.z - cz) * 8) / distanceToTarget;

    int oldx = cx / 256;
    int oldy = cy / 256;
    int oldz = cz / 128;
    double dist_close = distanceToTarget;
    // look note before, should be same increment
    double dist_dec = 1.0 * 8;

    if (!mtsurfaces_) {
        return block_mask;
    }

    while (dist_close > dist_dec) {
        int nx = (int)sx / 256;
        int ny = (int)sy / 256;
        int nz = (int)sz / 128;
        if (nx < 0 || nx >= mmax_x_ || ny < 0 || ny >= mmax_y_ || nz < 0 || nz >= mmax_z_) {
            break;
        }
        SurfaceType surfType = mtsurfaces_[nx + ny * mmax_x_ + nz * mmax_m_xy];
        
        if (oldx != nx || oldy != ny || oldz != nz || SurfaceUtils::isStairs(surfType)) {
            if (SurfaceUtils::isSolid(surfType)) {
                bool is_blocked = false;
                int offz = (int)sz % 128;
                switch (surfType) {
                    case SurfaceType::kSlopeSN:
                        if (offz <= (127 - (((int)sy % 256) >> 1)))
                            is_blocked = true;
                        break;
                    case SurfaceType::kSlopeNS:
                        if (offz <= (((int)sy % 256) >> 1))
                            is_blocked = true;
                        break;
                    case SurfaceType::kSlopeEW:
                        if (offz <= (((int)sx % 256) >> 1))
                            is_blocked = true;
                        break;
                    case SurfaceType::kSlopeWE:
                        if (offz <= (127 - (((int)sx % 256) >> 1)))
                            is_blocked = true;
                        break;
                    default:
                        is_blocked = true;
                }
                if (is_blocked) {
                    sx -= incrX;
                    sy -= incrY;
                    sz -= incrZ;
                    double dsx = sx - (double)cx;
                    double dsy = sy - (double)cy;
                    double dsz = sz - (double)cz;
                    tmpTargetWLoc.x = (int)sx;
                    tmpTargetWLoc.y = (int)sy;
                    tmpTargetWLoc.z = (int)sz;
                    dist_close = sqrt(dsx * dsx + dsy * dsy + dsz * dsz);
                    // set mask to indicate path is blocked by a tile
                    if (block_mask == kBMaskBlockerTargetInRange)
                        block_mask = kBMaskBlockerBlockedByTile;
                    else
                        block_mask |= kBMaskBlockerBlockedByTile;
                    if (updateLoc) {
                        pTargetPosW->x = (int)sx;
                        pTargetPosW->y = (int)sy;
                        pTargetPosW->z = (int)sz;
                    }
                    break;
                }
            }
            oldx = nx;
            oldy = ny;
            oldz = nz;
        }
        sx += incrX;
        sy += incrY;
        sz += incrZ;
        dist_close -= dist_dec;
    } // end while

    return block_mask;
}

/*!
 * \param originLoc
 * \param pTarget
 * \param pTargetPosW
 * \param distTo
 * \return mask where bits are:
 *   - 0b : target in range(1)
 *   - 1b : blocker is object, pTarget is set(2)
 *   - 2b : blocker object, "pn" is set(4)
 *   - 3b : reachable point set (8)
 *   - 4b : blocker tile, "pn" is set(16)
 *   - 5b : out of visible reach(32)
 * NOTE: only if "pn" or "t" are not null, variables are set

*/
uint8_t Mission::checkIfBlockersInShootingLine(const WorldPoint & originLoc,  const BlockerCriteria crits, 
        ShootableMapObject **pTarget, WorldPoint *pTargetPosW, double * distTo)
{
    // search for a tile blocking the path towards the target
    // tmpPosW will hold the updated position after that search
    WorldPoint tmpPosW;
    if (pTarget && *pTarget) {
        tmpPosW.convertFromTilePoint((*pTarget)->position());
    } else {
        tmpPosW = *pTargetPosW;
    }

    uint8_t bfBlockerFound = checkBlockedByTile(originLoc, &tmpPosW, true, crits.maxr, distTo);
    if (bfBlockerFound == kBMaskBlockerTargetOutOfMap) {
        // coords are out of map limits
        return bfBlockerFound;
    }

    if (crits.setBlocker) {
        *pTargetPosW = tmpPosW;
    }

    if (crits.checkTileOnly)
        return bfBlockerFound;

    WorldPoint tmpOrigin = originLoc;
    WorldPoint tmpEnd = tmpPosW;

    // We search for a possible object blocking the way on the path
    // between origin and the reached position
    int dx = tmpPosW.x - originLoc.x;
    int dy = tmpPosW.y - originLoc.y;
    int dz = tmpPosW.z - originLoc.z;
    double distToBlocker = sqrt((double)(dx * dx + dy * dy + dz * dz));
    MapObject *blockerObj = checkBlockedByObject(&tmpOrigin, &tmpEnd, &distToBlocker, crits.pOrigin);

    if (blockerObj) {
        if (bfBlockerFound == kBMaskBlockerTargetInRange)
            bfBlockerFound = 0;

        if (crits.setBlocker) {
            if (pTargetPosW) {
                *pTargetPosW = tmpOrigin;
                bfBlockerFound |= kBMaskBlockerTargetPosUpdated;
            }
            if (pTarget) {
                *pTarget = (ShootableMapObject *)blockerObj;
                bfBlockerFound |= kBMaskBlockerTargetObjectUpdated;
            }
        } else {
            if (pTarget && *pTarget) {
                if (*pTarget != blockerObj)
                    bfBlockerFound |= kBMaskBlockerTargetObjectUpdated | kBMaskBlockerTargetPosUpdated;
                else
                    bfBlockerFound = kBMaskBlockerTargetInRange;
            } else
                bfBlockerFound |= kBMaskBlockerTargetObjectUpdated | kBMaskBlockerTargetPosUpdated;
        }
    } else {
        if (crits.setBlocker) {
            if (bfBlockerFound != kBMaskBlockerTargetInRange && pTarget)
                *pTarget = nullptr;
        }
    }

    return bfBlockerFound;
}

/*!
 * Returns all dynamic objects present at tile tile this tick.
 * The returned reference is valid until the next call to buildDynamicSpatialGrid().
 * @param tile Tile coordinate
 * @return Reference to the list of MapObject* at that tile in the dynamic grid.
 */
const std::vector<MapObject*>& Mission::getObjectsAtTile(const TilePoint & tile) const {
    static const std::vector<MapObject*> kEmptyObjects;
    if (tile.tx < 0 || tile.tx >= mmax_x_ || tile.ty < 0 || tile.ty >= mmax_y_ || tile.tz < 0 || tile.tz >= mmax_z_) {
        return kEmptyObjects;
    }
    const size_t idx = static_cast<size_t>(tile.tx + tile.ty * mmax_x_ + tile.tz * mmax_m_xy);
    if (idx >= dynamicSpatialGrid_.size()) {
        return kEmptyObjects;
    }
    return dynamicSpatialGrid_[idx];
}

/*!
 * Returns the length of the path between a ped and a object if such a path exists and it is
 * shorter than the maximum length allowed.
 * \param pPed The origin of the path
 * \param objectToReach The end of the path
 * \param maxLength The length of the path must not exceed this value
 * \param length The returned length if the path exists
 * \return 0 if a path exists, else path does not exist so length is not set.
 */
uint8_t Mission::getPathLengthBetween(PedInstance *pPed, ShootableMapObject* objectToReach, double maxLength, double *length) {
    WorldPoint cur_xyz(pPed->position());
    cur_xyz.z += (pPed->sizeZ() >> 1);
    // TODO : it's not inRangeCPos that must be called but a method for path calculation
    BlockerCriteria crits;
    crits.checkTileOnly = true;
    crits.maxr = maxLength;
    crits.pOrigin = pPed;
    uint8_t res = checkIfBlockersInShootingLine(cur_xyz, crits, &objectToReach, nullptr, length);
    return res == 1 ? 0 : 1;
}

SurfaceType Mission::surfaceAt(int x, int y, int z) const
{
    if (x < 0 || y < 0 || z < 0 || x >= mmax_x_ || y >= mmax_y_ || z >= mmax_z_)
        return SurfaceType::kUnknown;

    return mtsurfaces_[x + y * mmax_x_ + z * mmax_m_xy];
}


/*!
 * Finds the first visible tile that the player has clicked on when shooting.
 * Starts with the highest tile and decrease the tz coord until reaching zero.
 * @param basePt 
 * @return 
 */
bool Mission::findShootableTileFromBase(TilePoint &basePt) {
    bool gotIt = false;
    TilePoint candidate;

    for (int tz = mmax_z_; (tz > 0 && !gotIt); tz--) {
        int bzm = tz - 1;

        candidate = p_map_->projectToZLevel(basePt, tz);

        const SurfaceType surfaceType = surfaceAt(candidate.tx, candidate.ty, bzm);
        int dx = 0, dy = 0;

        if (surfaceType == SurfaceType::kSlopeSN) {
            dy = (candidate.oy * 2) / 3;
            dx = candidate.ox - dy / 2;
            gotIt = (dx >= 0) || tryShiftX(candidate.tx, candidate.ty, bzm, candidate.ox, candidate.oy, -1, dx + 256, dy, SurfaceType::kSlopeSN);
        } else if (surfaceType == SurfaceType::kSlopeNS) {
            dy = (candidate.oy - 128) * 2;
            dx = (candidate.ox + dy / 2) - 128;
            if (dy >= 0)
                gotIt = (dx >= 0 && dx < 256) || tryShiftX(candidate.tx, candidate.ty, bzm, candidate.ox, candidate.oy, (dx < 0) ? -1 : 1, (dx + ((dx < 0) ? 256 : -256)), dy, SurfaceType::kSlopeNS);
        } else if (surfaceType == SurfaceType::kSlopeEW) {
            dx = (candidate.ox - 128) * 2;
            dy = (candidate.oy + dx / 2) - 128;
            if (dx >= 0)
                gotIt = (dy >= 0 && dy < 256) || tryShiftY(candidate.tx, candidate.ty, bzm, candidate.ox, candidate.oy, (dy < 0) ? -1 : 1, dx, (dy + ((dy < 0) ? 256 : -256)), SurfaceType::kSlopeEW);
        } else if (surfaceType == SurfaceType::kSlopeWE) {
            dx = (candidate.ox * 2) / 3;
            dy = candidate.oy - dx / 2;
            gotIt = (dy >= 0) || tryShiftY(candidate.tx, candidate.ty, bzm, candidate.ox, candidate.oy, -1, dx, dy + 256, SurfaceType::kSlopeWE);
        } else {
            gotIt = SurfaceUtils::isSolid(surfaceType);
        }

        if (!gotIt)
            gotIt = tryNeighbourAdjustments(candidate.tx, candidate.ty, bzm, candidate.ox, candidate.oy);
    }

    if (gotIt) {
        finalizeTile(candidate, basePt);
    }

    return gotIt;
}


bool Mission::tryShiftX(int &bx, int by, int bzm, int &box, int &boy, int shift, int newBox, int newBoy, SurfaceType expected) {
    int newBx = bx + shift;
    if (surfaceAt(newBx, by, bzm) == expected) {
        bx = newBx;
        box = newBox;
        boy = newBoy;
        return true;
    }
    return false;
}

bool Mission::tryShiftY(int bx, int &by, int bzm, int &box, int &boy, int shift, int newBox, int newBoy, SurfaceType expected) {
    int newBy = by + shift;
    if (surfaceAt(bx, newBy, bzm) == expected) {
        by = newBy;
        box = newBox;
        boy = newBoy;
        return true;
    }
    return false;
}

bool Mission::tryNeighbourAdjustments(int &bx, int &by, int bzm, int &box, int &boy) {
    auto check = [&](int offsetX, int offsetY) -> bool {
        const SurfaceType twd = surfaceAt(bx + offsetX, by + offsetY, bzm);
        if (twd == SurfaceType::kSlopeSN || twd == SurfaceType::kSlopeWE) {
            int dx = (twd == SurfaceType::kSlopeSN) ? ((boy + 256 * (offsetY != 0)) * 2) / 3 : ((box + 256 * (offsetX != 0)) * 2) / 3;
            int dy = (twd == SurfaceType::kSlopeSN) ? (box + 256 * (offsetX != 0)) - dx / 2 : (boy + 256 * (offsetY != 0)) - dx / 2;

            if (dx >= 0 && dx < 256 && dy >= 0 && dy < 256) {
                bx += offsetX;
                by += offsetY;
                box = (twd == SurfaceType::kSlopeSN) ? dy : dx;
                boy = (twd == SurfaceType::kSlopeSN) ? dx : dy;
                return true;
            }
        }
        return false;
    };

    return check(-1, 0) || check(0, -1) || check(-1, -1);
}

void Mission::finalizeTile(TilePoint tempTile, TilePoint &pLocT) {
    SurfaceType twd = surfaceAt(tempTile.tx, tempTile.ty, tempTile.tz - 1);

    switch (twd)
    {
    case SurfaceType::kSlopeSN:
        pLocT.oz = 127 - (tempTile.oy >> 1);
        --tempTile.tz;
        break;
    case SurfaceType::kSlopeNS:
        pLocT.oz = tempTile.oy >> 1;
        --tempTile.tz;
        break;
    case SurfaceType::kSlopeEW:
        pLocT.oz = tempTile.ox >> 1;
        --tempTile.tz;
        break;
    case SurfaceType::kSlopeWE:
        pLocT.oz = 127 - (tempTile.ox >> 1);
        --tempTile.tz;
        break;
    default:
        finalizeDefault(tempTile, pLocT);
        break;
    }

    pLocT.tx = tempTile.tx;
    pLocT.ty = tempTile.ty;
    pLocT.tz = tempTile.tz;
    pLocT.ox = tempTile.ox;
    pLocT.oy = tempTile.oy;

    assert(tempTile.tz >= 0);
}

void Mission::finalizeDefault(TilePoint &tempTile, TilePoint &pLocT) {
    SurfaceType twd = surfaceAt(tempTile.tx, tempTile.ty, tempTile.tz);
    if (SurfaceUtils::isSolid(twd))
    {
        pLocT.oz = (tempTile.ox > 192 || tempTile.oy > 192) ? (((tempTile.ox >= tempTile.oy) ? (256 - tempTile.ox) : (256 - tempTile.oy)) << 1) : 128;
        tempTile.tx = (pLocT.tx * 256 + pLocT.ox + 128 * (tempTile.tz - 1) + pLocT.oz) / 256;
        tempTile.ox = (pLocT.tx * 256 + pLocT.ox + 128 * (tempTile.tz - 1) + pLocT.oz) % 256;
        tempTile.ty = (pLocT.ty * 256 + pLocT.oy + 128 * (tempTile.tz - 1) + pLocT.oz) / 256;
        tempTile.oy = (pLocT.ty * 256 + pLocT.oy + 128 * (tempTile.tz - 1) + pLocT.oz) % 256;
        tempTile.tz += pLocT.oz / 128;
        pLocT.oz %= 128;
    }
    else
    {
        pLocT.oz = 0;
    }
}

/*bool Mission::isTileSolid(const TilePoint &point) {
    SurfaceType surface = surfaceAt(point.tx, point.ty, point.tz);
    switch (surface) {
        case 0x00:
        case 0x0C:
        case 0x10:
            solid = false;
            break;
        case 0x01:
            if (oz > (127 - (oy >> 1)))
                solid = false;
            break;
        case 0x02:
            if (oz > (oy >> 1))
                solid = false;
            break;
        case 0x03:
            if (oz > (ox >> 1))
                solid = false;
            break;
        case 0x04:
            if (oz > (127 - (ox >> 1)))
                solid = false;
            break;
        case SurfaceType::kRoadMark:
            return true;
        default:
            return false;
    }
}*/

/**
 * Gets the linear index for a tile in the map arrays.
 */
int Mission::getTileIndex(const TilePoint &point) const {
    if (point.tx < 0 || point.tx >= mmax_x_ || point.ty < 0 || point.ty >= mmax_y_ || point.tz < 0 || point.tz >= mmax_z_) {
        return -1;
    }
    return point.tx + point.ty * mmax_x_ + point.tz * mmax_m_xy;
}

/**
 * Checks if a tile at the given index is marked as reachable (ie walkable and with a path to it).
 */
bool Mission::isTileReachable(int tileIndex) const {
    int mmax_m_all = mmax_x_ * mmax_y_ * mmax_z_;
    if (tileIndex < 0 || tileIndex >= mmax_m_all || !mdpoints_) return false;
    return (mdpoints_[tileIndex].flags & kWalkable) == kWalkable;
}

void Mission::removeObjective(size_t index) {
    if (index >= objectives_.size()) return;
    delete objectives_[index];
    objectives_.erase(objectives_.begin() + index);
}

void Mission::insertObjective(size_t index, ObjectiveDesc *pObjective) {
    if (index >= objectives_.size()) {
        objectives_.push_back(pObjective);
    } else {
        objectives_.insert(objectives_.begin() + index, pObjective);
    }
}

void Mission::replaceObjective(size_t index, ObjectiveDesc *pObjective) {
    if (index >= objectives_.size()) return;
    delete objectives_[index];
    objectives_[index] = pObjective;
}

void Mission::clearObjectives() {
    for (ObjectiveDesc *obj : objectives_) {
        delete obj;
    }
    objectives_.clear();
}

PedInstance* Mission::spawnPed(PedInstance::PedType type, const TilePoint &pos, uint8_t orientation, uint16_t baseAnim, int factionId) {
    if (!p_map_) return nullptr;
    if (peds_.size() >= 256) return nullptr;

    uint16_t newId = 8;
    if (type == PedInstance::kPedTypeAgent && factionId >= 1 && factionId <= 7) {
        int allocatedSlot = allocateFactionSlot(factionId, nullptr);
        if (allocatedSlot >= 0) {
            newId = static_cast<uint16_t>(allocatedSlot);
        } else {
            // Faction is at capacity (max 8 agents) or map is full
            return nullptr;
        }
    } else {
        std::set<uint16_t> usedIds;
        for (PedInstance *p : peds_) {
            if (p) usedIds.insert(p->id());
        }
        // For non-agents, prefer slots >= 64 so we don't occupy faction blocks 8..63
        if (type != PedInstance::kPedTypeAgent) {
            newId = 64;
            while (newId <= 255 && usedIds.find(newId) != usedIds.end()) {
                newId++;
            }
            if (newId > 255) {
                // If >= 64 is exhausted, fallback to 8..63
                newId = 8;
                while (newId < 64 && usedIds.find(newId) != usedIds.end()) {
                    newId++;
                }
            }
        } else {
            newId = 8;
            while (newId <= 255 && usedIds.find(newId) != usedIds.end()) {
                newId++;
            }
        }
        if (newId > 255 || usedIds.find(newId) != usedIds.end()) {
            return nullptr;
        }
    }

    int speed = PedManager::getDefaultSpeed(type);
    if (type == PedInstance::kPedTypeAgent) {
        speed = PedManager::getStats(PedInstance::kPedTypeAgent, true).speed;
    }
    if (baseAnim == 0) {
        baseAnim = PedManager::getDefaultBaseAnim(type);
    }

    int hp = PedManager::getStats(type, type == PedInstance::kPedTypeAgent).health;

    PedInstance *p = new PedInstance(newId, p_map_, type, false, speed);
    p->setDirection(orientation);
    p->setStartHealth(hp);
    p->setHealth(hp);
    if (rs_eng::AnimationManager::singletonPtr() != nullptr) {
        p->setAnimations(baseAnim);
        p->goToState(kPedActionStateStanding, true);
    }
    p->setSize(32, 32, 256);
    p->setPosition(pos.tx, pos.ty, pos.tz, pos.ox, pos.oy, pos.oz);

    p->initAllLevelsForIPAType(IPAStim::Adrenaline, 0, 0, 0);
    p->initAllLevelsForIPAType(IPAStim::Intelligence, 0, 0, 0);
    p->initAllLevelsForIPAType(IPAStim::Perception, 0, 0, 0);

    unsigned int mt = p->type();
    p->setObjGroupDef(mt);
    if (mt == PedInstance::og_dmAgent) {
        PedManager::initEnemyAgent(p, 0, hp);
        p->setCyberwareOverridden(false);
        p->applyCyberwareTier(techLevel_);
        if (factionId >= 1 && factionId <= 7) {
            p->setFactionId(factionId);
        }
    } else if (mt == PedInstance::og_dmGuard) {
        PedManager::initGuard(p, hp);
    } else if (mt == PedInstance::og_dmPolice) {
        PedManager::initPolice(p, hp);
    } else if (mt == PedInstance::og_dmCivilian) {
        PedManager::initCivilian(p, hp);
    } else if (mt == PedInstance::og_dmCriminal) {
        PedManager::initCriminal(p, hp);
    }

    addPed(p);
    return p;
}

int Mission::countPlayerAgents() const {
    int count = 0;
    for (PedInstance *p : peds_) {
        if (p && p->isOurAgent()) {
            count++;
        }
    }
    return count;
}

int Mission::findNextFreePlayerSlot() const {
    bool occupied[4] = {false, false, false, false};
    for (PedInstance *p : peds_) {
        if (p && p->isOurAgent() && p->id() < 4) {
            occupied[p->id()] = true;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (!occupied[i]) return i;
    }
    return -1;
}

int Mission::countFactionAgents(int factionId, PedInstance *pExclude) {
    if (factionId < 1 || factionId > 7) return 0;
    int count = 0;
    for (PedInstance *p : peds_) {
        if (!p || p == pExclude || p->isOurAgent()) continue;
        if (p->type() == PedInstance::kPedTypeAgent && p->factionId() == factionId) {
            count++;
        }
    }
    return count;
}

int Mission::allocateFactionSlot(int factionId, PedInstance *pPed) {
    if (factionId < 1 || factionId > 7) return -1;
    if (countFactionAgents(factionId, pPed) >= 8) {
        return -1; // Faction is full (max 8 agents)
    }
    if (!pPed && peds_.size() >= 256) {
        return -1; // Map reached absolute capacity of 256 peds
    }

    uint16_t startSlot = static_cast<uint16_t>(factionId * 8);
    uint16_t endSlot = startSlot + 7;

    // Collect all ped IDs currently in use
    std::unordered_map<uint16_t, PedInstance*> idMap;
    for (PedInstance *p : peds_) {
        if (p && p != pPed) {
            idMap[p->id()] = p;
        }
    }

    // Pass 1: Check if any slot in [startSlot..endSlot] is completely unused
    for (uint16_t s = startSlot; s <= endSlot; ++s) {
        if (idMap.find(s) == idMap.end()) {
            if (pPed) pPed->setId(s);
            return s;
        }
    }

    // Pass 2: If all slots in [startSlot..endSlot] are occupied, find one occupied by a non-agent (civilian, guard, police)
    for (uint16_t s = startSlot; s <= endSlot; ++s) {
        PedInstance *occupant = idMap[s];
        if (occupant && occupant->type() != PedInstance::kPedTypeAgent) {
            // Find a free slot for the non-agent: prefer slots >= 64, then fallback to 8..63
            uint16_t newSlot = 64;
            while (newSlot <= 255 && idMap.find(newSlot) != idMap.end()) {
                newSlot++;
            }
            if (newSlot > 255) {
                newSlot = 8;
                while (newSlot < 64 && (idMap.find(newSlot) != idMap.end() || (newSlot >= startSlot && newSlot <= endSlot))) {
                    newSlot++;
                }
            }
            if (newSlot <= 255 && idMap.find(newSlot) == idMap.end()) {
                occupant->setId(newSlot);
                idMap.erase(s);
                idMap[newSlot] = occupant;
                if (pPed) pPed->setId(s);
                return s;
            }
        }
    }

    return -1;
}

PedInstance* Mission::spawnPlayerAgent(size_t squadSlot, const TilePoint &pos, uint8_t orientation) {
    if (!p_map_ || squadSlot >= Squad::kMaxSlot) return nullptr;

    uint16_t id = static_cast<uint16_t>(squadSlot);
    // If a player agent already exists at this squad slot, reposition and update orientation
    for (PedInstance *existing : peds_) {
        if (existing && existing->isOurAgent() && existing->id() == id) {
            existing->setPosition(pos.tx, pos.ty, pos.tz, pos.ox, pos.oy, pos.oz);
            existing->setDirection(orientation);
            setSquadSpawnPoint(squadSlot, TilePoint(pos.tx, pos.ty, pos.tz, pos.ox, pos.oy, pos.oz), orientation);
            return existing;
        }
    }

    int speed = PedManager::getStats(PedInstance::kPedTypeAgent, true).speed;
    int hp = PedManager::getStats(PedInstance::kPedTypeAgent, true).health;

    PedInstance *p = new PedInstance(id, p_map_, PedInstance::kPedTypeAgent, true, speed);
    p->setDirection(orientation);
    p->setStartHealth(hp);
    p->setHealth(hp);
    if (rs_eng::AnimationManager::singletonPtr() != nullptr) {
        p->setAnimations(1);
        p->goToState(kPedActionStateStanding, true);
    }
    p->setSize(32, 32, 256);
    p->setPosition(pos.tx, pos.ty, pos.tz, pos.ox, pos.oy, pos.oz);

    p->initAllLevelsForIPAType(IPAStim::Adrenaline, 0, 0, 0);
    p->initAllLevelsForIPAType(IPAStim::Intelligence, 0, 0, 0);
    p->initAllLevelsForIPAType(IPAStim::Perception, 0, 0, 0);

    p->setObjGroupDef(PedInstance::og_dmAgent);
    addPed(p);
    if (squad_) {
        squad_->setMember(squadSlot, p);
    }
    setSquadSpawnPoint(squadSlot, TilePoint(pos.tx, pos.ty, pos.tz, pos.ox, pos.oy, pos.oz), orientation);
    return p;
}

bool Mission::removePed(PedInstance *pPed) {
    if (!pPed) return false;
    auto it = std::find(peds_.begin(), peds_.end(), pPed);
    if (it == peds_.end()) return false;

    removeArmedPed(pPed);
    auto escIt = std::find(escapedPeds_.begin(), escapedPeds_.end(), pPed);
    if (escIt != escapedPeds_.end()) {
        escapedPeds_.erase(escIt);
    }

    if (squad_) {
        for (size_t i = 0; i < Squad::kMaxSlot; ++i) {
            if (squad_->member(i) == pPed) {
                squad_->setMember(i, nullptr);
            }
        }
    }

    for (Vehicle *v : vehicles_) {
        if (v) {
            v->dropPassenger(pPed);
            if (v->getDriver() == pPed) {
                v->setDriver(nullptr);
            }
        }
    }

    peds_.erase(it);
    delete pPed;
    return true;
}

Vehicle* Mission::spawnVehicle(uint8_t type, const TilePoint &pos, uint8_t orientation, uint16_t baseAnim) {
    if (!p_map_) return nullptr;

    uint16_t newId = 0;
    std::set<uint16_t> usedIds;
    for (Vehicle *v : vehicles_) {
        if (v) usedIds.insert(v->id());
    }
    while (usedIds.find(newId) != usedIds.end() && newId < 64) {
        newId++;
    }

    Vehicle *v = nullptr;
    if (type == Vehicle::kVehicleTypeTrainHead || type == Vehicle::kVehicleTypeTrainBody || type == 0xFE) {
        orientation = static_cast<uint8_t>(((orientation + 32) / 64 * 64) % 256);
    }
    bool isMoveOnXAxis = (orientation == 192 || orientation == 64);
    if (type == 0xFE) { // Train (Full Group)
        TrainHead *pHead = dynamic_cast<TrainHead*>(spawnVehicle(Vehicle::kVehicleTypeTrainHead, pos, orientation));
        if (!pHead) return nullptr;

        WorldPoint headW(pos);
        // Distance offsets along the movement axis behind the front engine.
        // Compensates for sprite pivot differences between Sprite 1687 (facing positive)
        // and Sprite 1683 (facing negative) to eliminate visual gaps in all diagonal directions.
        int dists[5];
        if (orientation == 64 || orientation == 0) {
            dists[0] = -232;
            dists[1] = -232 - 256;
            dists[2] = -232 - 512;
            dists[3] = -232 - 768;
            dists[4] = -232 - 768 - 184;
        } else {
            dists[0] = 184;
            dists[1] = 184 + 256;
            dists[2] = 184 + 512;
            dists[3] = 184 + 768;
            dists[4] = 184 + 768 + 232;
        }

        for (int step = 1; step <= 5; ++step) {
            WorldPoint segW = headW;
            if (isMoveOnXAxis) {
                segW.x += dists[step - 1];
            } else {
                segW.y += dists[step - 1];
            }
            if (segW.x < 0) segW.x = 0;
            if (segW.y < 0) segW.y = 0;

            TilePoint carriagePos(segW.x / 256, segW.y / 256, pos.tz, segW.x % 256, segW.y % 256, pos.oz);
            uint8_t cType = (step == 5) ? Vehicle::kVehicleTypeTrainHead : Vehicle::kVehicleTypeTrainBody;
            uint8_t cOri = (step == 5) ? static_cast<uint8_t>((orientation + 128) % 256) : orientation;
            // Authentic 6-car layout (e.g. Siberia):
            // Step 1: Windows (428), Step 2: Door (434), Step 3: Windows (428), Step 4: Raised Roof (440), Step 5: Rear Locomotive (416 facing backwards)
            uint16_t cAnim = 428;
            if (step == 2) cAnim = 434;
            else if (step == 4) cAnim = 440;
            else if (step == 5) cAnim = 416;

            Vehicle *pCarriage = spawnVehicle(cType, carriagePos, cOri, cAnim);
            if (pCarriage) {
                TrainBody *pBody = dynamic_cast<TrainBody*>(pCarriage);
                if (pBody) {
                    pHead->appendTrainBody(pBody);
                }
            }
        }
        return pHead;
    } else if (type == Vehicle::kVehicleTypeTrainHead) {
        const auto &headStats = VehicleManager::getStats(Vehicle::kVehicleTypeTrainHead);
        v = new TrainHead(newId, Vehicle::kVehicleTypeTrainHead, p_map_, headStats.health, isMoveOnXAxis, headStats.speed);
        v->setAnimations(headStats.base_anim);
        if (rs_eng::AnimationManager::singletonPtr() != nullptr) {
            v->playAnimation(v->regularAnimation());
        }
    } else if (type == Vehicle::kVehicleTypeTrainBody) {
        const auto &bodyStats = VehicleManager::getStats(Vehicle::kVehicleTypeTrainBody);
        v = new TrainBody(newId, Vehicle::kVehicleTypeTrainBody, p_map_, bodyStats.health, isMoveOnXAxis);
        uint16_t bAnim = (baseAnim != 0) ? baseAnim : bodyStats.base_anim;
        v->setAnimations(bAnim);
        if (rs_eng::AnimationManager::singletonPtr() != nullptr) {
            v->playAnimation(v->regularAnimation());
        }
    } else {
        const auto &vStats = VehicleManager::getStats(type);
        uint16_t bAnim = (baseAnim != 0) ? baseAnim : vStats.base_anim;
        GenericCar *car = new GenericCar(newId, type, p_map_, vStats.speed, vStats.acceleration);
        car->setHealth(vStats.health);
        car->setStartHealth(vStats.health);
        car->setAnimations(bAnim);
        if (rs_eng::AnimationManager::singletonPtr() != nullptr) {
            car->playAnimation(car->regularAnimation());
        }
        v = car;
    }

    if (v) {
        v->setSize(256, 256, 192);
        v->setPosition(pos.tx, pos.ty, pos.tz, pos.ox, pos.oy, pos.oz);
        v->setDirection(orientation);
        addVehicle(v);
    }
    return v;
}

bool Mission::removeVehicle(Vehicle *pVehicle) {
    if (!pVehicle) return false;
    auto it = std::find(vehicles_.begin(), vehicles_.end(), pVehicle);
    if (it == vehicles_.end()) return false;

    // Unlink train carriage pointers
    for (Vehicle *v : vehicles_) {
        if (auto *tb = dynamic_cast<TrainBody*>(v)) {
            if (tb->getNext() == pVehicle) {
                TrainBody *deletedTb = dynamic_cast<TrainBody*>(pVehicle);
                tb->setNext(deletedTb ? deletedTb->getNext() : nullptr);
            }
        }
    }

    vehicles_.erase(it);
    delete pVehicle;
    return true;
}

void Mission::moveVehicleToIndex(size_t fromIndex, size_t toIndex) {
    if (fromIndex >= vehicles_.size() || toIndex >= vehicles_.size() || fromIndex == toIndex) return;
    Vehicle *v = vehicles_[fromIndex];
    vehicles_.erase(vehicles_.begin() + fromIndex);
    vehicles_.insert(vehicles_.begin() + toIndex, v);
}

Static* Mission::spawnStatic(uint16_t staticId, const TilePoint &pos, uint16_t baseAnim, uint8_t orientation) {
    if (!p_map_) return nullptr;

    uint16_t newId = 0;
    std::set<uint16_t> usedIds;
    for (const auto &s : statics_) {
        if (s) usedIds.insert(s->id());
    }
    while (usedIds.find(newId) != usedIds.end() && newId < 400) {
        newId++;
    }

    LevelData::Statics sref;
    memset(&sref, 0, sizeof(sref));
    sref.desc = 0x04;
    sref.type = 0x03;
    sref.sub_type = static_cast<uint8_t>(staticId);
    sref.orientation = (orientation == Static::kStaticOrientationEW || orientation == 0x40) ? 0x40 : 0x00;
    sref.mapposx[0] = static_cast<uint8_t>(pos.ox);
    sref.mapposx[1] = static_cast<uint8_t>(pos.tx);
    sref.mapposy[0] = static_cast<uint8_t>(pos.oy);
    sref.mapposy[1] = static_cast<uint8_t>(pos.ty);
    rs_utl::WRITE_LE_UINT16(sref.mapposz, static_cast<uint16_t>((pos.tz << 7) | (pos.oz & 0x7F)));

    if (staticId == 0x01) { // Phone Booth
        uint16_t anim = (sref.orientation == 0x40) ? 619 : 618;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, anim);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    } else if (staticId >= 0x05 && staticId <= 0x08) { // Semaphore / Traffic light
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, 1040);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, 1040);
    } else if (staticId == 0x0A) { // Neon Sign
        uint16_t anim = (baseAnim != 0) ? baseAnim : 1045;
        if (sref.orientation == 0x40 && (anim % 2 != 0)) anim++;
        else if (sref.orientation == 0x00 && (anim % 2 == 0)) anim--;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, anim);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    } else if (staticId >= 0x0C && staticId <= 0x0F) { // Sliding Door
        uint16_t b = (baseAnim != 0) ? baseAnim : 1077;
        uint16_t famBase = (b >= 1077 && b <= 1115) ? (1077 + ((b - 1077) / 8) * 8) : b;
        uint16_t anim = (sref.orientation == 0x40) ? (famBase + 1) : famBase;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, anim);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    } else if (staticId == 0x12) { // Open Window
        uint16_t anim = (sref.orientation == 0x40) ? 553 : 552;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, anim);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    } else if (staticId == 0x13) { // Closed Window
        uint16_t anim = (sref.orientation == 0x40) ? 551 : 550;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, anim);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    } else if (staticId == 0x15) { // Damaged Window
        uint16_t anim = (sref.orientation == 0x40) ? 557 : 556;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, anim);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    } else if (staticId == 0x16) { // Tree / Plant
        uint16_t anim = (baseAnim != 0) ? baseAnim : 227;
        if (anim < 200) {
            if (anim == 1) anim = 227;
            else if (anim == 2) anim = 230;
            else if (anim == 3) anim = 233;
            else anim = 227;
        }
        uint16_t variant = (anim - 227) / 3 + 1;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, variant);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    } else if (staticId == 0x19) { // Mailbox (fixed swap)
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, 561);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, 561);
    } else if (staticId == 0x1A) { // Trash Bin (fixed swap)
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, 562);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, 562);
    } else if (staticId == 0x1F) { // Wall Advertisement
        uint16_t anim = (baseAnim != 0) ? baseAnim : 570;
        if (anim == 572 || anim == 573) anim = (sref.orientation == 0x40) ? 573 : 572;
        else if (anim == 574 || anim == 575) anim = (sref.orientation == 0x40) ? 575 : 574;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, anim);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    } else if (staticId == 0x20) { // Animated Window
        uint16_t anim = (baseAnim != 0) ? baseAnim : 1959;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, anim);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    } else if (staticId == 0x26) { // Large Door / Gate
        uint16_t anim = (sref.orientation == 0x40) ? 392 : 395;
        rs_utl::WRITE_LE_UINT16(sref.index_base_anim, anim);
        rs_utl::WRITE_LE_UINT16(sref.index_current_anim, anim);
    }

    Static *s = Static::loadInstance(reinterpret_cast<uint8_t*>(&sref), newId, p_map_);
    if (s) {
        s->setPosition(pos.tx, pos.ty, pos.tz, pos.ox, pos.oy, pos.oz);
        if (staticId == 0x16) {
            s->setBaseAnim(rs_utl::READ_LE_UINT16(sref.index_current_anim));
        } else {
            s->setBaseAnim(rs_utl::READ_LE_UINT16(sref.index_base_anim));
        }
        addStatic(std::unique_ptr<Static>(s));
        return s;
    }
    return nullptr;
}

bool Mission::removeStatic(Static *pStatic) {
    if (!pStatic) return false;
    for (auto it = statics_.begin(); it != statics_.end(); ++it) {
        if (it->get() == pStatic) {
            statics_.erase(it);
            return true;
        }
    }
    return false;
}

WeaponInstance* Mission::spawnGroundWeapon(Weapon *pWeapon, const TilePoint &pos) {
    if (!p_map_ || !pWeapon) return nullptr;
    WeaponInstance *wi = WeaponInstance::createInstance(pWeapon);
    if (!wi) return nullptr;
    wi->setMap(p_map_);
    wi->setPosition(pos.tx, pos.ty, pos.tz, pos.ox, pos.oy, pos.oz);
    wi->setDrawable(true);
    addWeaponToGround(wi);
    return wi;
}

WeaponInstance* Mission::spawnGroundWeapon(Weapon::WeaponType wType, const TilePoint &pos) {
    if (!p_map_) return nullptr;
    if (rs_knl::WeaponManager::singletonPtr() == nullptr) return nullptr;
    Weapon *pw = g_weaponMgr.getWeapon(wType);
    if (!pw) return nullptr;
    return spawnGroundWeapon(pw, pos);
}

bool Mission::removeGroundWeapon(WeaponInstance *pWeapon) {
    if (!pWeapon) return false;
    auto it = std::find(weaponsOnGround_.begin(), weaponsOnGround_.end(), pWeapon);
    if (it == weaponsOnGround_.end()) return false;
    weaponsOnGround_.erase(it);
    delete pWeapon;
    return true;
}

void Mission::setSquadSpawnPoint(size_t agentIndex, const TilePoint &pos, uint8_t orientation) {
    if (agentIndex >= 4) return;
    squadSpawnPoints_[agentIndex] = pos;
    squadSpawnOrientation_[agentIndex] = orientation;
    if (squad_) {
        PedInstance *p = squad_->member(agentIndex);
        if (p) {
            p->setPosition(pos.tx, pos.ty, pos.tz, pos.ox, pos.oy, pos.oz);
            p->setDirection(orientation);
        }
    }
}

TilePoint Mission::getSquadSpawnPoint(size_t agentIndex) const {
    if (agentIndex >= 4) return TilePoint(0, 0, 0);
    return squadSpawnPoints_[agentIndex];
}

uint8_t Mission::getSquadSpawnOrientation(size_t agentIndex) const {
    if (agentIndex >= 4) return 0;
    return squadSpawnOrientation_[agentIndex];
}

bool Mission::hasCustomTactics() const {
    const auto &t = rawLevelData_.tactics;
    for (int i = 0; i < 128; ++i) {
        if (t.entries[i].mode > 0 && !(t.entries[i].sub_filter & LevelData::kAITacticFlagDisabled)) {
            return true;
        }
    }
    return false;
}

size_t Mission::numTactics() const {
    size_t count = 0;
    const auto &t = rawLevelData_.tactics;
    for (int i = 0; i < 128; ++i) {
        if (t.entries[i].mode > 0 && !(t.entries[i].sub_filter & LevelData::kAITacticFlagDisabled)) {
            count++;
        }
    }
    return count;
}

bool Mission::isCombatSweep() const {
    for (int objIdx = 0; objIdx < 6; ++objIdx) {
        uint16_t objType = rs_utl::READ_LE_UINT16(rawLevelData_.objectives[objIdx].type);
        if (objType == LevelData::kObjectiveTypeCombatSweep || objType == LevelData::kObjectiveTypeCombatSweepPolice) {
            return true;
        }
    }
    return false;
}

int Mission::getRootTacticIndex(uint8_t factionId) const {
    const auto &t = rawLevelData_.tactics;

    // Resolve a root tactic index (1..127) past any disabled nodes along next_tactic;
    // index 0 is reserved as the final fallback when no tactic in 1..127 applies.
    auto resolveEffective = [&](int rootIdx) -> int {
        if (rootIdx == 0) {
            if (t.entries[0].mode > 0 && !(t.entries[0].sub_filter & LevelData::kAITacticFlagDisabled)) {
                return 0;
            }
            return -1;
        }
        int cur = rootIdx;
        bool visited[128] = {};
        while (cur > 0 && cur < 128 && !visited[cur]) {
            visited[cur] = true;
            if (t.entries[cur].mode > 0 && !(t.entries[cur].sub_filter & LevelData::kAITacticFlagDisabled)) {
                return cur;
            }
            cur = rs_utl::READ_LE_UINT16(t.entries[cur].next_tactic);
        }
        return -1;
    };

    if (factionId == LevelData::kAIFactionWildcard) {
        for (int i = 1; i < 128; ++i) {
            if (t.entries[i].mode > 0 && t.entries[i].faction_id == LevelData::kAIFactionWildcard) {
                uint16_t prev = rs_utl::READ_LE_UINT16(t.entries[i].prev_tactic);
                if (prev == 0) {
                    int eff = resolveEffective(i);
                    if (eff >= 0) return eff;
                }
            }
        }
        if (t.entries[0].faction_id == LevelData::kAIFactionWildcard) {
            return resolveEffective(0);
        }
        return -1;
    }

    // Replicate original Bullfrog Syndicate tactic assignment (syndicat_main 0x14588):
    // Squads 1..N claim unclaimed root tactics (1..127) sequentially; once all matching
    // root tactics have been claimed, remaining squads wrap around to reuse an already-claimed
    // root in 1..127, only falling back to entry 0 if no tactic in 1..127 matches.
    bool claimed[128] = {};
    uint8_t target = (factionId <= 7) ? factionId : static_cast<uint8_t>(1);
    int result = -1;

    for (uint8_t s = 1; s <= target; ++s) {
        int assigned = -1;

        auto findRoot = [&](uint8_t matchFaction, bool requireUnclaimed) -> int {
            for (int i = 1; i < 128; ++i) {
                if (requireUnclaimed && claimed[i]) continue;
                if (t.entries[i].mode > 0 && t.entries[i].faction_id == matchFaction) {
                    uint16_t prev = rs_utl::READ_LE_UINT16(t.entries[i].prev_tactic);
                    if (prev == 0) {
                        int eff = resolveEffective(i);
                        if (eff >= 0) {
                            if (requireUnclaimed) claimed[i] = true;
                            return eff;
                        }
                    }
                }
            }
            return -1;
        };

        // Pass 1: unclaimed explicit faction, then unclaimed wildcard
        assigned = findRoot(s, true);
        if (assigned < 0) assigned = findRoot(LevelData::kAIFactionWildcard, true);

        // Pass 2: reuse already-claimed explicit faction or wildcard in 1..127
        if (assigned < 0) assigned = findRoot(s, false);
        if (assigned < 0) assigned = findRoot(LevelData::kAIFactionWildcard, false);

        // Fallback: entry 0 (Tactic 0)
        if (assigned < 0 && (t.entries[0].faction_id == s || t.entries[0].faction_id == LevelData::kAIFactionWildcard)) {
            assigned = resolveEffective(0);
        }

        if (s == target) {
            result = assigned;
        }
    }

    return result;
}

std::vector<int> Mission::getTacticChain(uint8_t factionId) const {
    std::vector<int> chain;
    int root = -1;
    for (int i = 1; i < 128; ++i) {
        const auto &e = rawLevelData_.tactics.entries[i];
        if (e.mode > 0) {
            uint16_t prev = rs_utl::READ_LE_UINT16(e.prev_tactic);
            if (prev == 0 && (e.faction_id == factionId || (factionId == LevelData::kAIFactionWildcard && e.faction_id == LevelData::kAIFactionWildcard))) {
                root = i;
                break;
            }
        }
    }
    if (root <= 0) return chain;

    int cur = root;
    std::vector<bool> visited(128, false);
    while (cur > 0 && cur < 128 && !visited[cur]) {
        visited[cur] = true;
        chain.push_back(cur);
        cur = rs_utl::READ_LE_UINT16(rawLevelData_.tactics.entries[cur].next_tactic);
    }

    uint8_t rootFaction = rawLevelData_.tactics.entries[root].faction_id;
    for (int i = 1; i < 128; ++i) {
        if (!visited[i] && rawLevelData_.tactics.entries[i].mode > 0 &&
            (rawLevelData_.tactics.entries[i].faction_id == factionId ||
             (rootFaction == LevelData::kAIFactionWildcard && rawLevelData_.tactics.entries[i].faction_id == LevelData::kAIFactionWildcard))) {
            chain.push_back(i);
        }
    }

    return chain;
}

int Mission::addTactic(uint8_t factionId, uint8_t mode, uint16_t p1, uint16_t p2, uint16_t p3, int prevIndex) {
    auto &t = rawLevelData_.tactics;
    int slot = -1;
    for (int i = 1; i < 128; ++i) {
        if (t.entries[i].mode == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1) return -1;

    auto &e = t.entries[slot];
    memset(&e, 0, sizeof(e));
    e.mode = mode;
    e.sub_mode = 0;
    rs_utl::WRITE_LE_UINT16(e.param1, p1);
    rs_utl::WRITE_LE_UINT16(e.param2, p2);
    rs_utl::WRITE_LE_UINT16(e.param3, p3);
    e.faction_id = factionId;
    e.sub_filter = 0;

    if (prevIndex >= 0 && prevIndex < 128) {
        rs_utl::WRITE_LE_UINT16(e.prev_tactic, static_cast<uint16_t>(prevIndex));
        rs_utl::WRITE_LE_UINT16(t.entries[prevIndex].next_tactic, static_cast<uint16_t>(slot));
    } else {
        rs_utl::WRITE_LE_UINT16(e.prev_tactic, 0);
    }
    rs_utl::WRITE_LE_UINT16(e.next_tactic, 0);

    return slot;
}

bool Mission::deleteTactic(int index) {
    if (index < 0 || index >= 128) return false;
    auto &t = rawLevelData_.tactics;
    uint16_t prev = rs_utl::READ_LE_UINT16(t.entries[index].prev_tactic);
    uint16_t next = rs_utl::READ_LE_UINT16(t.entries[index].next_tactic);

    if (prev < 128 && index != 0) {
        rs_utl::WRITE_LE_UINT16(t.entries[prev].next_tactic, next);
    }
    if (next > 0 && next < 128) {
        rs_utl::WRITE_LE_UINT16(t.entries[next].prev_tactic, prev);
    }

    memset(&t.entries[index], 0, sizeof(t.entries[index]));
    return true;
}

void Mission::clearTactics(int factionId) {
    auto &t = rawLevelData_.tactics;
    for (int i = 0; i < 128; ++i) {
        if (factionId == -1 || t.entries[i].faction_id == factionId) {
            memset(&t.entries[i], 0, sizeof(t.entries[i]));
        }
    }
}

void Mission::initDefaultTactics() {
    auto &t = rawLevelData_.tactics;
    memset(&t, 0, sizeof(t));
    t.header.num_squads = 5;
    t.header.peds_per_squad = 8;
    t.header.max_rush_factions = 0;

    for (int i = 0; i < 128; ++i) {
        t.entries[i].sub_filter = LevelData::kAITacticFlagDisabled;
    }
}

void Mission::updateSquadTactic(uint8_t factionId, int nextTacticIdx) {
    for (size_t i = 0; i < numPeds(); ++i) {
        PedInstance *p = ped(i);
        if (p && p->type() == PedInstance::kPedTypeAgent && p->factionId() == factionId) {
            auto *pHostile = p->behaviour().getComponent<PlayerHostileBehaviourComponent>();
            if (pHostile) {
                pHostile->initFromTactic(this, nextTacticIdx);
                p->setHunter(pHostile->isHunter());
                if (pHostile->isHunter()) {
                    p->setIPAAmount(IPAStim::Adrenaline, 100);
                    p->clearDestination();
                    p->destroyAllActions(true);
                }
            }
        }
    }
}

}