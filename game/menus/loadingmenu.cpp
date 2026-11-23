/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2010, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net>
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

#include "loadingmenu.h"

#include "rs-kernel/model/mission.h"
#include "core/gamecontroller.h"
#include "rs-engine/menus/menumanager.h"
#include "menus/gamemenuid.h"
#include <SDL3/SDL.h>

LoadingMenu::LoadingMenu(rs_eng::MenuManager * m):rs_eng::Menu(m, rs_game_menus::kMenuIdLoading, rs_game_menus::kMenuIdMain, true),
    timer_(2000)
{
    isCachable_ = false;
    do_load_ = true;
    addStatic(0, 180, rs_eng::kScreenWidth, "#LDGAME_TITLE", rs_eng::FontManager::SIZE_4, true);
}

bool LoadingMenu::handleTick(uint32_t elapsed)
{
    if (do_load_) {
        // Loads mission
        if (!g_gameCtrl.loadSelectedMission()) {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                "Mission Load Error",
                "Failed to load mission.\n\n"
                "This mission requires Syndicate: American Revolt data assets (Air Strike / Clone Shield / Data Disk),\n"
                "but your currently configured data directory only contains original Syndicate files.\n\n"
                "Please point your data directory to an American Revolt installation (such as DATADISK\\DATA) via Settings.",
                nullptr);
            menu_manager_->gotoMenu(rs_game_menus::kMenuIdBrief);
            return true;
        }

        do_load_ = false;
    }

    if (timer_.update(elapsed) && g_missionCtrl.mission()) {
        menu_manager_->gotoMenu(rs_game_menus::kMenuIdGameplay);
    }

    return true;
}
