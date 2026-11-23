/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
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

#ifndef EDITOR_AI_TACTICS_DIALOG_H_
#define EDITOR_AI_TACTICS_DIALOG_H_

#include <windows.h>
#include <string>
#include <vector>
#include "rs-kernel/model/mission.h"
#include "editor_commands.h"

namespace rs_win_editor {

class WinMapViewport;

/*!
 * Dark-themed modal dialog for viewing, editing, and creating
 * AI Tactics Finite State Machines (FSM) for mission enemy factions.
 */
class AiTacticsDialog {
public:
    static bool show(HWND hParent, WinMapViewport *pViewport, rs_knl::Mission *pMission,
                     EditorCommandStack *pCommandStack, int preselectedFaction = -1);
};

} // namespace rs_win_editor

#endif // EDITOR_AI_TACTICS_DIALOG_H_
