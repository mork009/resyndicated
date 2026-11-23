/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
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

#include "mainmenu.h"

#include <stdio.h>
#include <assert.h>
#include <filesystem>

#include "rs-engine/system/system.h"
#include "core/gamecontroller.h"
#include "core/gamesession.h"
#include "menus/gamemenuid.h"

using rs_eng::FontManager;

MainMenu::MainMenu(rs_eng::MenuManager * m):rs_eng::Menu(m, rs_game_menus::kMenuIdMain, rs_game_menus::kMenuIdMain, true)
{
    isCachable_ = false;
    cursorOnShow_ = kMenuCursor;
    addStatic(0, 40, rs_eng::kScreenWidth, "#MAIN_TITLE", FontManager::SIZE_4, false);
    addStatic(174, 306, "F6", FontManager::SIZE_2, false);

    int id = addOption(206, 130, 330, 25, "#MAIN_CONF", FontManager::SIZE_3, rs_game_menus::kMenuIdConf, true, false);
    registerHotKey(rs_eng::kKeyCode_F1, id);
    id = addOption(206, 164, 330, 25, "#MAIN_BEGIN", FontManager::SIZE_3, rs_game_menus::kMenuIdMap, true, false);
    registerHotKey(rs_eng::kKeyCode_F2, id);
    customButId_ = addOption(206, 198, 330, 25, "#MAIN_CUSTOM", FontManager::SIZE_3, kMenuIdNoMenu, true, false);
    registerHotKey(rs_eng::kKeyCode_F3, customButId_);
    id = addOption(206, 232, 330, 25, "#MAIN_LOAD_SAVE", FontManager::SIZE_3, rs_game_menus::kMenuIdLdSave, true, false);
    registerHotKey(rs_eng::kKeyCode_F4, id);
    resetButId_ = addOption(206, 266, 330, 25, "#MAIN_RESET", FontManager::SIZE_3, rs_game_menus::kMenuIdMain, true, false);
    registerHotKey(rs_eng::kKeyCode_F5, resetButId_);
    quitButId_ = addOption(206, 300, 330, 25, "#MAIN_QUIT", FontManager::SIZE_3, kMenuIdNoMenu, true, false);
    registerHotKey(rs_eng::kKeyCode_F6, quitButId_);
}

bool MainMenu::handleBeforeShow()
{
    if (g_Session.isCustomMission()) {
        g_Session.clearCustomMission();
        g_gameCtrl.resetCustomMissionDir();
    }
    return true;
}

void MainMenu::handleBeforeRender()
{
    constexpr int kExtraBoxWidth = 36;
    constexpr rs_eng::RSColor kBoxTint{128, 165, 0, 10};
    constexpr rs_eng::RSColor kSquareBevel{96, 255, 37, 45};

    // Uniformly widen the right-hand semi-transparent boxes for all 6 menu rows (F1..F6)
    for (int row = 0; row < 6; ++row) {
        const int rowY = 130 + row * 34;
        g_System.drawFillRect({502, rowY}, kExtraBoxWidth, 26, kBoxTint);

        // Row 6 (F6 Quit To DOS) does not exist in the original 5-option Bullfrog
        // MOPTION.DAT animation background; fill in its base boxes and green square.
        if (row == 5) {
            g_System.drawFillRect({144, rowY}, 26, 26, kBoxTint);
            g_System.drawFillRect({172, rowY}, 28, 26, kBoxTint);
            g_System.drawFillRect({202, rowY}, 300, 26, kBoxTint);
            g_System.drawFillRect({148, rowY + 4}, 18, 18, kSquareBevel);
            g_System.drawFillRect({150, rowY + 6}, 14, 14, menu_manager_->kMenuColorLightGreen);
        }
    }
}

void MainMenu::handleAction(const ActionDesc &action)
{
    if (action.id == customButId_) {
        std::filesystem::path selectedPath;
        if (g_System.browseFile(
                "Select Custom Syndicate Mission",
                "Syndicate Mission Files (*.dat)",
                "game*.dat;map*.dat;miss*.dat;*.dat",
                selectedPath)) {
            std::string errorMsg;
            if (g_gameCtrl.prepareCustomMissionFromFile(selectedPath, errorMsg)) {
                menu_manager_->gotoMenu(rs_game_menus::kMenuIdBrief);
            } else {
                g_System.showError(errorMsg.c_str());
            }
        }
        return;
    }
    if (action.id == resetButId_)
        g_gameCtrl.reset();
    if (action.id == quitButId_)
        menu_manager_->gotoMenu(Menu::kMenuIdLogout);
}
