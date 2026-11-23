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
#include "editor_config.h"
#include "playtest_loadout_dialog.h"
#include "sprite_rasterizer.h"
#include "new_mission_dialog.h"
#include "save_mission_as_dialog.h"
#include "mission_properties_dialog.h"
#include "rs-kernel/mgr/mission_triad.h"
#include "rs-utils/io/configfile.h"
#include "rs-utils/io/resyndconfig.h"
#include "rs-utils/io/file.h"
#include "rs-engine/ui/settings_dialog.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/appcontext.h"
#include "rs-kernel/mgr/missionmanager.h"
#include "rs-kernel/mgr/pedmanager.h"
#include "rs-kernel/mgr/weaponmanager.h"
#include "rs-kernel/model/weapon.h"

#include <sstream>
#include <filesystem>

using namespace rs_win_editor;

TEST_CASE("EditorConfig Loadout Serialization", "[editor][config]") {
    SECTION("Round-trip serialization of agents and loadout") {
        PlaytestLoadoutData original;
        original.activeSquadSize = 3;
        for (int i = 0; i < 4; ++i) {
            original.agents[i].active = (i < 3);
        }
        original.agents[0].modLegs = 3;
        original.agents[0].modChest = 2;
        original.agents[0].modArms = 1;
        original.agents[0].modHeart = 0;
        original.agents[0].modEyes = 2;
        original.agents[0].modBrain = 3;
        original.agents[0].weapons = { "minigun", "flamer", "shield" };

        original.agents[1].modLegs = 1;
        original.agents[1].weapons = { "pistol", "persuadertron" };

        auto entries = EditorConfig::loadoutDataToEntries(original);
        REQUIRE_FALSE(entries.empty());

        PlaytestLoadoutData restored;
        EditorConfig::entriesToLoadoutData(entries, restored);

        REQUIRE(restored.activeSquadSize == 3);
        REQUIRE(restored.agents[0].active == true);
        REQUIRE(restored.agents[1].active == true);
        REQUIRE(restored.agents[2].active == true);
        REQUIRE(restored.agents[3].active == false);

        REQUIRE(restored.agents[0].modLegs == 3);
        REQUIRE(restored.agents[0].modChest == 2);
        REQUIRE(restored.agents[0].modArms == 1);
        REQUIRE(restored.agents[0].modHeart == 0);
        REQUIRE(restored.agents[0].modEyes == 2);
        REQUIRE(restored.agents[0].modBrain == 3);
        REQUIRE(restored.agents[0].weapons.size() == 3);
        REQUIRE(restored.agents[0].weapons[0] == "minigun");
        REQUIRE(restored.agents[0].weapons[1] == "flamer");
        REQUIRE(restored.agents[0].weapons[2] == "shield");

        REQUIRE(restored.agents[1].modLegs == 1);
        REQUIRE(restored.agents[1].weapons.size() == 2);
        REQUIRE(restored.agents[1].weapons[0] == "pistol");
        REQUIRE(restored.agents[1].weapons[1] == "persuadertron");
    }

    SECTION("Out-of-range mod clamping and case insensitivity") {
        std::vector<std::pair<std::string, std::string>> rawEntries = {
            { "squad_size", "2" },
            { "agent.0.active", "TRUE" },
            { "agent.0.mods.legs", "V3" },
            { "agent.0.mods.arms", "v2" },
            { "agent.0.weapons", "MINIGUN, Shotgun" }
        };

        PlaytestLoadoutData data;
        EditorConfig::entriesToLoadoutData(rawEntries, data);

        REQUIRE(data.activeSquadSize == 2);
        REQUIRE(data.agents[0].active == true);
        REQUIRE(data.agents[0].modLegs == 3);
        REQUIRE(data.agents[0].modArms == 2);
        REQUIRE(data.agents[0].weapons.size() == 2);
        REQUIRE(data.agents[0].weapons[0] == "minigun");
        REQUIRE(data.agents[0].weapons[1] == "shotgun");
    }

    SECTION("Round-trip serialization of faction settings (funds, company name, color, logo)") {
        PlaytestLoadoutData original;
        original.faction.funds = 250000;
        original.faction.companyName = "CYBERDYNE";
        original.faction.color = 4; // Yellow
        original.faction.logo = 17;

        auto entries = EditorConfig::loadoutDataToEntries(original);
        REQUIRE_FALSE(entries.empty());

        PlaytestLoadoutData restored;
        EditorConfig::entriesToLoadoutData(entries, restored);

        REQUIRE(restored.faction.funds == 250000);
        REQUIRE(restored.faction.companyName == "CYBERDYNE");
        REQUIRE(restored.faction.color == 4);
        REQUIRE(restored.faction.logo == 17);
    }

    SECTION("EditorWindowConfig default and tileGridHeight persistence") {
        EditorWindowConfig cfg;
        REQUIRE(cfg.tileGridHeight == 224);
        REQUIRE(cfg.entityGridHeight == 224);
        cfg.tileGridHeight = 350;
        cfg.entityGridHeight = 280;
        REQUIRE(cfg.tileGridHeight == 350);
        REQUIRE(cfg.entityGridHeight == 280);
    }
}

TEST_CASE("ConfigFile Multisection INI Parsing", "[utils][config]") {
    SECTION("Consecutive sections do not swallow section headers as multiline continuation") {
        std::string iniContent =
            "[Window]\n"
            "width = 1600\n"
            "height = 960\n"
            "[Paths]\n"
            "data_dir = C:/Games/Syndicate/Data\n"
            "[Loadout]\n"
            "squad_size = 4\n";

        std::istringstream iss(iniContent);
        ConfigFile cf;
        iss >> cf;

        REQUIRE(cf.keyExists("width"));
        REQUIRE(cf.read<int>("width") == 1600);
        REQUIRE(cf.keyExists("height"));
        REQUIRE(cf.read<int>("height") == 960);
        REQUIRE(cf.keyExists("data_dir"));
        REQUIRE(cf.read<std::string>("data_dir") == "C:/Games/Syndicate/Data");
        REQUIRE(cf.keyExists("squad_size"));
        REQUIRE(cf.read<int>("squad_size") == 4);
    }
}

TEST_CASE("NewMission and SaveMissionAs Dialog Data", "[editor][dialogs]") {
    SECTION("NewMissionDialogData initialization and defaults") {
        NewMissionDialogData data;
        REQUIRE(data.missionId == 99);
        REQUIRE(data.isBlankMap == true);
        REQUIRE(data.baseMapId == 1);
        REQUIRE(data.spawnTx == 64);
        REQUIRE(data.spawnTy == 48);
        REQUIRE(data.spawnTz == 2);
        REQUIRE(data.briefingTitle == "OPERATION NEW DAWN");
    }

    SECTION("MissionTriad path resolution for custom mission") {
        auto paths = rs_knl::MissionTriad::resolveFromId(99, "custom_missions");
        REQUIRE(paths.missionId == 99);
        REQUIRE(paths.gameFile.filename().string() == "game99.dat");
        REQUIRE(paths.mapFile.filename().string() == "map99.dat");
        REQUIRE(paths.missFile.filename().string() == "miss99.dat");
    }

    SECTION("SaveMissionAsDialogData structure resolution") {
        SaveMissionAsDialogData data;
        data.targetMissionId = 42;
        data.targetMapId = 42;
        data.targetDir = "C:/Syndicate/data";
        data.resolvedPaths.gameFile = data.targetDir / "game42.dat";
        data.resolvedPaths.mapFile = data.targetDir / "map42.dat";
        data.resolvedPaths.missFile = data.targetDir / "miss42.dat";

        REQUIRE(data.targetMissionId == 42);
        REQUIRE(data.targetMapId == 42);
        REQUIRE(data.resolvedPaths.gameFile.filename().string() == "game42.dat");
        REQUIRE(data.resolvedPaths.mapFile.filename().string() == "map42.dat");
        REQUIRE(data.resolvedPaths.missFile.filename().string() == "miss42.dat");
    }
}

TEST_CASE("MissionProperties Multi-Language Briefing Tracking", "[editor][briefing]") {
    SECTION("Briefing filename generation across languages") {
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(4, 0) == "miss04.dat");
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(4, 1) == "miss104.dat");
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(4, 2) == "miss204.dat");
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(4, 3) == "miss304.dat");

        REQUIRE(MissionPropertiesDialog::getBriefingFilename(99, 0) == "miss99.dat");
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(99, 1) == "miss199.dat");
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(99, 2) == "miss299.dat");
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(99, 3) == "miss399.dat");
    }

    SECTION("MissionPropertiesData in-memory stashing and slot switching") {
        MissionPropertiesData data;
        data.missionId = 4;
        data.baseBriefing = "English Briefing: Infiltrate base.";
        data.info1Briefing = "Level 1 Intel: Guards at gate.";
        data.syncActiveToLang(0);
        data.langs[0].modified = true;

        REQUIRE(data.langs[0].available == true);
        REQUIRE(data.langs[0].modified == true);
        REQUIRE(data.langs[0].baseBriefing == "English Briefing: Infiltrate base.");
        REQUIRE(data.langs[0].info1Briefing == "Level 1 Intel: Guards at gate.");
        REQUIRE(data.langs[1].available == false);

        // Switch to French (slot 1), initialize with French text
        data.langs[1].available = true;
        data.langs[1].baseBriefing = "Briefing Français: Infiltrer la base.";
        data.langs[1].info1Briefing = "Renseignement Niveau 1: Gardes à la porte.";
        data.syncLangToActive(1);

        REQUIRE(data.language == 1);
        REQUIRE(data.baseBriefing == "Briefing Français: Infiltrer la base.");
        REQUIRE(data.info1Briefing == "Renseignement Niveau 1: Gardes à la porte.");

        // Switch back to English (slot 0)
        data.syncLangToActive(0);
        REQUIRE(data.language == 0);
        REQUIRE(data.baseBriefing == "English Briefing: Infiltrate base.");
        REQUIRE(data.info1Briefing == "Level 1 Intel: Guards at gate.");

        // Modify English, stash it, switch back to French
        data.baseBriefing = "English Briefing: Infiltrate base via north gate.";
        data.syncActiveToLang(0);
        data.syncLangToActive(1);
        REQUIRE(data.baseBriefing == "Briefing Français: Infiltrer la base.");

        data.syncLangToActive(0);
        REQUIRE(data.baseBriefing == "English Briefing: Infiltrate base via north gate.");
    }

    SECTION("SaveMissionAsDialogData multi-language flags") {
        SaveMissionAsDialogData data;
        REQUIRE(data.availableLanguages[0] == true);
        REQUIRE(data.availableLanguages[1] == false);
        REQUIRE(data.availableLanguages[2] == false);
        REQUIRE(data.availableLanguages[3] == false);

        data.availableLanguages[1] = true;
        data.modifiedLanguages[1] = true;
        REQUIRE(data.availableLanguages[1] == true);
        REQUIRE(data.modifiedLanguages[1] == true);
    }

    SECTION("Session edited persistence and Save As display status") {
        MissionPropertiesData props;
        props.langs[0].available = true;
        props.langs[0].modified = false;
        props.langs[0].sessionEdited = true; // Was edited in session, saved to local disk

        props.langs[1].available = true;
        props.langs[1].modified = false;
        props.langs[1].sessionEdited = false; // Loaded from disk, unedited

        bool isEdited0 = props.langs[0].modified || props.langs[0].sessionEdited;
        bool isEdited1 = props.langs[1].modified || props.langs[1].sessionEdited;

        REQUIRE(isEdited0 == true);
        REQUIRE(isEdited1 == false);
    }
}

TEST_CASE("Original Data Structural Verification and Signature", "[data][verification]") {
    SECTION("computeDataSignature on valid and invalid directories") {
        std::filesystem::path syndData = "synd/data";
        if (std::filesystem::exists(syndData)) {
            std::filesystem::path bakPath;
            rs_utl::File::getResyndConfFullPath(bakPath);
            std::string backupContent;
            bool hadExisting = std::filesystem::exists(bakPath);
            if (hadExisting) {
                std::ifstream bakIn(bakPath, std::ios::binary);
                backupContent.assign((std::istreambuf_iterator<char>(bakIn)), std::istreambuf_iterator<char>());
            }
            std::string sig = rs_utl::File::computeDataSignature(syndData);
            REQUIRE_FALSE(sig.empty());
            REQUIRE(sig.find(':') != std::string::npos);

            // Calling it again produces the exact same signature
            std::string sig2 = rs_utl::File::computeDataSignature(syndData);
            REQUIRE(sig == sig2);
        }

        // Non-existent directory returns empty
        std::string emptySig = rs_utl::File::computeDataSignature("non_existent_folder_xyz_123");
        REQUIRE(emptySig.empty());
    }

    SECTION("verifyOriginalData on valid synd/data") {
        std::filesystem::path syndData = "synd/data";
        if (std::filesystem::exists(syndData)) {
            std::string errorMsg;
            bool ok = rs_utl::File::verifyOriginalData(syndData, &errorMsg);
            REQUIRE(ok == true);
            REQUIRE(errorMsg.empty());

            // verifyDataDirectories also succeeds
            auto val = rs_eng::verifyDataDirectories(syndData);
            REQUIRE(val.isValid == true);
            REQUIRE(val.missingCount == 0);
            REQUIRE(val.missionCount == 60);
            REQUIRE(val.originalChecks.size() == 7);
            REQUIRE(val.originalChecks[6].isPresent == true);
            REQUIRE(val.hasAmericanRevolt == false);
            REQUIRE(val.summaryMessage.find(L"Original Syndicate (Base Game)") != std::wstring::npos);
            REQUIRE(rs_utl::File::hasAmericanRevoltData(syndData) == false);
        }
    }

    SECTION("verifyOriginalData on American Revolt / Data Disk directory if present") {
        std::filesystem::path datadiskData = "synd/DATADISK/DATA";
        if (!std::filesystem::exists(datadiskData)) {
            datadiskData = "C:/Games/Syndicate Plus/DATADISK/DATA";
        }
        if (std::filesystem::exists(datadiskData)) {
            std::string errorMsg;
            bool ok = rs_utl::File::verifyOriginalData(datadiskData, &errorMsg);
            REQUIRE(ok == true);
            REQUIRE(errorMsg.empty());

            auto val = rs_eng::verifyDataDirectories(datadiskData);
            REQUIRE(val.isValid == true);
            REQUIRE(val.missingCount == 0);
            REQUIRE(val.originalChecks[2].isPresent == true); // Gameplay Sprites
            REQUIRE(val.originalChecks[3].isPresent == true); // Menu Sprites
            REQUIRE(val.hasAmericanRevolt == true);
            REQUIRE(val.summaryMessage.find(L"Syndicate: American Revolt") != std::wstring::npos);
            REQUIRE(rs_utl::File::hasAmericanRevoltData(datadiskData) == true);
        }

        std::filesystem::path syndicatData = "synd/SYNDICAT/DATA";
        if (std::filesystem::exists(syndicatData)) {
            REQUIRE(rs_utl::File::hasAmericanRevoltData(syndicatData) == false);
        }
    }

    SECTION("missionRequiresAmericanRevolt detection and AR weapon checks") {
        LevelData::LevelDataAll lvl{};
        REQUIRE(rs_knl::MissionManager::missionRequiresAmericanRevolt(lvl) == false);

        // Standard weapon types (e.g. Pistol = 0x01, Minigun = 0x03)
        lvl.weapons[0].desc = 0x05;
        lvl.weapons[0].sub_type = 0x01;
        lvl.weapons[1].desc = 0x05;
        lvl.weapons[1].sub_type = 0x03;
        REQUIRE(rs_knl::MissionManager::missionRequiresAmericanRevolt(lvl) == false);

        // Air Strike (0x0E)
        lvl.weapons[2].desc = 0x05;
        lvl.weapons[2].sub_type = 0x0E;
        REQUIRE(rs_knl::MissionManager::missionRequiresAmericanRevolt(lvl) == true);

        // Clone Shield (0x0F)
        lvl.weapons[2].sub_type = 0x00;
        lvl.weapons[5].desc = 0x05;
        lvl.weapons[5].sub_type = 0x0F;
        REQUIRE(rs_knl::MissionManager::missionRequiresAmericanRevolt(lvl) == true);

        // Check weapon model helper
        REQUIRE(rs_knl::Weapon::isAmericanRevoltOnly(rs_knl::Weapon::AirStrike) == true);
        REQUIRE(rs_knl::Weapon::isAmericanRevoltOnly(rs_knl::Weapon::CloneShield) == true);
        REQUIRE(rs_knl::Weapon::isAmericanRevoltOnly(rs_knl::Weapon::Pistol) == false);
        REQUIRE(rs_knl::Weapon::isAmericanRevoltOnly(rs_knl::Weapon::Minigun) == false);
        REQUIRE(rs_knl::Weapon::isAmericanRevoltOnly(rs_knl::Weapon::EnergyShield) == false);
    }

    SECTION("verifyOriginalData on invalid directory fails gracefully") {
        std::filesystem::path tempEmpty = std::filesystem::temp_directory_path() / "resynd_empty_test_dir";
        std::filesystem::create_directories(tempEmpty);

        std::string errorMsg;
        bool ok = rs_utl::File::verifyOriginalData(tempEmpty, &errorMsg);
        REQUIRE(ok == false);
        REQUIRE_FALSE(errorMsg.empty());

        auto val = rs_eng::verifyDataDirectories(tempEmpty);
        REQUIRE(val.isValid == false);
        REQUIRE(val.missingCount > 0);

        std::filesystem::remove(tempEmpty);
    }

    SECTION("EditorPathsConfig dataSignature round-trip") {
        EditorPathsConfig cfg;
        cfg.dataDir = "test_dir";
        cfg.dataSignature = "123:45678:99999";
        REQUIRE(cfg.dataDir == "test_dir");
        REQUIRE(cfg.dataSignature == "123:45678:99999");
    }

    SECTION("saveOriginalDataDirToResyndConf and signature persistence") {
        std::filesystem::path syndData = "synd/data";
        if (std::filesystem::exists(syndData)) {
            std::filesystem::path bakPath;
            rs_utl::File::getResyndConfFullPath(bakPath);
            std::string backupContent;
            bool hadExisting = std::filesystem::exists(bakPath);
            if (hadExisting) {
                std::ifstream bakIn(bakPath, std::ios::binary);
                backupContent.assign((std::istreambuf_iterator<char>(bakIn)), std::istreambuf_iterator<char>());
            }
            std::string sig = rs_utl::File::computeDataSignature(syndData);
            bool saved = rs_eng::saveOriginalDataDirToResyndConf(syndData.string(), sig);
            REQUIRE(saved == true);

            // Read resynd.conf back to verify signature persisted under [Paths] while preserving [Game]
            std::filesystem::path resyndConfPath;
            REQUIRE(rs_utl::File::getResyndConfFullPath(resyndConfPath) == true);
            rs_utl::ResyndConfig cfg;
            REQUIRE(cfg.load(resyndConfPath) == true);
            REQUIRE(cfg.getString("Paths", "data_signature") == sig);
            REQUIRE(cfg.getString("Paths", "data_dir") == syndData.string());

            // Verify modifying [Game] preserves [Paths] and vice-versa
            cfg.setInt("Game", "sound_volume", 77);
            REQUIRE(cfg.save(resyndConfPath) == true);
            REQUIRE(rs_eng::saveOriginalDataDirToResyndConf(syndData.string(), sig) == true);

            rs_utl::ResyndConfig cfgAfter;
            REQUIRE(cfgAfter.load(resyndConfPath) == true);
            REQUIRE(cfgAfter.getInt("Game", "sound_volume") == 77);
            REQUIRE(cfgAfter.getString("Paths", "data_signature") == sig);

            if (hadExisting) {
                std::ofstream bakOut(bakPath, std::ios::binary | std::ios::trunc);
                bakOut.write(backupContent.data(), static_cast<std::streamsize>(backupContent.size()));
            }
        }
    }

    SECTION("WeaponManager supports Air Strike and Clone Shield") {
        rs_eng::AppContext appCtx;
        rs_knl::WeaponManager weaponManager;
        weaponManager.reset();
        auto *pAir = weaponManager.getWeapon(rs_knl::Weapon::AirStrike);
        REQUIRE(pAir != nullptr);
        REQUIRE(pAir->getType() == rs_knl::Weapon::AirStrike);
        REQUIRE(pAir->cost() == 5000);
        REQUIRE(pAir->selector() == 1615);
        REQUIRE(pAir->getSmallIconId() == 27);
        REQUIRE(pAir->getBigIconId() == 1180);

        auto *pShield = weaponManager.getWeapon(rs_knl::Weapon::CloneShield);
        REQUIRE(pShield != nullptr);
        REQUIRE(pShield->getType() == rs_knl::Weapon::CloneShield);
        REQUIRE(pShield->cost() == 9000);
        REQUIRE(pShield->selector() == 1617);
        REQUIRE(pShield->getSmallIconId() == 29);
        REQUIRE(pShield->getBigIconId() == 83);

        // WeaponInstance creation
        auto *pInstAir = rs_knl::WeaponInstance::createInstance(pAir);
        REQUIRE(pInstAir != nullptr);
        delete pInstAir;

        auto *pInstShield = rs_knl::WeaponInstance::createInstance(pShield);
        REQUIRE(pInstShield != nullptr);
        delete pInstShield;
    }

    SECTION("isAmericanRevoltWeapon helper detects AR weapons accurately") {
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("airstrike") == true);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("air_strike") == true);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("Air Strike") == true);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("cloneshield") == true);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("clone_shield") == true);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("Clone Shield") == true);

        REQUIRE(EditorConfig::isAmericanRevoltWeapon("pistol") == false);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("minigun") == false);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("laser") == false);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("shield") == false);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("medikit") == false);
        REQUIRE(EditorConfig::isAmericanRevoltWeapon("persuadatron") == false);
    }

    SECTION("sanitizeLoadout strips AR weapons when data path is not AR") {
        std::filesystem::path emptyDir = std::filesystem::temp_directory_path() / "resynd_non_ar_test";
        std::filesystem::create_directories(emptyDir);

        PlaytestLoadoutData loadout;
        loadout.agents[0].weapons = { "minigun", "airstrike", "cloneshield", "pistol" };
        loadout.agents[1].weapons = { "cloneshield" };
        loadout.agents[2].weapons = { "laser", "shield" };
        loadout.agents[3].weapons = { "airstrike" };

        // Empty / non-AR directory must strip AR weapons
        bool changed = EditorConfig::sanitizeLoadout(loadout, emptyDir);
        REQUIRE(changed == true);
        REQUIRE(loadout.agents[0].weapons == std::vector<std::string>{ "minigun", "pistol" });
        REQUIRE(loadout.agents[1].weapons.empty());
        REQUIRE(loadout.agents[2].weapons == std::vector<std::string>{ "laser", "shield" });
        REQUIRE(loadout.agents[3].weapons.empty());

        // Subsequent call on already sanitized loadout returns false
        bool changedAgain = EditorConfig::sanitizeLoadout(loadout, emptyDir);
        REQUIRE(changedAgain == false);

        std::filesystem::remove(emptyDir);
    }

    SECTION("sanitizeLoadout preserves AR weapons when data path has AR assets") {
        std::filesystem::path datadiskData = "C:/Games/Syndicate Plus/DATADISK/DATA";
        if (std::filesystem::exists(datadiskData) && rs_utl::File::hasAmericanRevoltData(datadiskData)) {
            PlaytestLoadoutData loadout;
            loadout.agents[0].weapons = { "minigun", "airstrike", "cloneshield", "pistol" };
            loadout.agents[1].weapons = { "cloneshield" };

            bool changed = EditorConfig::sanitizeLoadout(loadout, datadiskData);
            REQUIRE(changed == false);
            REQUIRE(loadout.agents[0].weapons.size() == 4);
            REQUIRE(loadout.agents[1].weapons.size() == 1);
        }
    }

    SECTION("Round-trip serialization preserves Air Strike and Clone Shield") {
        PlaytestLoadoutData loadout;
        loadout.agents[0].weapons = { "airstrike", "cloneshield" };

        auto entries = EditorConfig::loadoutDataToEntries(loadout);
        PlaytestLoadoutData deserialized;
        EditorConfig::entriesToLoadoutData(entries, deserialized);

        REQUIRE(deserialized.agents[0].weapons.size() == 2);
        REQUIRE(deserialized.agents[0].weapons[0] == "airstrike");
        REQUIRE(deserialized.agents[0].weapons[1] == "cloneshield");
    }
}

TEST_CASE("AnimationManager and SpriteRasterizer Player Faction Color", "[engine][editor][color]") {
    SECTION("SpriteRasterizer player faction color default and mutation") {
        SpriteRasterizer::setPlayerFactionColor(6);
        REQUIRE(SpriteRasterizer::playerFactionColor() == 6);

        SpriteRasterizer::setPlayerFactionColor(11); // Yellow
        REQUIRE(SpriteRasterizer::playerFactionColor() == 11);

        SpriteRasterizer::setPlayerFactionColor(6); // restore default
        REQUIRE(SpriteRasterizer::playerFactionColor() == 6);
    }

    SECTION("AnimationManager player faction color default and mutation") {
        rs_eng::AnimationManager animMgr;
        REQUIRE(animMgr.playerFactionColor() == 6);

        animMgr.setPlayerFactionColor(14); // Orange
        REQUIRE(animMgr.playerFactionColor() == 14);
        REQUIRE(g_AnimMgr.playerFactionColor() == 14);

        animMgr.setPlayerFactionColor(6); // restore default
        REQUIRE(animMgr.playerFactionColor() == 6);
    }
}

TEST_CASE("MissionPropertiesData Structure and Source Directory Tracking", "[editor][properties]") {
    SECTION("Default values and source tracking") {
        MissionPropertiesData props;
        REQUIRE(props.missionId == 1);
        REQUIRE(props.techLevel == 0);
        REQUIRE(props.initialTechLevel == 0);
        REQUIRE(props.sourceDir.empty());
        REQUIRE(props.gameFilePath.empty());

        props.sourceDir = "synd/SYNDICAT/DATA/custom";
        props.gameFilePath = "synd/SYNDICAT/DATA/custom/game99.dat";
        props.techLevel = 3;
        props.initialTechLevel = 3;

        REQUIRE(props.sourceDir == "synd/SYNDICAT/DATA/custom");
        REQUIRE(props.gameFilePath == "synd/SYNDICAT/DATA/custom/game99.dat");
        REQUIRE(props.techLevel == 3);
        REQUIRE(props.initialTechLevel == 3);

        REQUIRE(MissionPropertiesDialog::getBriefingFilename(99, 0) == "miss99.dat");
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(99, 1) == "miss199.dat");
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(99, 2) == "miss299.dat");
        REQUIRE(MissionPropertiesDialog::getBriefingFilename(99, 3) == "miss399.dat");
    }
}

TEST_CASE("Entity Default Health Alignment With Campaign Missions", "[kernel][editor][health]") {
    SECTION("PedManager default health and speed stats match original DOS campaign values") {
        const auto &agent = rs_knl::PedManager::getStats(rs_knl::PedInstance::kPedTypeAgent, false);
        const auto &enemyAgent = rs_knl::PedManager::getStats(rs_knl::PedInstance::kPedTypeAgent, true);
        const auto &guard = rs_knl::PedManager::getStats(rs_knl::PedInstance::kPedTypeGuard);
        const auto &police = rs_knl::PedManager::getStats(rs_knl::PedInstance::kPedTypePolice);
        const auto &civilian = rs_knl::PedManager::getStats(rs_knl::PedInstance::kPedTypeCivilian);
        const auto &criminal = rs_knl::PedManager::getStats(rs_knl::PedInstance::kPedTypeCriminal);

        REQUIRE(agent.health == 16);
        REQUIRE(enemyAgent.health == 16);
        REQUIRE(guard.health == 1);
        REQUIRE(police.health == 1);
        REQUIRE(civilian.health == 1);
        REQUIRE(criminal.health == 1);

        REQUIRE(guard.speed == 128);
        REQUIRE(police.speed == 128);
        REQUIRE(guard.speed < agent.speed);
        REQUIRE(police.speed < agent.speed);
    }
}






