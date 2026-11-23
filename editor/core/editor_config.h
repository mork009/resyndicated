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

#ifndef EDITOR_CONFIG_H_
#define EDITOR_CONFIG_H_

#include <filesystem>
#include <string>

namespace rs_win_editor {

struct EditorWindowConfig {
    int x = -1;
    int y = -1;
    int width = 1280;
    int height = 800;
    bool maximized = false;
    int rightPaneWidth = 320;
    int leftPaneWidth = -1;
    int entityGridHeight = 224;
    int tileGridHeight = 224;
};

struct EditorPathsConfig {
    std::string dataDir;
    std::string dataSignature;

    const std::string &originalDataDir() const {
        return dataDir;
    }
};

struct PlaytestLoadoutData;

class EditorConfig {
public:
    static std::filesystem::path getConfigPath();
    static bool loadWindowConfig(EditorWindowConfig &config);
    static bool saveWindowConfig(const EditorWindowConfig &config);
    static bool loadPathsConfig(EditorPathsConfig &config);
    static bool savePathsConfig(const EditorPathsConfig &config);
    static bool loadLoadoutConfig(PlaytestLoadoutData &data);
    static bool saveLoadoutConfig(const PlaytestLoadoutData &data);
    static void ensureDefaultLoadoutSection();
    static PlaytestLoadoutData getDefaultLoadout();
    static std::vector<std::pair<std::string, std::string>> loadoutDataToEntries(const PlaytestLoadoutData &data);
    static void entriesToLoadoutData(const std::vector<std::pair<std::string, std::string>> &entries, PlaytestLoadoutData &data);
    static bool isAmericanRevoltWeapon(const std::string &weaponName);
    static bool sanitizeLoadout(PlaytestLoadoutData &data, const std::filesystem::path &originalDir = "");
};

} // namespace rs_win_editor

#endif // EDITOR_CONFIG_H_
