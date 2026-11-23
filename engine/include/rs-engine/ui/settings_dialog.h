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

#ifndef RS_ENGINE_SETTINGS_DIALOG_H_
#define RS_ENGINE_SETTINGS_DIALOG_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <string>
#include <vector>
#include <filesystem>

namespace rs_eng {

struct AssetCheckItem {
    std::string description;
    std::string filename;
    bool isPresent = false;
    std::string detail;
};

struct DataValidationResult {
    bool isValid = false;
    std::filesystem::path originalDir;
    std::vector<AssetCheckItem> originalChecks;
    int missingCount = 0;
    int missionCount = 0;
    bool bundledDataValid = true;
    bool hasAmericanRevolt = false;
    std::wstring summaryMessage;
};

//! Validates whether originalDir contains required Syndicate DOS files and bundled ReSyndicated assets exist
DataValidationResult verifyDataDirectories(const std::filesystem::path &originalDir);

bool showGameDataSetup(const std::string &initialPath, std::string &selectedPath);

#ifdef _WIN32
//! Opens the native Windows folder picker
bool browseForFolder(HWND hOwner, const std::wstring &title, std::wstring &outFolder);

//! First-time or invalid-path setup dialog. Prompts user to configure original game data path.
//! Returns true if confirmed with a valid directory, false if cancelled/exited.
bool showDataDirSetupDialog(HWND hParent, const std::string &initialPath, std::string &outConfiguredPath, const std::wstring &appTitle = L"ReSyndicated");

//! Runtime Settings dialog accessible from the window menu bar.
//! Returns true if user clicked Save/OK with a valid path, false if cancelled.
bool showSettingsDialog(HWND hParent, const std::string &currentPath, std::string &outNewDataDir, const std::wstring &appTitle = L"ReSyndicated");

//! Compares two filesystem paths (canonicalizing separators, relative/absolute differences, case-insensitivity on Windows)
//! Returns true if they point to different filesystem locations.
bool pathsAreDifferent(const std::string &p1, const std::string &p2);
bool pathsAreDifferent(const std::wstring &p1, const std::wstring &p2);

#endif

//! Helper to persist data_dir and optional data_signature into resynd.conf
bool saveOriginalDataDirToResyndConf(const std::string &dataDir, const std::string &dataSignature = "");

} // namespace rs_eng

#endif // RS_ENGINE_SETTINGS_DIALOG_H_
