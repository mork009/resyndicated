/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
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

#ifndef MAINMENU_H
#define MAINMENU_H

#include "rs-engine/menus/menumanager.h"

/*!
 * Main Menu class.
 */
class MainMenu : public rs_eng::Menu {
public:
    MainMenu(rs_eng::MenuManager *m);

    bool handleBeforeShow() override;
    void handleAction(const ActionDesc &action) override;

protected:
    void handleBeforeRender() override;

    /*! Id of the load custom mission button.*/
    int customButId_;
    /*! Id of the reset button.*/
    int resetButId_;
    /*! Id of the quit button.*/
    int quitButId_;
};

#endif
