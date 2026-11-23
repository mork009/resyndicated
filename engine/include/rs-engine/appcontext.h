/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
 *   Copyright (C) 2010  Bohdan Stelmakh <chamel@users.sourceforge.net>
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

#ifndef CORE_APPCONTEXT_H_
#define CORE_APPCONTEXT_H_

#include <filesystem>

#include "rs-utils/common.h"
#include "rs-utils/misc/singleton.h"
#include "rs-utils/io/configfile.h"
#include "rs-engine/enginecommon.h"

namespace rs_eng {
/*!
 * This class stores application level parameters.
 */
class AppContext : public Singleton < AppContext > {
public:
    AppContext();
    ~AppContext();

    //! Reads the configuration file from the given folder (or default ~/.resynd/resynd.conf)
    void readConfiguration(const std::string& confFolder);

    bool isPlayIntro() { return playIntro_; }

    bool isTestFiles() { return test_files_; }
    //! Return the sound volume parameter
    int getSoundVolume() { return soundVolume_; }

    //! Sets the intro flag to false in the config file
    void updateIntroFlag();
    //! Sets the test files flag to false in the config file
    void deactivateTestFlag();
    //! Sets the sound volume parameter to the given value in the config file
    void updateSoundVolume(int volume);

    uint32_t getTimeForClick() { return time_for_click_; }

    double getMenuAnimationSpeed() const { return menu_animation_speed_; }

    bool isPausePanningAllowed() const { return pause_panning_; }

    const std::string &getDataSignature() const { return data_signature_; }
    void setDataSignature(const std::string &sig) { data_signature_ = sig; }

    rs_eng::RS_Lang currLanguage(void) {return curr_language_; }
    std::string getMessage(const std::string & id);
    void getMessage(const std::string & id, std::string & msg);

private:
    void readLanguage(const int languageId);
    void readOrCreateResyndConf(const std::string& confFolder);

private:
    /*! True means the intro will be played.*/
    bool playIntro_;
    /*! Time range between mouse up and down that is treated as click,
     * if it will be longer it will be treated as dragging
    */
    uint32_t time_for_click_;
    /*! True means data files will be verified.*/
    bool test_files_;
    /*! Language file. */
    ConfigFile  *language_;
    rs_eng::RS_Lang curr_language_;
    //! Volume of sound : from 0 to 128
    int soundVolume_;
    double menu_animation_speed_;
    //! True means panning map while game is paused is allowed
    bool pause_panning_;
    std::string data_signature_;
};

}

#define g_Ctx   rs_eng::AppContext::singleton()

#endif // CORE_APPCONTEXT_H_
