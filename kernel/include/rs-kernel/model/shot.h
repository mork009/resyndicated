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

#ifndef MODEL_SHOT_H_
#define MODEL_SHOT_H_

#include <vector>
#include <list>

#include "rs-kernel/model/sfxobject.h"

namespace rs_knl {

class Mission;
class WeaponInstance;
class PedInstance;

/*!
 * A shot is the result of the action of a weapon.
 * It's the shot that inflicts damage.
 */
class Shot {
 public:
    explicit Shot(const DamageToInflict &dmg) {
        dmg_ = dmg;
    }
    virtual ~Shot() {}

    virtual void inflictDamage(Mission *pMission) = 0;

    DamageToInflict & getAttributes() { return dmg_; }
 protected:
    //! The damage that will be inflicted by this shot
    DamageToInflict dmg_;
};

/*!
 * This class represents a shot where the impacts are immediatly
 * made. Weapons who use this type of shot are pistol, minigun, Shotgun,
 * uzi, laser, long range.
 */
class InstantImpactShot : public Shot {
 public:
    explicit InstantImpactShot(
        const DamageToInflict &dmg) : Shot(dmg) {}

    void inflictDamage(Mission *pMission) override;
 private:
    //! Spread the impact on the ground
    void diffuseImpact(Mission *m, const WorldPoint &originLocW,
                       WorldPoint *pImpactPosW);

    //!
    void createImpactAnimation(Mission *pMission,
                               ShootableMapObject * pTargetHit,
                               const WorldPoint impactPosW);
};

/*!
 * This class represents an explosion caused by a time bomb, an agent's
 * suicide or a car explosion.
 */
class Explosion : public Shot {
 public:
    //! Creates an explosion at the location of owner
    static void createExplosion(Mission *pMission, ShootableMapObject *pOwner,
                                double range, int dmgValue = 16);
    //! Creates an explosion at the given location
    static void createExplosion(Mission *pMission, ShootableMapObject *pOwner,
                                const WorldPoint &location, double range,
                                int dmgValue);

    explicit Explosion(const DamageToInflict &dmg);

    void inflictDamage(Mission *pMission) override;
 private:
    void generateFlameWaves(Mission *pMission, WorldPoint *pOrigin, double dmg_rng);
    void generateGaussImpactFlames(Mission *pMission, WorldPoint *pOrigin, double dmg_rng);
    //!
    void getAllShootablesWithinRange(
        Mission *pMission,
        const WorldPoint &originLocW,
        std::vector<ShootableMapObject *> &objInRangeLst);

 private:
    /*! Type of animation for the explosion.*/
    SFXObject::SfxType rngDmgAnim_;
};

/*!
 * Base class of shots whose path is drawn.
 */
class ProjectileShot: public Shot {
 public:
    //! Constructor
    explicit ProjectileShot(const DamageToInflict &dmg);

    //! Animate the shot
    virtual bool animate(uint32_t elapsed, Mission *m);

    //! Returns true if shot can be destroyed
    bool isLifeOver() { return lifeOver_; }

    //! Returns true if this projectile represents an American Revolt Air Strike bomber run
    virtual bool isAirStrike() const { return false; }

    //! Current world position of the projectile / bomber
    const WorldPoint &currentPosition() const { return curPosW_; }
    //! Unit direction vector X component
    double dirX() const { return incX_; }
    //! Unit direction vector Y component
    double dirY() const { return incY_; }

 protected:
    //! Update projectile position
    virtual bool moveProjectile(uint32_t elapsed, Mission *pMission);
    virtual void drawTrace(Mission *pMission) = 0;
 protected:
    /*! This tells if the shot object shot be destroyed.*/
    bool lifeOver_;
    bool startMove_;
    /*! Projectile speed.*/
    double speed_;

    /*! Current position of projectile.*/
    WorldPoint curPosW_;
    /*! Position of the target.*/
    WorldPoint targetLocW_;

    /*! Projectile position incrementation on X axis.*/
    double incX_;
    /*! Projectile position incrementation on Y axis.*/
    double incY_;
    /*! Projectile position incrementation on Z axis.*/
    double incZ_;

    /*! Maximum distance the projectile can go.*/
    double distanceMax_;
    /*! Updated distance from origin to current projectile's position.*/
    double currentDistance_;
    /*! flag to know if we can draw the impact of the shot.*/
    bool drawImpact_;
    /*! Not null if the projectile has to touch smth destroyable.*/
    ShootableMapObject * pShootableHit_;
};

/*!
 * Shot made by Gauss Gun.
 * The Gauss Gun shoots a projectile that flies until it hist its target
 * then it explodes burning everything around.
 * When flying, the projectile leaves a trace of smoke behind it.
 */
class GaussGunShot: public ProjectileShot {
 public:
    //! Constructor
    explicit GaussGunShot(const DamageToInflict &dmg);
    //! Destructor
    ~GaussGunShot() {}

    void inflictDamage(Mission *pMission) override;
 protected:
    //! Update projectile position
    void drawTrace(Mission *pMission);
 protected:
    /*! Position of the last trace animation.*/
    double lastAnimDist_;
};

/*!
 * Shot made with the flame thrower.
 */
class FlamerShot: public ProjectileShot {
 public:
    //! Constructor
    explicit FlamerShot(Mission *pMission, const DamageToInflict &dmg);

    void inflictDamage(Mission *pMission) override;

 protected:
    bool moveProjectile(uint32_t elapsed, Mission *pMission) override;
    void drawTrace(Mission *pMission) override;
    //! Interpolates a world position at arc distance s along trailPoints_
    WorldPoint sampleTrailAt(double s) const;
 protected:
    //! Arc distance along trailPoints_ of the last dropped flame sprite
    double lastAnimDist_{0.0};
    //! Distance traveled along trailPoints_ so far
    double traveledDist_{0.0};
    //! Total length of the curved DOS wind polyline
    double totalLength_{0.0};
    //! Cumulative arc lengths for each vertex in trailPoints_
    std::vector<double> cumulativeLens_;
    //! Curved world-space segments traversed by the flame stream (affected by wind)
    std::vector<WorldPoint> trailPoints_;
};

/*!
 * American Revolt Air Strike bombardment run.
 * Simulates a high-altitude bomber plane flying across the map over the target
 * coordinates (rendered as a moving white triangle on the minimap) and dropping
 * a rolling multi-tile carpet of high-explosive bombs along the flight corridor.
 */
class AirStrikeShot: public ProjectileShot {
 public:
    //! Constructor. If flightAngleRad is < 0, computes randomized inward angle based on target quadrant.
    explicit AirStrikeShot(const DamageToInflict &dmg, double flightAngleRad = -1.0);
    ~AirStrikeShot() override = default;

    bool isAirStrike() const override { return true; }
    void inflictDamage(Mission *pMission) override;

    //! Returns the world positions of all 5 planes in the V-formation
    std::vector<WorldPoint> getPlanePositions() const;

 protected:
    bool moveProjectile(uint32_t elapsed, Mission *pMission) override;
    void drawTrace(Mission *pMission) override;
    void executeStrafing(Mission *pMission, uint32_t elapsed);
    void initBombGrid();
    void updateBombingSweep(uint32_t elapsed, Mission *pMission);
    void detonateBomb(Mission *pMission, const WorldPoint &impactLocW, uint32_t durationMs);

 private:
    struct ScheduledBomb {
        WorldPoint location;
        int32_t timerMs;
        uint32_t durationMs;
        bool detonated{false};
    };

    WorldPoint targetCenterW_;
    double targetDistAlongRun_;
    double lastSmokeDist_;
    uint32_t strafeTimer_{0};
    uint32_t strafeSoundTimer_{0};
    uint32_t explosionSoundTimer_{0};
    bool flyoverSoundPlayed_{false};
    bool bulletStrafeCompleted_{false};
    bool bombGridInitialized_{false};
    std::vector<ScheduledBomb> scheduledBombs_;
    int remainingBombsToDetonate_{0};
    int remainingBombsToFinish_{0};
};

}

#endif  // MODEL_SHOT_H_
