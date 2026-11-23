/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
 *   Copyright (C) 2011  Bohdan Stelmakh <chamel@users.sourceforge.net>
 *   Copyright (C) 2011  Joey Parrish  <joey.parrish@gmail.com>
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
#ifndef MODOWNER_H
#define MODOWNER_H

#include "rs-kernel/model/mod.h"

namespace rs_knl {

class ModOwner {
public:
    ModOwner() {
        for (int i = 0; i < 6; i++)
            slots_[i] = NULL;
    }
    /*!
    * Returns true if the agent can be equiped with that mod version.
    */
    bool canHaveMod(Mod *pNewMod) {
        if (pNewMod == NULL) {
            return false;
        }

        Mod *pMod = slots_[pNewMod->getType()];
        if (pMod) {
            // Agent has a mod of the same type
            // Returns true if equiped version if less than new version
            return (pMod->getVersion() < pNewMod->getVersion());
        }

        // There is no mod of that type so agent can be equiped
        return true;
    }

    void addMod(Mod *pNewMod) {
        if (pNewMod) {
            slots_[pNewMod->getType()] = pNewMod;
        }
    }

    Mod *slot(int n) {
        assert(n < 6);
        return slots_[n];
    }

    /*!
     * Returns true if the owner is equiped with mod of the
     * given type and at least of given version.
     */
    bool hasMinimumVersionOfMod(Mod::EModType type, Mod::EModVersion version) {
        Mod *pMod = slots_[type];
        return (pMod && pMod->getVersion() >= version);
    }

    void clearSlots() {
        for (int i = 0; i < 6; i++)
            slots_[i] = NULL;
    }

    /*!
     * This method returns the amount of time before health
     * is restored when a ped owns the right version of Chest.
     * \return 0 if ped do not have good chest.
     */
    uint16_t getHealthRegenerationPeriod() {
        Mod *pMod = slots_[Mod::MOD_CHEST];
        if (pMod) {
            switch (pMod->getVersion()) {
                case Mod::MOD_V1:
                    return 12800;
                case Mod::MOD_V2:
                    return 6400;
                case Mod::MOD_V3:
                    return 3200;
            }
        }

        return 0;
    }

    /*!
     * Return the Adrenaline speed boost multiplier scale unlocked by Leg Mods.
     * Unmodded legs allow +1.36x max boost, Legs V1: +1.90x, Legs V2: +2.45x, Legs V3: +3.00x.
     */
    float getLegModAdrenalineBoostMultiplier() {
        Mod *pMod = slots_[Mod::MOD_LEGS];
        if (pMod) {
            switch(pMod->getVersion()) {
                case Mod::MOD_V1:
                    return 1.90f;
                case Mod::MOD_V2:
                    return 2.45f;
                case Mod::MOD_V3:
                    return 3.00f;
            }
        }
        return 1.36f;
    }

    float getSpeedMultiplier() {
        return 1.0f;
    }

    int getMaxWeight() {
        Mod *pMod = slots_[Mod::MOD_ARMS];
        if (pMod) {
            switch(pMod->getVersion()) {
                case Mod::MOD_V1:
                    return 11000;
                case Mod::MOD_V2:
                    return 21000;
                case Mod::MOD_V3:
                    return 31000;
            }
        }
        return 1000;
    }

    void transferMods(ModOwner &modOwner) {
        for (int i = 0; i < 6; i++) {
            modOwner.addMod(slots_[i]);
        }
    }

protected:
    Mod *slots_[6];
};

};

#endif
