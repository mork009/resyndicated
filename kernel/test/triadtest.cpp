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

#include <catch2/catch_test_macros.hpp>
#include "rs-kernel/mgr/mission_triad.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-engine/gfx/tilemanager.h"
#include "rs-engine/appcontext.h"
#include "rs-utils/io/file.h"
#include "testcase.h"

using namespace rs_knl;

TEST_CASE("MissionTriad Resolution", "[editor][triad]") {
    SECTION("Resolve from gameXX.dat") {
        MissionTriadPaths paths;
        REQUIRE(MissionTriad::resolveFromAnyFile("C:/Missions/game100.dat", paths));
        REQUIRE(paths.missionId == 100);
        REQUIRE(paths.gameFile.filename().string() == "game100.dat");
        REQUIRE(paths.mapFile.filename().string() == "map100.dat");
        REQUIRE(paths.missFile.filename().string() == "miss100.dat");
    }

    SECTION("Resolve from mapXX.dat") {
        MissionTriadPaths paths;
        REQUIRE(MissionTriad::resolveFromAnyFile("C:/Missions/map100.dat", paths));
        REQUIRE(paths.missionId == 100);
        REQUIRE(paths.gameFile.filename().string() == "game100.dat");
        REQUIRE(paths.mapFile.filename().string() == "map100.dat");
        REQUIRE(paths.missFile.filename().string() == "miss100.dat");
    }

    SECTION("Resolve from missXX.dat") {
        MissionTriadPaths paths;
        REQUIRE(MissionTriad::resolveFromAnyFile("C:/Missions/miss100.dat", paths));
        REQUIRE(paths.missionId == 100);
        REQUIRE(paths.gameFile.filename().string() == "game100.dat");
        REQUIRE(paths.mapFile.filename().string() == "map100.dat");
        REQUIRE(paths.missFile.filename().string() == "miss100.dat");
    }

    SECTION("Resolve two-digit IDs") {
        MissionTriadPaths paths;
        REQUIRE(MissionTriad::resolveFromAnyFile("game07.dat", paths));
        REQUIRE(paths.missionId == 7);
    }

    SECTION("Invalid filename rejection") {
        MissionTriadPaths paths;
        REQUIRE_FALSE(MissionTriad::resolveFromAnyFile("", paths));
    }
}

TEST_CASE("MissionTriad Validation", "[editor][triad]") {
    initTestDataFolders();
    SECTION("Vanilla Mission 1 Triad is Valid") {
        MissionTriadPaths paths = MissionTriad::resolveFromId(1);
        TriadValidationResult res = MissionTriad::validate(paths);
        REQUIRE(res.isValid);
        REQUIRE(res.gameOk);
        REQUIRE(res.mapOk);
        REQUIRE(res.missOk);
    }

    SECTION("Missing Triad Companion Detection") {
        MissionTriadPaths paths;
        paths.missionId = 999;
        paths.mapId = 999;
        paths.gameFile = "nonexistent_game999.dat";
        paths.mapFile = "nonexistent_map999.dat";
        paths.missFile = "nonexistent_miss999.dat";

        TriadValidationResult res = MissionTriad::validate(paths);
        REQUIRE_FALSE(res.isValid);
        REQUIRE_FALSE(res.gameOk);
        REQUIRE_FALSE(res.mapOk);
        REQUIRE_FALSE(res.missOk);
        REQUIRE_FALSE(res.summaryMessage.empty());
        // Verify proper Unicode bullet character and absence of mojibake (â€¢)
        REQUIRE(res.summaryMessage.find(L"\u2022") != std::wstring::npos);
        REQUIRE(res.summaryMessage.find(L"\u00E2\u20AC\u00A2") == std::wstring::npos);
    }
}

TEST_CASE("MissionTriad Overwrite Detection", "[editor][triad]") {
    initTestDataFolders();
    SECTION("No files exist") {
        MissionTriadPaths paths;
        paths.gameFile = "nonexistent_dir/game9999.dat";
        paths.mapFile = "nonexistent_dir/map9999.dat";
        paths.missFile = "nonexistent_dir/miss9999.dat";

        TriadOverwriteInfo info = MissionTriad::checkOverwrite(paths);
        REQUIRE_FALSE(info.anyExists);
        REQUIRE_FALSE(info.gameExists);
        REQUIRE_FALSE(info.mapExists);
        REQUIRE_FALSE(info.missExists);
        REQUIRE(info.warningMessage.empty());
    }

    SECTION("All files exist (Vanilla Mission 1)") {
        MissionTriadPaths paths = MissionTriad::resolveFromId(1);
        TriadOverwriteInfo info = MissionTriad::checkOverwrite(paths);
        REQUIRE(info.anyExists);
        REQUIRE(info.gameExists);
        REQUIRE(info.mapExists);
        REQUIRE(info.missExists);
        REQUIRE(info.warningMessage.find(L"game01.dat") != std::wstring::npos);
        REQUIRE(info.warningMessage.find(L"map01.dat") != std::wstring::npos);
        REQUIRE(info.warningMessage.find(L"miss01.dat") != std::wstring::npos);
        REQUIRE(info.warningMessage.find(L"\u2022") != std::wstring::npos);
        REQUIRE(info.warningMessage.find(L"\u00E2\u20AC\u00A2") == std::wstring::npos);
    }

    SECTION("Single companion file exists (e.g. miss101.dat scenario)") {
        MissionTriadPaths paths = MissionTriad::resolveFromId(101);

        // In data/, miss101.dat exists from the user's test, while game101 and map101 do not
        TriadOverwriteInfo info = MissionTriad::checkOverwrite(paths);
        REQUIRE(info.anyExists);
        REQUIRE(info.missExists);
        REQUIRE_FALSE(info.gameExists);
        REQUIRE_FALSE(info.mapExists);
        REQUIRE(info.warningMessage.find(L"miss101.dat") != std::wstring::npos);
        REQUIRE(info.warningMessage.find(L"\u2022") != std::wstring::npos);
        REQUIRE(info.warningMessage.find(L"\u00E2\u20AC\u00A2") == std::wstring::npos);
    }
}

TEST_CASE("Custom Mission Triad & RNC Briefing Load", "[editor][triad]") {
    initTestDataFolders();
    std::filesystem::path customGame22 = "synd/syndicat/data/custom/game22.dat";
    if (!std::filesystem::exists(customGame22)) {
        std::filesystem::path cur = std::filesystem::current_path();
        for (int i = 0; i < 6; ++i) {
            if (std::filesystem::exists(cur / "synd" / "syndicat" / "data" / "custom" / "game22.dat")) {
                customGame22 = cur / "synd" / "syndicat" / "data" / "custom" / "game22.dat";
                break;
            }
            if (!cur.has_parent_path() || cur == cur.parent_path()) break;
            cur = cur.parent_path();
        }
    }

    if (std::filesystem::exists(customGame22)) {
        MissionTriadPaths paths;
        REQUIRE(MissionTriad::resolveFromAnyFile(customGame22, paths));
        REQUIRE(paths.missionId == 22);

        TriadValidationResult val = MissionTriad::validate(paths);
        REQUIRE(val.isValid);
        REQUIRE(val.gameOk);
        REQUIRE(val.mapOk);
        REQUIRE(val.missOk);

        rs_eng::AppContext appCtx;
        MemoryTileManager tileMgr;
        MissionManager misMgr(&tileMgr);
        misMgr.mapManager().setCustomDataDir(customGame22.parent_path().string());

        MissionBriefing *pBriefDirect = misMgr.loadBriefingFromPath(paths.missFile.string());
        REQUIRE(pBriefDirect != nullptr);
        delete pBriefDirect;

        LevelData::LevelDataAll ld;
        REQUIRE(misMgr.load_level_data(paths.missionId, ld));
        Map *pMap = misMgr.mapManager().getMap(ld.mapinfos);
        REQUIRE(pMap != nullptr);

        MissionBriefing *pBrief = misMgr.loadBriefing(paths.missionId);
        REQUIRE(pBrief != nullptr);
        REQUIRE(pBrief->minimap() != nullptr);
        delete pBrief;

        misMgr.mapManager().setCustomDataDir("");
        misMgr.mapManager().clearCache();
    }
}
