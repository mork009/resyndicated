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

#ifndef EDITOR_NEW_MISSION_DIALOG_H_
#define EDITOR_NEW_MISSION_DIALOG_H_

#include <windows.h>
#include <string>

namespace rs_win_editor {

struct NewMissionDialogData {
    int missionId = 99;
    bool isBlankMap = true;
    int baseMapId = 1;
    int spawnTx = 64;
    int spawnTy = 48;
    int spawnTz = 2;
    std::string briefingTitle = "OPERATION NEW DAWN";
};

class NewMissionDialog {
public:
    static bool show(HWND hParent, NewMissionDialogData &outData);
};

} // namespace rs_win_editor

#endif // EDITOR_NEW_MISSION_DIALOG_H_
