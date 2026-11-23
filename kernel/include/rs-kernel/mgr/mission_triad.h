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

#ifndef RS_KERNEL_MGR_MISSION_TRIAD_H_
#define RS_KERNEL_MGR_MISSION_TRIAD_H_

#include <filesystem>
#include <string>
#include <cstdint>

namespace rs_knl {

struct MissionTriadPaths {
    std::filesystem::path gameFile;
    std::filesystem::path mapFile;
    std::filesystem::path missFile;
    int missionId = 0;
    int mapId = 0;
};

struct TriadValidationResult {
    bool isValid = false;
    bool gameOk = false;
    bool mapOk = false;
    bool missOk = false;
    std::wstring gameStatus;
    std::wstring mapStatus;
    std::wstring missStatus;
    std::wstring summaryMessage;
};

struct TriadOverwriteInfo {
    bool anyExists = false;
    bool gameExists = false;
    bool mapExists = false;
    bool missExists = false;
    std::wstring warningMessage;
};

class MissionTriad {
public:
    /*!
     * Given any file chosen by the user (gameXX.dat, mapXX.dat, or missXX.dat),
     * resolves all 3 files belonging to the mission triad.
     */
    static bool resolveFromAnyFile(const std::filesystem::path &chosenPath, MissionTriadPaths &outPaths);

    /*!
     * Given a mission ID and optional custom directory, resolves standard triad paths.
     */
    static MissionTriadPaths resolveFromId(int missionId, const std::filesystem::path &customDir = "");

    /*!
     * Validates the existence and internal integrity of the 3 required mission files.
     */
    static TriadValidationResult validate(const MissionTriadPaths &paths);

    /*!
     * Checks if any file in the triad already exists on disk and formats an overwrite warning message.
     */
    static TriadOverwriteInfo checkOverwrite(const MissionTriadPaths &paths);

    /*!
     * Helper to read the map ID from a game binary file without full instantiation.
     */
    static int readMapIdFromGameFile(const std::filesystem::path &gameFilePath);
};

} // namespace rs_knl

#endif // RS_KERNEL_MGR_MISSION_TRIAD_H_
