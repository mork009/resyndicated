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

#ifndef LOADINGMENU_H
#define LOADINGMENU_H

#include "rs-utils/misc/timer.h"
#include "rs-engine/menus/menu.h"

/*!
 * This menu is in charge of loading the mission.
 */
class LoadingMenu : public rs_eng::Menu {
public:
    LoadingMenu(rs_eng::MenuManager *m);

    bool handleTick(uint32_t elapsed) override;

protected:
    /*! This is a flag to load the mission only once.*/
    bool do_load_;
    /*! This timer is used to be sure we can see the loading screen.
     * We don't want the application to be too fast!
     */
    rs_utl::Timer timer_;
};

#endif
