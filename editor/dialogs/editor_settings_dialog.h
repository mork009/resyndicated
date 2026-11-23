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

#ifndef EDITOR_SETTINGS_DIALOG_H_
#define EDITOR_SETTINGS_DIALOG_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>

#include "editor_config.h"
#include "rs-engine/ui/settings_dialog.h"
#include "rs-utils/io/file.h"

namespace rs_win_editor {

using AssetFileCheck = rs_eng::AssetCheckItem;
using DataDirValidationResult = rs_eng::DataValidationResult;

inline DataDirValidationResult verifyEditorDataDirectory(const std::filesystem::path &dir) {
    return rs_eng::verifyDataDirectories(dir);
}

inline bool browseForDirectory(HWND hOwner, const std::wstring &title, std::wstring &outFolder) {
    return rs_eng::browseForFolder(hOwner, title, outFolder);
}

inline bool showDataDirSetupDialog(HWND hParent, const EditorPathsConfig &initialPaths, EditorPathsConfig &outConfiguredPaths) {
    std::string outPath;
    if (rs_eng::showDataDirSetupDialog(hParent, initialPaths.dataDir, outPath, L"ReSyndicated Editor")) {
        outConfiguredPaths.dataDir = outPath;
        return true;
    }
    return false;
}

inline bool showDataDirSetupDialog(HWND hParent, const std::string &initialInvalidPath, std::string &outConfiguredPath) {
    return rs_eng::showDataDirSetupDialog(hParent, initialInvalidPath, outConfiguredPath, L"ReSyndicated Editor");
}

inline bool showEditorSettingsDialog(HWND hParent, EditorPathsConfig &inOutPaths) {
    std::string outPath;
    if (rs_eng::showSettingsDialog(hParent, inOutPaths.dataDir, outPath, L"ReSyndicated Editor")) {
        inOutPaths.dataDir = outPath;
        inOutPaths.dataSignature = rs_utl::File::computeDataSignature(outPath);
        return true;
    }
    return false;
}

inline bool showEditorSettingsDialog(HWND hParent, std::string &outNewDataDir) {
    return rs_eng::showSettingsDialog(hParent, outNewDataDir, outNewDataDir, L"ReSyndicated Editor");
}

} // namespace rs_win_editor

#endif // EDITOR_SETTINGS_DIALOG_H_
