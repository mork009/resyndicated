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

#ifndef EDITOR_PLAYTEST_MANAGER_H_
#define EDITOR_PLAYTEST_MANAGER_H_

#include <string>
#include <vector>
#include <windows.h>
#include "rs-kernel/model/mission.h"
#include "mission_properties_dialog.h"

namespace rs_win_editor {

struct MissionValidationReport {
    std::vector<std::wstring> errors;
    std::vector<std::wstring> warnings;
    bool isValid() const { return errors.empty(); }
};

constexpr UINT WM_PLAYTEST_FINISHED = WM_USER + 201;

class PlaytestManager {
public:
    /*!
     * Returns true if a playtest session is currently executing in the background.
     */
    static bool isPlaytesting();

    /*!
     * Validates mission playability before launching playtest.
     * Checks:
     * - Player's Agents (Slots 0..3) locations and walkability
     * - Objective combinations (e.g. Persuade, Protect, Use Vehicle must be paired with Evacuate)
     * - Target entities existence
     */
    static MissionValidationReport validateMissionPlayability(rs_knl::Mission *pMission);

    /*!
     * Exports current mission & map state to an isolated temporary directory and launches ReSyndicated.
     * Original files on disk are completely untouched.
     * \param hParent Parent window for error/warning message dialogs
     * \param pMission The active mission model
     * \param currentMissionId The active mission ID being edited
     * \param props Active mission briefing and objective properties
     * \param loadoutPath Path to squad loadout descriptor file (empty = use configured loadout)
     * \return true if playtest launched and returned successfully
     */
    static bool launchPlaytest(HWND hParent,
                               rs_knl::Mission *pMission,
                               int currentMissionId,
                               const MissionPropertiesData &props,
                               const std::string &loadoutPath = "");
};

} // namespace rs_win_editor

#endif // EDITOR_PLAYTEST_MANAGER_H_
