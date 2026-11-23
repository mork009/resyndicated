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
 *   Copyright (C) 2013, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-kernel/model/sfxobject.h"

#include <cmath>
#include <cstdlib>

#include "rs-utils/log/log.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-kernel/model/mission.h"

namespace rs_knl {
uint16_t SFXObject::sfxIdCnt = 0;

/*!
 * Constructor of the class.
 * \param pMap a pointer to the map
 * \param type Type of SfxObject (see SFXObject::SfxTypeEnum)
 * \param drawable True means the object will be drawn by default
 * \param addShow Used for fire animation to add randomized time to animation
 */
SFXObject::SFXObject(Map *pMap, SfxType type, bool drawable, uint32_t addShow) : 
        MapObject(sfxIdCnt++, pMap, kNatureSfx), type_(type), sfx_life_over_(false), elapsed_left_(0) {
    setDrawable(drawable);
    
    switch(type) {
        case SFXObject::sfxt_Unknown:
            RSERR(Log::k_FLG_UI, "SFXObject", "SFXObject", ("Sfx object of type Unknown created"));
            sfx_life_over_ = true;
            break;
        case SFXObject::sfxt_BulletHit:
            animationId_ = animationPlayer_->addAnimation(382, rs_eng::kAnimationModeSingle, 12, addShow);
            break;
        case SFXObject::sfxt_FlamerFire:
            if (addShow > 0) {
                // Endpoint linger sprites hold briefly at the tip past the fading stream
                animationId_ = animationPlayer_->addAnimation(383, rs_eng::kAnimationModeLoop, 16, addShow);
            } else {
                animationId_ = animationPlayer_->addAnimation(383, rs_eng::kAnimationModeSingle, 16);
                startFrame_ = static_cast<uint8_t>(std::rand() % 2);
            }
            break;
        case SFXObject::sfxt_Smoke:
            animationId_ = animationPlayer_->addAnimation(244, rs_eng::kAnimationModeSingle, 6);
            break;
        case SFXObject::sfxt_Fire_LongSmoke:
            // point of impact for laser
            animationId_ = animationPlayer_->addAnimation(389, rs_eng::kAnimationModeSingle, 6);
            break;
        case SFXObject::sfxt_ExplosionFire:
            animationId_ = animationPlayer_->addAnimation(390, rs_eng::kAnimationModeSingle, (addShow > 0 ? 3 : 6), addShow);
            break;
        case SFXObject::sfxt_ExplosionBall:
            animationId_ = animationPlayer_->addAnimation(391, rs_eng::kAnimationModeSingle, (addShow > 0 ? 3 : 6), addShow);
            break;
        case SFXObject::sfxt_LargeFire:
            animationId_ = animationPlayer_->addAnimation(243, rs_eng::kAnimationModeLoop, 8, 3000 + addShow);
            break;
        case SFXObject::sfxt_SelArrow:
            animationId_ = animationPlayer_->addAnimation(601, rs_eng::kAnimationModeSingle, 6);
            break;
        case SFXObject::sfxt_AgentFirst:
            animationId_ = animationPlayer_->addAnimation(1951, rs_eng::kAnimationModeLoop, 4);
            break;
        case SFXObject::sfxt_AgentSecond:
            animationId_ = animationPlayer_->addAnimation(1952, rs_eng::kAnimationModeLoop, 4);
            break;
        case SFXObject::sfxt_AgentThird:
            animationId_ = animationPlayer_->addAnimation(1953, rs_eng::kAnimationModeLoop, 4);
            break;
        case SFXObject::sfxt_AgentFourth:
            animationId_ = animationPlayer_->addAnimation(1954, rs_eng::kAnimationModeLoop, 4);
            break;
    }
}

void SFXObject::draw(const Point2D &screenPos) {
    animationPlayer_->draw(addOffs(screenPos), 0);
}

bool SFXObject::stepFlamerPoint(Mission *pMission,
                                double &posX, double &posY, double &posZ,
                                double incX, double incY, double incZ,
                                bool checkObjects,
                                ShootableMapObject *pOrigin,
                                ShootableMapObject **ppHitOut) {
    WorldPoint prevPosW(static_cast<int>(std::lround(posX)),
                        static_cast<int>(std::lround(posY)),
                        static_cast<int>(std::lround(posZ)));

    double fwd = 38.0 + static_cast<double>(std::rand() % 11);
    double wx = 0.0, wy = 0.0;
    if (pMission) {
        pMission->windDeltaPerSubstep(wx, wy);
    }

    double nx = posX + incX * fwd + wx;
    double ny = posY + incY * fwd + wy;
    double nz = posZ + incZ * fwd;

    bool blocked = false;
    if (nx < 0.0) { nx = 0.0; blocked = true; }
    if (ny < 0.0) { ny = 0.0; blocked = true; }
    if (nz < 0.0) { nz = 0.0; blocked = true; }
    if (pMission) {
        if (pMission->mmax_x_ > 0 && nx > static_cast<double>((pMission->mmax_x_ - 1) * 256)) {
            nx = static_cast<double>((pMission->mmax_x_ - 1) * 256);
            blocked = true;
        }
        if (pMission->mmax_y_ > 0 && ny > static_cast<double>((pMission->mmax_y_ - 1) * 256)) {
            ny = static_cast<double>((pMission->mmax_y_ - 1) * 256);
            blocked = true;
        }
        if (pMission->mmax_z_ > 0 && nz > static_cast<double>((pMission->mmax_z_ - 1) * 128)) {
            nz = static_cast<double>((pMission->mmax_z_ - 1) * 128);
            blocked = true;
        }
    }

    WorldPoint nextPosW(static_cast<int>(std::lround(nx)),
                        static_cast<int>(std::lround(ny)),
                        static_cast<int>(std::lround(nz)));

    if (pMission && !blocked) {
        double segDx = static_cast<double>(nextPosW.x - prevPosW.x);
        double segDy = static_cast<double>(nextPosW.y - prevPosW.y);
        double segDz = static_cast<double>(nextPosW.z - prevPosW.z);
        double segLen = std::sqrt(segDx * segDx + segDy * segDy + segDz * segDz);
        if (segLen > 0.0) {
            if (checkObjects) {
                Mission::BlockerCriteria crits;
                crits.setBlocker = true;
                crits.maxr = segLen + 1.0;
                crits.pOrigin = pOrigin;
                uint8_t mask = pMission->checkIfBlockersInShootingLine(prevPosW, crits, ppHitOut, &nextPosW);
                if ((mask & (Mission::kBMaskBlockerBlockedByTile | Mission::kBMaskBlockerTargetObjectUpdated)) != 0) {
                    blocked = true;
                }
            } else {
                uint8_t mask = pMission->checkBlockedByTile(prevPosW, &nextPosW, true, segLen + 1.0);
                if ((mask & (Mission::kBMaskBlockerBlockedByTile | Mission::kBMaskBlockerTargetOutOfMap)) != 0) {
                    blocked = true;
                }
            }
        }
    }

    posX = static_cast<double>(nextPosW.x);
    posY = static_cast<double>(nextPosW.y);
    posZ = static_cast<double>(nextPosW.z);
    return blocked;
}

void SFXObject::doUpdateState(uint32_t elapsed) {
    if (type_ == SFXObject::sfxt_ExplosionBall) {
        int z = pos_.tz * 128 + pos_.oz;
        // 250 per sec
        z += ((elapsed + elapsed_left_) >> 2);
        elapsed_left_ = elapsed &3;
        if (z > (pMap_->maxTz() - 1) * 128)
            z = (pMap_->maxTz() - 1) * 128;
        pos_.tz = z / 128;
        pos_.oz = z % 128;
    }
}

void SFXObject::handleAnimationEnded() {
    sfx_life_over_ = true;
}

/** \brief
 *
 * \param mapMaxZ int
 * \return void
 *
 */
void SFXObject::correctZ(int mapMaxZ) {
    if (type_ == SFXObject::sfxt_ExplosionBall) {
        int z = pos_.tz * 128 + pos_.oz;
        z += 512;

        if (z > (mapMaxZ - 1) * 128)
            z = (mapMaxZ - 1) * 128;
        pos_.tz = z / 128;
        pos_.oz = z % 128;
    }
}

}