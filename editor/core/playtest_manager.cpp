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

#include "playtest_manager.h"
#include "playtest_loadout_dialog.h"
#include "editor_config.h"
#include "mission_properties_dialog.h"
#include "dialog_theme.h"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <string>
#include <vector>
#include <unordered_set>
#include <atomic>
#include <thread>
#include <windows.h>

#include "tile_rasterizer.h"
#include "rs-utils/io/file.h"
#include "rs-utils/log/log.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/model/objectivedesc.h"

namespace rs_win_editor {

static std::atomic<bool> s_isPlaytestingActive{false};

bool PlaytestManager::isPlaytesting() {
    return s_isPlaytestingActive.load();
}

MissionValidationReport PlaytestManager::validateMissionPlayability(rs_knl::Mission *pMission) {
    MissionValidationReport report;
    if (!pMission) {
        report.errors.push_back(L"No mission loaded.");
        return report;
    }

    // Always rebuild the navigation graph before validation so walkability checks
    // accurately evaluate all newly painted or modified map tiles!
    pMission->buildNavigationGraph();

    // 1. Check for Player's Agents (Slots 0..3)
    int playerAgentCount = 0;
    int unwalkableSpawnCount = 0;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        rs_knl::PedInstance *p = pMission->ped(i);
        if (p && p->isOurAgent()) {
            playerAgentCount++;
            if (pMission->map() && !pMission->isTileWalkable(p->tileX(), p->tileY(), p->tileZ())) {
                unwalkableSpawnCount++;
                rs_eng::Tile *t = pMission->map()->getTileAt(p->tileX(), p->tileY(), p->tileZ());
                uint8_t tid = t ? static_cast<uint8_t>(t->id()) : 0;
                std::string typeName = t ? TileRasterizer::instance().getTileTypeName(static_cast<uint8_t>(t->type())) : "Out of bounds";
                std::wstring wTypeName(typeName.begin(), typeName.end());
                report.warnings.push_back(std::format(
                    L"Player's Agent #{} at ({}, {}, {}) is on tile #{:03} [{}] which is not walkable. Place it on a walkable Ground, Road, or Platform tile.",
                    p->id() + 1, p->tileX(), p->tileY(), p->tileZ(), tid, wTypeName
                ));
            }
        }
    }

    if (playerAgentCount != 4) {
        report.errors.push_back(std::format(
            L"Player's Agents: The mission contains {} Player's Agents. Exactly 4 player agents must be placed on the map.",
            playerAgentCount
        ));
    }

    if (pMission->numPeds() > 256) {
        report.errors.push_back(std::format(
            L"Too Many Pedestrians: The mission has {} pedestrians, exceeding the maximum limit of 256 supported by the Syndicate binary level format.",
            pMission->numPeds()
        ));
    }

    std::unordered_set<uint16_t> seenPlaytestPedIds;
    for (size_t i = 0; i < pMission->numPeds(); ++i) {
        auto *p = pMission->ped(i);
        if (!p) continue;
        if (seenPlaytestPedIds.count(p->id()) > 0) {
            report.errors.push_back(std::format(
                L"Duplicate Pedestrian ID #{}: Multiple pedestrians share ID #{}. Each pedestrian must have a unique ID (0..255).",
                p->id(), p->id()
            ));
            break;
        }
        seenPlaytestPedIds.insert(p->id());
    }

    if (pMission->numVehicles() > 64) {
        report.errors.push_back(std::format(
            L"Too Many Vehicles: The mission has {} vehicles, exceeding the maximum limit of 64 supported by the Syndicate binary level format.",
            pMission->numVehicles()
        ));
    }

    if (pMission->numStatics() > 400) {
        report.errors.push_back(std::format(
            L"Too Many Static Objects: The mission has {} static objects, exceeding the maximum limit of 400 supported by the Syndicate binary level format.",
            pMission->numStatics()
        ));
    }

    // Check if there are ANY walkable tiles reached on the entire map
    int totalWalkableTiles = 0;
    if (pMission->map()) {
        int maxTx = pMission->map()->maxTx();
        int maxTy = pMission->map()->maxTy();
        int maxTz = pMission->map()->maxTz();
        for (int z = 0; z < maxTz; ++z) {
            for (int y = 0; y < maxTy; ++y) {
                for (int x = 0; x < maxTx; ++x) {
                    if (pMission->isTileWalkable(x, y, z)) {
                        totalWalkableTiles++;
                    }
                }
            }
        }
    }

    if (playerAgentCount > 0 && totalWalkableTiles == 0) {
        report.warnings.push_back(
            L"No Walkable Tiles: The map contains 0 walkable tiles reachable by the squad. In Syndicate, Tile #000 is empty void. Paint walkable ground (e.g., Concrete, Pavement, Road) under the Player's Agents so they can stand and navigate."
        );
    }

    // 2. Check Objectives
    if (pMission->numObjectives() == 0) {
        report.warnings.push_back(
            L"No Mission Objectives: The mission has no objectives defined. The squad will have no victory conditions."
        );
    } else {
        if (pMission->numObjectives() > 6) {
            report.warnings.push_back(
                L"Too Many Objectives: Mission defines more than 6 objectives (Syndicate binary level format supports up to 6)."
            );
        }

        bool hasPersuadeOrProtectOrUseVehicle = false;
        bool hasEvacuate = false;

        for (size_t i = 0; i < pMission->numObjectives(); ++i) {
            rs_knl::ObjectiveDesc *obj = pMission->objective(i);
            if (!obj) continue;

            if (auto *op = dynamic_cast<rs_knl::ObjPersuade*>(obj)) {
                hasPersuadeOrProtectOrUseVehicle = true;
                if (!op->target()) {
                    report.errors.push_back(std::format(L"Objective #{}: Persuade target Ped is missing or invalid.", i + 1));
                }
            } else if (auto *oa = dynamic_cast<rs_knl::ObjAssassinate*>(obj)) {
                if (!oa->target()) {
                    report.errors.push_back(std::format(L"Objective #{}: Assassinate target Ped is missing or invalid.", i + 1));
                }
            } else if (auto *opr = dynamic_cast<rs_knl::ObjProtect*>(obj)) {
                hasPersuadeOrProtectOrUseVehicle = true;
                if (!opr->target()) {
                    report.errors.push_back(std::format(L"Objective #{}: Protect target Ped is missing or invalid.", i + 1));
                }
            } else if (auto *odv = dynamic_cast<rs_knl::ObjDestroyVehicle*>(obj)) {
                if (!odv->target()) {
                    report.errors.push_back(std::format(L"Objective #{}: Destroy Vehicle target is missing or invalid.", i + 1));
                }
            } else if (auto *ouv = dynamic_cast<rs_knl::ObjUseVehicle*>(obj)) {
                hasPersuadeOrProtectOrUseVehicle = true;
                if (!ouv->target()) {
                    report.errors.push_back(std::format(L"Objective #{}: Use Vehicle target is missing or invalid.", i + 1));
                }
            } else if (dynamic_cast<rs_knl::ObjEvacuate*>(obj)) {
                hasEvacuate = true;
            }
        }

        if (hasPersuadeOrProtectOrUseVehicle && !hasEvacuate) {
            report.warnings.push_back(
                L"Missing Evacuate Objective: The mission contains a Persuade, Protect, or Use Vehicle objective, but no Evacuate zone is defined. These objectives typically require an Evacuate objective to complete the mission."
            );
        }
    }

    return report;
}

bool PlaytestManager::launchPlaytest(HWND hParent,
                                     rs_knl::Mission *pMission,
                                     int currentMissionId,
                                     const MissionPropertiesData &props,
                                     const std::string &loadoutPath) {
    if (!pMission) return false;

    if (s_isPlaytestingActive.load()) {
        showEditorMessageBox(hParent,
                    L"A playtest session is already running.\nPlease close ReSyndicated before starting another playtest.",
                    L"Playtest Active",
                    MB_OK | MB_ICONINFORMATION);
        return false;
    }

    // 1. Run Pre-Playtest Mission Validation
    MissionValidationReport report = validateMissionPlayability(pMission);

    if (!report.errors.empty()) {
        std::wstring msg = L"Cannot start playtest due to mission errors:\n\n";
        for (const auto &err : report.errors) {
            msg += L"• " + err + L"\n";
        }
        msg += L"\nPlease correct these issues in the editor before playtesting.";
        showEditorMessageBox(hParent, msg.c_str(), L"Playtest Mission Validation Failed", MB_ICONERROR | MB_OK);
        return false;
    }

    if (!report.warnings.empty()) {
        std::wstring msg = L"Playtest mission validation warnings:\n\n";
        for (const auto &w : report.warnings) {
            msg += L"• " + w + L"\n";
        }
        msg += L"\nDo you want to proceed with playtesting anyway?";
        int res = showEditorMessageBox(hParent, msg.c_str(), L"Playtest Mission Warnings", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
        if (res != IDYES) {
            return false;
        }
    }

    // 2. Create an isolated temporary directory for playtest files so real files on disk are untouched
    std::error_code ec;
    std::filesystem::path tempDir = std::filesystem::temp_directory_path() / "resynd_playtest";
    std::filesystem::create_directories(tempDir, ec);

    int playtestMissionId = (currentMissionId > 0) ? currentMissionId : 1;
    std::filesystem::path gameFullPath = tempDir / std::format("game{:02}.dat", playtestMissionId);
    std::filesystem::path mapFullPath = tempDir / std::format("map{:02}.dat", playtestMissionId);
    std::filesystem::path missFullPath = tempDir / std::format("miss{:02}.dat", playtestMissionId);

    LOG(Log::k_FLG_INFO, "PlaytestManager", "launchPlaytest",
        ("Exporting playtest mission to isolated temp dir: %s", tempDir.string().c_str()));

    // Synchronize level data map index, map ID, and scroll limits before export
    rs_utl::WRITE_LE_UINT16(pMission->getRawLevelData().mapinfos.map, static_cast<uint16_t>(playtestMissionId));
    if (pMission->map()) {
        pMission->map()->setId(static_cast<uint16_t>(playtestMissionId));
        int maxTx = pMission->map()->maxTx();
        int maxTy = pMission->map()->maxTy();
        LevelData::LevelDataAll &rawLd = pMission->getRawLevelData();
        uint16_t curMinX = rs_utl::READ_LE_UINT16(rawLd.mapinfos.min_x);
        uint16_t curMaxX = rs_utl::READ_LE_UINT16(rawLd.mapinfos.max_x);
        uint16_t curMinY = rs_utl::READ_LE_UINT16(rawLd.mapinfos.min_y);
        uint16_t curMaxY = rs_utl::READ_LE_UINT16(rawLd.mapinfos.max_y);
        if (curMaxX <= curMinX || curMaxY <= curMinY) {
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.min_x, 0);
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.min_y, 0);
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.max_x, static_cast<uint16_t>((maxTx - 1) * 2));
            rs_utl::WRITE_LE_UINT16(rawLd.mapinfos.max_y, static_cast<uint16_t>((maxTy - 1) * 2));
        }
    }

    if (!rs_knl::MissionManager::exportMissionToBinary(pMission, playtestMissionId, gameFullPath.string())) {
        RSERR(Log::k_FLG_IO, "PlaytestManager", "launchPlaytest",
              ("Failed to export level data to %s", gameFullPath.string().c_str()));
        std::filesystem::remove_all(tempDir, ec);
        return false;
    }

    if (pMission->map()) {
        if (!pMission->map()->exportBinary(mapFullPath.string())) {
            RSERR(Log::k_FLG_IO, "PlaytestManager", "launchPlaytest",
                  ("Failed to export map data to %s", mapFullPath.string().c_str()));
            std::filesystem::remove_all(tempDir, ec);
            return false;
        }
    }

    MissionPropertiesData playtestProps = props;
    playtestProps.missionId = playtestMissionId;
    playtestProps.mapId = playtestMissionId;
    if (playtestProps.baseBriefing.empty()) {
        playtestProps.baseBriefing = "PLAYTEST MISSION OBJECTIVE:\nInfiltrate and test the mission environment.";
    }
    MissionPropertiesDialog::saveBriefingDataToPath(missFullPath.string(), playtestProps);

    // 3. Resolve loadout path
    std::string resolvedLoadout = loadoutPath;
    if (resolvedLoadout.empty()) {
        resolvedLoadout = PlaytestLoadoutDialog::getEffectiveLoadoutPath();
    }
    if (!std::filesystem::exists(resolvedLoadout)) {
        std::string candidate = rs_utl::File::getResyndDataFullPath("ref/sample_loadout.conf");
        if (std::filesystem::exists(candidate)) {
            resolvedLoadout = candidate;
        }
    }

    // 4. Locate resynd_game.exe executable
    char moduleBuf[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, moduleBuf, MAX_PATH);
    std::filesystem::path moduleDir = std::filesystem::path(moduleBuf).parent_path();

    std::vector<std::filesystem::path> searchCandidates = {
        moduleDir / "resynd_game.exe",
        moduleDir / "../game/resynd_game.exe",
        moduleDir / "../../game/resynd_game.exe",
        moduleDir / "../../../build/ninja-msvc-x64-debug/game/resynd_game.exe",
        moduleDir / "../../../build/ninja-msvc-x64-release/game/resynd_game.exe",
        std::filesystem::current_path() / "resynd_game.exe",
        std::filesystem::current_path() / "build/ninja-msvc-x64-debug/game/resynd_game.exe",
        std::filesystem::current_path() / "build/ninja-msvc-x64-release/game/resynd_game.exe"
    };

    std::filesystem::path exeFullPath;
    for (const auto &p : searchCandidates) {
        if (std::filesystem::exists(p, ec)) {
            exeFullPath = std::filesystem::canonical(p, ec);
            if (exeFullPath.empty()) {
                exeFullPath = std::filesystem::absolute(p, ec);
            }
            break;
        }
    }

    if (exeFullPath.empty()) {
        std::wstring errMsg = std::format(
            L"Could not locate resynd_game.exe for playtesting.\n\n"
            L"Searched relative to editor binary directory:\n{}\n\n"
            L"Please ensure the game has been built.",
            moduleDir.wstring()
        );
        showEditorMessageBox(hParent, errMsg.c_str(), L"Playtest Error", MB_ICONERROR);
        std::filesystem::remove_all(tempDir, ec);
        return false;
    }

    // 5. Construct execution command line targeting the isolated temp directory
    std::string cmd = std::format("\"{}\" -m {} -D \"{}\"", exeFullPath.string(), playtestMissionId, tempDir.string());

    // Explicitly forward the editor's active data directory so playtesting always matches editor assets (e.g. American Revolt)
    std::string activeDataDir = rs_utl::File::getOriginalDataFolder().string();
    if (!activeDataDir.empty() && std::filesystem::exists(activeDataDir)) {
        cmd += std::format(" -d \"{}\"", activeDataDir);
    }

    if (std::filesystem::exists(resolvedLoadout)) {
        cmd += std::format(" -L \"{}\"", resolvedLoadout);
    }

    // Apply pre-configured window size, zoom, and debug options from Playtest Configuration
    PlaytestLoadoutData playtestConfig = PlaytestLoadoutDialog::getDefaultLoadout();
    EditorConfig::loadLoadoutConfig(playtestConfig);

    if (playtestConfig.launch.fullscreen) {
        cmd += " -f";
    } else if (playtestConfig.launch.windowScale >= 1 && playtestConfig.launch.windowScale <= 4) {
        cmd += std::format(" -w {}", playtestConfig.launch.windowScale);
    }

    if (playtestConfig.launch.mapZoom > 0.0f && std::abs(playtestConfig.launch.mapZoom - 1.0f) > 0.01f) {
        cmd += std::format(" -z {:.1f}", playtestConfig.launch.mapZoom);
    }

    if (playtestConfig.launch.noSound) {
        cmd += " -n";
    }

    if (playtestConfig.launch.showCoordinates) {
        cmd += " --show-coords";
    }
    if (playtestConfig.launch.showAgentPaths) {
        cmd += " --show-paths";
    }
    if (playtestConfig.launch.showStats) {
        cmd += " --show-stats";
    }

    // Logging flags
    std::string effectiveLogFlags;
    if (playtestConfig.launch.logOption == "ALL") {
        effectiveLogFlags = "ALL";
    } else if (playtestConfig.launch.logOption == "GAME") {
        effectiveLogFlags = "GAME";
    } else if (playtestConfig.launch.logOption == "KERNEL") {
        effectiveLogFlags = "KERNEL";
    } else if (playtestConfig.launch.logOption == "Custom") {
        effectiveLogFlags = playtestConfig.launch.customLogFlags;
    }
    if (!effectiveLogFlags.empty()) {
        cmd += std::format(" -l {}", effectiveLogFlags);
    }

    LOG(Log::k_FLG_INFO, "PlaytestManager", "launchPlaytest", ("Executing: %s", cmd.c_str()));

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};
    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    std::string workDir = exeFullPath.parent_path().string();
    if (CreateProcessA(nullptr, cmdBuffer.data(), nullptr, nullptr, FALSE, 0, nullptr, workDir.c_str(), &si, &pi)) {
        s_isPlaytestingActive.store(true);
        HANDLE hProcess = pi.hProcess;
        HANDLE hThread = pi.hThread;
        std::thread([hParent, hProcess, hThread, tempDir]() {
            WaitForSingleObject(hProcess, INFINITE);
            DWORD exitCode = 0;
            GetExitCodeProcess(hProcess, &exitCode);
            CloseHandle(hProcess);
            CloseHandle(hThread);
            std::error_code ec;
            std::filesystem::remove_all(tempDir, ec);
            s_isPlaytestingActive.store(false);
            if (IsWindow(hParent)) {
                PostMessageW(hParent, WM_PLAYTEST_FINISHED, static_cast<WPARAM>(exitCode), 0);
            }
        }).detach();
        return true;
    } else {
        DWORD errCode = GetLastError();
        RSERR(Log::k_FLG_IO, "PlaytestManager", "launchPlaytest",
            ("Failed to create process: %s (error %lu)", cmd.c_str(), errCode));
        std::wstring errMsg = std::format(
            L"Failed to launch resynd_game.exe (Error {}):\n\n"
            L"Executable: {}\n"
            L"Working Directory: {}\n"
            L"Command: {}",
            errCode, exeFullPath.wstring(),
            exeFullPath.parent_path().wstring(),
            std::wstring(cmd.begin(), cmd.end())
        );
        showEditorMessageBox(hParent, errMsg.c_str(), L"Playtest Launch Failed", MB_ICONERROR);
        std::filesystem::remove_all(tempDir, ec);
        return false;
    }
}

} // namespace rs_win_editor
