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

#ifndef PEDMANAGER_H
#define PEDMANAGER_H

#include <vector>

#include "rs-utils/common.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/leveldata.h"

namespace rs_knl {

struct CharacterStats {
    int speed = 128;
    int health = 8;
    int sight_range = 1500;
    float base_accuracy = 0.50f;
    int time_before_check = 400;
    int persuasion_cost = 1;
};

/*!
 * Pedestrians manager class.
 */
class PedManager {
public:
    PedManager();
    virtual ~PedManager() {}

    static void loadCharacterStats();
    static const CharacterStats & getStats(PedInstance::PedType type, bool isEnemyAgent = false);

    PedInstance *loadInstance(const LevelData::People & ped_data, uint16_t ped_idx, Map *pMap, uint32_t playerGroupId, bool isHunter = false);
    //! Return the pedtype from the value in data    
    static PedInstance::PedType getTypeFromValue(uint8_t value);
    //! Get default max speed for a given type of Ped
    static int getDefaultSpeed(PedInstance::PedType type);
    //! Get default base animation index in HSTA-0.ANI for a given type of Ped
    static uint16_t getDefaultBaseAnim(PedInstance::PedType type);
    //! Initialize the ped instance as our agent
    static void initOurAgent(Agent *p_agent, unsigned int obj_group_id, PedInstance *pPed);
    //! Initialize the ped instance as an enemy agent
    static void initEnemyAgent(PedInstance *pPed, uint16_t mods_info = 0, int levelHp = 0, bool isHunter = false);
    //! Initialize the ped instance as a guard
    static void initGuard(PedInstance *pPed, int levelHp = 0);
    //! Initialize the ped instance as a police
    static void initPolice(PedInstance *pPed, int levelHp = 0);
    //! Initialize the ped instance as a civilian
    static void initCivilian(PedInstance *pPed, int levelHp = 0);
    //! Initialize the ped instance as a criminal
    static void initCriminal(PedInstance *pPed, int levelHp = 0);
};

}
#endif
