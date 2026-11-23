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

#include "rs-kernel/mgr/pedmanager.h"

#include <stdio.h>
#include <assert.h>

#include "rs-utils/log/log.h"
#include "rs-utils/io/file.h"
#include "rs-utils/io/configfile.h"
#include "rs-kernel/mgr/agentmanager.h"
#include "rs-kernel/mgr/modmanager.h"

namespace rs_knl {

static CharacterStats s_agentStats;
static CharacterStats s_enemyAgentStats;
static CharacterStats s_guardStats;
static CharacterStats s_policeStats;
static CharacterStats s_civilianStats;
static CharacterStats s_criminalStats;
static bool s_statsLoaded = false;

PedManager::PedManager() {
    if (!s_statsLoaded) {
        loadCharacterStats();
    }
}

void PedManager::loadCharacterStats() {
    std::string filename(rs_utl::File::getResyndDataFullPath("ref/characters.dat"));
    try {
        ConfigFile conf(filename);

        auto loadStats = [&](const std::string &prefix, CharacterStats &stats, int defSpeed, int defHp, int defSight, float defAcc, int defCheck, int defCost) {
            stats.speed = conf.read<int>(prefix + ".speed", defSpeed);
            stats.health = conf.read<int>(prefix + ".health", defHp);
            stats.sight_range = conf.read<int>(prefix + ".sight_range", defSight);
            stats.base_accuracy = static_cast<float>(conf.read<double>(prefix + ".base_accuracy", defAcc));
            stats.time_before_check = conf.read<int>(prefix + ".time_before_check", defCheck);
            stats.persuasion_cost = conf.read<int>(prefix + ".persuasion_cost", defCost);
        };

        loadStats("character.agent", s_agentStats, 178, 16, 1792, 0.50f, 400, 8);
        loadStats("character.enemy_agent", s_enemyAgentStats, 178, 16, 1500, 0.50f, 400, 8);
        loadStats("character.guard", s_guardStats, 128, 1, 1500, 0.45f, 300, 2);
        loadStats("character.police", s_policeStats, 128, 1, 1500, 0.40f, 400, 4);
        loadStats("character.civilian", s_civilianStats, 128, 1, 1500, 0.20f, 600, 1);
        loadStats("character.criminal", s_criminalStats, 128, 1, 1500, 0.20f, 500, 1);

        s_statsLoaded = true;
    } catch (...) {
        LOG(Log::k_FLG_IO, "PedManager", "loadCharacterStats", ("Failed to load %s, using defaults", filename.c_str()));
    }
}

const CharacterStats & PedManager::getStats(PedInstance::PedType type, bool isEnemyAgent) {
    if (!s_statsLoaded) {
        loadCharacterStats();
    }
    switch (type) {
        case PedInstance::kPedTypeCivilian: return s_civilianStats;
        case PedInstance::kPedTypeAgent:    return isEnemyAgent ? s_enemyAgentStats : s_agentStats;
        case PedInstance::kPedTypePolice:   return s_policeStats;
        case PedInstance::kPedTypeGuard:    return s_guardStats;
        case PedInstance::kPedTypeCriminal:  return s_criminalStats;
        default:                            return s_civilianStats;
    }
}

/*!
 * Instanciate a PedInstance from the given data.
 * \param gamdata
 * \param ped_idx Index of the ped in the file.
 * \param map id of the map
 * \return NULL if the ped could not be created.
 */
PedInstance *PedManager::loadInstance(const LevelData::People & gamdata, uint16_t ped_idx, Map *pMap, uint32_t playerGroupId, bool isHunter)
{
    if(gamdata.type_ped == 0x0 || gamdata.location == LevelData::kPeopleLocNotVisible)
        return NULL;

    bool isOurAgent = ped_idx < Squad::kMaxSlot;
    if (isOurAgent && !g_agentMgr.isSquadSlotActive(ped_idx)) {
        // Creates agent only if he's active
        return NULL;
    } if (ped_idx >= 4 && ped_idx < 8) {
        // Ped between index 4 and 7 are not used
        // In original game must be the place where persuaded agents were stored
        return NULL;
    }

    PedInstance::PedType pedType = getTypeFromValue(gamdata.type_ped);
    PedInstance *newped = new PedInstance(ped_idx, pMap, pedType, isOurAgent, getDefaultSpeed(pedType));
    newped->setInitialLevelState(gamdata.state);

    uint16_t bAnim = rs_utl::READ_LE_UINT16(gamdata.index_base_anim);
    if (bAnim == 0) {
        bAnim = getDefaultBaseAnim(pedType);
    }
    newped->setAnimations(bAnim);

    int rawHp = rs_utl::READ_LE_INT16(gamdata.health);
    bool isDeadInLevelData = (gamdata.state == LevelData::kPeopleStateDead ||
                              gamdata.state == 0x13 ||
                              (rawHp <= 0 && gamdata.location == LevelData::kPeopleLocAboveWalkSurf));

    int hp = rawHp;
    if (isOurAgent) {
        hp = getStats(PedInstance::kPedTypeAgent, false).health;
    } else if (hp <= 0 && !isDeadInLevelData) {
        hp = getStats(pedType).health;
    }

    newped->setDirection(gamdata.orientation);
    if (isDeadInLevelData && !isOurAgent) {
        newped->setStartHealth(0);
        newped->playDeadAnimation();
        newped->setHealth(-1);
    } else {
        newped->setStartHealth(hp);
        newped->setHealth(hp);
        newped->goToState(kPedActionStateStanding, true);
    }

    int px = gamdata.mapposx[1];
    int py = gamdata.mapposy[1];
    int z = rs_utl::READ_LE_UINT16(gamdata.mapposz) >> 7;
    int oz = gamdata.mapposz[0] & 0x7F;

    newped->setSize(32, 32, 256);
    newped->setPosition(px, py, z, gamdata.mapposx[0], gamdata.mapposy[0], oz);

    // In original GAMEXX.DAT People structs, *_amount (0x4f) stores the neutral midpoint (128),
    // while *_dependency (0x50), *_effect (0x51), and the preset lock byte (unkn11/12/13 at 0x4a/0x4e/0x52)
    // hold the active DOS NPC IPA levels (MAIN.EXE 0x2d0d8 / 0x2d7a8 / 0x2d808 / 0x2d868).
    auto resolveNpcIpaLevels = [](uint8_t dep, uint8_t eff, uint8_t lock) -> std::pair<uint8_t, uint8_t> {
        if (lock != 0) {
            return { static_cast<uint8_t>(lock + 128), lock };
        }
        return { eff, dep };
    };

    if (isOurAgent) {
        newped->initAllLevelsForIPAType(IPAStim::Adrenaline,
            gamdata.adrena_amount, gamdata.adrena_dependency, gamdata.adrena_effect);
        newped->initAllLevelsForIPAType(IPAStim::Intelligence,
            gamdata.inteli_amount, gamdata.inteli_dependency, gamdata.inteli_effect);
        newped->initAllLevelsForIPAType(IPAStim::Perception,
            gamdata.percep_amount, gamdata.percep_dependency, gamdata.percep_effect);
    } else {
        auto [adrEff, adrDep] = resolveNpcIpaLevels(gamdata.adrena_dependency, gamdata.adrena_effect, gamdata.unkn11);
        auto [intEff, intDep] = resolveNpcIpaLevels(gamdata.inteli_dependency, gamdata.inteli_effect, gamdata.unkn12);
        auto [perEff, perDep] = resolveNpcIpaLevels(gamdata.percep_dependency, gamdata.percep_effect, gamdata.unkn13);

        newped->initAllLevelsForIPAType(IPAStim::Adrenaline, adrEff, adrDep, adrEff);
        newped->initAllLevelsForIPAType(IPAStim::Intelligence, intEff, intDep, intEff);
        newped->initAllLevelsForIPAType(IPAStim::Perception, perEff, perDep, perEff);
    }

    if (isOurAgent) {
        Agent *pAg = g_agentMgr.squadMember(ped_idx);
        initOurAgent(pAg, playerGroupId, newped);
    } else {
        unsigned int mt = newped->type();
        newped->setObjGroupDef(mt);
        if (mt == PedInstance::og_dmAgent) {
            initEnemyAgent(newped, rs_utl::READ_LE_UINT16(gamdata.mods_info), hp, isHunter);
        } else if (mt == PedInstance::og_dmGuard) {
            initGuard(newped, hp);
        } else if (mt == PedInstance::og_dmPolice) {
            initPolice(newped, hp);
        } else if (mt == PedInstance::og_dmCivilian) {
            initCivilian(newped, hp);
        } else if (mt == PedInstance::og_dmCriminal) {
            initCriminal(newped, hp);
        }
    }

    return newped;
}

PedInstance::PedType PedManager::getTypeFromValue(uint8_t value) {
    if (value & PedInstance::kPedTypeAgent)
        return PedInstance::kPedTypeAgent;
    if (value & PedInstance::kPedTypeGuard)
        return PedInstance::kPedTypeGuard;
    if (value & PedInstance::kPedTypePolice)
        return PedInstance::kPedTypePolice;
    if (value & PedInstance::kPedTypeCriminal)
        return PedInstance::kPedTypeCriminal;

    return PedInstance::kPedTypeCivilian;
}

int PedManager::getDefaultSpeed(PedInstance::PedType type) {
    return getStats(type).speed;
}

uint16_t PedManager::getDefaultBaseAnim(PedInstance::PedType type) {
    switch (type) {
        case PedInstance::kPedTypeAgent:    return 1;
        case PedInstance::kPedTypePolice:   return 833;
        case PedInstance::kPedTypeGuard:    return 1744;
        case PedInstance::kPedTypeCriminal: return 1123;
        case PedInstance::kPedTypeCivilian:
        default:                            return 1537;
    }
}

/*!
 * Initialize the ped instance as one of our agent.
 * \param pAgent The agent reference
 * \param obj_group_id Id of the agent's group.
 * \param pPed The ped to initialize
 */
void PedManager::initOurAgent(Agent *pAgent, unsigned int obj_group_id, PedInstance *pPed) {
    LOG(Log::k_FLG_GAME, "PedManager","initOurAgent", ("Create player agent with id %d", pPed->id()))

    const CharacterStats &stats = getStats(PedInstance::kPedTypeAgent, false);

    if (pAgent) {
        while (pAgent->numWeapons()) {
            WeaponInstance *wi = pAgent->removeWeaponAtIndex(0);
            pPed->addWeapon(wi);
            wi->setOwner(pPed);
        }
        *((ModOwner *)pPed) = *((ModOwner *)pAgent);
    }

    pPed->setObjGroupID(obj_group_id);
    pPed->setObjGroupDef(PedInstance::og_dmAgent);
    pPed->addEnemyGroupDef(2);
    pPed->addEnemyGroupDef(3);
    pPed->setMaxSpeed(stats.speed);
    pPed->setSightRange(stats.sight_range);
    pPed->setTimeBeforeCheck(stats.time_before_check);
    pPed->setBaseModAcc(stats.base_accuracy);

    // Set components of behaviour for our agent
    pPed->behaviour().addComponent(new CommonAgentBehaviourComponent(pPed));
    pPed->behaviour().addComponent(new PersuaderBehaviourComponent());
}

/*!
 * Initialize the ped instance as an enemy agent.
 * \param pPed The ped to initialize
 * \param mods_info Bitmask of mods to load
 * \param levelHp Health from level binary
 */
void PedManager::initEnemyAgent(PedInstance *pPed, uint16_t mods_info, int levelHp, bool isHunter) {
    LOG(Log::k_FLG_GAME, "PedManager","initEnemyAgent", ("Create enemy agent with id %d", pPed->id()))

    const CharacterStats &stats = getStats(PedInstance::kPedTypeAgent, true);

    pPed->setObjGroupID(2);
    pPed->setObjGroupDef(PedInstance::og_dmAgent);
    pPed->addEnemyGroupDef(1);

    // Parse and add mods according to level data mods_info bitmask:
    // Leg: bits 1-2, Arm: bits 3-4, Chest: bits 5-6, Heart: bits 7-8, Eye: bits 9-10, Brain: bits 11-12
    auto addModFromMask = [&](Mod::EModType type, int bit_shift) {
        int version = (mods_info >> bit_shift) & 3;
        if (version >= 1 && version <= 3) {
            pPed->addMod(g_modMgr.getMod(type, static_cast<Mod::EModVersion>(version - 1)));
        }
    };

    addModFromMask(Mod::MOD_LEGS, 1);
    addModFromMask(Mod::MOD_ARMS, 3);
    addModFromMask(Mod::MOD_CHEST, 5);
    addModFromMask(Mod::MOD_HEART, 7);
    addModFromMask(Mod::MOD_EYES, 9);
    addModFromMask(Mod::MOD_BRAIN, 11);

    int hp = (levelHp > 0) ? levelHp : stats.health;
    pPed->setStartHealth(hp);
    if (pPed->health() > 0) {
        pPed->setHealth(hp);
    }

    pPed->setMaxSpeed(stats.speed);
    pPed->setSightRange(stats.sight_range);
    pPed->setTimeBeforeCheck(stats.time_before_check);
    pPed->setBaseModAcc(stats.base_accuracy);

    auto *pHos = new PlayerHostileBehaviourComponent();
    pHos->setHunter(isHunter);
    pPed->behaviour().addComponent(pHos);
}

/*!
 * Initialize the ped instance as a guard.
 * \param pPed The ped to initialize
 * \param levelHp Health from level binary
 */
void PedManager::initGuard(PedInstance *pPed, int levelHp) {
    LOG(Log::k_FLG_GAME, "PedManager","initGuard", ("Create guard with id %d", pPed->id()))

    const CharacterStats &stats = getStats(PedInstance::kPedTypeGuard);

    pPed->setObjGroupID(3);
    pPed->addEnemyGroupDef(1);

    int hp = (levelHp > 0) ? levelHp : stats.health;
    pPed->setStartHealth(hp);
    if (pPed->health() > 0) {
        pPed->setHealth(hp);
    }

    pPed->setMaxSpeed(stats.speed);
    pPed->setSightRange(stats.sight_range);
    pPed->setTimeBeforeCheck(stats.time_before_check);
    pPed->setBaseModAcc(stats.base_accuracy);

    pPed->behaviour().addComponent(new PlayerHostileBehaviourComponent());
}

/*!
 * Initialize the ped instance as a police.
 * \param pPed The ped to initialize
 * \param levelHp Health from level binary
 */
void PedManager::initPolice(PedInstance *pPed, int levelHp) {
    LOG(Log::k_FLG_GAME, "PedManager","initPolice", ("Create police with id %d", pPed->id()))

    const CharacterStats &stats = getStats(PedInstance::kPedTypePolice);

    pPed->setObjGroupID(4);

    int hp = (levelHp > 0) ? levelHp : stats.health;
    pPed->setStartHealth(hp);
    if (pPed->health() > 0) {
        pPed->setHealth(hp);
    }

    pPed->setMaxSpeed(stats.speed);
    pPed->setSightRange(stats.sight_range);
    pPed->setTimeBeforeCheck(stats.time_before_check);
    pPed->setBaseModAcc(stats.base_accuracy);

    pPed->behaviour().addComponent(new PoliceBehaviourComponent());
}

/*!
 * Initialize the ped instance as a civilian.
 * \param pPed The ped to initialize
 * \param levelHp Health from level binary
 */
void PedManager::initCivilian(PedInstance *pPed, int levelHp) {
    LOG(Log::k_FLG_GAME, "PedManager","initCivilian", ("Create civilian with id %d", pPed->id()))

    const CharacterStats &stats = getStats(PedInstance::kPedTypeCivilian);

    pPed->setObjGroupID(5);
    pPed->addEnemyGroupDef(6);

    int hp = (levelHp > 0) ? levelHp : stats.health;
    pPed->setStartHealth(hp);
    if (pPed->health() > 0) {
        pPed->setHealth(hp);
    }

    pPed->setMaxSpeed(stats.speed);
    pPed->setSightRange(stats.sight_range);
    pPed->setTimeBeforeCheck(stats.time_before_check);
    pPed->setBaseModAcc(stats.base_accuracy);

    pPed->behaviour().addComponent(new PanicComponent());
}

/*!
 * Initialize the ped instance as a criminal.
 * \param pPed The ped to initialize
 * \param levelHp Health from level binary
 */
void PedManager::initCriminal(PedInstance *pPed, int levelHp) {
    LOG(Log::k_FLG_GAME, "PedManager","initCriminal", ("Create criminal with id %d", pPed->id()))

    const CharacterStats &stats = getStats(PedInstance::kPedTypeCriminal);

    pPed->setObjGroupID(6);

    int hp = (levelHp > 0) ? levelHp : stats.health;
    pPed->setStartHealth(hp);
    if (pPed->health() > 0) {
        pPed->setHealth(hp);
    }

    pPed->setMaxSpeed(stats.speed);
    pPed->setSightRange(stats.sight_range);
    pPed->setTimeBeforeCheck(stats.time_before_check);
    pPed->setBaseModAcc(stats.base_accuracy);
}

}