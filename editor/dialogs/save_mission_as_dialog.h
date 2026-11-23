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

#ifndef EDITOR_SAVE_MISSION_AS_DIALOG_H_
#define EDITOR_SAVE_MISSION_AS_DIALOG_H_

#include <windows.h>
#include <string>
#include <filesystem>
#include "rs-kernel/mgr/mission_triad.h"
#include "mission_properties_dialog.h"

namespace rs_win_editor {

struct SaveMissionAsDialogData {
    int targetMissionId = 1;
    int targetMapId = 1;
    int targetLanguage = 0;
    std::filesystem::path targetDir;
    bool saveCustomMapCopy = true;
    rs_knl::MissionTriadPaths resolvedPaths;
    bool availableLanguages[4] = { true, false, false, false };
    bool modifiedLanguages[4] = { false, false, false, false };
};

class SaveMissionAsDialog {
public:
    static bool show(HWND hParent, int currentMissionId, int currentMapId,
                     const MissionPropertiesData &props,
                     const std::filesystem::path &initialDir, SaveMissionAsDialogData &outData);
};

} // namespace rs_win_editor

#endif // EDITOR_SAVE_MISSION_AS_DIALOG_H_
