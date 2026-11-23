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
 *   Copyright (C) 2010, 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef APP_H
#define APP_H

#include <memory>

#include "rs-utils/common.h"
#include "rs-engine/base_app.h"
#include "rs-engine/appcontext.h"
#include "rs-kernel/mgr/mapmanager.h"
#include "core/gamecontroller.h"

class GameCliParam : public rs_eng::CliParam {

public:
    GameCliParam();

    int getStartingMission() const { return startMission_; }

    std::string getCheatCodes() const { return cheatCodes_; }
    //! Returns true if user has set cheatcodes
    bool hasCheatCodes() { return cheatCodes_.length() != 0; }

    std::string getLoadoutPath() const { return loadoutPath_; }
    //! Returns true if user has set custom loadout file
    bool hasLoadoutPath() const { return !loadoutPath_.empty(); }

    std::string getMissionDir() const { return missionDir_; }
    //! Returns true if user has specified custom mission directory
    bool hasMissionDir() const { return !missionDir_.empty(); }

protected:
    void addOptions(CLI::App &app) override;

private:
    /*!
     * Use in development mode to start directly on a mission and skip menus.
     * Set the variable to a mission id using CLI param "-m".
     * Note : the argument is the index of the block in the structure g_MissionNumbers
     * as defined in briefmenu.cpp and not the mission number itself.
     */
    int startMission_;

    /*!
     * This parameter is used to specify cheat codes on command line (option -c).
     * You can specify multiple codes using the ':' as a separator.
     * See available cheat codes in App::setCheatCode()
     * example -c "DO IT AGAIN:NUK THEM"
     */
    std::string cheatCodes_;

    /*!
     * Specifies path to a loadout descriptor file (option -l / --loadout) to equip
     * squad agents with custom weapons and cybernetic modifications.
     */
    std::string loadoutPath_;

    /*!
     * Specifies custom directory containing mission triad files (gameXX.dat, mapXX.dat, missXX.dat).
     */
    std::string missionDir_;
};

/*!
 * Application class.
 * Used for managing game settings and workflows.
 */
class App : public rs_eng::BaseApp {
  public:
    App();
    virtual ~App();

#ifdef _DEBUG
public:
    uint8_t debug_breakpoint_trigger_;
#endif

protected:
    //! Initialize application
    bool doInitialize() override;
    //! Reload game controller and session after data directory change
    bool doReloadAssets() override;
    //! Destroy all components
    void doDestroy() override;
    //! Define the menuid that will be displayed at the application's start
    int getStartMenuId() override;

    rs_eng::CliParam & getCliParam() override {
        return cliParam_;
    };

private:
    /*! Controls the game logic. */
    std::unique_ptr<GameController> game_ctlr_;
    GameCliParam cliParam_;
};

#endif  // APP_H

