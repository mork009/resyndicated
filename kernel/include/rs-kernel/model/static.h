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
 *   Copyright (C) 2013, 2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef MAPOBJECT_H
#define MAPOBJECT_H

#include <math.h>
#include <list>

#include "rs-kernel/model/mapobject.h"
#include "rs-utils/misc/timer.h"
#include "rs-engine/sound/sound.h"

namespace rs_knl {

/*!
 * Static map object class.
 */
class Static : public ShootableMapObject {
public:
    /*! Const for orientation on North-South axis of Static.*/
    static const int kStaticOrientationNS;
    /*! Const for orientation on East-West axis of Static.*/
    static const int kStaticOrientationEW;

    enum StaticType {
        // NOTE: should be the same name as Class
        smt_None = 0,
        smt_Advertisement,
        smt_Semaphore,
        smt_Door,
        smt_LargeDoor,
        smt_Tree,
        smt_Window,
        smt_AnimatedWindow,
        smt_NeonSign
    };

    enum StateDoors {
        kStateDoorClosed = 0,
        kStateDoorClosing,
        kStateDoorOpen,
        kStateDoorOpening
    };

public:
    static Static *loadInstance(uint8_t *data, uint16_t id, Map *pMap);
    virtual ~Static() {}

    //! Return the type of statics
    StaticType type() { return type_; }
    //! Get Bullfrog level data sub_type
    uint8_t subType() const { return subType_; }
    //! Set Bullfrog level data sub_type
    void setSubType(uint8_t st) { subType_ = st; }
    //! Set the orientation of statics
    virtual void setOrientation(int anOrientation) { orientation_ = anOrientation; }
    //! Return the orientation of statics
    int orientation() const { return orientation_; }
    //! Get base animation id
    uint16_t baseAnim() const { return baseAnim_ != 0 ? baseAnim_ : getDrawAnimation(); }
    //! Set base animation id
    void setBaseAnim(uint16_t a) { baseAnim_ = a; }

    //! Return true if this static is attached to a wall (e.g. window, ad, sign, door)
    bool isWallMounted() const {
        return type_ == smt_Window ||
               type_ == smt_AnimatedWindow ||
               type_ == smt_Advertisement ||
               type_ == smt_NeonSign ||
               type_ == smt_Door ||
               type_ == smt_LargeDoor;
    }

protected:
    Static(uint16_t anId, Map *pMap, StaticType aType) :
            ShootableMapObject(anId, pMap, MapObject::kNatureStatic),
            type_(aType), orientation_(kStaticOrientationNS), subType_(0), baseAnim_(0) {
    }

protected:
    /*! Type of statics.*/
    StaticType type_;
    /*! Some statics can be displayed in 1 of 2 orientations : SW or SE.*/
    int orientation_;
    /*! Original Bullfrog level data sub_type.*/
    uint8_t subType_;
    /*! Base animation ID for this static entity.*/
    uint16_t baseAnim_;
};

/*!
 * Door are one tile doors. It can be open or closed when a Ped is in front.
 */
class Door : public Static {
public:
    Door(uint16_t id, Map *pMap, uint16_t baseAnim, Static::StateDoors initialState);
    virtual ~Door() {}

    void draw(const Point2D &screenPos) override;
    void setOrientation(int anOrientation) override;

    void setStateTo(StateDoors newState);
    StateDoors doorState() const { return state_; }
    uint16_t closedAnim() const { return closedAnim_; }
    uint16_t openingAnim() const { return openingAnim_; }
    uint16_t openedAnim() const { return openedAnim_; }
    uint16_t closingAnim() const { return closingAnim_; }
    rs_eng::InGameSample doorSound() const { return doorSound_; }
    uint32_t openHoldTimer() const { return openHoldTimer_; }

protected:
    void doUpdateState(uint32_t elapsed) override;
    void handleAnimationEnded() override;

protected:
    //! Id of the closed door animation
    uint16_t closedAnim_;
    //! Id of the closing door animation
    uint16_t closingAnim_;
    //! Id of the opened door animation
    uint16_t openedAnim_;
    //! Id of the opening door animation
    uint16_t openingAnim_;
    //! State to manage if door is open or closed
    StateDoors state_{kStateDoorClosed};
    //! Specific door sound assigned to this door style
    rs_eng::InGameSample doorSound_{rs_eng::DOOR};
    //! Timer in ms to keep the door open before auto-closing
    uint32_t openHoldTimer_{0};
};

class Mission;

/*!
 * LargeDoor are portal on the roads.
 */
class LargeDoor : public Static {
public:
    LargeDoor(uint16_t id, Map *pMap, uint16_t baseAnim);
    virtual ~LargeDoor();

    void draw(const Point2D &screenPos) override;
    void setOrientation(int anOrientation) override;

    void setStateTo(StateDoors newState);
    StateDoors doorState() const { return state_; }
    uint16_t closedAnim() const { return closedAnim_; }
    uint16_t openingAnim() const { return openingAnim_; }
    uint16_t closingAnim() const { return closingAnim_; }

    //! Initialize navigation graph blocking for the tiles occupied by this gate
    void initNavigationBlocker(Mission *pMission);
    const std::vector<int> &coveredTiles() const { return coveredTiles_; }

protected:
    void doUpdateState(uint32_t elapsed) override;
    void handleAnimationEnded() override;

protected:
    //! Id of the closed door animation (default animation)
    uint16_t closedAnim_;
    //! Id of the closing door animation
    uint16_t closingAnim_;
    //! Id of the opening door animation
    uint16_t openingAnim_;
    //! State to manage if door is open or closed
    StateDoors state_{kStateDoorClosed};
    //! Tile indices in Mission::mdpoints_ covered by this gate
    std::vector<int> coveredTiles_;
    //! Pointer to active mission
    Mission *pMission_ = nullptr;
};
/*!
 * Tree map object class.
 */
class Tree : public Static {
public:
    Tree(uint16_t id, Map *pMap, uint16_t baseAnim);
    virtual ~Tree() {}

    void draw(const Point2D &screenPos) override;
    
    void handleHit(DamageToInflict &d)override;

    uint16_t idleAnim() const { return idleAnim_; }
    uint16_t burningAnim() const { return burningAnim_; }
    uint16_t burntAnim() const { return burntAnim_; }

protected:
    void handleAnimationEnded() override;

protected:
    uint16_t idleAnim_;
    uint16_t burningAnim_;
    uint16_t burntAnim_;
};

/*!
 * WindowObj represents simple windows that are open or close.
 * It can be broken with bullet or explosion.
 */
class WindowObj : public Static {
public:
    //! List of initial states for windows
    enum StateWindows {
        kStateWindowClosed = 0,
        kStateWindowOpen,
        kStateWindowDamaged
    };

    WindowObj(uint16_t id, Map *pMap, StateWindows state, uint16_t anim);
    virtual ~WindowObj() {}

    void draw(const Point2D &screenPos) override;
    void handleHit(DamageToInflict &d) override;
    void setOrientation(int anOrientation) override;
    StateWindows state() const { return state_; }
    uint16_t breakingAnim() const { return breakingAnim_; }
    uint16_t damagedAnim() const { return damagedAnim_; }
    void handleAnimationEnded() override;

protected:
    //! Initial/current state of the window
    StateWindows state_;
    //! Animation for when window is breaking
    uint16_t breakingAnim_;
    //! After window wa broken, it is damaged
    uint16_t damagedAnim_;
};

/*!
 * EtcObj map object class.
 */
class EtcObj : public Static {
public:
    EtcObj(uint16_t id, Map *pMap, uint16_t baseAnim, StaticType type = smt_None);
    virtual ~EtcObj() {}

    void draw(const Point2D &screenPos) override;
    void setOrientation(int anOrientation) override;

protected:
    uint16_t idleAnim_;
};

/*!
 * NeonSign map object class.
 */
class NeonSign : public Static {
public:
    NeonSign(uint16_t id, Map *pMap, uint16_t anim, uint16_t currentFrame);
    virtual ~NeonSign() {}

    void draw(const Point2D &screenPos) override;
    void setOrientation(int anOrientation) override;
};

/*!
 * Semaphore map object class.
 * That thing that bounces on crossroad.
 * For animation, there are 4 animations when the semaphore is up
 * and 1 animation when the semaphore has been damaged. We cycle through
 * the animation by incrementing animOffset_.
 */
class Semaphore : public Static {
public:
    Semaphore(uint16_t id, Map *pMap, uint16_t animationOffset);
    virtual ~Semaphore() {}

    void draw(const Point2D &screenPos) override;
    uint16_t getDrawAnimation() const override { return Static::getDrawAnimation() + animOffset_; }

    void handleHit(DamageToInflict &d) override;

protected:
    void doUpdateState(uint32_t elapsed) override;

protected:
    //! Number of animations when semaphore is on
    static const uint16_t kSemaphoreMaxColorAnim;
    //! Offset for the damaged animation relative to the base animation
    static const uint16_t kSemaphoreDamagedOffset;

    /*! used to make animation of movement up/down,
     * when damaged, stores time not consumed for movement down
     */
    int elapsed_left_smaller_;
    /*! animation color rotation,
     * when damaged, stores target Z distance to fall
     */
    int elapsed_left_bigger_;
    //! switch for moving up or down
    int up_down_;
    //! Timer to control the change of light for the semaphore
    rs_utl::Timer colorTimer_;
    //! Use to cycle through different animations including damaged.
    uint16_t animOffset_;
};

/*!
 * AnimatedWindow has multiple animations.
 */
class AnimWindow : public Static {
public:
    AnimWindow(uint16_t id, Map *pMap, uint16_t anim);
    virtual ~AnimWindow() {}

    void draw(const Point2D &screenPos) override;
    void setOrientation(int anOrientation) override;

    void playLightOffAnimation() {
        animationPlayer_->play(animLigthOff_, 0, static_cast<uint32_t> (30000 + (rand() % 30000)));
    }

    void playLightSwitchingAnimation() {
        animationPlayer_->play(animLigthSwitching_, 0, static_cast<uint32_t> (1000 + (rand() % 1000)));
    }

    void playShowPedAnimation() {
        animationPlayer_->play(animShowPed_, 0, static_cast<uint32_t> (15000 + (rand() % 5000)));
    }

    void playedDisappearAnimation() {
        animationPlayer_->play(animPedDisappears_);
    }

    void playLightOnAnimation() {
        animationPlayer_->play(animLightOn_, 0, static_cast<uint32_t> (30000 + (rand() % 30000)));
    }

protected:
    void handleAnimationEnded() override;

public:
    //! Id of the light off animation
    uint16_t animLigthOff_;
    //! Id of the light switching animation
    uint16_t animLigthSwitching_;
    //! This is special as there is no animation for this
    uint16_t animLightOn_;
    //! Id of the ped appearing animation
    uint16_t animPedAppears_;
    //! Id of the ped showed animation
    uint16_t animShowPed_;
    //! Id of the ped disappearing animation
    uint16_t animPedDisappears_;
};

}
#endif
