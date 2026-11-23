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

#ifndef EDITOR_MISSION_CHOOSER_H_
#define EDITOR_MISSION_CHOOSER_H_

#include <windows.h>
#include <string>
#include <vector>

namespace rs_win_editor {

struct MissionInfo {
    int id;
    std::wstring name;
};

class MissionChooserDialog {
public:
    static bool show(HWND hParent, int currentId, int &outMissionId, std::wstring &outCustomPath);
    static const std::vector<MissionInfo>& getMissionList();

private:
    static LRESULT CALLBACK DialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

} // namespace rs_win_editor

#endif // EDITOR_MISSION_CHOOSER_H_
