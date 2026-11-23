/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net>
 *   Copyright (C) 2011  Joey Parrish  <joey.parrish@gmail.com>
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

#include "app.h"

#include <windows.h>

#include <iostream>
#include <fstream>
#include <set>

#include "rs-utils/log/log.h"
#include "rs-utils/io/configfile.h"
#include "rs-utils/io/portablefile.h"
#include "menus/gamemenufactory.h"
#include "menus/gamemenuid.h"

GameCliParam::GameCliParam():startMission_(-1), cheatCodes_(""), loadoutPath_(""), missionDir_("") {}

void GameCliParam::addOptions(CLI::App &app) {
    app.add_option("-m,--mission", startMission_, "Jump directly to the specified mission.")
        ->option_text("<id>")->check(CLI::PositiveNumber);
    app.add_option("-c,--cheat", cheatCodes_, "Set the list of cheatcodes separated by colon.")->option_text("<codes>");
    app.add_option("-L,--loadout", loadoutPath_, "Specify descriptor file to set agent weapons and mods.")->option_text("<path>");
    app.add_option("-D,--mission-dir", missionDir_, "Specify custom directory containing mission triad files (gameXX.dat, mapXX.dat, missXX.dat).")->option_text("<path>");
}

App::App()
    : BaseApp(new GameMenuFactory()),
      game_ctlr_(std::make_unique<GameController>())
{
#ifdef _DEBUG
    debug_breakpoint_trigger_ = 0;
#endif
}

App::~App() {
}

/*!
 * Initialize application.
 * \return True if initialization is ok.
 */
bool App::doInitialize() {
    if (!game_ctlr_->initialize()) {
        return false;
    }

    if (cliParam_.hasMissionDir()) {
        game_ctlr_->setDefaultCustomMissionDir(cliParam_.getMissionDir());
    }

    menus().cheatCallback = [this](int cheatId) {
        if (cheatId == 3001) {
            game_ctlr_->cheatFemaleRecruits();
            game_ctlr_->cheatFunds();
            game_ctlr_->cheatAnyMission();
            game_ctlr_->cheatWeaponsAndMods();
            game_ctlr_->cheatEquipAllMods();
            game_ctlr_->cheatEquipFancyWeapons();
        } else if (cheatId == 3002) {
            game_ctlr_->cheatAnyMission();
        } else if (cheatId == 3003) {
            game_ctlr_->cheatFunds();
        } else if (cheatId == 3004) {
            game_ctlr_->cheatWeaponsAndMods();
        } else if (cheatId == 3005) {
            game_ctlr_->cheatEquipAllMods();
        } else if (cheatId == 3006) {
            game_ctlr_->cheatEquipFancyWeapons();
        }

        if (menus().getCurrentMenu()) {
            menus().gotoMenu(menus().getCurrentMenu()->getId());
        }
    };

    menus().zoomCallback = [](double /*zoomFactor*/) {
        // g_mapZoom is updated globally in System. No menu reload needed.
    };

    // setting the cheat codes
    if (cliParam_.hasCheatCodes()) {
        std::string cheatCodes(cliParam_.getCheatCodes());
        char *cheats = (char *)cheatCodes.c_str();
        char *token = strtok(cheats, ":");
        while ( token != NULL ) {
            LOG(Log::k_FLG_INFO, "Main", "main", ("Cheat code activated : %s", token))
            game_ctlr_->setCheatCode(token);
            token = strtok(NULL, ":");
        }
    }

    // loading custom squad loadout descriptor
    if (cliParam_.hasLoadoutPath()) {
        LOG(Log::k_FLG_INFO, "Main", "main", ("Loading custom loadout descriptor : %s", cliParam_.getLoadoutPath().c_str()));
        game_ctlr_->loadSquadLoadoutFromFile(cliParam_.getLoadoutPath());
    }

    return true;
}

/*!
 * Destroy the application.
 */
void App::doDestroy() {
     game_ctlr_->destroy();
}

bool App::doReloadAssets() {
    if (!game_ctlr_->initialize()) {
        return false;
    }

    if (cliParam_.hasCheatCodes()) {
        std::string cheatCodes(cliParam_.getCheatCodes());
        char *cheats = (char *)cheatCodes.c_str();
        char *token = strtok(cheats, ":");
        while (token != NULL) {
            game_ctlr_->setCheatCode(token);
            token = strtok(NULL, ":");
        }
    }

    if (cliParam_.hasLoadoutPath()) {
        game_ctlr_->loadSquadLoadoutFromFile(cliParam_.getLoadoutPath());
    }

    menus().gotoMenu(rs_game_menus::kMenuIdMain);
    return true;
}

/*!
 * This method defines the application loop.
 * \param start_mission Mission id used to start the application in debug mode
 * In standard mode start_mission is always -1.
 */
/*!
 * This method returns the menu Id used to start the app.
 */
int App::getStartMenuId() {
    if (cliParam_.getStartingMission() == -1) {
        if (context_->isPlayIntro()) {
            // Update intro flag so intro won't be played next time
            context_->updateIntroFlag();
            return rs_game_menus::kMenuIdFliIntro;
        } else {
            // play title before going to main menu
            return rs_game_menus::kMenuIdFliTitle;
        }
    }
    else {
        // Debug scenario : start directly with the brief menu
        // in the given mission
        // First, we find the block associated with the given
        // mission number
        bool found = false;
        for (uint8_t i = 0; i < 50; i++) {
            if (g_Session.getBlock(i).mis_id == cliParam_.getStartingMission()) {
                g_Session.setSelectedBlockId(i);
                found = true;
                break;
            }
        }
        if (!found) {
            g_Session.setCustomMission(cliParam_.getStartingMission());
        }
        // Then we go to the brief menu
        return rs_game_menus::kMenuIdLoading;
    }
}
