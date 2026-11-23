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

#include "editor_config.h"
#include "playtest_loadout_dialog.h"
#include "rs-utils/io/file.h"
#include "rs-utils/io/resyndconfig.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>

namespace rs_win_editor {

std::filesystem::path EditorConfig::getConfigPath() {
    rs_utl::File::getOrCreateUserConfFolder("");
    std::filesystem::path confPath;
    rs_utl::File::getResyndConfFullPath(confPath);
    return confPath;
}

static const char *kConfModNames[] = { "none", "v1", "v2", "v3" };

std::vector<std::pair<std::string, std::string>> EditorConfig::loadoutDataToEntries(const PlaytestLoadoutData &data) {
    std::vector<std::pair<std::string, std::string>> entries;
    entries.push_back({ "squad_size", std::to_string(data.activeSquadSize) });
    entries.push_back({ "funds", std::to_string(data.faction.funds) });
    entries.push_back({ "company_name", data.faction.companyName });
    entries.push_back({ "color", std::to_string(data.faction.color) });
    entries.push_back({ "logo", std::to_string(data.faction.logo) });

    for (int i = 0; i < 4; ++i) {
        const auto &ag = data.agents[i];
        std::string prefix = "agent." + std::to_string(i) + ".";
        entries.push_back({ prefix + "active", ag.active ? "true" : "false" });
        entries.push_back({ prefix + "mods.legs", kConfModNames[std::clamp(ag.modLegs, 0, 3)] });
        entries.push_back({ prefix + "mods.arms", kConfModNames[std::clamp(ag.modArms, 0, 3)] });
        entries.push_back({ prefix + "mods.chest", kConfModNames[std::clamp(ag.modChest, 0, 3)] });
        entries.push_back({ prefix + "mods.heart", kConfModNames[std::clamp(ag.modHeart, 0, 3)] });
        entries.push_back({ prefix + "mods.eyes", kConfModNames[std::clamp(ag.modEyes, 0, 3)] });
        entries.push_back({ prefix + "mods.brain", kConfModNames[std::clamp(ag.modBrain, 0, 3)] });

        std::string wpnStr;
        for (size_t w = 0; w < ag.weapons.size(); ++w) {
            wpnStr += ag.weapons[w];
            if (w + 1 < ag.weapons.size()) wpnStr += ", ";
        }
        entries.push_back({ prefix + "weapons", wpnStr });
    }
    return entries;
}

void EditorConfig::entriesToLoadoutData(const std::vector<std::pair<std::string, std::string>> &entries, PlaytestLoadoutData &data) {
    auto toLowerStr = [](std::string s) -> std::string {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    };
    auto trimStr = [](const std::string &s) -> std::string {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    };
    auto parseMod = [&](const std::string &val) -> int {
        std::string s = toLowerStr(trimStr(val));
        if (s == "v1" || s == "1") return 1;
        if (s == "v2" || s == "2") return 2;
        if (s == "v3" || s == "3") return 3;
        return 0;
    };
    auto parseWeaponList = [&](const std::string &val) -> std::vector<std::string> {
        std::vector<std::string> res;
        std::stringstream ss(val);
        std::string item;
        while (std::getline(ss, item, ',')) {
            std::string trimmed = toLowerStr(trimStr(item));
            if (!trimmed.empty() && res.size() < 8) {
                res.push_back(trimmed);
            }
        }
        return res;
    };

    for (const auto &kv : entries) {
        std::string key = toLowerStr(trimStr(kv.first));
        std::string val = trimStr(kv.second);

        if (key == "squad_size" || key == "active_agents") {
            int sz = std::atoi(val.c_str());
            if (sz >= 1 && sz <= 4) {
                data.activeSquadSize = sz;
                for (int i = 0; i < 4; ++i) {
                    data.agents[i].active = (i < sz);
                }
            }
        } else if (key == "funds" || key == "money") {
            data.faction.funds = (std::max)(0, std::atoi(val.c_str()));
        } else if (key == "company_name" || key == "company") {
            if (!val.empty()) {
                data.faction.companyName = val.substr(0, 16);
            }
        } else if (key == "color" || key == "logo_colour" || key == "company_color") {
            data.faction.color = std::clamp(std::atoi(val.c_str()), 0, 7);
        } else if (key == "logo" || key == "logo_id") {
            data.faction.logo = std::clamp(std::atoi(val.c_str()), 0, 39);
        } else if (key.rfind("agent.", 0) == 0) {
            if (key.size() > 8 && std::isdigit(static_cast<unsigned char>(key[6]))) {
                int aIdx = key[6] - '0';
                if (aIdx >= 0 && aIdx < 4) {
                    std::string sub = key.substr(8);
                    if (sub == "active") {
                        std::string lowerVal = toLowerStr(val);
                        data.agents[aIdx].active = (lowerVal == "true" || lowerVal == "1" || lowerVal == "yes");
                    } else if (sub == "mods.legs") {
                        data.agents[aIdx].modLegs = parseMod(val);
                    } else if (sub == "mods.arms") {
                        data.agents[aIdx].modArms = parseMod(val);
                    } else if (sub == "mods.chest") {
                        data.agents[aIdx].modChest = parseMod(val);
                    } else if (sub == "mods.heart") {
                        data.agents[aIdx].modHeart = parseMod(val);
                    } else if (sub == "mods.eyes") {
                        data.agents[aIdx].modEyes = parseMod(val);
                    } else if (sub == "mods.brain") {
                        data.agents[aIdx].modBrain = parseMod(val);
                    } else if (sub == "weapons") {
                        data.agents[aIdx].weapons = parseWeaponList(val);
                    }
                }
            }
        } else if (key.rfind("mods.", 0) == 0) {
            std::string modName = key.substr(5);
            int mVal = parseMod(val);
            for (int i = 0; i < 4; ++i) {
                if (modName == "legs") data.agents[i].modLegs = mVal;
                else if (modName == "arms") data.agents[i].modArms = mVal;
                else if (modName == "chest") data.agents[i].modChest = mVal;
                else if (modName == "heart") data.agents[i].modHeart = mVal;
                else if (modName == "eyes") data.agents[i].modEyes = mVal;
                else if (modName == "brain") data.agents[i].modBrain = mVal;
            }
        } else if (key == "weapons") {
            std::vector<std::string> wpns = parseWeaponList(val);
            for (int i = 0; i < 4; ++i) {
                data.agents[i].weapons = wpns;
            }
        }
    }
}

bool EditorConfig::isAmericanRevoltWeapon(const std::string &weaponName) {
    std::string s = weaponName;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    std::string clean;
    for (char c : s) {
        if (c != ' ' && c != '_' && c != '-') clean += c;
    }
    return (clean == "airstrike" || clean == "cloneshield");
}

bool EditorConfig::sanitizeLoadout(PlaytestLoadoutData &data, const std::filesystem::path &originalDir) {
    std::filesystem::path targetDir = originalDir;
    if (targetDir.empty()) {
        targetDir = rs_utl::File::getOriginalDataFolder();
    }
    if (targetDir.empty()) {
        EditorPathsConfig pathsCfg;
        if (loadPathsConfig(pathsCfg) && !pathsCfg.dataDir.empty()) {
            targetDir = pathsCfg.dataDir;
        }
    }

    if (rs_utl::File::hasAmericanRevoltData(targetDir)) {
        return false;
    }

    bool modified = false;
    for (int i = 0; i < 4; ++i) {
        auto &wpns = data.agents[i].weapons;
        auto it = std::remove_if(wpns.begin(), wpns.end(), [](const std::string &w) {
            return isAmericanRevoltWeapon(w);
        });
        if (it != wpns.end()) {
            wpns.erase(it, wpns.end());
            modified = true;
        }
    }
    return modified;
}

bool EditorConfig::loadWindowConfig(EditorWindowConfig &config) {
    std::filesystem::path path = getConfigPath();
    rs_utl::ResyndConfig cfg;
    if (!cfg.load(path)) return false;

    const std::string secName = cfg.hasSection("EditorWindow") ? "EditorWindow" : "Window";
    auto entries = cfg.getSectionEntries(secName);
    bool foundAny = false;
    for (const auto &kv : entries) {
        try {
            if (kv.first == "x") { config.x = std::stoi(kv.second); foundAny = true; }
            else if (kv.first == "y") { config.y = std::stoi(kv.second); foundAny = true; }
            else if (kv.first == "width") { config.width = std::stoi(kv.second); foundAny = true; }
            else if (kv.first == "height") { config.height = std::stoi(kv.second); foundAny = true; }
            else if (kv.first == "maximized") {
                config.maximized = (kv.second == "1" || kv.second == "true" || kv.second == "yes");
            }
            else if (kv.first == "right_pane_width" || kv.first == "art_panel_width") {
                config.rightPaneWidth = std::stoi(kv.second);
                foundAny = true;
            }
            else if (kv.first == "left_pane_width") {
                config.leftPaneWidth = std::stoi(kv.second);
                foundAny = true;
            }
            else if (kv.first == "entity_grid_height" || kv.first == "entity_splitter_height") {
                config.entityGridHeight = std::stoi(kv.second);
                foundAny = true;
            }
            else if (kv.first == "tile_grid_height" || kv.first == "tile_splitter_height") {
                config.tileGridHeight = std::stoi(kv.second);
                foundAny = true;
            }
        } catch (...) {
            // Ignore parse errors on individual fields
        }
    }
    return foundAny;
}

bool EditorConfig::saveWindowConfig(const EditorWindowConfig &config) {
    std::filesystem::path path = getConfigPath();
    rs_utl::ResyndConfig cfg;
    cfg.load(path);

    cfg.setInt("EditorWindow", "x", config.x);
    cfg.setInt("EditorWindow", "y", config.y);
    cfg.setInt("EditorWindow", "width", config.width);
    cfg.setInt("EditorWindow", "height", config.height);
    cfg.setString("EditorWindow", "maximized", config.maximized ? "1" : "0");
    if (config.rightPaneWidth > 0) {
        cfg.setInt("EditorWindow", "right_pane_width", config.rightPaneWidth);
    }
    if (config.leftPaneWidth > 0) {
        cfg.setInt("EditorWindow", "left_pane_width", config.leftPaneWidth);
    }
    if (config.entityGridHeight > 0) {
        cfg.setInt("EditorWindow", "entity_grid_height", config.entityGridHeight);
    }
    if (config.tileGridHeight > 0) {
        cfg.setInt("EditorWindow", "tile_grid_height", config.tileGridHeight);
    }

    return cfg.save(path);
}

bool EditorConfig::loadPathsConfig(EditorPathsConfig &config) {
    std::filesystem::path path = getConfigPath();
    rs_utl::ResyndConfig cfg;
    if (!cfg.load(path)) return false;

    for (const auto &secName : { "Paths", "General" }) {
        for (const auto &kv : cfg.getSectionEntries(secName)) {
            if (kv.first == "data_dir" || kv.first == "original_data_dir" || kv.first == "data_folder" || kv.first == "assets_dir") {
                if (config.dataDir.empty()) config.dataDir = kv.second;
            } else if (kv.first == "data_signature") {
                if (config.dataSignature.empty()) config.dataSignature = kv.second;
            }
        }
    }

    return !config.dataDir.empty();
}

bool EditorConfig::savePathsConfig(const EditorPathsConfig &config) {
    std::filesystem::path path = getConfigPath();
    rs_utl::ResyndConfig cfg;
    cfg.load(path);

    // Remove legacy original_data_dir if present to only keep data_dir
    cfg.removeKey("Paths", "original_data_dir");
    cfg.setString("Paths", "data_dir", config.dataDir);
    if (!config.dataSignature.empty()) {
        cfg.setString("Paths", "data_signature", config.dataSignature);
    }

    // If Loadout section does not exist, include default loadout!
    if (cfg.getSectionEntries("Loadout").empty()) {
        PlaytestLoadoutData defaultData = getDefaultLoadout();
        cfg.setSectionEntries("Loadout", loadoutDataToEntries(defaultData));
    }

    return cfg.save(path);
}

PlaytestLoadoutData EditorConfig::getDefaultLoadout() {
    PlaytestLoadoutData d;
    d.activeSquadSize = 4;

    // Agent 0 - Commander
    d.agents[0].active = true;
    d.agents[0].modLegs = 3;
    d.agents[0].modArms = 3;
    d.agents[0].modChest = 3;
    d.agents[0].modHeart = 2;
    d.agents[0].modEyes = 2;
    d.agents[0].modBrain = 2;
    d.agents[0].weapons = { "minigun", "laser", "shield", "medikit" };

    // Agent 1 - Heavy Assault
    d.agents[1].active = true;
    d.agents[1].modLegs = 1;
    d.agents[1].modArms = 2;
    d.agents[1].modChest = 2;
    d.agents[1].modHeart = 1;
    d.agents[1].modEyes = 1;
    d.agents[1].modBrain = 1;
    d.agents[1].weapons = { "minigun", "gauss", "shield", "medikit" };

    // Agent 2 - Close Quarters
    d.agents[2].active = true;
    d.agents[2].modLegs = 1;
    d.agents[2].modArms = 1;
    d.agents[2].modChest = 1;
    d.agents[2].weapons = { "shotgun", "uzi", "shield", "medikit" };

    // Agent 3 - Infiltrator / Support
    d.agents[3].active = true;
    d.agents[3].modEyes = 1;
    d.agents[3].modBrain = 1;
    d.agents[3].weapons = { "persuadatron", "scanner", "pistol", "medikit" };

    d.faction.funds = 100000;
    d.faction.companyName = "EUROCORP";
    d.faction.color = 0;
    d.faction.logo = 0;

    return d;
}

bool EditorConfig::loadLoadoutConfig(PlaytestLoadoutData &data) {
    std::filesystem::path path = getConfigPath();
    rs_utl::ResyndConfig cfg;
    cfg.load(path);

    auto loadoutEntries = cfg.getSectionEntries("Loadout");
    bool foundLoadout = !loadoutEntries.empty();
    if (foundLoadout) {
        data = getDefaultLoadout();
        entriesToLoadoutData(loadoutEntries, data);
    } else {
        data = getDefaultLoadout();
        saveLoadoutConfig(data);
    }

    // Automatically remove AR-only items if the active data directory does not contain American Revolt assets
    if (sanitizeLoadout(data)) {
        saveLoadoutConfig(data);
    }

    // Load [Playtest] launch settings
    for (const auto &kv : cfg.getSectionEntries("Playtest")) {
        if (kv.first == "window_scale") data.launch.windowScale = std::atoi(kv.second.c_str());
        else if (kv.first == "map_zoom") data.launch.mapZoom = static_cast<float>(std::atof(kv.second.c_str()));
        else if (kv.first == "fullscreen") data.launch.fullscreen = (kv.second == "1" || kv.second == "true");
        else if (kv.first == "no_sound") data.launch.noSound = (kv.second == "1" || kv.second == "true");
        else if (kv.first == "log_option") data.launch.logOption = kv.second;
        else if (kv.first == "custom_log_flags") data.launch.customLogFlags = kv.second;
        else if (kv.first == "show_coordinates") data.launch.showCoordinates = (kv.second == "1" || kv.second == "true");
        else if (kv.first == "show_agent_paths") data.launch.showAgentPaths = (kv.second == "1" || kv.second == "true");
        else if (kv.first == "show_stats") data.launch.showStats = (kv.second == "1" || kv.second == "true");
    }

    return foundLoadout;
}

bool EditorConfig::saveLoadoutConfig(const PlaytestLoadoutData &data) {
    std::filesystem::path path = getConfigPath();
    rs_utl::ResyndConfig cfg;
    cfg.load(path);

    cfg.setSectionEntries("Loadout", loadoutDataToEntries(data));

    std::vector<std::pair<std::string, std::string>> playtestEntries = {
        { "window_scale", std::to_string(data.launch.windowScale) },
        { "map_zoom", std::format("{:.1f}", data.launch.mapZoom) },
        { "fullscreen", data.launch.fullscreen ? "1" : "0" },
        { "no_sound", data.launch.noSound ? "1" : "0" },
        { "log_option", data.launch.logOption },
        { "custom_log_flags", data.launch.customLogFlags },
        { "show_coordinates", data.launch.showCoordinates ? "1" : "0" },
        { "show_agent_paths", data.launch.showAgentPaths ? "1" : "0" },
        { "show_stats", data.launch.showStats ? "1" : "0" }
    };
    cfg.setSectionEntries("Playtest", playtestEntries);

    return cfg.save(path);
}

void EditorConfig::ensureDefaultLoadoutSection() {
    std::filesystem::path path = getConfigPath();
    if (path.empty()) return;
    rs_utl::ResyndConfig cfg;
    cfg.load(path);

    if (!cfg.getSectionEntries("Loadout").empty()) {
        return;
    }

    PlaytestLoadoutData defaultData = getDefaultLoadout();
    cfg.setSectionEntries("Loadout", loadoutDataToEntries(defaultData));
    cfg.save(path);
}

} // namespace rs_win_editor
