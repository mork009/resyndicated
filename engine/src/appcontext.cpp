/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2013, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-engine/appcontext.h"

#include <locale>
#include <format>

#include "rs-utils/io/file.h"
#include "rs-utils/io/resyndconfig.h"
#include "rs-utils/log/log.h"
#include "rs-engine/system/system.h"

namespace rs_eng {

AppContext::AppContext() {
    time_for_click_ = 80;
    playIntro_ = true;
    language_ = NULL;
    curr_language_ = ENGLISH;
    soundVolume_ = 0;
    menu_animation_speed_ = 1.5;
    pause_panning_ = false;
}

AppContext::~AppContext() {
    if (language_) {
        delete language_;
        language_ = NULL;
    }
}

/*!
 * Read configuration in resynd.conf
 * @param confFolder Directory containing resynd.conf (empty for default ~/.resynd)
 * @throw InitializationFailedException if any problem during reading config file
 */
void AppContext::readConfiguration(const std::string& confFolder) {
    // ReSyndicated data files are bundled with the executable build
    rs_utl::File::setResyndDataFolder("");

    readOrCreateResyndConf(confFolder);
}

/*!
 * \param confFolder const std::string& Only set if there was a CLI param
 * @throw InitializationFailedException if any problem during reading config file
 */
void AppContext::readOrCreateResyndConf(const std::string& confFolder) {
    if (!rs_utl::File::getOrCreateUserConfFolder(confFolder)) {
        throw InitializationFailedException(
                std::format("Failed to read configuration in folder: {}", confFolder));
    }

    fs::path confFullpath;
    const bool confExist = rs_utl::File::getResyndConfFullPath(confFullpath);

    rs_utl::ResyndConfig cfg;
    if (confExist) {
        RSINFO(Log::k_FLG_IO, "AppContext", "readOrCreateResyndConf", ("Reading configuration from existing file: %s", confFullpath.string().c_str()));
        if (!cfg.load(confFullpath)) {
            throw InitializationFailedException("Failed to read resynd.conf");
        }
    }

    const bool needsGameInit = !confExist
        || !cfg.hasSection("Game")
        || !cfg.hasKey("Game", "play_intro")
        || !cfg.hasKey("Game", "time_for_click");

    playIntro_ = cfg.getBool("Game", "play_intro", true);
    test_files_ = cfg.getBool("Game", "test_data", true);
    soundVolume_ = cfg.getInt("Game", "sound_volume", 100);
    const int languageID = cfg.getInt("Game", "language", 0);
    time_for_click_ = cfg.getUInt("Game", "time_for_click", 80u);
    menu_animation_speed_ = cfg.getDouble("Game", "menu_animation_speed", 1.5);
    pause_panning_ = cfg.getBool("Game", "pause_panning", false);

    std::string defaultSaveDir;
    rs_utl::File::getDefaultSaveFolder(defaultSaveDir);
    std::string saveDataDir = cfg.getString("Game", "save_data_dir", defaultSaveDir);
    if (saveDataDir.empty()) {
        saveDataDir = defaultSaveDir;
    }
    rs_utl::File::upsertSaveDataFolder(saveDataDir);

    std::string confDataDir = cfg.getString("Paths", "data_dir", "");
    if (!confDataDir.empty()) {
        rs_utl::File::setOriginalDataFolder(confDataDir);
    }
    data_signature_ = cfg.getString("Paths", "data_signature", "");

    if (needsGameInit) {
        RSINFO(Log::k_FLG_IO, "AppContext", "readOrCreateResyndConf", ("Initializing configuration file in %s\n", confFullpath.string().c_str()));
        if (!cfg.hasKey("Paths", "data_dir")) {
            cfg.setString("Paths", "data_dir", confDataDir);
        }
        if (!data_signature_.empty() && !cfg.hasKey("Paths", "data_signature")) {
            cfg.setString("Paths", "data_signature", data_signature_);
        }
        cfg.setBool("Game", "play_intro", playIntro_);
        cfg.setBool("Game", "test_data", test_files_);
        cfg.setInt("Game", "language", languageID);
        cfg.setString("Game", "save_data_dir", saveDataDir);
        cfg.setInt("Game", "sound_volume", soundVolume_);
        cfg.setUInt("Game", "time_for_click", time_for_click_);
        cfg.setDouble("Game", "menu_animation_speed", menu_animation_speed_);
        cfg.setBool("Game", "pause_panning", pause_panning_);

        if (!cfg.save(confFullpath)) {
            throw InitializationFailedException(std::format("Failed to update configuration in file: {}", confFullpath.string()));
        }
    }

    readLanguage(languageID);
}

/*!
 * Loads the right language file based on the given language id.
 * If id is 0, then look in the CTYPE category of the system Locale,
 * to find the language. By default, the language is English.
 * @param languageId const int
 * @throw InitializationFailedException if any problem during reading language file
 *
 */
void AppContext::readLanguage(const int languageId) {
    std::string filename;
        
    switch (languageId) {
    case 1:
        curr_language_ = rs_eng::FRENCH;
        break;
    case 2:
        curr_language_ = rs_eng::ITALIAN;
        break;
    case 3:
        curr_language_ = rs_eng::GERMAN;
        break;
    case 4:
        curr_language_ = rs_eng::ENGLISH;
        break;
    default: // In this case, we use the OS locale to define the language
        curr_language_ = g_System.getLanguageFromSystem();
        break;
    }

    switch (curr_language_) {
        case rs_eng::FRENCH:
            filename = "lang/french.lng";
            break;
        case rs_eng::ITALIAN:
            filename = "lang/italian.lng";
            break;
        case rs_eng::GERMAN:
            filename = "lang/german.lng";
            break;
        default:
            filename = "lang/english.lng";
            break;
    }

    try {
        language_ = new ConfigFile(rs_utl::File::getResyndDataFullPath(filename));
    } catch (...) {
        language_ = NULL;
        throw InitializationFailedException(std::format("Unable to load language file : {}", filename));
    }
}

std::string AppContext::getMessage(const std::string & id) {
    std::string msg;
    getMessage(id, msg);
    return msg;
}

void AppContext::getMessage(const std::string & id, std::string & msg) {
    if (!language_ || !language_->readInto(msg, id)) {
        msg = "?";
    }
}

/*!
 * Set the play intro parameter in the config file.
 * \param files
 */
void AppContext::updateIntroFlag() {
    std::filesystem::path confPath;
    rs_utl::File::getResyndConfFullPath(confPath);
    LOG(Log::k_FLG_IO, "AppContext", "updateIntroFlag", ("Setting play_intro to false in %s", confPath.c_str()))
    rs_utl::ResyndConfig cfg;
    cfg.load(confPath);
    cfg.setBool("Game", "play_intro", false);
    cfg.save(confPath);
}

void AppContext::deactivateTestFlag() {
    std::filesystem::path confPath;
    rs_utl::File::getResyndConfFullPath(confPath);
    LOG(Log::k_FLG_IO, "AppContext", "deactivateTestFlag", ("Setting test_data to false in %s", confPath.c_str()))
    rs_utl::ResyndConfig cfg;
    cfg.load(confPath);
    cfg.setBool("Game", "test_data", false);
    cfg.save(confPath);
}

/*!
 * Update the sound_volume parameter in the config file 
 * @param volume new volume. Must greater or equal zero
 */
void AppContext::updateSoundVolume(int volume) {
    if (volume < 0) {
        return;
    }
    std::filesystem::path confPath;
    rs_utl::File::getResyndConfFullPath(confPath);
    LOG(Log::k_FLG_IO, "AppContext", "updateSoundVolume", ("Updating sound volume to %d in %s", volume, confPath.c_str()))
    rs_utl::ResyndConfig cfg;
    cfg.load(confPath);
    cfg.setInt("Game", "sound_volume", volume);
    cfg.save(confPath);
}

}
