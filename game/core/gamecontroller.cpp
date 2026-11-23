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
 *   Copyright (C) 2010, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "core/gamecontroller.h"

#include <list>
#include <sstream>

#include "rs-utils/log/log.h"
#include "rs-utils/io/file.h"
#include "rs-utils/io/configfile.h"
#include "rs-utils/io/portablefile.h"
#include "rs-kernel/model/squad.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/mgr/mission_triad.h"
#include "utf8.h"

#include "core/gamesession.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/logomanager.h"
#include <algorithm>
#include <cctype>
#include <string>

//! The maximum number of syndicate in the game
constexpr uint8_t kMaxSyndicate { 8 };

GameController::GameController() :
        missionMgr_(&tileMgr_),
        session_(std::make_unique<GameSession>(&weaponMgr_, &mods_)) {
    agents_.setModManager(&mods_);
    agents_.setWeaponManager(&weaponMgr_);
}

GameController::~GameController() {}

bool GameController::initialize() {

    LOG(Log::k_FLG_INFO, "GameController", "initialize", ("loading game tileset..."))
    if (!tileMgr_.loadTiles()) {
        return false;
    }

    LOG(Log::k_FLG_INFO, "GameController", "initialize", ("Loading game data..."))
    return reset();
}

void GameController::destroy() {
    LOG(Log::k_FLG_MEM, "GameController", "destroy", ("Destruction..."))
    agents_.destroy();
}

bool GameController::reset() {
    g_missionCtrl.destroyMission();
    missionMgr_.mapManager().clearCache();
    // Reset default mods and weapons
    mods_.reset();
    weaponMgr_.reset();
    // TODO add reading cheatcode for onlywomen parameter
    agents_.reset();

    // Reset user session
    if (!g_Session.reset()) {
        return false;
    }

    if (rs_utl::File::hasAmericanRevoltData()) {
        weaponMgr_.cheatEnableAllWeapons();
        mods_.cheatEnableAllMods();
    }

    if (false) {
        return false;
    }

    return true;
}

/*!
 * Activate a cheat code with the given name. Possible
 * cheat codes are :
 * - DO IT AGAIN / JOLLY WOLLY : Possibility to replay a completed mission
 * - NUK THEM : Enable all missions and resurrect dead agents
 * - OWN THEM : All countries belong to the user
 * - ROB A BANK : Puts $100 000 000 in funds
 * - TO THE TOP : Puts $100 000 000 in funds and activates all missions
 * - COOPER TEAM : $100 000 000 in funds, select any mission, all weapons
 *   and mods
 * - WATCH THE CLOCK : Accelerates time
 *
 * \param name The name of a cheat code.
 */
static std::string clean_cheat_name(const char *name) {
    if (!name) return "";
    std::string s(name);
    // Trim leading spaces
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    // Trim trailing spaces
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
    // Convert to uppercase
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    return s;
}

void GameController::setCheatCode(const char *name) {
    std::string clean = clean_cheat_name(name);
    const char *upper = clean.c_str();

    // Repeat mission with previously obtained items, press 'C' or 'Ctrl-C'
    // to instantly complete a mission
    if (!strcmp(upper, "DO IT AGAIN") || !strcmp(upper, "JOLLY WOLLY"))
        cheatRepeatOrCompleteMission();
    else if (!strcmp(upper, "NUK THEM")) {
        // Select any mission, resurrect dead agents
        cheatAnyMission();
        cheatResurrectAgents();
    }
    else if (!strcmp(upper, "OWN THEM")) {
        // Own all countries
        cheatOwnAllCountries();
    }
    else if (!strcmp(upper, "ROB A BANK")) {
        // $100 000 000 in funds
        cheatFunds();
    }
    else if (!strcmp(upper, "TO THE TOP")) {
        // $100 000 000 in funds, select any mission
        cheatFunds();
        cheatAnyMission();
    }
    else if (!strcmp(upper, "COOPER TEAM")) {
        // $100 000 000 in funds, select any mission, all weapons and mods
        cheatFemaleRecruits();
        cheatFunds();
        cheatAnyMission();
        cheatWeaponsAndMods();
        cheatEquipAllMods();
        cheatEquipFancyWeapons();
    }
    else if (!strcmp(upper, "MARKS TEAM")) {
        // $100 000 000 in funds, select any mission, all weapons and mods
        cheatFunds();
        cheatAnyMission();
        cheatWeaponsAndMods();
        cheatEquipAllMods();
        cheatEquipFancyWeapons();
    }
    else if (!strcmp(upper, "WATCH THE CLOCK")) {
        // Accelerate time for faster research completion
        cheatAccelerateTime();
    }
}

/*!
 * Changes the user informations.
 */
void GameController::change_user_infos(const char *company_name, const char *player_name,
                            int new_logo, int new_color) {
    g_Session.setCompanyName(company_name);
    g_Session.setUserName(player_name);
    g_Session.setLogo(new_logo);
    g_Session.setLogoColour(new_color);
    g_AnimMgr.setPlayerFactionColor(g_LogoMgr.getColorAtIndex(new_color));

    if (company_name) {
        setCheatCode(company_name);
    }
}

/*!
 * Updates the game time based on the given elapsed millisecond since
 * the last update.
 * For every day passed, it calls the updateCountries() method and update
 * the user's amount of money.
 * \param elapsed The number of millisecond since the last update.
 * \return True if time has changed.
 */
bool GameController::updateTime(uint32_t elapsed) {
    rs_knl::GameTime previousTime = session_->currentTime();
    uint32_t hours = session_->addElapsedTime(elapsed);

    // first update countries to collect money
    uint32_t daysElapsed = session_->currentTime().diffInDays(previousTime);
    for (uint32_t i=0; i < daysElapsed; i++) {
        session_->increaseMoney(session_->updateCountries());
    }

    // then update ongoing research
    session_->decreaseMoney(session_->researchManager().process(hours, session_->getMoney()));

    return hours != 0;
}

rs_knl::MissionBriefing * GameController::loadBriefing(int n) {
    return missionMgr_.loadBriefing(n);
}

/*!
 * Load a mission based on the selection by the player
 * @return 
 */
bool GameController::loadSelectedMission() {
    // Ensure at least one squad slot is assigned so debug mission boots (-m) don't crash
    bool squadHasMembers = false;
    for (uint8_t i = 0; i < rs_knl::Squad::kMaxSlot; i++) {
        if (agents().squadMember(i)) {
            squadHasMembers = true;
            break;
        }
    }
    if (!squadHasMembers) {
        for (uint8_t i = 0; i < rs_knl::Squad::kMaxSlot; i++) {
            if (agents().agent(i)) {
                agents().setSquadMember(i, agents().agent(i));
            }
        }
    }

    Block block = session_->getSelectedBlock();
    rs_knl::Mission *pMission = missionMgr_.loadMission(block.mis_id, block.paletteId);

    return pMission != nullptr;
}

bool GameController::prepareCustomMissionFromFile(const std::filesystem::path &chosenPath, std::string &outErrorMsg) {
    rs_knl::MissionTriadPaths paths;
    if (!rs_knl::MissionTriad::resolveFromAnyFile(chosenPath, paths)) {
        outErrorMsg = "Invalid or unrecognized mission file name.\n\n"
                      "Expected format: gameXX.dat, mapXX.dat, or missXX.dat.";
        return false;
    }

    rs_knl::TriadValidationResult valRes = rs_knl::MissionTriad::validate(paths);
    if (!valRes.isValid) {
        outErrorMsg.clear();
        utf8::utf16to8(valRes.summaryMessage.begin(), valRes.summaryMessage.end(), std::back_inserter(outErrorMsg));
        return false;
    }

    std::string prevCustomDir = missionMgr_.mapManager().customDataDir();
    std::string customDir = chosenPath.parent_path().string();
    setCustomMissionDir(customDir);

    // Validate level data and American Revolt asset requirements
    auto pLevelData = std::make_unique<LevelData::LevelDataAll>();
    if (!missionMgr_.load_level_data(paths.missionId, *pLevelData)) {
        setCustomMissionDir(prevCustomDir);
        outErrorMsg = "Failed to load mission level data (gameXX.dat).";
        return false;
    }

    if (rs_knl::MissionManager::missionRequiresAmericanRevolt(*pLevelData) && !rs_utl::File::hasAmericanRevoltData()) {
        setCustomMissionDir(prevCustomDir);
        outErrorMsg = "This mission requires Syndicate: American Revolt data assets (Air Strike / Clone Shield / Data Disk),\n"
                      "but your currently configured data directory only contains original Syndicate files.\n\n"
                      "Please point your data directory to an American Revolt installation (such as DATADISK\\DATA) via Settings.";
        return false;
    }

    // Validate map and palette
    int paletteId = rs_knl::MissionManager::getMissionPaletteId(paths.missionId);
    if (!missionMgr_.mapManager().getMap(pLevelData->mapinfos) || !missionMgr_.mapManager().loadPalette(paletteId)) {
        setCustomMissionDir(prevCustomDir);
        outErrorMsg = "Failed to load mission map or environment palette.";
        return false;
    }

    // Validate that briefing data and minimap load cleanly
    rs_knl::MissionBriefing *pBrief = missionMgr_.loadBriefing(paths.missionId);
    if (!pBrief) {
        setCustomMissionDir(prevCustomDir);
        outErrorMsg = "Failed to validate mission briefing data.\n\n"
                      "The briefing file (missXX.dat) may be corrupted or unreadable.";
        return false;
    }
    delete pBrief;

    session_->setCustomMission(paths.missionId, paletteId);
    return true;
}

void GameController::handle_mission_end(rs_knl::Mission *p_mission) {
    uint32_t elapsed = p_mission->stats()->missionDuration();
    updateTime(elapsed);

    // synch ped agents with agent from cryo chamber
    transferAgentToCryoChamber(p_mission);

    if (p_mission->completed()) {
        g_Session.mark_selected_block_completed();

        // Make the next missions available
        Block &blk = g_Session.getSelectedBlock();
        if (blk.next != NULL) {
            char s[50];
            strcpy(s, blk.next);
            char *token = strtok(s, ":");
            while ( token != NULL ) {
                int id = atoi(token);
                Block &next_blk = g_Session.getBlock(id);
                if (next_blk.status == BLK_UNAVAIL) {
                    // Make available only if the mission is not already finished
                    next_blk.status = BLK_AVAIL;
                }
                token = strtok(NULL, ":");
            }
        }

        // simulate other syndicates activity
        simulate_enemy_moves();
    }
}

void GameController::transferAgentToCryoChamber(rs_knl::Mission *pMission) {
    // Update for squad
    for (size_t i = rs_knl::Squad::kSlot1; i < rs_knl::Squad::kMaxSlot; i++) {
        rs_knl::PedInstance *pPedAgent = pMission->getSquad()->member(i);
        if (pPedAgent) {
            rs_knl::Agent *pAg = agents().squadMember(i);
            if (pPedAgent->isDead()) {
                // an agent died -> remove him from cryo
                agents().destroyAgentSlot(i);
            } else {
                // synch only weapons
                pPedAgent->transferWeapons(*pAg);
            }
        }
    }

    // Add persuaded agents only in case of mission success
    if (pMission->stats()->nbAgentCaptured() > 0 && pMission->completed()) {
        for (size_t i = pMission->getSquad()->size(); i < pMission->numPeds(); i++) {
            rs_knl::PedInstance *pPed = pMission->ped(i);
            if (pPed->objGroupDef() == rs_knl::PedInstance::og_dmAgent) {
                rs_knl::Agent *pAg = agents().createAgent(false);
                if (pAg) {
                    pPed->transferWeapons(*pAg);
                    pPed->transferMods(*pAg);
                }
            }
        }
    }
}

/*!
 * This method simulates enemy syndicates activtiy
 * by changing ownership of a random number of countries (depending on
 * the number of remaining syndicates).
 */
void GameController::simulate_enemy_moves() {
    // Total number of active syndicates ie syndicates
    // that own at least one country
    int nb_active_synds = 0;
    // Each list of the array contains the id of countries
    // owned by a given syndicate.
    std::list<int> blocks_per_synd[kMaxSyndicate - 1];
    for (int i=0; i<GameSession::NB_MISSION; i++) {
        Block & blk = g_Session.getBlock(i);
        if (blk.status == BLK_AVAIL || blk.status == BLK_UNAVAIL) {
            if (blocks_per_synd[blk.syndicate_owner].empty()) {
                // We found a country not owned by player
                // so we can count its owner as an active syndicate
                nb_active_synds++;
            }
            blocks_per_synd[blk.syndicate_owner].push_back(i);
        }
    }

    // Computes how many movements (ie change country ownership) we will simulate
    int nb_mvt = get_nb_mvt_for_active_synds(nb_active_synds);

    // Simulate movements

    // this list stores id of missions that shifted
    // to prevent movint them multiple times
    std::list<int> used_block;
    while (nb_mvt > 0) {
        nb_mvt--;
        bool move_done = false;
        do {
            // randomly chose a syndicate
            // that will lose a country
            uint8_t synd_from_id = session_->getRandomEnemySyndicateId();
            // Distribution for choosing a random country
            std::uniform_int_distribution<std::size_t> countryDist(0,
                                                        blocks_per_synd[synd_from_id].size() - 1);
            if (!blocks_per_synd[synd_from_id].empty()) {
                // randomly chose one of its countries
                size_t m_pos = countryDist(session_->getRandomGenerator());
                size_t i = 0;
                for (std::list < int >::iterator it = blocks_per_synd[synd_from_id].begin();
                        it != blocks_per_synd[synd_from_id].end(); it++) {
                    if (i == m_pos) {
                        // First check to see if mission hasn't already been shited before
                        bool is_in_list = false;
                        for (std::list < int >::iterator used_it = used_block.begin();
                                used_it != used_block.end(); used_it++) {
                            if (*it == *used_it) {
                                is_in_list = true;
                                break;
                            }
                        }
                        if (!is_in_list) {
                            uint8_t synd_to_id;
                            do {
                                // find another syndicate that owns countries.
                                // synds that do not have countries are out
                                synd_to_id = session_->getRandomEnemySyndicateId();
                            } while (synd_from_id == synd_to_id || blocks_per_synd[synd_to_id].empty());

                            // Gives the country to the other syndicate
                            g_Session.getBlock(*it).syndicate_owner = synd_to_id;
                            used_block.push_back(*it); // remember we used this mission
                            move_done = true;
                        }
                        // out anyway
                        break;
                    }

                    i++;
                } // end for
            } // end if
        } while (!move_done);
    }
}

/*!
 * Returns a random number of shifts given the number of
 * syndicates in activity.
 */
int GameController::get_nb_mvt_for_active_synds(int nb_active_synds) {
    switch(nb_active_synds) {
    case 7:
        return rand() % 9 + 2; // between 2 and 10 moves
    case 6:
        return rand() % 7 + 2; // between 2 and 8 moves
    case 5:
        return rand() % 6 + 2; // between 2 and 7 moves
    case 4:
        return rand() % 5 + 1; // between 1 and 5 moves
    case 3:
        return rand() % 4 + 1; // between 1 and 4 moves
    case 2:
        return rand() % 2 + 1; // between 1 and 2 moves
    default:
        return 0;
    }
}

bool GameController::saveGameToFile(int fileSlot, std::string name) {
    LOG(Log::k_FLG_IO, "GameController", "saveGameToFile", ("Saving %s in slot %d", name.c_str(), fileSlot))

    rs_utl::PortableFile outfile;
    std::string path;

    rs_utl::File::getFullPathForSaveSlot(fileSlot, path);
    LOG(Log::k_FLG_IO, "GameController", "saveGameToFile", ("Saving to file %s", path.c_str()))

    outfile.open_to_overwrite(path.c_str());

    if (outfile) {
        // write file format version
        outfile.write8(1); // major
        outfile.write8(3); // minor

        // Slot name is 31 characters long, nul-padded
        outfile.write_string(name, 31);

        // Session
        g_Session.saveToFile(outfile);

        // Weapons
        weaponManager().saveToFile(outfile);

        // Mods
        mods().saveToFile(outfile);

        // Agents
        // TODO move in sesion saveToFile
        agents().saveToFile(outfile);

        // save researches
        // TODO move in sesion saveToFile
        g_Session.researchManager().saveToFile(outfile);

        return true;
    }

    return false;
}

bool GameController::loadGameFromFile(int fileSlot) {

    std::string path;
    rs_utl::PortableFile infile;

    rs_utl::File::getFullPathForSaveSlot(fileSlot, path);

    infile.open_to_read(path.c_str());

    if (infile) {
        // FIXME: detect original game saves

        // Read version
        unsigned char vMaj = infile.read8();
        unsigned char vMin = infile.read8();
        rs_utl::FormatVersion v(vMaj, vMin);

        // validate that this is a supported version.
        if (v.gt(1,3)) {
            RSERR(Log::k_FLG_IO, "GameController", "loadGameFromFile", ("Cannot load file, unsupported version %d.%d", vMaj, vMin))
            return false;
        }

        if (v == 0x0100) {
            // the 1.0 format is in native byte order instead of big-endian.
            infile.set_system_endian();
        }

        reset();

        // Read slot name
        std::string slotName;
        // Original game: 20 chars on screen, 20 written, 19 read.
        // v1.0: 25 characters.
        // v1.1: 31 characters.
        slotName = infile.read_string((v == 0x0100) ? 25 : 31, true);

        g_Session.loadFromFile(infile, v);

        // Weapons
        weaponManager().loadFromFile(infile, v);

        // Mods
        mods().loadFromFile(infile, v);

        // Agents
        agents().loadFromFile(infile, v);

        // Research
        g_Session.researchManager().loadFromFile(infile, v);

        return true;
    }

    return false;
}

void GameController::cheatFunds() {
    g_Session.setMoney(100000000);
}
/*!
 * Activate cheat mode in which all completed missions can be replayed.
 */
void GameController::cheatRepeatOrCompleteMission() {
    g_Session.cheatReplayMission();
}

void GameController::cheatWeaponsAndMods() {
    weaponManager().cheatEnableAllWeapons();
    mods().cheatEnableAllMods();
}

void GameController::cheatEquipAllMods() {
    for (uint8_t agent = 0; agent < rs_knl::AgentManager::MAX_AGENT; agent++) {
        rs_knl::Agent *pAgent = agents().agent(agent);
        if (pAgent) {
            pAgent->clearSlots();

            pAgent->addMod(mods().getMod(rs_knl::Mod::MOD_LEGS, rs_knl::Mod::MOD_V3));
            pAgent->addMod(mods().getMod(rs_knl::Mod::MOD_ARMS, rs_knl::Mod::MOD_V3));
            pAgent->addMod(mods().getMod(rs_knl::Mod::MOD_EYES, rs_knl::Mod::MOD_V3));
            pAgent->addMod(mods().getMod(rs_knl::Mod::MOD_BRAIN, rs_knl::Mod::MOD_V3));
            pAgent->addMod(mods().getMod(rs_knl::Mod::MOD_CHEST, rs_knl::Mod::MOD_V3));
            pAgent->addMod(mods().getMod(rs_knl::Mod::MOD_HEART, rs_knl::Mod::MOD_V3));
        }
    }
}

/*!
 * Activate cheat mode in which all missions are playable.
 */
void GameController::cheatAnyMission() {
    g_Session.cheatEnableAllMission();
}

void GameController::cheatResurrectAgents() {
    // TODO: Implement cheatResurrectAgents()
}

void GameController::cheatOwnAllCountries() {
    // TODO: Implement cheatOwnAllCountries()
}

void GameController::cheatAccelerateTime() {
    g_Session.currentTime().cheatAccelerateTime();
}

void GameController::cheatFemaleRecruits() {
    agents().reset(true);

    for (uint8_t i = 0; i < rs_knl::Squad::kMaxSlot; i++)
        agents().setSquadMember(i, agents().agent(i));
}

void GameController::cheatEquipFancyWeapons() {
    for (uint8_t i = 0; i < rs_knl::AgentManager::MAX_AGENT; i++) {
        if (agents().agent(i)) {
            agents().agent(i)->destroyAllWeapons();
            agents().agent(i)->addWeapon(
                    rs_knl::WeaponInstance::createInstance(weaponManager().getWeapon(rs_knl::Weapon::Minigun)));
            agents().agent(i)->addWeapon(
                    rs_knl::WeaponInstance::createInstance(weaponManager().getWeapon(rs_knl::Weapon::Minigun)));
            agents().agent(i)->addWeapon(
                    rs_knl::WeaponInstance::createInstance(weaponManager().getWeapon(rs_knl::Weapon::Persuadatron)));
            agents().agent(i)->addWeapon(
                    rs_knl::WeaponInstance::createInstance(weaponManager().getWeapon(rs_knl::Weapon::TimeBomb)));
            agents().agent(i)->addWeapon(
                    rs_knl::WeaponInstance::createInstance(weaponManager().getWeapon(rs_knl::Weapon::EnergyShield)));
            agents().agent(i)->addWeapon(
                    rs_knl::WeaponInstance::createInstance(weaponManager().getWeapon(rs_knl::Weapon::EnergyShield)));
            agents().agent(i)->addWeapon(
                    rs_knl::WeaponInstance::createInstance(weaponManager().getWeapon(rs_knl::Weapon::Laser)));
            agents().agent(i)->addWeapon(
                    rs_knl::WeaponInstance::createInstance(weaponManager().getWeapon(rs_knl::Weapon::Laser)));
        }
    }
}

bool GameController::loadSquadLoadoutFromFile(const std::string &path) {
    if (!std::filesystem::exists(path)) {
        LOG(Log::k_FLG_INFO, "GameController", "loadSquadLoadoutFromFile", ("Loadout file not found: %s", path.c_str()));
        return false;
    }

    ConfigFile config(path);

    auto toLowerStr = [](std::string s) -> std::string {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    };

    auto parseModVersion = [&](const std::string &val, rs_knl::Mod::EModVersion &outVer) -> bool {
        std::string s = toLowerStr(val);
        if (s == "v1" || s == "1") { outVer = rs_knl::Mod::MOD_V1; return true; }
        if (s == "v2" || s == "2") { outVer = rs_knl::Mod::MOD_V2; return true; }
        if (s == "v3" || s == "3") { outVer = rs_knl::Mod::MOD_V3; return true; }
        return false;
    };

    auto parseWeaponType = [&](const std::string &val) -> rs_knl::Weapon::WeaponType {
        std::string s = toLowerStr(val);
        if (s == "pistol") return rs_knl::Weapon::Pistol;
        if (s == "gauss" || s == "gaussgun") return rs_knl::Weapon::GaussGun;
        if (s == "shotgun") return rs_knl::Weapon::Shotgun;
        if (s == "uzi") return rs_knl::Weapon::Uzi;
        if (s == "minigun") return rs_knl::Weapon::Minigun;
        if (s == "laser") return rs_knl::Weapon::Laser;
        if (s == "flamer" || s == "plasma") return rs_knl::Weapon::Flamer;
        if (s == "longrange" || s == "rifle") return rs_knl::Weapon::LongRange;
        if (s == "scanner") return rs_knl::Weapon::Scanner;
        if (s == "medikit") return rs_knl::Weapon::MediKit;
        if (s == "timebomb" || s == "mine" || s == "grenade") return rs_knl::Weapon::TimeBomb;
        if (s == "accesscard") return rs_knl::Weapon::AccessCard;
        if (s == "energyshield" || s == "shield") return rs_knl::Weapon::EnergyShield;
        if (s == "persuadatron" || s == "persuadotron" || s == "persuadertron") return rs_knl::Weapon::Persuadatron;
        if (s == "airstrike" || s == "air_strike") return rs_knl::Weapon::AirStrike;
        if (s == "cloneshield" || s == "clone_shield") return rs_knl::Weapon::CloneShield;
        return rs_knl::Weapon::Unknown;
    };

    auto parseCommaList = [](const std::string &str) -> std::vector<std::string> {
        std::vector<std::string> result;
        std::stringstream ss(str);
        std::string item;
        while (std::getline(ss, item, ',')) {
            size_t start = item.find_first_not_of(" \t\r\n");
            size_t end = item.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos) {
                result.push_back(item.substr(start, end - start + 1));
            }
        }
        return result;
    };

    // Determine active squad size if specified in loadout config
    if (config.keyExists("funds")) {
        g_Session.setMoney(static_cast<uint32_t>(std::max(0, config.read<int>("funds"))));
    } else if (config.keyExists("money")) {
        g_Session.setMoney(static_cast<uint32_t>(std::max(0, config.read<int>("money"))));
    }

    if (config.keyExists("company_name")) {
        g_Session.setCompanyName(config.read<std::string>("company_name").c_str());
    } else if (config.keyExists("company")) {
        g_Session.setCompanyName(config.read<std::string>("company").c_str());
    }

    if (config.keyExists("color")) {
        int col = std::clamp(config.read<int>("color"), 0, 7);
        g_Session.setLogoColour(col);
        g_AnimMgr.setPlayerFactionColor(g_LogoMgr.getColorAtIndex(col));
    } else if (config.keyExists("logo_colour")) {
        int col = std::clamp(config.read<int>("logo_colour"), 0, 7);
        g_Session.setLogoColour(col);
        g_AnimMgr.setPlayerFactionColor(g_LogoMgr.getColorAtIndex(col));
    }

    if (config.keyExists("logo")) {
        g_Session.setLogo(std::clamp(config.read<int>("logo"), 0, 39));
    } else if (config.keyExists("logo_id")) {
        g_Session.setLogo(std::clamp(config.read<int>("logo_id"), 0, 39));
    }

    int squadSize = rs_knl::Squad::kMaxSlot;
    if (config.keyExists("squad_size")) {
        squadSize = config.read<int>("squad_size");
    } else if (config.keyExists("active_agents")) {
        squadSize = config.read<int>("active_agents");
    }
    squadSize = std::clamp(squadSize, 1, static_cast<int>(rs_knl::Squad::kMaxSlot));

    for (uint8_t i = 0; i < rs_knl::Squad::kMaxSlot; i++) {
        bool slotActive = (i < squadSize);
        std::string activeKey = "agent." + std::to_string(i) + ".active";
        if (config.keyExists(activeKey)) {
            std::string actVal = toLowerStr(config.read<std::string>(activeKey));
            slotActive = (actVal == "true" || actVal == "1" || actVal == "yes");
        }

        if (slotActive) {
            if (!agents().squadMember(i) && agents().agent(i)) {
                agents().setSquadMember(i, agents().agent(i));
            }
            if (agents().squadMember(i)) {
                agents().squadMember(i)->setActive(true);
            }
        } else {
            if (agents().squadMember(i)) {
                agents().squadMember(i)->setActive(false);
            }
            agents().setSquadMember(i, nullptr);
        }
    }

    bool anyActive = false;
    for (uint8_t i = 0; i < rs_knl::Squad::kMaxSlot; i++) {
        if (agents().isSquadSlotActive(i)) {
            anyActive = true;
            break;
        }
    }
    if (!anyActive && agents().agent(0)) {
        agents().agent(0)->setActive(true);
        agents().setSquadMember(0, agents().agent(0));
    }

    const std::vector<rs_knl::Mod::EModType> modTypes = {
        rs_knl::Mod::MOD_LEGS, rs_knl::Mod::MOD_ARMS, rs_knl::Mod::MOD_CHEST,
        rs_knl::Mod::MOD_HEART, rs_knl::Mod::MOD_EYES, rs_knl::Mod::MOD_BRAIN
    };
    const std::vector<std::string> modKeys = {
        "legs", "arms", "chest", "heart", "eyes", "brain"
    };

    for (uint8_t i = 0; i < rs_knl::AgentManager::MAX_AGENT; i++) {
        rs_knl::Agent *pAgent = agents().agent(i);
        if (!pAgent) continue;

        pAgent->clearSlots();
        pAgent->destroyAllWeapons();

        // 1. Cybernetic Mods
        for (size_t m = 0; m < modTypes.size(); ++m) {
            std::string agentKey = "agent." + std::to_string(i) + ".mods." + modKeys[m];
            std::string globalKey = "mods." + modKeys[m];
            std::string modVal;

            if (config.keyExists(agentKey)) {
                modVal = config.read<std::string>(agentKey);
            } else if (config.keyExists(globalKey)) {
                modVal = config.read<std::string>(globalKey);
            }

            if (!modVal.empty()) {
                rs_knl::Mod::EModVersion ver;
                if (parseModVersion(modVal, ver)) {
                    pAgent->addMod(mods().getMod(modTypes[m], ver));
                }
            }
        }

        // 2. Weapons / Items
        std::string agentWpnKey = "agent." + std::to_string(i) + ".weapons";
        std::string globalWpnKey = "weapons";
        std::string wpnStr;

        if (config.keyExists(agentWpnKey)) {
            wpnStr = config.read<std::string>(agentWpnKey);
        } else if (config.keyExists(globalWpnKey)) {
            wpnStr = config.read<std::string>(globalWpnKey);
        }

        if (!wpnStr.empty()) {
            std::vector<std::string> tokens = parseCommaList(wpnStr);
            for (const auto &token : tokens) {
                rs_knl::Weapon::WeaponType wType = parseWeaponType(token);
                if (wType != rs_knl::Weapon::Unknown) {
                    if (rs_knl::Weapon::isAmericanRevoltOnly(wType) && !rs_utl::File::hasAmericanRevoltData()) {
                        LOG(Log::k_FLG_INFO, "GameController", "loadSquadLoadoutFromFile",
                            ("Skipping AR weapon '%s' because data path does not contain American Revolt assets", token.c_str()));
                        continue;
                    }
                    pAgent->addWeapon(rs_knl::WeaponInstance::createInstance(weaponManager().getWeapon(wType)));
                }
            }
        }
    }

    LOG(Log::k_FLG_INFO, "GameController", "loadSquadLoadoutFromFile", ("Successfully loaded loadout from %s", path.c_str()));
    return true;
}

