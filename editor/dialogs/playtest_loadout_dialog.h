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

#ifndef EDITOR_PLAYTEST_LOADOUT_DIALOG_H_
#define EDITOR_PLAYTEST_LOADOUT_DIALOG_H_

#include <windows.h>
#include <string>
#include <vector>

namespace rs_win_editor {

struct AgentLoadoutData {
    bool active = true;
    int modLegs = 0;   // 0=None, 1=V1, 2=V2, 3=V3
    int modArms = 0;
    int modChest = 0;
    int modHeart = 0;
    int modEyes = 0;
    int modBrain = 0;
    std::vector<std::string> weapons; // max 8 weapons
};

struct PlaytestLaunchSettings {
    int windowScale = 2; // 1, 2, 3, 4
    float mapZoom = 1.0f; // 1.0, 1.5, 2.0, 2.5, 3.0
    bool fullscreen = false;
    bool noSound = false;
    std::string logOption = "None"; // "None", "ALL", "GAME", "KERNEL", "Custom"
    std::string customLogFlags;
    bool showCoordinates = false;
    bool showAgentPaths = false;
    bool showStats = false;
};

struct PlaytestFactionSettings {
    int funds = 100000;
    std::string companyName = "EUROCORP";
    int color = 0; // 0..7
    int logo = 0;  // 0..39
};

struct PlaytestLoadoutData {
    int activeSquadSize = 4; // 1 to 4
    AgentLoadoutData agents[4];
    PlaytestLaunchSettings launch;
    PlaytestFactionSettings faction;
};

using PlaytestConfigurationData = PlaytestLoadoutData;

class PlaytestLoadoutDialog {
public:
    static bool show(HWND hParent, PlaytestLoadoutData &data);
    static bool loadFromConfigFile(const std::string &path, PlaytestLoadoutData &data);
    static bool saveToConfigFile(const std::string &path, const PlaytestLoadoutData &data);
    static std::string getEffectiveLoadoutPath();
    static PlaytestLoadoutData getDefaultLoadout();
    static bool sanitizeLoadout(PlaytestLoadoutData &data);
    static bool isAmericanRevoltWeapon(const std::string &weaponName);
};

} // namespace rs_win_editor

#endif // EDITOR_PLAYTEST_LOADOUT_DIALOG_H_
