/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
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
#include <catch2/catch_test_macros.hpp>

#include "rs-kernel/model/weaponholder.h"
#include "rs-engine/appcontext.h"

#include "testcase.h"



TEST_CASE( "Weapon holder", "[kernel][weaponholder]" ) {
    rs_eng::AppContext appCtx;
    rs_knl::WeaponHolder holder;
    ConfigFile config;
    initWeaponConfigFile(config);
    rs_knl::Weapon scannerClass(rs_knl::Weapon::Scanner, config);
    rs_knl::Weapon energyClass(rs_knl::Weapon::EnergyShield, config);
    rs_knl::Weapon pistolClass(rs_knl::Weapon::Pistol, config);
    rs_knl::WeaponInstance scanner(&scannerClass, 0, nullptr);
    rs_knl::WeaponInstance energy(&energyClass, 1, nullptr);
    rs_knl::WeaponInstance pistol1(&pistolClass, 2, nullptr);
    rs_knl::WeaponInstance pistol2(&pistolClass, 3, nullptr, 0);
    rs_knl::WeaponInstance pistol3(&pistolClass, 4, nullptr);

    SECTION( "Add and remove weapons") {
        REQUIRE( holder.numWeapons() == 0);
        REQUIRE_FALSE( holder.isAnyWeaponSelected() );

        holder.addWeapon(&pistol1);
        REQUIRE( holder.numWeapons() == 1 );

        holder.addWeapon(&energy);
        REQUIRE( holder.numWeapons() == 2 );

        // Adding the same object is not possible
        holder.addWeapon(&energy);
        REQUIRE( holder.numWeapons() == 2 );

        holder.removeWeapon(&pistol1);
        REQUIRE( holder.numWeapons() == 1 );

        holder.removeWeaponAtIndex(0);
        REQUIRE( holder.numWeapons() == 0 );
    }

    SECTION( "has weapons") {
        REQUIRE_FALSE( holder.hasWeapon(rs_knl::Weapon::Scanner) );

        holder.addWeapon(&pistol1);
        REQUIRE_FALSE( holder.hasWeapon(rs_knl::Weapon::Scanner) );

        holder.addWeapon(&scanner);
        REQUIRE( holder.hasWeapon(rs_knl::Weapon::Scanner) );
    }

    SECTION( "Select weapon") {
        holder.addWeapon(&pistol1);
        holder.addWeapon(&energy);
        holder.addWeapon(&scanner);
        holder.addWeapon(&pistol2);

        SECTION( "Should select by index") {
            holder.selectWeapon(1);
            REQUIRE( holder.isAnyWeaponSelected() );
            REQUIRE( holder.isWeaponSelected(&energy) );
            REQUIRE (holder.selectedWeapon() == &energy );
        }

        SECTION( "Should select by instance") {
            holder.selectWeapon(&pistol2);
            REQUIRE( holder.isWeaponSelected(&pistol2) );
            REQUIRE ( holder.deselectWeapon() == &pistol2 );
            REQUIRE_FALSE ( holder.isAnyWeaponSelected() );
        }

        SECTION( "Should not select non selectable weapon") {
            holder.selectWeapon(&scanner);
            REQUIRE_FALSE( holder.isWeaponSelected(&scanner) );
        }

        SECTION( "Should select Energy by criteria of Medikit or shield") {
            holder.selectMedikitOrShield(rs_knl::Weapon::EnergyShield);
            REQUIRE( holder.isWeaponSelected(&energy) );
        }

        SECTION( "Should select loaded gun by criteria of ammo") {
            holder.selectShootingWeaponWithAmmo();
            REQUIRE( holder.isWeaponSelected(&pistol1) );
        }

        SECTION( "Should select loaded gun by criteria of same type") {
            holder.selectShootingWeaponWithSameTypeFirst(&pistol3);
            REQUIRE( holder.isWeaponSelected(&pistol1) );
        }
    }

    SECTION( "Remove weapon") {
        holder.addWeapon(&pistol1);
        holder.addWeapon(&energy); 
        holder.addWeapon(&pistol2);
        holder.addWeapon(&scanner);

        SECTION( "Remove selection should leave no selection") {
            holder.selectWeapon(&energy);
            holder.removeWeapon(&energy);
            REQUIRE_FALSE ( holder.isAnyWeaponSelected() );
        }

        SECTION( "Remove weapon should not select new weapon when selection was already empty") {
            holder.removeWeapon(&energy);
            REQUIRE_FALSE ( holder.isAnyWeaponSelected() );
        }

        SECTION( "Remove weapon before selected weapon should not change selection") {
            holder.selectWeapon(&pistol2);
            holder.removeWeapon(&energy);
            REQUIRE ( holder.isWeaponSelected(&pistol2) );
        }

        SECTION( "Remove weapon after selected weapon should not change selection") {
            holder.selectWeapon(&pistol2);
            holder.removeWeapon(&scanner);
            REQUIRE ( holder.isWeaponSelected(&pistol2) );
        }
    }

    SECTION( "Get inventory weight") {
        holder.addWeapon(&pistol1);
        holder.addWeapon(&energy);
        holder.addWeapon(&scanner);
        holder.addWeapon(&pistol2);

        REQUIRE ( holder.getInventoryWeight() == 250 );
    }
}