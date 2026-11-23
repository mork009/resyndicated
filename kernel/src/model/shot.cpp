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

#include "rs-kernel/model/shot.h"

#include <cmath>
#include <map>

#include "rs-engine/sound/soundmanager.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/vehicle.h"

namespace rs_knl {

void InstantImpactShot::inflictDamage(Mission *pMission) {
    if (!dmg_.pWeapon || !dmg_.pWeapon->getClass()) return;

    // get how much impacts does the weapon generate
    int nbImpacts = dmg_.pWeapon->getClass()->impactsPerAmmo();

    // If there are many impacts, a target can be hit by several impacts
    // so this map stores number of impacts for a target
    std::map<ShootableMapObject *, int> hitsByObject;

    for (int i = 0; i < nbImpacts; ++i) {
        WorldPoint impactPosW = dmg_.aimedLocW;

        if (nbImpacts > 1) {
            // When multiple impacts, they're spread
            diffuseImpact(pMission, dmg_.originLocW, &impactPosW);
        }

        ShootableMapObject *pTargetHit = NULL;
        if (pMission) {
            // Verify if shot hit something or was blocked by a tile
            Mission::BlockerCriteria crits;
            crits.setBlocker = true;
            crits.maxr = dmg_.pWeapon->range();
            crits.pOrigin = dmg_.d_owner;
            pMission->checkIfBlockersInShootingLine(dmg_.originLocW, crits, &pTargetHit, &impactPosW);
        }

        if (pTargetHit != NULL) {
            hitsByObject[pTargetHit] = hitsByObject[pTargetHit] + 1;
        }
        // creates impact sprite
        createImpactAnimation(pMission, pTargetHit, impactPosW);
    }

    // finally distribute damage
    std::map<ShootableMapObject *, int>::iterator it;
    for (it = hitsByObject.begin(); it != hitsByObject.end(); it++) {
        dmg_.dvalue = (*it).second * dmg_.pWeapon->getClass()->damagePerShot();
        (*it).first->handleHit(dmg_);

        if (pMission && dmg_.pWeapon->owner() && dmg_.pWeapon->owner()->isOurAgent()) {
            // we don't differentiate hitting a tree and a ped
            pMission->stats()->incrHits();
        }
    }
}

void InstantImpactShot::diffuseImpact([[maybe_unused]] Mission *pMission, const WorldPoint &originLocW, WorldPoint *pImpactPosW) {
    if (!dmg_.pWeapon || !dmg_.pWeapon->getClass() || !pImpactPosW)
        return;

    PedInstance *pShooter = dynamic_cast<PedInstance *>(dmg_.d_owner);
    if (!pShooter && dmg_.pWeapon) {
        pShooter = dmg_.pWeapon->owner();
    }

    double accuracy = 0.5;
    if (pShooter) {
        pShooter->getAccuracy(accuracy);
    }

    Weapon *pWeaponClass = dmg_.pWeapon->getClass();
    double effectiveSpreadAngleDeg = pWeaponClass->effectiveSpreadAngle(accuracy);
    double effectiveSpreadDepth = pWeaponClass->effectiveSpreadDepth(accuracy);

    double spreadAngleRad = (effectiveSpreadAngleDeg * rs_utl::kPI) / 180.0;

    if (spreadAngleRad <= 0.0001 && effectiveSpreadDepth <= 0.0001)
        return;

    double dx = static_cast<double>(pImpactPosW->x - originLocW.x);
    double dy = static_cast<double>(pImpactPosW->y - originLocW.y);
    double d2D = sqrt(dx * dx + dy * dy);

    if (d2D <= 1.0)
        return;

    // Unit direction vector in horizontal plane
    double ux = dx / d2D;
    double uy = dy / d2D;

    // Perpendicular (lateral) unit vector across the shot line
    double perpX = -uy;
    double perpY = ux;

    // Random lateral deflection angle in [-spreadAngleRad, +spreadAngleRad]
    double randLateral = (static_cast<double>(rand() % 2001) - 1000.0) / 1000.0; // [-1.0, +1.0]
    double theta = randLateral * spreadAngleRad;
    double lateralOffset = d2D * tan(theta);

    // Random front-to-back longitudinal deflection in [-effectiveSpreadDepth, +effectiveSpreadDepth]
    double randLongitudinal = (static_cast<double>(rand() % 2001) - 1000.0) / 1000.0; // [-1.0, +1.0]
    double longitudinalOffset = randLongitudinal * effectiveSpreadDepth;

    pImpactPosW->x += static_cast<int>(round(lateralOffset * perpX + longitudinalOffset * ux));
    pImpactPosW->y += static_cast<int>(round(lateralOffset * perpY + longitudinalOffset * uy));
    // Note: z is preserved at target elevation so all impacts in a burst strike at the target plane!
}

/*!
 * Creates the impact animation based on the type of target hit.
 * Some weapons can have no animations for an impact.
 */
void InstantImpactShot::createImpactAnimation(Mission *pMission, ShootableMapObject * pTargetHit, const WorldPoint impactPosW) {
    SFXObject::SfxType impactAnimId =
        (pTargetHit != NULL ?
            dmg_.pWeapon->getClass()->impactAnims()->objectHit :
            dmg_.pWeapon->getClass()->impactAnims()->groundHit);

        if (pMission && impactAnimId != SFXObject::sfxt_Unknown) {
            auto sfx = std::make_unique<SFXObject>(pMission->map(), impactAnimId);
            sfx->setPosition(impactPosW);
            sfx->correctZ(pMission->map()->maxTz());
            pMission->addSfxObject(std::move(sfx));
        }
}

/*!
 * Convenient method to create explosions.
 * \param pMission Mission data
 * \param pOwner What's at the origin of the explosion
 * \param The range for damage
 * \param The value of the damage
 */
void Explosion::createExplosion(Mission *pMission, ShootableMapObject *pOwner, double range, int dmgValue) {
    WorldPoint location(pOwner->position());

    createExplosion(pMission, pOwner, location, range, dmgValue);
}

void Explosion::createExplosion(Mission *pMission, ShootableMapObject *pOwner, const WorldPoint &location, double range, int dmgValue) {
    DamageToInflict dmg;
    if (pOwner && pOwner->is(MapObject::kNatureWeapon)) {
        // Explosion is coming from a Bomb or a GaussGun
        dmg.pWeapon = dynamic_cast<WeaponInstance *>(pOwner);
    } else {
        // Else it can be car or an agent suicide
        dmg.pWeapon = NULL;
    }

    dmg.d_owner = pOwner;
    dmg.dtype = kDmgTypeExplosion;
    dmg.range = range;
    dmg.dvalue =  dmgValue;
    dmg.originLocW = location;
    dmg.originLocW.z += 8;

    Explosion explosion(dmg);
    explosion.inflictDamage(pMission);
}

Explosion::Explosion(const DamageToInflict &dmg) : Shot(dmg) {
    // GaussGun has a different animation for explosion
    if (dmg_.pWeapon && dmg_.pWeapon->isInstanceOf(Weapon::GaussGun)) {
        rngDmgAnim_ = SFXObject::sfxt_LargeFire;
    } else {
        rngDmgAnim_ = SFXObject::sfxt_ExplosionFire;
    }
};

/*!
 *
 */
void Explosion::inflictDamage(Mission *pMission) {
    std::vector<ShootableMapObject *> objInRangeLst;
    // Get all destructible objects in range
    getAllShootablesWithinRange(pMission, dmg_.originLocW, objInRangeLst);

    bool updateStat = false;
    bool isGauss = (dmg_.pWeapon && dmg_.pWeapon->isInstanceOf(Weapon::GaussGun));
    if (isGauss) {
        // We only count explosions from Gaussgun in stats
        // Gaussgun should always have a PedInstance as an owner
        if (dmg_.pWeapon->owner() && dmg_.pWeapon->owner()->isOurAgent()) {
            updateStat = true;
        }
    }

    // Always draw an explosion ball at the center of explosion
    auto centerExplosion = std::make_unique<SFXObject>(pMission->map(), SFXObject::sfxt_ExplosionBall);
    centerExplosion->setPosition(dmg_.originLocW);
    centerExplosion->correctZ(pMission->map()->maxTz());
    pMission->addSfxObject(std::move(centerExplosion));

    for (std::vector<ShootableMapObject *>::iterator it = objInRangeLst.begin();
        it != objInRangeLst.end(); it++)
    {
        ShootableMapObject *smo = *it;
        // distribute damage
        smo->handleHit(dmg_);
        if (updateStat && smo->is(MapObject::kNaturePed)) {
            // We update stats on when a ped is damaged and we stop
            // at the first ped otherwise hit percentage is > 100% for 1 shot
            pMission->stats()->incrHits();
            updateStat = false;
        }
        // draw an explosion ball above each object that was hit (if not already at center)
        WorldPoint smoPos(smo->position());
        if (smoPos.x != dmg_.originLocW.x || smoPos.y != dmg_.originLocW.y) {
            auto sfx = std::make_unique<SFXObject>(pMission->map(), SFXObject::sfxt_ExplosionBall);
            sfx->setPosition(smoPos);
            sfx->correctZ(pMission->map()->maxTz());
            pMission->addSfxObject(std::move(sfx));
        }
    }

    if (isGauss) {
        // Gauss gun explosion: organic cluster of tall roaring fire pillars (sfxt_LargeFire)
        generateGaussImpactFlames(pMission, &(dmg_.originLocW), dmg_.range);
    } else {
        // Time bomb and other explosions: expanding shockwave flame waves
        generateFlameWaves(pMission, &(dmg_.originLocW), dmg_.range);
    }

    if (rs_eng::SoundManager::singletonPtr()) {
        g_SoundMgr.play(rs_eng::EXPLOSION_BIG);
    }
}

/*! Draws organic cluster of tall roaring fire pillars at gauss gun impact point
 * \param pMission Mission data
 * \param pOrigin center of explosion
 * \param dmg_rng effective range for drawing
 */
void Explosion::generateGaussImpactFlames(Mission *pMission, WorldPoint *pOrigin, double dmg_rng)
{
    WorldPoint base_pos = *pOrigin;
    pOrigin->z += 4;
    if (pOrigin->z > (pMission->mmax_z_ - 1) * 128)
        pOrigin->z = (pMission->mmax_z_ - 1) * 128;

    // Spawn central large fire pillar at impact epicenter
    auto centerFlame = std::make_unique<SFXObject>(pMission->map(),
                                                   SFXObject::sfxt_LargeFire, true,
                                                   100 * (rand() % 16) + (rand() % 400));
    centerFlame->setPosition(base_pos);
    pMission->addSfxObject(std::move(centerFlame));

    // Gauss explosion produces an organic cluster of tall roaring fire pillars (sfxt_LargeFire)
    // scattered organically around the impact point, rather than geometric concentric rings.
    const int numFlames = 12 + (rand() % 3); // 12 to 14 flames
    const double maxRadius = std::min(dmg_rng * 0.65, 200.0);

    for (int i = 0; i < numFlames; i++) {
        // Distribute in a natural organic cluster using polar coordinates with radial scatter
        double angle = ((double)rand() / (double)RAND_MAX) * 2.0 * rs_utl::kPI;
        // Sqrt distribution gives even areal density across the circular footprint
        double dist = (0.20 + 0.80 * sqrt((double)rand() / (double)RAND_MAX)) * maxRadius;
        int jitterX = (rand() % 25) - 12;
        int jitterY = (rand() % 25) - 12;

        WorldPoint flamePosW;
        flamePosW.x = base_pos.x + (int)(dist * cos(angle)) + jitterX;
        flamePosW.y = base_pos.y + (int)(dist * sin(angle)) + jitterY;
        flamePosW.z = base_pos.z;

        uint8_t block_mask = pMission->checkBlockedByTile(*pOrigin, &flamePosW, true, dmg_rng);
        if (block_mask != 32) {
            auto sfx = std::make_unique<SFXObject>(pMission->map(),
                                                   SFXObject::sfxt_LargeFire, true,
                                                   100 * (rand() % 16) + (rand() % 400));
            sfx->setPosition(flamePosW);
            pMission->addSfxObject(std::move(sfx));
        }
    }
}

/*! Draws animation of impact/explosion
 * \param pMission Mission data
 * \param pOrigin center of explosion
 * \param dmg_rng effective range for drawing
 */
void Explosion::generateFlameWaves(Mission *pMission, WorldPoint *pOrigin, double dmg_rng)
{
    WorldPoint base_pos = *pOrigin;
    pOrigin->z += 4;
    if (pOrigin->z > (pMission->mmax_z_ - 1) * 128)
        pOrigin->z = (pMission->mmax_z_ - 1) * 128;
    // TODO: exclude flames on water, put these flames to the ground,
    // don't draw in air(, stairs problem?)
    double angle_inc = rs_utl::kPI;
    const uint8_t waves = (int)dmg_rng / 144 + 1;

    for (uint8_t i = 0; i < waves; i++) {
        double base_angle = 0.0;
        if (rand() % 100 > 74)
            base_angle += angle_inc;

        for (int j = 0; j < (4 << i); j++) {
            double x = (double)(144 * i) * cos(base_angle);
            double y = (double)(144 * i) * sin(base_angle);
            base_angle += angle_inc;
            WorldPoint flamePosW;
            flamePosW.x = base_pos.x + (int)x;
            flamePosW.y = base_pos.y + (int)y;
            flamePosW.z = base_pos.z;

            uint8_t block_mask = pMission->checkBlockedByTile(*pOrigin, &flamePosW, true, dmg_rng);
            if (block_mask != 32) {
                auto sfx = std::make_unique<SFXObject>(pMission->map(),
                                                        rngDmgAnim_, true, 100 * (rand() % 16));
                sfx->setPosition(flamePosW);
                pMission->addSfxObject(std::move(sfx));
            }
        }
        angle_inc /= 2.0;
    }
}

/*!
 * Returns all ShootableMapObject that are alive and in range of
 * weapon who generated this explosion.
 * Every object found is added to the objInRangeVec vector.
 * \param pMission Mission data
 * \param originLocW Origin of explosion
 * \param objInRangeVec Result list
 */
void Explosion::getAllShootablesWithinRange(Mission *pMission,
                                       const WorldPoint &originLocW,
                                       std::vector<ShootableMapObject *> &objInRangeVec) {
    // Look at all peds alive, in range of explosion and not in a vehicle
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        PedInstance *p = pMission->ped(i);
        if (p->isAlive() && p->isCloseTo(originLocW, dmg_.range) && p->inVehicle() == NULL) {
            WorldPoint pedPosW(p->position());
            if (pMission->checkBlockedByTile(originLocW, &pedPosW, false, dmg_.range) == 1) {
                objInRangeVec.push_back(p);
            }
        }
    }

    for (size_t i = 0; i < pMission->numStatics(); ++i) {
        Static *st = pMission->statics(i);
        if (st->canBlock() && st->isAlive() && st->isCloseTo(originLocW, dmg_.range)) {
            WorldPoint staticPosW(st->position());
            if (pMission->checkBlockedByTile(originLocW, &staticPosW, false, dmg_.range) == 1) {
                objInRangeVec.push_back(st);
            }
        }
    }

    // look at all vehicles
    for (size_t i = 0; i < pMission->numVehicles(); ++i) {
        ShootableMapObject *v = pMission->vehicle(i);
        if (v->isAlive() && v->isCloseTo(originLocW, dmg_.range)) {
            WorldPoint vehiclePosW(v->position());
            if (pMission->checkBlockedByTile(originLocW, &vehiclePosW, false, dmg_.range) == 1) {
                objInRangeVec.push_back(v);
            }
        }
    }

    // look at all bombs on the ground except the weapon that generated the shot
    for (size_t i = 0; i < pMission->numWeaponsOnGround(); ++i) {
        WeaponInstance *w = pMission->weaponOnGround(i);
        if (w->isInstanceOf(Weapon::TimeBomb) && w != dmg_.pWeapon && !w->hasOwner() && w->isAlive()) {
            WorldPoint weaponPosW(w->position());
            if (pMission->checkBlockedByTile(originLocW, &weaponPosW, false, dmg_.range) == 1) {
                objInRangeVec.push_back(w);
            }
        }
    }
}

ProjectileShot::ProjectileShot(const DamageToInflict &dmg) : Shot(dmg) {
    startMove_ = false;
    curPosW_ = dmg.originLocW;
    currentDistance_ = 0;
    lifeOver_ = false;
    drawImpact_ = false;
    pShootableHit_ = NULL;

    speed_ = dmg.pWeapon->getClass()->shotSpeed();
    // distance from origin of shoot to target on each axis
    targetLocW_.x = dmg.aimedLocW.x;
    targetLocW_.y = dmg.aimedLocW.y;
    targetLocW_.z = dmg.aimedLocW.z;

    double diffx = (double)(targetLocW_.x - curPosW_.x);
    double diffy = (double)(targetLocW_.y - curPosW_.y);
    double diffz = (double)(targetLocW_.z - curPosW_.z);

    double distanceToTarget  = sqrt(diffx * diffx + diffy * diffy + diffz * diffz);
    if (distanceToTarget != 0) {
        incX_ = diffx / distanceToTarget;
        incY_ = diffy / distanceToTarget;
        incZ_ = diffz / distanceToTarget;
    }

    distanceMax_ = dmg.pWeapon->getClass()->range();
    if (distanceToTarget < distanceMax_) {
        distanceMax_ = distanceToTarget;
    }
}

bool ProjectileShot::animate(uint32_t elapsed, Mission *pMission) {
    if (!startMove_) {
        // It's the first time the animate method is called since shot
        // was created : start counting
        startMove_ = true;
        return false;
    }

    if (moveProjectile(elapsed, pMission)) {
        inflictDamage(pMission);
    }

    return true;
}

/*!
 * Updates the projectile position and animation of trace.
 * \param elapsed Time elapsed since last frame.
 * \param pMission Mission data
 * \return True if projectile has reached its target or max distance.
 */
bool ProjectileShot::moveProjectile(uint32_t elapsed, Mission *pMission) {
    bool endMove = false;

    // Distance crossed in the elapsed time
    double inc_dist = speed_ * (double)elapsed / 1000;
    if ((currentDistance_ + inc_dist) > distanceMax_) {
        // Projectile reached the maximum distance
        if (currentDistance_ > distanceMax_) {
            currentDistance_ = distanceMax_;
        }
        inc_dist = distanceMax_ - currentDistance_;
        endMove = true;
    }

    // This is the distance after the move
    double nextDist = currentDistance_ + inc_dist;
    WorldPoint nextPosW; // This is the position of projectile after this move
    bool do_recalc = false;

    nextPosW.x = dmg_.originLocW.x + (int)(incX_ * nextDist);
    if (nextPosW.x < 0) {
        nextPosW.x = 0;
        endMove = true;
        do_recalc = true;
    } else if (nextPosW.x > (pMission->mmax_x_ - 1) * 256) {
        nextPosW.x = (pMission->mmax_x_ - 1) * 256;
        endMove = true;
        do_recalc = true;
    }
    if (do_recalc) {
        do_recalc = false;
        if (incX_ != 0) {
            nextDist = (double)(nextPosW.x - dmg_.originLocW.x) / incX_;
        }
    }

    nextPosW.y = dmg_.originLocW.y + (int)(incY_ * nextDist);
    if (nextPosW.y < 0) {
        nextPosW.y = 0;
        endMove = true;
        do_recalc = true;
    } else if (nextPosW.y > (pMission->mmax_y_ - 1) * 256) {
        nextPosW.y = (pMission->mmax_y_ - 1) * 256;
        endMove = true;
        do_recalc = true;
    }
    if (do_recalc) {
        do_recalc = false;
        if (incY_ != 0) {
            nextDist = (double)(nextPosW.y - dmg_.originLocW.y) / incY_;
            nextPosW.x = dmg_.originLocW.x + (int)(incX_ * nextDist);
        }
    }

    nextPosW.z = dmg_.originLocW.z + (int)(incZ_ * nextDist);
    if (nextPosW.z < 0) {
        nextPosW.z = 0;
        endMove = true;
        do_recalc = true;
    } else if (nextPosW.z > (pMission->mmax_z_ - 1) * 128) {
        nextPosW.z = (pMission->mmax_z_ - 1) * 128;
        endMove = true;
        do_recalc = true;
    }
    if (do_recalc) {
        if (incZ_ != 0) {
            nextDist = (double)(nextPosW.z - dmg_.originLocW.z) / incZ_;
            nextPosW.x = dmg_.originLocW.x + (int)(incX_ * nextDist);
            nextPosW.y = dmg_.originLocW.y + (int)(incY_ * nextDist);
        }
    }


    Mission::BlockerCriteria crits;
    crits.setBlocker = true;
    // maxr here is set to maximum that projectile can fly from its
    // current position
    crits.maxr = distanceMax_ - currentDistance_;
    crits.pOrigin = dmg_.d_owner;
    uint8_t block_mask = pMission->checkIfBlockersInShootingLine(curPosW_, crits,
                                                                    &pShootableHit_, &nextPosW);

    if (block_mask == 1) {
        // Projectile has reached initial target
        if (nextPosW.equals(targetLocW_)) {
            // we can stop the move and draw the explosion
            drawImpact_ = true;
            endMove = true;
        }
    } else if (block_mask == 32) {
        // projectile is out of map : do not draw explosion
        // not sure if necessary
        endMove = true;
    } else {
        // projectile has hit something
        drawImpact_ = true;
        endMove = true;
    }

    curPosW_ = nextPosW;
    currentDistance_ = nextDist;
    drawTrace(pMission);

    return endMove;
}

GaussGunShot::GaussGunShot(const DamageToInflict &dmg) : ProjectileShot(dmg) {
    lastAnimDist_ = 0;
}

void GaussGunShot::inflictDamage(Mission *pMission) {
    lifeOver_ = true;

    if (drawImpact_) {
        int dmgRange = dmg_.pWeapon->getClass()->rangeDmg();
        Explosion::createExplosion(pMission, dmg_.pWeapon, curPosW_, dmgRange, dmg_.dvalue);
    }
}

/*!
 * Draws the animation of smoke behind the projectile.
 * \param pMission Mission data
 */
void GaussGunShot::drawTrace(Mission *pMission) {
    // distance between 2 animations
    double anim_d = 64;

    double diffx = (double) (dmg_.originLocW.x - curPosW_.x);
    double diffy = (double) (dmg_.originLocW.y - curPosW_.y);
    double diffz = (double) (dmg_.originLocW.z - curPosW_.z);
    double d = sqrt(diffx * diffx + diffy * diffy
        + diffz * diffz);

    if (d > lastAnimDist_) {
        int diff_dist = (int) ((d - lastAnimDist_) / anim_d);
        if (diff_dist != 0) {
            for (int i = 1; i <= diff_dist; i++) {
                WorldPoint t;
                lastAnimDist_ += anim_d;
                t.x = dmg_.originLocW.x + (int)(lastAnimDist_ * incX_);
                t.y = dmg_.originLocW.y + (int)(lastAnimDist_ * incY_);
                t.z = dmg_.originLocW.z + (int)(lastAnimDist_ * incZ_);

                t.z += 128;
                if (t.z > (pMission->mmax_z_ - 1) * 128)
                    t.z = (pMission->mmax_z_ - 1) * 128;

                auto sfx = std::make_unique<SFXObject>(pMission->map(), 
                                dmg_.pWeapon->getClass()->impactAnims()->trace_anim);
                sfx->setPosition(t);
                pMission->addSfxObject(std::move(sfx));
            }
        }
    }
}

FlamerShot::FlamerShot(Mission *pMission, const DamageToInflict &dmg) :
        ProjectileShot(dmg) {
    lastAnimDist_ = 0.0;
    traveledDist_ = 0.0;
    totalLength_ = 0.0;

    // Normalize direction cleanly from barrel origin to aimed target
    double dx = static_cast<double>(dmg.aimedLocW.x - dmg.originLocW.x);
    double dy = static_cast<double>(dmg.aimedLocW.y - dmg.originLocW.y);
    double dz = static_cast<double>(dmg.aimedLocW.z - dmg.originLocW.z);
    double d = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (d > 0.0) {
        incX_ = dx / d;
        incY_ = dy / d;
        incZ_ = dz / d;
    } else {
        incX_ = 1.0;
        incY_ = 0.0;
        incZ_ = 0.0;
    }

    trailPoints_.push_back(dmg.originLocW);
    cumulativeLens_.push_back(0.0);

    auto appendPoint = [&](const WorldPoint &pt) {
        const WorldPoint &prev = trailPoints_.back();
        double sx = static_cast<double>(pt.x - prev.x);
        double sy = static_cast<double>(pt.y - prev.y);
        double sz = static_cast<double>(pt.z - prev.z);
        double segLen = std::sqrt(sx * sx + sy * sy + sz * sz);
        if (segLen > 0.0) {
            totalLength_ += segLen;
            trailPoints_.push_back(pt);
            cumulativeLens_.push_back(totalLength_);
        }
    };

    // Extend initial +96 from barrel (+64 from ShootAction) to match DOS +160 muzzle offset,
    // then walk (range / 38) + 1 = 14 DOS wind-modulated substeps along the stream path.
    constexpr double kBarrelToMuzzle = 96.0;
    double px = static_cast<double>(dmg.originLocW.x) + incX_ * kBarrelToMuzzle;
    double py = static_cast<double>(dmg.originLocW.y) + incY_ * kBarrelToMuzzle;
    double pz = static_cast<double>(dmg.originLocW.z) + incZ_ * kBarrelToMuzzle;

    if (px < 0.0) px = 0.0;
    if (py < 0.0) py = 0.0;
    if (pz < 0.0) pz = 0.0;
    if (pMission) {
        if (pMission->mmax_x_ > 0 && px > static_cast<double>((pMission->mmax_x_ - 1) * 256))
            px = static_cast<double>((pMission->mmax_x_ - 1) * 256);
        if (pMission->mmax_y_ > 0 && py > static_cast<double>((pMission->mmax_y_ - 1) * 256))
            py = static_cast<double>((pMission->mmax_y_ - 1) * 256);
        if (pMission->mmax_z_ > 0 && pz > static_cast<double>((pMission->mmax_z_ - 1) * 128))
            pz = static_cast<double>((pMission->mmax_z_ - 1) * 128);
    }

    WorldPoint muzzlePosW(static_cast<int>(std::lround(px)),
                          static_cast<int>(std::lround(py)),
                          static_cast<int>(std::lround(pz)));
    bool blocked = false;
    if (pMission) {
        Mission::BlockerCriteria crits;
        crits.setBlocker = true;
        crits.maxr = kBarrelToMuzzle + 1.0;
        crits.pOrigin = dmg_.d_owner;
        uint8_t mask = pMission->checkIfBlockersInShootingLine(dmg.originLocW, crits, &pShootableHit_, &muzzlePosW);
        if ((mask & (Mission::kBMaskBlockerBlockedByTile | Mission::kBMaskBlockerTargetObjectUpdated)) != 0) {
            blocked = true;
        }
    }
    appendPoint(muzzlePosW);

    if (!blocked) {
        int wepRange = (dmg.pWeapon && dmg.pWeapon->getClass()) ? dmg.pWeapon->getClass()->range() : 512;
        int steps = std::max(1, wepRange / 38) + 1;
        px = static_cast<double>(muzzlePosW.x);
        py = static_cast<double>(muzzlePosW.y);
        pz = static_cast<double>(muzzlePosW.z);

        for (int s = 0; s < steps; ++s) {
            bool stepBlocked = SFXObject::stepFlamerPoint(
                pMission, px, py, pz, incX_, incY_, incZ_,
                true, dmg_.d_owner, &pShootableHit_);
            appendPoint(WorldPoint(static_cast<int>(std::lround(px)),
                                   static_cast<int>(std::lround(py)),
                                   static_cast<int>(std::lround(pz))));
            if (stepBlocked) {
                break;
            }
        }
    }

    curPosW_ = dmg.originLocW;
}

WorldPoint FlamerShot::sampleTrailAt(double s) const {
    if (trailPoints_.empty()) {
        return dmg_.originLocW;
    }
    if (s <= 0.0 || trailPoints_.size() == 1) {
        return trailPoints_.front();
    }
    if (s >= totalLength_) {
        return trailPoints_.back();
    }
    for (size_t i = 1; i < cumulativeLens_.size(); ++i) {
        if (s <= cumulativeLens_[i]) {
            double segStart = cumulativeLens_[i - 1];
            double segLen = cumulativeLens_[i] - segStart;
            double t = (segLen > 0.0) ? ((s - segStart) / segLen) : 0.0;
            const WorldPoint &a = trailPoints_[i - 1];
            const WorldPoint &b = trailPoints_[i];
            return WorldPoint(
                a.x + static_cast<int>(std::lround((b.x - a.x) * t)),
                a.y + static_cast<int>(std::lround((b.y - a.y) * t)),
                a.z + static_cast<int>(std::lround((b.z - a.z) * t)));
        }
    }
    return trailPoints_.back();
}

bool FlamerShot::moveProjectile(uint32_t elapsed, Mission *pMission) {
    traveledDist_ += speed_ * (static_cast<double>(elapsed) / 1000.0);
    bool endMove = false;
    if (traveledDist_ >= totalLength_) {
        traveledDist_ = totalLength_;
        endMove = true;
    }

    curPosW_ = sampleTrailAt(traveledDist_);
    drawTrace(pMission);

    if (endMove) {
        if (pMission && dmg_.pWeapon && dmg_.pWeapon->getClass()) {
            // Short endpoint linger so the tip briefly outlasts the body of the flame jet
            auto linger = std::make_unique<SFXObject>(pMission->map(),
                dmg_.pWeapon->getClass()->impactAnims()->trace_anim, true, 360);
            linger->setPosition(curPosW_);
            pMission->addSfxObject(std::move(linger));
        }
        drawImpact_ = true;
        return true;
    }
    return false;
}

/*!
 * Draws dense overlapping flame sprites along the traversed wind-curved path.
 * \param pMission Mission data
 */
void FlamerShot::drawTrace(Mission *pMission) {
    if (!pMission || !dmg_.pWeapon || !dmg_.pWeapon->getClass()) return;
    constexpr double kAnimStep = 32.0;
    while (lastAnimDist_ + kAnimStep <= traveledDist_) {
        lastAnimDist_ += kAnimStep;
        WorldPoint pt = sampleTrailAt(lastAnimDist_);
        // Small perpendicular jitter avoids a sterile machine-straight grid look
        pt.x += (std::rand() % 9) - 4;
        pt.y += (std::rand() % 9) - 4;
        auto sfx = std::make_unique<SFXObject>(pMission->map(),
            dmg_.pWeapon->getClass()->impactAnims()->trace_anim);
        sfx->setPosition(pt);
        pMission->addSfxObject(std::move(sfx));
    }
}

static double distSqToSegment2D(const WorldPoint &p, const WorldPoint &a, const WorldPoint &b) {
    double abx = (double)(b.x - a.x);
    double aby = (double)(b.y - a.y);
    double ab2 = abx * abx + aby * aby;
    if (ab2 == 0) {
        double dx = (double)(p.x - a.x), dy = (double)(p.y - a.y);
        return dx * dx + dy * dy;
    }
    double apx = (double)(p.x - a.x);
    double apy = (double)(p.y - a.y);
    double t = (apx * abx + apy * aby) / ab2;
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    double projx = (double)a.x + t * abx;
    double projy = (double)a.y + t * aby;
    double dx = (double)p.x - projx;
    double dy = (double)p.y - projy;
    return dx * dx + dy * dy;
}

void FlamerShot::inflictDamage(Mission *pMission) {
    lifeOver_ = true;
    if (!pMission) return;
    PedInstance *pShooter = dmg_.pWeapon ? dmg_.pWeapon->owner() : nullptr;
    const double kFlameHitRadiusSq = 96.0 * 96.0;

    bool hitAnything = false;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        PedInstance *pPed = pMission->ped(i);
        if (!pPed || !pPed->isAlive() || pPed->isInVehicle()) continue;
        if (pPed == pShooter) continue;
        if (pShooter && pShooter->isFriendlyFireProtected(pPed)) continue;

        bool hitPed = false;
        if (trailPoints_.size() >= 2) {
            for (size_t s = 0; s + 1 < trailPoints_.size(); ++s) {
                if (distSqToSegment2D(pPed->position(), trailPoints_[s], trailPoints_[s + 1]) <= kFlameHitRadiusSq) {
                    hitPed = true;
                    break;
                }
            }
        } else if (distSqToSegment2D(pPed->position(), dmg_.originLocW, curPosW_) <= kFlameHitRadiusSq) {
            hitPed = true;
        }

        if (hitPed) {
            pPed->handleHit(dmg_);
            hitAnything = true;
        }
    }

    if (pShootableHit_ != NULL && !pShootableHit_->is(MapObject::kNaturePed)) {
        pShootableHit_->handleHit(dmg_);
        hitAnything = true;
    }

    if (hitAnything && pShooter && pShooter->isOurAgent()) {
        pMission->stats()->incrHits();
    }
}


AirStrikeShot::AirStrikeShot(const DamageToInflict &dmg, double flightAngleRad) : ProjectileShot(dmg) {
    targetCenterW_ = dmg.aimedLocW;

    if (flightAngleRad >= 0.0) {
        incX_ = std::cos(flightAngleRad);
        incY_ = std::sin(flightAngleRad);
    } else {
        // Original DOS Syndicate American Revolt: randomize inward flight angle based on target quadrant
        // Map center reference: 16384 x 12288
        double baseAngle = 0.0;
        if (targetCenterW_.x < 16384 && targetCenterW_.y < 12288) {
            baseAngle = 0.785398; // 45 deg (flying SE)
        } else if (targetCenterW_.x >= 16384 && targetCenterW_.y < 12288) {
            baseAngle = 2.356194; // 135 deg (flying SW)
        } else if (targetCenterW_.x >= 16384 && targetCenterW_.y >= 12288) {
            baseAngle = 3.926991; // 225 deg (flying NW)
        } else {
            baseAngle = 5.497787; // 315 deg (flying NE)
        }
        // Random variation of +/- 30 degrees (approx +/- 0.52 rad)
        double jitter = ((std::rand() % 61) - 30) * (3.141592653589793 / 180.0);
        double angle = baseAngle + jitter;
        incX_ = std::cos(angle);
        incY_ = std::sin(angle);
    }
    incZ_ = 0.0;

    // Flight parameters:
    // Approach: ~9.88s (168-tick original countdown) at 3200 units/s = 31616 units.
    // Target tract: length = 2800 units (-1400 to +1400 around target center).
    // Strafe finish: trailing outer wingmen pass +1400 at currentDistance = targetDistAlongRun + 2500.
    // Egress: planes fly 8192 units downstream off-map.
    constexpr double kApproachDist = 31616.0;
    constexpr double kExitDist = 8192.0;

    dmg_.originLocW.x = targetCenterW_.x - static_cast<int>(std::round(incX_ * kApproachDist));
    dmg_.originLocW.y = targetCenterW_.y - static_cast<int>(std::round(incY_ * kApproachDist));
    dmg_.originLocW.z = targetCenterW_.z;
    curPosW_ = dmg_.originLocW;

    targetLocW_.x = targetCenterW_.x + static_cast<int>(std::round(incX_ * (2500.0 + kExitDist)));
    targetLocW_.y = targetCenterW_.y + static_cast<int>(std::round(incY_ * (2500.0 + kExitDist)));
    targetLocW_.z = targetCenterW_.z;

    targetDistAlongRun_ = kApproachDist;
    distanceMax_ = kApproachDist + 2500.0 + kExitDist;
    currentDistance_ = 0.0;
    speed_ = 3200.0;
    lastSmokeDist_ = 0.0;
    strafeTimer_ = 60;
    strafeSoundTimer_ = 0;
    explosionSoundTimer_ = 0;
    flyoverSoundPlayed_ = false;
    bulletStrafeCompleted_ = false;
    bombGridInitialized_ = false;
    remainingBombsToDetonate_ = 0;
    remainingBombsToFinish_ = 0;
    startMove_ = true;
}

std::vector<WorldPoint> AirStrikeShot::getPlanePositions() const {
    std::vector<WorldPoint> positions;
    positions.reserve(5);

    double perpX = -incY_;
    double perpY = incX_;

    // Plane 0: Center leader
    positions.push_back(curPosW_);

    // Plane 1: Inner Left wingman (-550 along flight path, -550 lateral)
    positions.push_back(WorldPoint(
        curPosW_.x - static_cast<int>(std::round(incX_ * 550.0 + perpX * 550.0)),
        curPosW_.y - static_cast<int>(std::round(incY_ * 550.0 + perpY * 550.0)),
        curPosW_.z
    ));

    // Plane 2: Inner Right wingman (-550 along flight path, +550 lateral)
    positions.push_back(WorldPoint(
        curPosW_.x - static_cast<int>(std::round(incX_ * 550.0 - perpX * 550.0)),
        curPosW_.y - static_cast<int>(std::round(incY_ * 550.0 - perpY * 550.0)),
        curPosW_.z
    ));

    // Plane 3: Outer Left wingman (-1100 along flight path, -1100 lateral)
    positions.push_back(WorldPoint(
        curPosW_.x - static_cast<int>(std::round(incX_ * 1100.0 + perpX * 1100.0)),
        curPosW_.y - static_cast<int>(std::round(incY_ * 1100.0 + perpY * 1100.0)),
        curPosW_.z
    ));

    // Plane 4: Outer Right wingman (-1100 along flight path, +1100 lateral)
    positions.push_back(WorldPoint(
        curPosW_.x - static_cast<int>(std::round(incX_ * 1100.0 - perpX * 1100.0)),
        curPosW_.y - static_cast<int>(std::round(incY_ * 1100.0 - perpY * 1100.0)),
        curPosW_.z
    ));

    return positions;
}

bool AirStrikeShot::moveProjectile(uint32_t elapsed, Mission *pMission) {
    double incDist = speed_ * static_cast<double>(elapsed) / 1000.0;
    bool planeFlightEnd = false;
    if (currentDistance_ + incDist >= distanceMax_) {
        incDist = distanceMax_ - currentDistance_;
        planeFlightEnd = true;
    }

    currentDistance_ += incDist;
    curPosW_.x = dmg_.originLocW.x + static_cast<int>(std::round(incX_ * currentDistance_));
    curPosW_.y = dmg_.originLocW.y + static_cast<int>(std::round(incY_ * currentDistance_));
    curPosW_.z = targetCenterW_.z;

    double relDist = currentDistance_ - targetDistAlongRun_;

    // Play flyover sound when approaching the target tract
    if (relDist >= -2500.0 && !flyoverSoundPlayed_) {
        flyoverSoundPlayed_ = true;
        if (rs_eng::SoundManager::singletonPtr()) {
            g_SoundMgr.play(rs_eng::EXPLOSION_BIG);
        }
    }

    // 1. Bullet Strafe Phase:
    // Sweeps across the coverage tract (from relDist = -1400 to +2500) in precise V-formation
    if (relDist >= -1400.0 && relDist < 2500.0 && !bulletStrafeCompleted_) {
        executeStrafing(pMission, elapsed);
    } else if (relDist >= 2500.0 && !bulletStrafeCompleted_) {
        bulletStrafeCompleted_ = true;
    }

    // 2. Bombing Sweep Phase:
    // Staggered to appear essentially at the tail end of the bullet fire
    // as the bullet fire reaches the end of the coverage area
    if (bulletStrafeCompleted_) {
        if (!bombGridInitialized_) {
            initBombGrid();
        }
        updateBombingSweep(elapsed, pMission);
    }

    // Shot is complete when planes reach destination AND all bomb explosions have finished their 4-5s sprites
    return (planeFlightEnd && bulletStrafeCompleted_ && remainingBombsToFinish_ <= 0);
}

void AirStrikeShot::executeStrafing(Mission *pMission, uint32_t elapsed) {
    if (!pMission) return;

    strafeTimer_ += elapsed;
    strafeSoundTimer_ += elapsed;
    constexpr uint32_t kStrafeIntervalMs = 60;
    if (strafeTimer_ < kStrafeIntervalMs) {
        return;
    }
    strafeTimer_ = 0;

    int maxX = (pMission->mmax_x_ > 0) ? (pMission->mmax_x_ - 1) * 256 : 32767;
    int maxY = (pMission->mmax_y_ > 0) ? (pMission->mmax_y_ - 1) * 256 : 32767;

    double perpX = -incY_;
    double perpY = incX_;

    std::vector<WorldPoint> planes = getPlanePositions();

    for (const auto &planePos : planes) {
        // Check if this plane is within the coverage tract bounds (-1400 to +1400 along flight vector)
        double dx = planePos.x - targetCenterW_.x;
        double dy = planePos.y - targetCenterW_.y;
        double u_proj = dx * incX_ + dy * incY_;
        if (u_proj < -1400.0 || u_proj > 1400.0) {
            continue;
        }

        // 5 clusters (one per plane) where each cluster consists of 3 wide parallel rows of bullets
        // matching original American Revolt V-formation strafing pattern
        for (int r = -1; r <= 1; ++r) {
            double v_row = r * 150.0 + ((std::rand() % 31) - 15); // Lateral spacing: 150 units per row, 300 units total wingspan
            double u_off = 100.0 + ((std::rand() % 61) - 30);     // Ground impact slightly ahead of plane with longitudinal jitter

            WorldPoint impactPos;
            impactPos.x = planePos.x + static_cast<int>(std::round(incX_ * u_off + perpX * v_row));
            impactPos.y = planePos.y + static_cast<int>(std::round(incY_ * u_off + perpY * v_row));
            impactPos.z = targetCenterW_.z;

            if (impactPos.x < 0 || impactPos.x > maxX || impactPos.y < 0 || impactPos.y > maxY) {
                continue;
            }

            // Bullet spark plays full anim 382 at 12 fps, displaying large bright fireball burst (frames 1 & 2)
            auto sfx = std::make_unique<SFXObject>(pMission->map(), SFXObject::sfxt_BulletHit);
            sfx->setPosition(impactPos);
            sfx->correctZ(pMission->map()->maxTz());
            pMission->addSfxObject(std::move(sfx));

            // Deal bullet damage
            DamageToInflict bulletDmg;
            bulletDmg.dtype = kDmgTypeBullet;
            bulletDmg.dvalue = 35;
            bulletDmg.range = 140.0;
            bulletDmg.originLocW = impactPos;
            bulletDmg.d_owner = dmg_.d_owner;
            bulletDmg.pWeapon = dmg_.pWeapon;

            for (size_t i = 0; i < pMission->numPeds(); ++i) {
                PedInstance *p = pMission->ped(i);
                if (p && p->isAlive() && p->inVehicle() == nullptr && p->isCloseTo(impactPos, 140)) {
                    p->handleHit(bulletDmg);
                }
            }
            for (size_t i = 0; i < pMission->numVehicles(); ++i) {
                Vehicle *v = pMission->vehicle(i);
                if (v && v->isCloseTo(impactPos, 140)) {
                    v->handleHit(bulletDmg);
                }
            }
        }
    }

    if (strafeSoundTimer_ >= 120) {
        strafeSoundTimer_ = 0;
        if (rs_eng::SoundManager::singletonPtr()) {
            g_SoundMgr.play(rs_eng::MINIGUN);
        }
    }
}

void AirStrikeShot::initBombGrid() {
    bombGridInitialized_ = true;
    scheduledBombs_.clear();

    double perpX = -incY_;
    double perpY = incX_;

    // Generate dense coverage grid spanning the full tract (-1400 to +1400 long, -1300 to +1300 wide)
    // 22 rows x 18 columns = 396 explosion nodes, ensuring 100% gap-free coverage of all tiles
    constexpr int kRowsU = 22;
    constexpr int kColsV = 18;
    constexpr double kMinU = -1400.0;
    constexpr double kMaxU = 1400.0;
    constexpr double kMinV = -1300.0;
    constexpr double kMaxV = 1300.0;

    for (int r = 0; r < kRowsU; ++r) {
        double baseU = kMinU + r * ((kMaxU - kMinU) / (kRowsU - 1));
        double f_u = static_cast<double>(r) / (kRowsU - 1); // 0.0 at tract start, 1.0 at tract end

        for (int c = 0; c < kColsV; ++c) {
            double baseV = kMinV + c * ((kMaxV - kMinV) / (kColsV - 1));

            // Position jitter within cell
            double u_jit = (std::rand() % 71) - 35;
            double v_jit = (std::rand() % 71) - 35;

            WorldPoint loc;
            loc.x = targetCenterW_.x + static_cast<int>(std::round(incX_ * (baseU + u_jit) + perpX * (baseV + v_jit)));
            loc.y = targetCenterW_.y + static_cast<int>(std::round(incY_ * (baseU + u_jit) + perpY * (baseV + v_jit)));
            loc.z = targetCenterW_.z;

            // Timing:
            // Doubled progression speed: ~6.8 seconds total sweep across tract
            double t_longitudinal = f_u * 5750.0;
            // V-formation: centerline points lead, wings follow (~900 ms offset)
            double t_v = (std::abs(baseV) / kMaxV) * 900.0;
            // Randomness in timing of each sprite animation (+/- 600 ms)
            // allows some explosions a couple tiles ahead/behind to start concurrently
            double t_jit = (std::rand() % 1201) - 600;

            int32_t detonateMs = static_cast<int32_t>(std::round(t_longitudinal + t_v + t_jit));
            if (detonateMs < 0) detonateMs = 0;
            if (detonateMs > 6800) detonateMs = 6800;

            // Each sprite lasts approximately 4-5 seconds (4000 to 5000 ms)
            uint32_t spriteDuration = 4000 + (std::rand() % 1000);

            scheduledBombs_.push_back({ loc, detonateMs, spriteDuration, false });
        }
    }

    remainingBombsToDetonate_ = static_cast<int>(scheduledBombs_.size());
    remainingBombsToFinish_ = static_cast<int>(scheduledBombs_.size());
}

void AirStrikeShot::updateBombingSweep(uint32_t elapsed, Mission *pMission) {
    explosionSoundTimer_ += elapsed;

    for (auto &bomb : scheduledBombs_) {
        uint32_t remElapsed = elapsed;
        if (!bomb.detonated) {
            if (static_cast<int32_t>(remElapsed) >= bomb.timerMs) {
                remElapsed -= (bomb.timerMs > 0 ? static_cast<uint32_t>(bomb.timerMs) : 0);
                bomb.detonated = true;
                bomb.timerMs = 0;
                if (remainingBombsToDetonate_ > 0) {
                    --remainingBombsToDetonate_;
                }
                if (pMission) {
                    detonateBomb(pMission, bomb.location, bomb.durationMs);
                }
            } else {
                bomb.timerMs -= static_cast<int32_t>(remElapsed);
                remElapsed = 0;
            }
        }
        if (bomb.detonated && bomb.durationMs > 0) {
            if (remElapsed >= bomb.durationMs) {
                bomb.durationMs = 0;
                if (remainingBombsToFinish_ > 0) {
                    --remainingBombsToFinish_;
                }
            } else {
                bomb.durationMs -= remElapsed;
            }
        }
    }
}

void AirStrikeShot::detonateBomb(Mission *pMission, const WorldPoint &impactLocW, uint32_t durationMs) {
    if (!pMission) return;

    int maxX = (pMission->mmax_x_ > 0) ? (pMission->mmax_x_ - 1) * 256 : 32767;
    int maxY = (pMission->mmax_y_ > 0) ? (pMission->mmax_y_ - 1) * 256 : 32767;

    if (impactLocW.x < 0 || impactLocW.x > maxX || impactLocW.y < 0 || impactLocW.y > maxY) {
        return;
    }

    // Visual variety matching original American Revolt:
    // Mix sfxt_ExplosionFire (anim 390, starburst explosion with fiery shrapnel)
    // and sfxt_ExplosionBall (anim 391, billowing fireball)
    SFXObject::SfxType animType = (std::rand() % 100 < 50) 
        ? SFXObject::sfxt_ExplosionFire 
        : SFXObject::sfxt_ExplosionBall;

    auto sfx = std::make_unique<SFXObject>(pMission->map(), animType, true, durationMs);
    sfx->setPosition(impactLocW);
    sfx->correctZ(pMission->map()->maxTz());
    pMission->addSfxObject(std::move(sfx));

    // Lethal blast damage (800 HP), non-incendiary
    DamageToInflict bombDmg;
    bombDmg.dtype = kDmgTypeExplosion;
    bombDmg.dvalue = (dmg_.dvalue > 0) ? dmg_.dvalue : 800;
    bombDmg.range = 380.0;
    bombDmg.nonIncendiary = true;
    bombDmg.originLocW = impactLocW;
    bombDmg.d_owner = dmg_.d_owner;
    bombDmg.pWeapon = dmg_.pWeapon;

    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        PedInstance *p = pMission->ped(i);
        if (p && p->isAlive() && p->inVehicle() == nullptr && p->isCloseTo(impactLocW, 380)) {
            p->handleHit(bombDmg);
        }
    }
    for (size_t i = 0; i < pMission->numVehicles(); ++i) {
        Vehicle *v = pMission->vehicle(i);
        if (v && v->isCloseTo(impactLocW, 380)) {
            v->handleHit(bombDmg);
        }
    }
    for (size_t i = 0; i < pMission->numStatics(); ++i) {
        Static *st = pMission->statics(i);
        if (st && st->canBlock() && st->isAlive() && st->isCloseTo(impactLocW, 380)) {
            st->handleHit(bombDmg);
        }
    }

    if (explosionSoundTimer_ >= 280) {
        explosionSoundTimer_ = 0;
        if (rs_eng::SoundManager::singletonPtr()) {
            g_SoundMgr.play(rs_eng::EXPLOSION_BIG);
        }
    }
}

void AirStrikeShot::drawTrace(Mission *pMission) {
    (void)pMission;
    // No trailing smoke behind planes - matches original Syndicate American Revolt
}

void AirStrikeShot::inflictDamage(Mission *pMission) {
    if (!bombGridInitialized_) {
        initBombGrid();
    }
    if (pMission) {
        for (auto &b : scheduledBombs_) {
            if (!b.detonated) {
                b.detonated = true;
                detonateBomb(pMission, b.location, b.durationMs);
            }
        }
    }
    remainingBombsToDetonate_ = 0;
    remainingBombsToFinish_ = 0;
    lifeOver_ = true;
}

}
