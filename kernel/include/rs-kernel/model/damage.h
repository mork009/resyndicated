/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
 *   Copyright (C) 2007  Davor Ocelic <docelic@mail.inet.hr>
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net>
 *   Copyright (C) 2017, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef MODEL_DAMAGE_H_
#define MODEL_DAMAGE_H_

#include "rs-kernel/model/position.h"

namespace rs_knl {
    class ShootableMapObject;
    class WeaponInstance;
    
    /*!
     * These are all types of damage caused by weapons.
     */
    enum DamageType {
        kDmgTypeNone = 0x0000,
        kDmgTypeBullet = 0x0001,
        kDmgTypeLaser = 0x0002,
        kDmgTypeBurn = 0x0004,
        kDmgTypeExplosion = 0x0008,
        kDmgTypeCollision = 0x0010, // By car or door
        kDmgTypePersuasion = 0x0020,
    };

    /*!
     * This structure holds informations on the damage inflicted to a ShootableMapObject.
     */
    struct DamageToInflict {
        //! The type of damage
        DamageType dtype{kDmgTypeNone};
        //! Range of damage
        double range{0.0};
        //! The value of the damage
        int dvalue{0};
        //! direction damage comes from, should be angle 256 degree based
        int ddir{0};
        //! Location of aimed point
        WorldPoint aimedLocW;
        //! Location of origin of shot
        WorldPoint originLocW;
        //! The object that inflicted the damage
        ShootableMapObject * d_owner{nullptr};
        //! The weapon that generated this damage
        WeaponInstance *pWeapon{nullptr};
        //! If true, explosion damage does not ignite flame waves or burn victims
        bool nonIncendiary{false};
    };
};
#endif // MODEL_DAMAGE_H_
