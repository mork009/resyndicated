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
 *   Copyright (C) 2011  Mark <mentor66@users.sourceforge.net>
 *   Copyright (C) 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-kernel/model/weapon.h"

#include "rs-engine/appcontext.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/sound/soundmanager.h"
#include "rs-utils/log/log.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/model/shot.h"

namespace rs_knl {

#define Z_SHIFT_TO_AIR   4
#define WEAPON_PROPERTY_PATTERN "weapon.{}.{}"

uint16_t WeaponInstance::weaponIdCnt = 0;

Weapon::Weapon(WeaponType w_type, ConfigFile &conf)
{
    type_ = w_type;
    submittedToSearch_ = false;

    switch(w_type) {
        case Weapon::Pistol:
            idx_ = Weapon::Pistol_Anim;
            sample_ = rs_eng::PISTOL;
            dmg_type_ = kDmgTypeBullet;
            shot_property_ = Weapon::wspt_Pistol;
            impactAnims_.groundHit = SFXObject::sfxt_BulletHit;
            impactAnims_.objectHit = SFXObject::sfxt_BulletHit;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::Minigun:
            idx_ = Weapon::Minigun_Anim;
            sample_ = rs_eng::MINIGUN;
            dmg_type_ = kDmgTypeBullet;
            shot_property_ = Weapon::wspt_Minigun;
            impactAnims_.groundHit = SFXObject::sfxt_BulletHit;
            impactAnims_.objectHit = SFXObject::sfxt_BulletHit;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::Flamer:
            idx_ = Weapon::Flamer_Anim;
            sample_ = rs_eng::FLAME;
            dmg_type_ = kDmgTypeBurn;
            shot_property_ = Weapon::wspt_Flamer;
            impactAnims_.groundHit = SFXObject::sfxt_FlamerFire;
            impactAnims_.objectHit = SFXObject::sfxt_FlamerFire;
            impactAnims_.trace_anim = SFXObject::sfxt_FlamerFire;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::LongRange:
            idx_ = Weapon::LongRange_Anim;
            sample_ = rs_eng::LONGRANGE;
            dmg_type_ = kDmgTypeBullet;
            shot_property_ = Weapon::wspt_LongRange;
            impactAnims_.groundHit = SFXObject::sfxt_BulletHit;
            impactAnims_.objectHit = SFXObject::sfxt_BulletHit;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::EnergyShield:
            idx_ = Weapon::EnergyShield_Anim;
            sample_ = rs_eng::NO_SOUND;
            dmg_type_ = kDmgTypeNone;
            shot_property_ = Weapon::wspt_EnergyShield;
            impactAnims_.groundHit = SFXObject::sfxt_Unknown;
            impactAnims_.objectHit = SFXObject::sfxt_Unknown;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::Uzi:
            idx_ = Weapon::Uzi_Anim;
            sample_ = rs_eng::UZI;
            dmg_type_ = kDmgTypeBullet;
            shot_property_ = Weapon::wspt_Uzi;
            impactAnims_.groundHit = SFXObject::sfxt_BulletHit;
            impactAnims_.objectHit = SFXObject::sfxt_BulletHit;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::Laser:
            idx_ = Weapon::Laser_Anim;
            sample_ = rs_eng::LASER;
            dmg_type_ = kDmgTypeLaser;
            shot_property_ = Weapon::wspt_Laser;
            impactAnims_.groundHit = SFXObject::sfxt_Fire_LongSmoke;
            impactAnims_.objectHit = SFXObject::sfxt_Unknown;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::GaussGun:
            idx_ = Weapon::Gauss_Anim;
            sample_ = rs_eng::GAUSSGUN;
            dmg_type_ = kDmgTypeExplosion;
            shot_property_ = Weapon::wspt_GaussGun;
            impactAnims_.groundHit = SFXObject::sfxt_ExplosionFire;
            impactAnims_.objectHit = SFXObject::sfxt_ExplosionBall;
            impactAnims_.trace_anim = SFXObject::sfxt_Smoke;
            impactAnims_.rd_anim = SFXObject::sfxt_ExplosionBall;
            break;
        case Weapon::Shotgun:
            idx_ = Weapon::Shotgun_Anim;
            sample_ = rs_eng::SHOTGUN;
            dmg_type_ = kDmgTypeBullet;
            shot_property_ = Weapon::wspt_Shotgun;
            impactAnims_.groundHit = SFXObject::sfxt_BulletHit;
            impactAnims_.objectHit = SFXObject::sfxt_BulletHit;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::MediKit:
            idx_ = Weapon::Unarmed_Anim;
            sample_ = rs_eng::NO_SOUND;
            dmg_type_ = kDmgTypeNone;
            shot_property_ = Weapon::wspt_MediKit;
            impactAnims_.groundHit = SFXObject::sfxt_Unknown;
            impactAnims_.objectHit = SFXObject::sfxt_Unknown;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::Scanner:
            idx_ = Weapon::Unarmed_Anim;
            sample_ = rs_eng::NO_SOUND;
            dmg_type_ = kDmgTypeNone;
            shot_property_ = Weapon::wspt_Scanner;
            impactAnims_.groundHit = SFXObject::sfxt_Unknown;
            impactAnims_.objectHit = SFXObject::sfxt_Unknown;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::AccessCard:
            idx_ = Weapon::Unarmed_Anim;
            sample_ = rs_eng::NO_SOUND;
            dmg_type_ = kDmgTypeNone;
            shot_property_ = Weapon::wspt_AccessCard;
            impactAnims_.groundHit = SFXObject::sfxt_Unknown;
            impactAnims_.objectHit = SFXObject::sfxt_Unknown;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::TimeBomb:
            idx_ = Weapon::Unarmed_Anim;
            sample_ = rs_eng::EXPLOSION;
            dmg_type_ = kDmgTypeExplosion;
            shot_property_ = Weapon::wspt_TimeBomb;
            impactAnims_.groundHit = SFXObject::sfxt_ExplosionFire;
            impactAnims_.objectHit = SFXObject::sfxt_ExplosionBall;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_ExplosionFire;
            break;
        case Weapon::Persuadatron:
            idx_ = Weapon::Unarmed_Anim;
            sample_ = rs_eng::PERSUADE;
            dmg_type_ = kDmgTypePersuasion;
            shot_property_ = Weapon::wspt_Persuadatron;
            impactAnims_.groundHit = SFXObject::sfxt_Unknown;
            impactAnims_.objectHit = SFXObject::sfxt_Unknown;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        case Weapon::AirStrike:
            idx_ = Weapon::Unarmed_Anim;
            sample_ = rs_eng::AIRSTRIKE;
            dmg_type_ = kDmgTypeExplosion;
            shot_property_ = Weapon::wspt_AirStrike;
            impactAnims_.groundHit = SFXObject::sfxt_ExplosionFire;
            impactAnims_.objectHit = SFXObject::sfxt_ExplosionBall;
            impactAnims_.trace_anim = SFXObject::sfxt_Smoke;
            impactAnims_.rd_anim = SFXObject::sfxt_LargeFire;
            break;
        case Weapon::CloneShield:
            idx_ = Weapon::CloneShield_Anim;
            sample_ = rs_eng::NO_SOUND;
            dmg_type_ = kDmgTypeNone;
            shot_property_ = Weapon::wspt_CloneShield;
            impactAnims_.groundHit = SFXObject::sfxt_Unknown;
            impactAnims_.objectHit = SFXObject::sfxt_Unknown;
            impactAnims_.trace_anim = SFXObject::sfxt_Unknown;
            impactAnims_.rd_anim = SFXObject::sfxt_Unknown;
            break;
        default:
#if _DEBUG
            printf("unknown weapon loaded(%i), NULL passed", w_type);
#endif
            break;
    }

    // initialize other properties
    initFromConfig(w_type, conf);
}

void Weapon::initFromConfig(WeaponType w_type, ConfigFile &conf) {
    std::string propName;
    int typeAsInt = static_cast<int>(w_type);

    try {
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "name");
        name_ = g_Ctx.getMessage(conf.read<std::string>(propName));

        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "icon.small");
        small_icon_ = conf.read<int>(propName);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "icon.big");
        big_icon_ = conf.read<int>(propName);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "cost");
        cost_ = conf.read<int>(propName);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "ammo.nb");
        ammoCapacity_ = conf.read<int>(propName, 0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "ammo.price");
        ammo_cost_ = conf.read<int>(propName, 0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "range");
        range_ = conf.read<int>(propName);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "rank");
        rank_ = conf.read<int>(propName, -1);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "anim");
        anim_ = conf.read<int>(propName);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "ammopershot");
        ammo_per_shot_ = conf.read<int>(propName, 0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "bomb.explosiondelay");
        explosionDelay_ = conf.read<uint32_t>(propName, 0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "reloadtime");
        reloadTime_ = conf.read<int>(propName, 0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "damagerange");
        range_dmg_ = conf.read<int>(propName, 0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "shotanglemin");
        shot_angle_min_ = conf.read<double>(propName, 0.0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "shotanglemax");
        shot_angle_max_ = conf.read<double>(propName, 0.0);
        if (shot_angle_min_ == 0.0 && shot_angle_max_ == 0.0) {
            // Backward compatibility fallback to legacy "shotangle"
            propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "shotangle");
            double legacyAngle = conf.read<double>(propName, 0.0);
            shot_angle_min_ = legacyAngle;
            shot_angle_max_ = legacyAngle;
        }
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "shotdepthmin");
        shot_depth_min_ = conf.read<double>(propName, 0.0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "shotdepthmax");
        shot_depth_max_ = conf.read<double>(propName, 0.0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "shotspeed");
        shot_speed_ = conf.read<int>(propName, 0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "dmg_per_shot");
        dmg_per_shot_ = conf.read<int>(propName, 0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "ammo.impactNb");
        impactsPerAmmo_ = conf.read<int>(propName, 0);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "weight");
        weight_ = conf.read<int>(propName);
        propName = std::format(WEAPON_PROPERTY_PATTERN, typeAsInt, "auto.fire_rate");
        fireRate_ = conf.read<int>(propName, 0);
    } catch (...) {
        RSERR(Log::k_FLG_GAME, "Weapon", "initFromConfig", ("Cannot load weapon %d : %s\n", w_type, propName.c_str()))
    }
}

int Weapon::calculateReloadingCost(int remaingAmmo) {
    return (ammoCapacity_ - remaingAmmo) * ammo_cost_;
}

/*!
 * Creates a new instance of Weapon instance for the given weapon class.
 * \param pWeaponClass Class of weapon
 * \param Current amount of ammo. If value is -1, ammo is full
 * \return an instance of WeaponInstance
 */
WeaponInstance *WeaponInstance::createInstance(Weapon *pWeaponClass, int remainingAmmo) {
    return new WeaponInstance(pWeaponClass, weaponIdCnt++, NULL, remainingAmmo);
}

WeaponInstance::WeaponInstance(Weapon * pWeaponClass, uint16_t anId, Map *pMap, int remainingAmmo) :
        ShootableMapObject(anId, pMap, MapObject::kNatureWeapon),
        bombSoundTimer(pWeaponClass->reloadTime()), bombExplosionTimer(pWeaponClass->explosionDelay()),
        shieldTimer_(static_cast<uint32_t>(pWeaponClass->fireRate())) {
    pWeaponClass_ = pWeaponClass;
    ammo_remaining_ = remainingAmmo == -1 ? pWeaponClass->ammoCapacity() : remainingAmmo;
    pOwner_ = NULL;
    activated_ = false;
    if (pWeaponClass->getType() == Weapon::TimeBomb
        || pWeaponClass->getType() == Weapon::Flamer) {
        setSize(32, 32, 32);
    }
    health_ = 1;
    start_health_ = 1;
    pFlamerShot_ = NULL;

    onGroundAnim_ = animationPlayer_->addAnimation(
                                        pWeaponClass_->onGroundAnimationId(),
                                        rs_eng::kAnimationModeLoop);
}

/*!
 * Scanner is not selectable
 * @return 
 */
bool WeaponInstance::isSelectable() {
    return pWeaponClass_->getType() != Weapon::Scanner;
}

void WeaponInstance::doUpdateState(uint32_t elapsed) {
    if (isInstanceOf(Weapon::TimeBomb) && activated_) {
        if (bombSoundTimer.update(elapsed)) {
            if (rs_eng::SoundManager::singletonPtr()) {
                g_SoundMgr.play(rs_eng::TIMEBOMB);
            }
        }

        if (bombExplosionTimer.update(elapsed)) {
            DamageToInflict dmg;
            fire(g_missionCtrl.mission(), dmg, elapsed);
        }
    } else if ((isInstanceOf(Weapon::EnergyShield) || isInstanceOf(Weapon::CloneShield)) && activated_) {
        if (consumeAmmoForEnergyShield(elapsed)) {
            // no more ammo
            deactivate();
        }
    }
}

/*!
 * Consumes EnergyShield ammo at the weapon's fireRate() pace.
 * At most one shot's worth of ammo is consumed per call, matching the
 * single-fire-per-update idiom used by AutomaticShootAction::fireRateTimer_.
 * \param elapsed uint32_t Time elapsed since last call, in milliseconds.
 * \return bool True if there is no more ammo left.
 */
bool WeaponInstance::consumeAmmoForEnergyShield(uint32_t elapsed) {
    if (shieldTimer_.update(elapsed) && ammo_remaining_ > 0) {
        ammo_remaining_ -= pWeaponClass_->ammoPerShot();
        if (ammo_remaining_ < 0) {
            ammo_remaining_ = 0;
        }
    }
    return ammo_remaining_ == 0;
}

void WeaponInstance::draw(const Point2D &screenPos) {
    animationPlayer_->draw(addOffs(screenPos), 0);
}

/*!
 * Plays the sound associated with that weapon.
 */
void WeaponInstance::playSound() {
    if (rs_eng::SoundManager::singletonPtr()) {
        g_SoundMgr.play(pWeaponClass_->getSound());
    }
}

void WeaponInstance::activate() {
    activated_ = true;
    if ((isInstanceOf(Weapon::EnergyShield) || isInstanceOf(Weapon::CloneShield)) && pOwner_ != nullptr) {
        shieldTimer_.reset();
        playSound();
    }
}

void WeaponInstance::deactivate() {
    activated_ = false;
}

/*!
 * Use weapon and decrease ammo.
 * This method is used only for shooting weapons and Medikit.
 * \param pMission Mission data
 * \param dmg Information on the damage to perform
 * \param elapsed Time since last frame
 */
void WeaponInstance::fire(Mission *pMission, DamageToInflict &dmg, uint32_t elapsed) {
    bool updateStats = true;
    if (isInstanceOf(Weapon::MediKit)) {
        dmg.d_owner->resetHealth();
        updateStats = false;
    } else if (isInstanceOf(Weapon::GaussGun)) {
        pMission->addProjectileShot(std::make_unique<GaussGunShot>(dmg));
    } else if (isInstanceOf(Weapon::Flamer)) {
        pMission->addProjectileShot(std::make_unique<FlamerShot>(pMission, dmg));
    }  else if (isInstanceOf(Weapon::TimeBomb)) {
        updateStats = false;
        setDrawable(false);
        health_ = 0;
        deactivate();
        Explosion::createExplosion(pMission, this,
            (double)pWeaponClass_->rangeDmg(), pWeaponClass_->damagePerShot());
    } else if (isInstanceOf(Weapon::AirStrike)) {
        pMission->addProjectileShot(std::make_unique<AirStrikeShot>(dmg));
    } else {
        // For other weapons, damage are done immediatly because projectile speed
        // is too high to draw them
        InstantImpactShot shot(dmg);
        shot.inflictDamage(pMission);
    }

    if (!isInstanceOf(Weapon::AirStrike)) {
        int ammoUsed = pWeaponClass_->ammoPerShot();
        ammo_remaining_ -= ammoUsed;
        if (ammo_remaining_ < 0) {
            ammo_remaining_ = 0;
        }
    }

    if (updateStats) {
        if (pMission && pOwner_ && pOwner_->isOurAgent()) {
            pMission->stats()->incrShots(1);
        }
    }
}

void WeaponInstance::handleHit(DamageToInflict & d)
{
    // When a bomb is hit, it explodes
    if (isInstanceOf(Weapon::TimeBomb) && health_ > 0 &&
        (d.dtype == kDmgTypeLaser || d.dtype == kDmgTypeBullet || d.dtype == kDmgTypeExplosion)) {
        // we pass the given DamageToInflict just for the compiler
        // as it is not used by the fire method for a Bomb
        // same for elapsed
        fire(g_missionCtrl.mission(), d, 0);
    }
}
}