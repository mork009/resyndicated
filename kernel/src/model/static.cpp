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

#include "rs-kernel/model/static.h"

#include <algorithm>
#include "rs-utils/common.h"
#include "rs-utils/log/log.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/sound/soundmanager.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/ped.h"

namespace rs_knl {

const int Static::kStaticOrientationNS = 0;
const int Static::kStaticOrientationEW = 2;
const uint16_t Semaphore::kSemaphoreMaxColorAnim = 4;
const uint16_t Semaphore::kSemaphoreDamagedOffset = 4;

Static *Static::loadInstance(uint8_t * data, uint16_t id, Map *pMap)
{
    LevelData::Statics * gamdata =
        (LevelData::Statics *) data;
    Static *s = NULL;

    // TODO: verify whether object description is correct
    // subtype for doors, use instead orientation?

    // NOTE: objects states are usually mixed with type,
    // no separation between object type and its state
    uint16_t curanim = rs_utl::READ_LE_UINT16(gamdata->index_current_anim);
    uint16_t baseanim = rs_utl::READ_LE_UINT16(gamdata->index_base_anim);
    uint16_t curframe = rs_utl::READ_LE_UINT16(gamdata->index_current_frame);
    auto isOrientationNS = [](uint8_t ori) -> bool {
        uint8_t angle = ori & 0xC0;
        return (angle == 0x00 || angle == 0x80 || ori == 0x7E || ori == 0xFE);
    };

    switch(gamdata->sub_type) {
        case 0x01:
            // phone booth
            s = new EtcObj(id, pMap, curanim);
            s->setSize(128, 128, 128);
            break;
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
            // crossroad things
            s = new Semaphore(id, pMap, gamdata->sub_type - 0x05);
            s->setSize(48, 48, 48);
            //s->state_ = sttsem_Stt0 + (gamdata->sub_type - 0x05);
            s->setHealth(1);
            s->setStartHealth(1);
            break;
        case 0x0B:
            // 0x0270 animation, is this object present in original game?
            //s = new EtcObj(m, curanim, curanim, curanim);
            //printf("0x0B anim %X\n", curanim);
            break;
        case 0x0A:
            s = new NeonSign(id, pMap, curanim, curframe);
            break;
        case 0x0C: // closed door
        case 0x0D: {
            uint16_t famBase = (baseanim >= 1077 && baseanim <= 1115) ? (1077 + ((baseanim - 1077) / 8) * 8) : baseanim;
            if (isOrientationNS(gamdata->orientation)) {
                s = new Door(id, pMap, famBase, Static::kStateDoorClosed);
                s->setOrientation(kStaticOrientationNS);
                s->setSize(256, 1, 196);
            } else {
                s = new Door(id, pMap, famBase + 1, Static::kStateDoorClosed);
                s->setOrientation(kStaticOrientationEW);
                s->setSize(1, 256, 196);
            }
            break;
        }
        case 0x0E: // opening doors, not open
        case 0x0F: {
            uint16_t famBase = (baseanim >= 1077 && baseanim <= 1115) ? (1077 + ((baseanim - 1077) / 8) * 8) : baseanim;
            if (isOrientationNS(gamdata->orientation)) {
                s = new Door(id, pMap, famBase, kStateDoorOpening);
                s->setOrientation(kStaticOrientationNS);
                s->setSize(256, 1, 196);
            } else {
                s = new Door(id, pMap, famBase + 1, kStateDoorOpening);
                s->setOrientation(kStaticOrientationEW);
                s->setSize(1, 256, 196);
            }
            break;
        }
        case 0x11:
            // ???? what is this
            //s = new EtcObj(m, bas, curanim, curanim);
            //printf("0x11 anim %X\n", curanim);
            break;
        case 0x12:  // open window
            s = new WindowObj(id, pMap, WindowObj::kStateWindowOpen, curanim - 2);
            if (isOrientationNS(gamdata->orientation)) {
                s->setOrientation(kStaticOrientationNS);
            } else {
                s->setOrientation(kStaticOrientationEW);
            }
            s->setHealth(1);
            s->setStartHealth(1);
            break;
        case 0x13: // closed window
            s = new WindowObj(id, pMap, WindowObj::kStateWindowClosed, curanim);
            if (isOrientationNS(gamdata->orientation)) {
                s->setOrientation(kStaticOrientationNS);
            } else {
                s->setOrientation(kStaticOrientationEW);
            }
            s->setHealth(1);
            s->setStartHealth(1);
            break;
        case 0x15: // damaged window
            s = new WindowObj(id, pMap, WindowObj::kStateWindowDamaged, curanim - 6);
            if (isOrientationNS(gamdata->orientation)) {
                s->setOrientation(kStaticOrientationNS);
            } else {
                s->setOrientation(kStaticOrientationEW);
            }
            s->deactivateBlocking();
            s->setHealth(0);
            s->setStartHealth(1);
            break;
        case 0x16:
            if (curanim < 200) {
                uint16_t bAnim = rs_utl::READ_LE_UINT16(gamdata->index_base_anim);
                if (curanim == 1 || bAnim == 1) curanim = 227;
                else if (curanim == 2 || bAnim == 2) curanim = 230;
                else if (curanim == 3 || bAnim == 3) curanim = 233;
                else curanim = 227;
            }
            s = new Tree(id, pMap, curanim);
            s->setSize(64, 64, 256);
            s->setHealth(1);
            s->setStartHealth(1);
            break;
        case 0x19: // trash bin
            s = new EtcObj(id, pMap, curanim);
            s->setSize(64, 64, 96);
            break;
        case 0x1A: // mail box
            s = new EtcObj(id, pMap, curanim);
            s->setSize(64, 64, 96);
            break;
        case 0x1C: // ???? what is this?
            //s = new EtcObj(m, curanim, curanim, curanim);
            //printf("0x1C anim %X\n", curanim);
            break;
        case 0x1F: // advertisement on wall
            s = new EtcObj(id, pMap, curanim, smt_Advertisement);
            s->deactivateBlocking();
            break;

        case 0x20: // window without light
        {
            AnimWindow *pWin = new AnimWindow(id, pMap, curanim);
            pWin->playLightOffAnimation();
            s = pWin;
        }
            break;
        case 0x21: // window light turns on
        {
            AnimWindow *pWin = new AnimWindow(id, pMap, curanim - 2);
            pWin->playLightSwitchingAnimation();
            s = pWin;
        }
            break;
        // NOTE : 0x22 should have existed but it doesn't appear anywhere
        case 0x23:
        {
            // window with person's shadow non animated,
            // even though on 1 map person appears I will ignore it
            AnimWindow *pWin = new AnimWindow(id, pMap, 1959 + ((gamdata->orientation & 0x40) >> 6));
            pWin->playShowPedAnimation();
            s = pWin;
        }
            break;
        case 0x24:
        {
            // window with person's shadow, hides, actually animation
            // is of ped standing, but I will ignore it
            // TODO : find a mission with this type of window to check if animation id is ok
            AnimWindow *pWin = new AnimWindow(id, pMap, 1959 + 8 + ((gamdata->orientation & 0x40) >> 6));
            pWin->playedDisappearAnimation();
            s = pWin;
        }
            break;
        case 0x25:
        {
            AnimWindow *pWin = new AnimWindow(id, pMap, curanim);
            // NOTE : orientation, I assume, plays role of hidding object,
            // orientation 0x40, 0x80 are drawn (gamdata->desc always 7)
            // window without light
            if (gamdata->orientation == 0x40 || gamdata->orientation == 0x80) {
                pWin->playLightOffAnimation();
            } else {
                pWin->playLightOnAnimation();
            }
            s = pWin;
        }
            break;

        case 0x26:
            // 0x00,0x80 south - north = 0
            // 0x40,0xC0 weast - east = 2
            s = new LargeDoor(id, pMap, curanim);
            if (isOrientationNS(gamdata->orientation)) {
                s->setOrientation(kStaticOrientationNS);
                s->setSize(384, 64, 192);
            } else {
                s->setOrientation(kStaticOrientationEW);
                s->setSize(64, 384, 192);
            }
            break;
#ifdef _DEBUG
        default:
            printf("uknown static object type %02X , %02X, %X\n",
                gamdata->sub_type, gamdata->orientation,
                rs_utl::READ_LE_UINT16(gamdata->index_current_frame));
            printf("x = %i, xoff = %i, ", gamdata->mapposx[1],
                gamdata->mapposx[0]);
            printf("y = %i, yoff = %i, ", gamdata->mapposy[1],
                gamdata->mapposy[0]);
            printf("z = %i, zoff = %i\n", gamdata->mapposz[1],
                gamdata->mapposz[0]);
            break;
#endif
    }

    if (s) {
        s->setSubType(gamdata->sub_type);
        uint16_t bAnim = rs_utl::READ_LE_UINT16(gamdata->index_base_anim);
        uint16_t cAnim = rs_utl::READ_LE_UINT16(gamdata->index_current_anim);
        if (gamdata->sub_type == 0x16) {
            s->setBaseAnim(curanim);
        } else {
            s->setBaseAnim(bAnim != 0 ? bAnim : cAnim);
        }
        int z = rs_utl::READ_LE_UINT16(gamdata->mapposz) >> 7;
        int oz = gamdata->mapposz[0] & 0x7F;

        s->setPosition(gamdata->mapposx[1], gamdata->mapposy[1],
            z, gamdata->mapposx[0], gamdata->mapposy[0], oz);

        s->setDirection(gamdata->orientation);
    }

    return s;
}

/*!
 * @brief 
 * @param anId id of the static
 * @param pMap 
 * @param baseAnim The first animation
 * @param initialState initial state of the Door object
 */
Door::Door(uint16_t anId, Map *pMap, uint16_t baseAnim, Static::StateDoors initialState) : 
        Static(anId, pMap, Static::smt_Door), state_(initialState) {
    baseAnim_ = baseAnim;
    orientation_ = ((baseAnim >= 1077 && (baseAnim - 1077) % 2 != 0) || (baseAnim < 1077 && baseAnim % 2 != 0)) ? kStaticOrientationEW : kStaticOrientationNS;
    if (orientation_ == kStaticOrientationEW) {
        setSize(1, 256, 196);
    } else {
        setSize(256, 1, 196);
    }
    closedAnim_ = animationPlayer_->addAnimation(baseAnim);
    openingAnim_ = animationPlayer_->addAnimation(baseAnim + 2);
    openedAnim_ = animationPlayer_->addAnimation(baseAnim + 4);
    closingAnim_ = animationPlayer_->addAnimation(baseAnim + 6);

    uint16_t famBase = (baseAnim >= 1077 && baseAnim <= 1115) ? (1077 + ((baseAnim - 1077) / 8) * 8) : baseAnim;
    if (famBase == 1077 || famBase == 1101) {
        doorSound_ = rs_eng::DOOR_2;
    } else {
        doorSound_ = rs_eng::DOOR;
    }

    setStateTo(initialState);
}

void Door::setOrientation(int anOrientation) {
    if (orientation_ == anOrientation) return;
    Static::setOrientation(anOrientation);
    if (anOrientation == kStaticOrientationEW) {
        setSize(1, 256, 196);
    } else {
        setSize(256, 1, 196);
    }
    uint16_t b = baseAnim();
    if (b >= 1077 && b <= 1116) {
        uint16_t famBase = 1077 + ((b - 1077) / 8) * 8;
        uint16_t anim = (anOrientation == kStaticOrientationEW) ? (famBase + 1) : famBase;
        baseAnim_ = anim;
        closedAnim_ = animationPlayer_->addAnimation(anim);
        openingAnim_ = animationPlayer_->addAnimation(anim + 2);
        openedAnim_ = animationPlayer_->addAnimation(anim + 4);
        closingAnim_ = animationPlayer_->addAnimation(anim + 6);
        doorSound_ = (famBase == 1077 || famBase == 1101) ? rs_eng::DOOR_2 : rs_eng::DOOR;
        setStateTo(state_);
    }
}

/*!
 * Set the state of the static and react to the new state
 * @param newState The new state
 */
void Door::setStateTo(StateDoors newState) {
    bool stateChanged = (state_ != newState);
    state_ = newState;
    Mission *pMission = (rs_knl::MissionManager::singletonPtr() ? g_missionCtrl.mission() : nullptr);
    if (newState == Static::kStateDoorOpen) {
        deactivateBlocking();
        playAnimation(openedAnim_);
        openHoldTimer_ = 800;
    } else if (newState == Static::kStateDoorClosing) {
        activateBlocking();
        playAnimation(closingAnim_);
        if (stateChanged && rs_eng::SoundManager::singletonPtr()) {
            if (!pMission || pMission->isObjectInView(this)) {
                g_SoundMgr.play(doorSound_);
            }
        }
    } else if (newState == Static::kStateDoorOpening) {
        activateBlocking();
        playAnimation(openingAnim_);
        if (stateChanged && rs_eng::SoundManager::singletonPtr()) {
            if (!pMission || pMission->isObjectInView(this)) {
                g_SoundMgr.play(doorSound_);
            }
        }
    } else if (newState == Static::kStateDoorClosed) {
        activateBlocking();
        playAnimation(closedAnim_);
        openHoldTimer_ = 0;
    }
}

void Door::draw(const Point2D &screenPos) {
    animationPlayer_->draw(addOffs(screenPos), 0);
}

void Door::doUpdateState(uint32_t elapsed) {
    Mission *pMission = (rs_knl::MissionManager::singletonPtr() ? g_missionCtrl.mission() : nullptr);
    int x = tileX();
    int y = tileY();
    int z = tileZ();
    bool found = false;

    switch(state_) {
        case Static::kStateDoorOpen: {
            if (pMission) {
                char offsetX = 0, offsetY = 0;
                char *pActiveOffset = nullptr;
                if (orientation_ == kStaticOrientationNS) {
                    pActiveOffset = &offsetY;
                } else if (orientation_ == kStaticOrientationEW) {
                    pActiveOffset = &offsetX;
                }
                assert(pActiveOffset != nullptr);
                for (*pActiveOffset = -1; *pActiveOffset <= 1; *pActiveOffset += 1) {
                    for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetX, y + offsetY, z))) {
                        if (!obj->is(MapObject::kNaturePed)) continue;
                        auto *pPed = static_cast<ShootableMovableMapObject *>(obj);
                        if (pPed->isAlive()) {
                            found = true;
                            pPed->unblockPath();
                        }
                    }
                }
            }
            if (found) {
                openHoldTimer_ = 800;
            } else {
                if (openHoldTimer_ > elapsed) {
                    openHoldTimer_ -= elapsed;
                } else {
                    openHoldTimer_ = 0;
                    setStateTo(Static::kStateDoorClosing);
                }
            }
            break;
        }
        case Static::kStateDoorClosed: {
            if (pMission) {
                char offsetX = 0, offsetY = 0;
                char *pActiveOffset = nullptr;
                if (orientation_ == kStaticOrientationNS) {
                    pActiveOffset = &offsetY;
                } else if (orientation_ == kStaticOrientationEW) {
                    pActiveOffset = &offsetX;
                }
                assert(pActiveOffset != nullptr);
                for (*pActiveOffset = -1; *pActiveOffset <= 1; *pActiveOffset += 1) {
                    for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetX, y + offsetY, z))) {
                        if (!obj->is(MapObject::kNaturePed)) continue;
                        auto *pPed = static_cast<ShootableMovableMapObject *>(obj);
                        if (pPed->isAlive()) {
                            if (!found) { setStateTo(Static::kStateDoorOpening); found = true; }
                            pPed->blockPathWith(this, false, x, y, z);
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

/*!
 * @copydoc MapObject::handleAnimationEnded()
 */
void Door::handleAnimationEnded() {
    if (animationPlayer_->isCurrentAnimation(closingAnim_)) {
        setStateTo(Static::kStateDoorClosed);
    } else if (animationPlayer_->isCurrentAnimation(openingAnim_)) {
        setStateTo(Static::kStateDoorOpen);
    }
}


/*!
 * @brief 
 * @param anId 
 * @param pMap 
 * @param baseAnim 
 */
LargeDoor::LargeDoor(uint16_t anId, Map *pMap, uint16_t baseAnim):
        Static(anId, pMap, Static::smt_LargeDoor), state_(Static::kStateDoorClosed) {
    baseAnim_ = baseAnim;
    orientation_ = (baseAnim == 392) ? kStaticOrientationEW : kStaticOrientationNS;
    if (orientation_ == kStaticOrientationEW) {
        setSize(64, 384, 192);
    } else {
        setSize(384, 64, 192);
    }
    closedAnim_ = animationPlayer_->addAnimation(baseAnim);
    openingAnim_ = animationPlayer_->addAnimation(baseAnim + 1);
    closingAnim_ = animationPlayer_->addAnimation(baseAnim + 2);

    setStateTo(Static::kStateDoorClosed);
}

LargeDoor::~LargeDoor() {
    if (pMission_ && pMission_->mdpoints_) {
        for (int idx : coveredTiles_) {
            pMission_->mdpoints_[idx].flags |= kWalkable;
        }
    }
}

void LargeDoor::initNavigationBlocker(Mission *pMission) {
    pMission_ = pMission;
    coveredTiles_.clear();
    if (!pMission || !pMission->mdpoints_) return;

    int mx = pMission->mmax_x_;
    int my = pMission->mmax_y_;
    int mz = pMission->mmax_z_;
    int mxy = pMission->mmax_m_xy;
    int gridSize = mxy * mz;

    int gx = tileX();
    int gy = tileY();
    int gz = tileZ();

    for (int off = -1; off <= 1; ++off) {
        int tx = (orientation_ == kStaticOrientationEW) ? gx : gx + off;
        int ty = (orientation_ == kStaticOrientationEW) ? gy + off : gy;

        if (tx < 0 || tx >= mx || ty < 0 || ty >= my) continue;

        for (int dz = 0; dz >= -1; --dz) {
            int tz = gz + dz;
            if (tz < 0 || tz >= mz) continue;
            int idx = tx + ty * mx + tz * mxy;
            if (idx >= 0 && idx < gridSize) {
                if ((pMission->mdpoints_[idx].flags & kWalkable) == kWalkable) {
                    coveredTiles_.push_back(idx);
                }
            }
        }
    }

    std::sort(coveredTiles_.begin(), coveredTiles_.end());
    coveredTiles_.erase(std::unique(coveredTiles_.begin(), coveredTiles_.end()), coveredTiles_.end());

    if (state_ != Static::kStateDoorOpen) {
        for (int idx : coveredTiles_) {
            pMission->mdpoints_[idx].flags &= ~kWalkable;
        }
    }
}

void LargeDoor::setOrientation(int anOrientation) {
    if (orientation_ == anOrientation) return;
    Static::setOrientation(anOrientation);
    uint16_t anim = (anOrientation == kStaticOrientationEW) ? 392 : 395;
    baseAnim_ = anim;
    if (anOrientation == kStaticOrientationEW) {
        setSize(64, 384, 192);
    } else {
        setSize(384, 64, 192);
    }
    closedAnim_ = animationPlayer_->addAnimation(anim);
    openingAnim_ = animationPlayer_->addAnimation(anim + 1);
    closingAnim_ = animationPlayer_->addAnimation(anim + 2);
    if (pMission_) {
        if (pMission_->mdpoints_) {
            for (int idx : coveredTiles_) {
                pMission_->mdpoints_[idx].flags |= kWalkable;
            }
        }
        initNavigationBlocker(pMission_);
    } else {
        setStateTo(state_);
    }
}

/*!
 * Set the state of the static and react to the new state
 * @param newState The new state
 */
void LargeDoor::setStateTo(StateDoors newState) {
    state_ = newState;
    Mission *pMission = pMission_ ? pMission_ : (rs_knl::MissionManager::singletonPtr() ? g_missionCtrl.mission() : nullptr);

    if (newState == Static::kStateDoorOpen) {
        deactivateBlocking();
        // I set the closedAnim just to have an animation set as no frame
        // is drawn when the state is Open
        playAnimation(closedAnim_);
        if (pMission && pMission->mdpoints_) {
            for (int idx : coveredTiles_) {
                pMission->mdpoints_[idx].flags |= kWalkable;
            }
        }
    } else if (newState == Static::kStateDoorClosing) {
        activateBlocking();
        playAnimation(closingAnim_);
        if (pMission && pMission->mdpoints_) {
            for (int idx : coveredTiles_) {
                pMission->mdpoints_[idx].flags &= ~kWalkable;
            }
        }
    } else if (newState == Static::kStateDoorOpening) {
        activateBlocking();
        playAnimation(openingAnim_);
        if (pMission && pMission->mdpoints_) {
            for (int idx : coveredTiles_) {
                pMission->mdpoints_[idx].flags &= ~kWalkable;
            }
        }
    } else if (newState == Static::kStateDoorClosed) {
        activateBlocking();
        playAnimation(closedAnim_);
        if (pMission && pMission->mdpoints_) {
            for (int idx : coveredTiles_) {
                pMission->mdpoints_[idx].flags &= ~kWalkable;
            }
        }
    }
}

void LargeDoor::draw(const Point2D &screenPos) {
   // When a large door is opened, we just don't draw it
    if (state_ != Static::kStateDoorOpen) {
        animationPlayer_->draw(addOffs(screenPos), 0);
    }
}

void LargeDoor::doUpdateState([[maybe_unused]] uint32_t elapsed) {
    // TODO: there must be somewhere locked door
    Mission *pMission = g_missionCtrl.mission();
    int x = tileX();
    int y = tileY();
    int z = tileZ();
    char offsetX = 0, offsetY = 0;
    char *pActiveOffset = nullptr, *pFixedOffset = nullptr;
    bool found = false;
    std::vector<PedInstance *> found_peds;
    found_peds.reserve(256);
    std::vector<PedInstance *> found_peds_mid;
    found_peds_mid.reserve(256);
    char sign = 1;
    bool terminatePath = false;

    switch(state_) {
        case Static::kStateDoorOpen: {
            if (orientation_ == kStaticOrientationNS) {
                pActiveOffset = &offsetY;
                pFixedOffset = &offsetX;
            } else if (orientation_ == kStaticOrientationEW) {
                pActiveOffset = &offsetX;
                pFixedOffset = &offsetY;
            }
            assert(pActiveOffset != nullptr && pFixedOffset != nullptr);
            // Vehicles: tile coords use (x + offsetX, y + offsetY)
            *pFixedOffset = -1;
            for (*pActiveOffset = -2; *pActiveOffset < 3; (*pActiveOffset)++) {
                for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetX, y + offsetY, z))) {
                    if (obj->nature() != MapObject::kNatureVehicle) continue;
                    auto *pVehicle = static_cast<ShootableMovableMapObject *>(obj);
                    found = true;
                    pVehicle->unblockPath();
                    break;
                }
            }
            *pFixedOffset = 1;
            for (*pActiveOffset = -2; *pActiveOffset < 3; (*pActiveOffset)++) {
                for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetX, y + offsetY, z))) {
                    if (obj->nature() != MapObject::kNatureVehicle) continue;
                    auto *pVehicle = static_cast<ShootableMovableMapObject *>(obj);
                    found = true;
                    pVehicle->unblockPath();
                    break;
                }
            }
            if (!found) {
                setStateTo(Static::kStateDoorClosing);
            }
            // Peds: tile coords use (x + offsetY, y + offsetX)
            *pFixedOffset = -1;
            for (*pActiveOffset = -1; *pActiveOffset <= 1; (*pActiveOffset)++) {
                for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetY, y + offsetX, z))) {
                    if (obj->nature() != MapObject::kNaturePed) continue;
                    auto *pPed = static_cast<PedInstance *>(obj);
                    found_peds.push_back(pPed);
                }
            }
            *pFixedOffset = 1;
            for (*pActiveOffset = -1; *pActiveOffset <= 1; (*pActiveOffset)++) {
                for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetY, y + offsetX, z))) {
                    if (obj->nature() != MapObject::kNaturePed) continue;
                    auto *pPed = static_cast<PedInstance *>(obj);
                    found_peds.push_back(pPed);
                }
            }
            *pFixedOffset = 0;
            for (*pActiveOffset = -1; *pActiveOffset <= 1; (*pActiveOffset)++) {
                for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetY, y + offsetX, z))) {
                    if (obj->nature() != MapObject::kNaturePed) continue;
                    auto *pPed = static_cast<PedInstance *>(obj);
                    found_peds_mid.push_back(pPed);
                }
            }
            if (state_ == Static::kStateDoorOpen) {
                for (PedInstance *foundPed : found_peds) {
                    foundPed->unblockPath();
                }
                for (PedInstance *foundPedMid : found_peds_mid) {
                    foundPedMid->unblockPath();
                }
            } else {
                {
                    int xadj = (orientation_ == kStaticOrientationNS) ? 1 : 0;
                    int yadj = (orientation_ == kStaticOrientationEW) ? 1 : 0;
                    for (PedInstance *pFoundPed : found_peds) {
                        pFoundPed->blockPathWith(this, true, x, y, z, xadj, yadj);
                    }
                }
                for (PedInstance *pFoundPedMid : found_peds_mid) {
                    DamageToInflict d;
                    d.dtype = kDmgTypeCollision;
                    d.d_owner = NULL;
                    d.dvalue = 1024;
                    d.ddir = -1;
                    pFoundPedMid->handleHit(d);
                }
            }
            break;
        }
        case Static::kStateDoorClosed: {
            if (orientation_ == kStaticOrientationNS) {
                pActiveOffset = &offsetY;
                pFixedOffset = &offsetX;
                sign = 1;
            } else if (orientation_ == kStaticOrientationEW) {
                pActiveOffset = &offsetX;
                pFixedOffset = &offsetY;
                sign = -1;
            }
            assert(pActiveOffset != nullptr && pFixedOffset != nullptr);
            *pFixedOffset = static_cast<char>(-1 * sign);
            for (*pActiveOffset = -2; *pActiveOffset <= 2; (*pActiveOffset)++) {
                for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetX, y + offsetY, z))) {
                    if (obj->nature() != MapObject::kNatureVehicle) continue;
                    auto *pVehicle = static_cast<ShootableMovableMapObject *>(obj);
                    if (!found) { setStateTo(Static::kStateDoorOpening); found = true; }
                    pVehicle->blockPathWith(this);
                    break;
                }
            }
            *pFixedOffset = static_cast<char>(1 * sign);
            for (*pActiveOffset = -2; *pActiveOffset <= 2; (*pActiveOffset)++) {
                for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetX, y + offsetY, z))) {
                    if (obj->nature() != MapObject::kNatureVehicle) continue;
                    auto *pVehicle = static_cast<ShootableMovableMapObject *>(obj);
                    if (!found) { setStateTo(Static::kStateDoorOpening); found = true; }
                    pVehicle->blockPathWith(this);
                    break;
                }
            }
            for (*pFixedOffset = -1; *pFixedOffset <= 1; (*pFixedOffset)++) {
                for (*pActiveOffset = -1; *pActiveOffset <= 1; (*pActiveOffset)++) {
                    for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetY, y + offsetX, z))) {
                        if (obj->nature() != MapObject::kNaturePed) continue;
                        auto *pPed = static_cast<PedInstance *>(obj);
                        found_peds.push_back(pPed);
                    }
                }
            }
            terminatePath = (state_ != Static::kStateDoorOpening);
            {
                int xadj = (orientation_ == kStaticOrientationNS) ? 1 : 0;
                int yadj = (orientation_ == kStaticOrientationEW) ? 1 : 0;
                for (PedInstance *pFoundPed : found_peds) {
                    pFoundPed->blockPathWith(this, terminatePath, x, y, z, xadj, yadj);
                }
            }
            break;
        }
        case Static::kStateDoorOpening: {
            if (orientation_ == kStaticOrientationNS) {
                pActiveOffset = &offsetY;
                pFixedOffset = &offsetX;
                sign = 1;
            } else if (orientation_ == kStaticOrientationEW) {
                pActiveOffset = &offsetX;
                pFixedOffset = &offsetY;
                sign = -1;
            }
            assert(pActiveOffset != nullptr && pFixedOffset != nullptr);
            terminatePath = true;
            *pFixedOffset = static_cast<char>(-1 * sign);
            for (*pActiveOffset = -2; *pActiveOffset <= 2; (*pActiveOffset)++) {
                for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetX, y + offsetY, z))) {
                    if (obj->nature() != MapObject::kNatureVehicle) continue;
                    auto *pVehicle = static_cast<ShootableMovableMapObject *>(obj);
                    pVehicle->blockPathWith(this);
                    break;
                }
            }
            *pFixedOffset = static_cast<char>(1 * sign);
            for (*pActiveOffset = -2; *pActiveOffset <= 2; (*pActiveOffset)++) {
                for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetX, y + offsetY, z))) {
                    if (obj->nature() != MapObject::kNatureVehicle) continue;
                    auto *pVehicle = static_cast<ShootableMovableMapObject *>(obj);
                    pVehicle->blockPathWith(this);
                    break;
                }
            }
            for (*pFixedOffset = -1; *pFixedOffset <= 1; (*pFixedOffset)++) {
                for (*pActiveOffset = -1; *pActiveOffset <= 1; (*pActiveOffset)++) {
                    for (MapObject *obj : pMission->getObjectsAtTile(TilePoint(x + offsetY, y + offsetX, z))) {
                        if (obj->nature() != MapObject::kNaturePed) continue;
                        auto *pPed = static_cast<PedInstance *>(obj);
                        found_peds.push_back(pPed);
                    }
                }
            }
            {
                int xadj = (orientation_ == kStaticOrientationNS) ? 1 : 0;
                int yadj = (orientation_ == kStaticOrientationEW) ? 1 : 0;
                for (PedInstance *pFoundPed : found_peds) {
                    pFoundPed->blockPathWith(this, terminatePath, x, y, z, xadj, yadj);
                }
            }
            break;
        }
        default:
            break;
    }
}
    
void LargeDoor::handleAnimationEnded() {
    if (animationPlayer_->isCurrentAnimation(closingAnim_)) {
        setStateTo(Static::kStateDoorClosed);
    } else if (animationPlayer_->isCurrentAnimation(openingAnim_)) {
        setStateTo(Static::kStateDoorOpen);
    }
}


/*!
 * @brief 
 * @param anId 
 * @param pMap 
 * @param baseAnim 
 */
Tree::Tree(uint16_t anId, Map *pMap, uint16_t baseAnim) :
        Static(anId, pMap, Static::smt_Tree) {
    idleAnim_ = animationPlayer_->addAnimation(baseAnim);
    // Original DOS Syndicate tree burn duration is roughly 10 seconds (~160 ticks @ ~16 Hz = 10000 ms)
    burningAnim_ = animationPlayer_->addAnimation(baseAnim + 1, rs_eng::kAnimationModeLoop, 8, 10000);
    burntAnim_ = animationPlayer_->addAnimation(baseAnim + 2, rs_eng::kAnimationModeSingle, 2);
    animationPlayer_->play(idleAnim_);
}

void Tree::draw(const Point2D &screenPos) {
    Point2D posWithOffs = addOffs(screenPos);
    animationPlayer_->draw(posWithOffs, 0);
}

void Tree::handleAnimationEnded() {
    if (animationPlayer_->isCurrentAnimation(burningAnim_)) {
        animationPlayer_->play(burntAnim_);
    }
}

/*!
 * Implementation for the Tree. Tree burns only when hit by laser of fire.
 * \param d Damage information
 */
void Tree::handleHit(DamageToInflict &d) {
    if (isAlive() &&
        (d.dtype == kDmgTypeBullet || d.dtype == kDmgTypeLaser || d.dtype == kDmgTypeBurn || d.dtype == kDmgTypeExplosion)) {
        decreaseHealth(d.dvalue);
        if (isDead()) {
            animationPlayer_->play(burningAnim_);
            deactivateBlocking();
        }
    }
}

/*!
 * Constructor for the class
 * @param anId 
 * @param pMap 
 * @param state Initial state for the window
 * @param anim base animation. Other animations are always 2 after
 */
WindowObj::WindowObj(uint16_t anId, Map *pMap, StateWindows state, uint16_t anim):
        Static(anId, pMap, Static::smt_Window), state_(state) {
    orientation_ = -1;
    baseAnim_ = anim;
    // We don't need to retain the id of the open/close anim as we
    // never come back to them after we move to breaking/damage anim
    if (state == kStateWindowOpen) {
        playAnimation(animationPlayer_->addAnimation(anim + 2));
    } else if (state == kStateWindowClosed) {
        playAnimation(animationPlayer_->addAnimation(anim));
    }
    breakingAnim_ = animationPlayer_->addAnimation(anim + 4, rs_eng::kAnimationModeSingle, 20);
    damagedAnim_ = animationPlayer_->addAnimation(anim + 6);

    if (state == kStateWindowDamaged) {
        playAnimation(damagedAnim_);
    }
}

void WindowObj::setOrientation(int anOrientation) {
    if (orientation_ == anOrientation) return;
    Static::setOrientation(anOrientation);
    if (anOrientation == kStaticOrientationEW) {
        setSize(4, 96, 96);
    } else {
        setSize(96, 4, 96);
    }
    uint16_t anim = 550;
    if (state_ == kStateWindowOpen) {
        anim = (anOrientation == kStaticOrientationEW) ? 553 : 552;
    } else if (state_ == kStateWindowDamaged) {
        anim = (anOrientation == kStaticOrientationEW) ? 557 : 556;
    } else { // closed
        anim = (anOrientation == kStaticOrientationEW) ? 551 : 550;
    }
    baseAnim_ = anim;
    uint16_t base = (anOrientation == kStaticOrientationEW) ? 551 : 550;
    breakingAnim_ = animationPlayer_->addAnimation(base + 4, rs_eng::kAnimationModeSingle, 20);
    damagedAnim_ = animationPlayer_->addAnimation(base + 6);
    playAnimation(animationPlayer_->addAnimation(anim));
}

void WindowObj::handleAnimationEnded() {
    if (animationPlayer_->isCurrentAnimation(breakingAnim_)) {
        animationPlayer_->play(damagedAnim_);
    }
}

void WindowObj::draw(const Point2D &screenPos) {
    animationPlayer_->draw(addOffs(screenPos), 0);
}

/*!
 * @brief 
 * @param d 
 */
void WindowObj::handleHit(DamageToInflict &d) {
    if (isAlive() &&
        (d.dtype == kDmgTypeBullet || d.dtype == kDmgTypeExplosion || d.dtype == kDmgTypeLaser)) {
        decreaseHealth(d.dvalue);
        if (isDead()) {
            deactivateBlocking();
            playAnimation(breakingAnim_);
        }
    }
}

/*!
 * Constructor
 * @param anId 
 * @param pMap 
 * @param baseAnim 
 * @param aType 
 */
EtcObj::EtcObj(uint16_t anId, Map *pMap, uint16_t baseAnim, StaticType aType) :
        Static(anId, pMap, aType) {
    baseAnim_ = baseAnim;
    rs_eng::AnimationMode mode = aType == Static::smt_Advertisement ?
            rs_eng::kAnimationModeLoop : rs_eng::kAnimationModeSingle;
    idleAnim_ = animationPlayer_->addAnimation(baseAnim, mode);
    animationPlayer_->play(idleAnim_);
}

void EtcObj::draw(const Point2D &screenPos) {
    animationPlayer_->draw(addOffs(screenPos), 0);
}

void EtcObj::setOrientation(int anOrientation) {
    if (orientation_ == anOrientation) return;
    Static::setOrientation(anOrientation);
    if (subType_ == 0x01) { // Phone Booth (618 = NS, 619 = EW)
        uint16_t anim = (anOrientation == kStaticOrientationEW) ? 619 : 618;
        baseAnim_ = anim;
        idleAnim_ = animationPlayer_->addAnimation(anim, rs_eng::kAnimationModeSingle);
        animationPlayer_->play(idleAnim_);
    } else if (subType_ == 0x1F) { // Wall Ad / Billboard
        if (baseAnim_ == 572 || baseAnim_ == 573) {
            uint16_t anim = (anOrientation == kStaticOrientationEW) ? 573 : 572;
            baseAnim_ = anim;
            idleAnim_ = animationPlayer_->addAnimation(anim, rs_eng::kAnimationModeLoop);
            animationPlayer_->play(idleAnim_);
        } else if (baseAnim_ == 574 || baseAnim_ == 575) {
            uint16_t anim = (anOrientation == kStaticOrientationEW) ? 575 : 574;
            baseAnim_ = anim;
            idleAnim_ = animationPlayer_->addAnimation(anim, rs_eng::kAnimationModeLoop);
            animationPlayer_->play(idleAnim_);
        }
    }
}

/*!
 * @brief Constructor of the class
 * @param anId 
 * @param pMap 
 * @param anim 
 */
NeonSign::NeonSign(uint16_t anId, Map *pMap, uint16_t anim, uint16_t currentFrame) : 
        Static(anId, pMap, Static::smt_NeonSign) {
    baseAnim_ = anim;
    uint16_t animation = animationPlayer_->addAnimation(anim, rs_eng::kAnimationModeLoop);
    animationPlayer_->play(animation, g_SpriteMgr.getFrameFromFrameIndx(currentFrame));
    deactivateBlocking();
    setSize(32, 1, 48);
}

void NeonSign::draw(const Point2D &screenPos) {
    animationPlayer_->draw(addOffs(screenPos), 0);
}

void NeonSign::setOrientation(int anOrientation) {
    if (orientation_ == anOrientation) return;
    Static::setOrientation(anOrientation);
    if (anOrientation == kStaticOrientationEW) {
        setSize(1, 32, 48);
    } else {
        setSize(32, 1, 48);
    }
    uint16_t b = baseAnim();
    if (b != 0) {
        uint16_t anim = (anOrientation == kStaticOrientationEW) ? ((b % 2 == 0) ? b + 1 : b) : ((b % 2 != 0) ? b - 1 : b);
        baseAnim_ = anim;
        uint16_t animation = animationPlayer_->addAnimation(anim, rs_eng::kAnimationModeLoop);
        animationPlayer_->play(animation);
    }
}

/*!
 * Constructor
 * @param anId 
 * @param pMap 
 * @param animationOffset offset to the base animation which is 1040. Should not be higher than 4
 */
Semaphore::Semaphore(uint16_t anId, Map *pMap, uint16_t animationOffset) :
        Static(anId, pMap, Static::smt_Semaphore),
        elapsed_left_smaller_(0), elapsed_left_bigger_(0), up_down_(1),
        colorTimer_(700), animOffset_(animationOffset) {
    // regular animation
    playAnimation(animationPlayer_->addAnimation(1040));
}

void Semaphore::doUpdateState(uint32_t elapsed) {
    if (isDead()) {
        if (elapsed_left_bigger_ == 0)
            return;
        int chng = (elapsed + elapsed_left_smaller_) >> 1;
        elapsed_left_smaller_ = elapsed & 2;
        elapsed_left_bigger_ -= chng;
        if (elapsed_left_bigger_ < 0) {
            chng += elapsed_left_bigger_;
            elapsed_left_bigger_ = 0;
        }
        int z = pos_.tz * 128 + pos_.oz - chng;
        pos_.tz = z / 128;
        pos_.oz = z % 128;
        return;
    }

    int chng = (elapsed + elapsed_left_smaller_) >> 2;
    elapsed_left_smaller_ = elapsed & 4;
    if (chng) {
        int oz = pos_.oz + chng * up_down_;
        if (oz > 127) {
            oz = 127 - (oz & 0x7F);
            up_down_ -= 2;
        } else if (oz < 64) {
            oz = 64 + (64 - oz);
            up_down_ += 2;
        }
        pos_.oz = oz;
    }

    if (colorTimer_.update(elapsed)) {
        animOffset_++;
        if (animOffset_ >= kSemaphoreMaxColorAnim)
            animOffset_ = 0;
    }
}

/*!
 * Implementation for the Semaphore.
 * \param d Damage information
 */
void Semaphore::handleHit(DamageToInflict &d) {
    if (isAlive() &&
        (d.dtype == kDmgTypeLaser || d.dtype == kDmgTypeExplosion)) {
        decreaseHealth(d.dvalue);
        if (isDead()) {
            animOffset_ = kSemaphoreDamagedOffset;
            // When the semaphore is dead, it falls on the ground
            // In this case we use elapsed_left_bigger as the distance to the ground
            // to animate the falling of the semaphore
            elapsed_left_bigger_ = 0;
            TilePoint groundPoint = pos_;

            // look for the first tile below that is a ground
            while (groundPoint.tz >= 0) {
                groundPoint.tz--;
                if (pMap_->getTileAt(groundPoint)->isRoadMark()) {
                    elapsed_left_bigger_ = (pos_.tz - groundPoint.tz - 1) * 128 + pos_.oz;
                    break;
                }
            }
            
            
            deactivateBlocking();
        }
    }
}

void Semaphore::draw(const Point2D &screenPos) {
    animationPlayer_->draw(addOffs(screenPos), animOffset_);
}

AnimWindow::AnimWindow(uint16_t anId, Map *pMap, uint16_t anim) : Static(anId, pMap, smt_AnimatedWindow) {
    baseAnim_ = anim;
    orientation_ = (anim % 2 == 0) ? kStaticOrientationEW : kStaticOrientationNS;
    deactivateBlocking();
    //setFramesPerSec(4);
    animLigthOff_ = animationPlayer_->addAnimation(anim);
    animLigthSwitching_ = animationPlayer_->addAnimation(anim + 2);
    animPedAppears_ = animationPlayer_->addAnimation(anim + 4);
    animShowPed_ = animationPlayer_->addAnimation(anim + 6);
    animPedDisappears_ = animationPlayer_->addAnimation(anim + 8);
    animLightOn_ = animationPlayer_->addAnimation(anim);
}

void AnimWindow::setOrientation(int anOrientation) {
    if (orientation_ == anOrientation) return;
    Static::setOrientation(anOrientation);
    uint16_t b = baseAnim();
    if (b >= 1959 && b <= 1968) {
        uint16_t anim = (anOrientation == kStaticOrientationEW) ? ((b % 2 == 0) ? b : b + 1) : ((b % 2 != 0) ? b : b - 1);
        baseAnim_ = anim;
        animLigthOff_ = animationPlayer_->addAnimation(anim);
        animLigthSwitching_ = animationPlayer_->addAnimation(anim + 2);
        animPedAppears_ = animationPlayer_->addAnimation(anim + 4);
        animShowPed_ = animationPlayer_->addAnimation(anim + 6);
        animPedDisappears_ = animationPlayer_->addAnimation(anim + 8);
        animLightOn_ = animationPlayer_->addAnimation(anim);
        playLightOffAnimation();
    }
}

void AnimWindow::draw(const Point2D &screenPos) {
    // When light is on, don't draw window
    // because lighted window is part of the map
    if (animationPlayer_->isCurrentAnimation(animLightOn_))
        return;

    animationPlayer_->draw(addOffs(screenPos), 0);
}

void AnimWindow::handleAnimationEnded() {
    if (animationPlayer_->isCurrentAnimation(animLigthOff_)) {
        // decide to start switching lights on
        // or continue being in dark
        if (rand() % 100 > 60) {
            playLightOffAnimation();
        } else {
            playLightSwitchingAnimation();
        }
    } else if (animationPlayer_->isCurrentAnimation(animLigthSwitching_)) {
        playLightOnAnimation();
    } else if (animationPlayer_->isCurrentAnimation(animPedAppears_)) {
        playShowPedAnimation();
    } else if (animationPlayer_->isCurrentAnimation(animShowPed_)) {
        // continue showing ped or hide it
        if (rand() % 100 > 50) {
            playShowPedAnimation();
        } else {
            animationPlayer_->play(animPedDisappears_);
        }
    } else if (animationPlayer_->isCurrentAnimation(animPedDisappears_)) {
        playLightOnAnimation();
    } else if (animationPlayer_->isCurrentAnimation(animLightOn_)) {
        // we will continue showing lightson or switch
        // lights off or show ped
        int rnd_v = rand() % 100;
        if (rnd_v > 80) {
            playLightOnAnimation();
        } else if (rnd_v > 60) {
            animationPlayer_->play(animPedAppears_);
        } else if (rnd_v > 10) {
            playLightOffAnimation();
        } else {
            playLightSwitchingAnimation();
        }
    }
}

}