/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef ENGINE_BASEAPP_H
#define ENGINE_BASEAPP_H

#include <string>
#include <memory>
#include <concepts>

#include "CLI/CLI.hpp"

#include "rs-utils/log/log.h"
#include "rs-engine/appcontext.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/system/system.h"
#include "rs-engine/sound/soundmanager.h"
#include "rs-engine/sound/musicmanager.h"
#include "rs-engine/menus/menumanager.h"
#include "rs-engine/events/default_events.h"

namespace rs_eng {

class CliParam {

public:
    CliParam();
    virtual ~CliParam() {}

    //! Parse command line parameters
    int parseCommandLine(int argc, char *argv[]);

    bool isSoundDisabled() const { return disableSound_; }
    bool isFullscreen() const { return fullscreen_; }

    std::string getLogMask() const { return logMask_; }

    std::string getConfDir() const { return confFolderPath_; }

    bool hasWindowScale() const { return windowScale_ > 0; }
    int getWindowScale() const { return windowScale_; }
    bool hasCustomWindowSize() const { return customWidth_ > 0 && customHeight_ > 0; }
    int getCustomWidth() const { return customWidth_; }
    int getCustomHeight() const { return customHeight_; }

    bool hasMapZoom() const { return mapZoom_ > 0.0; }
    double getMapZoom() const { return mapZoom_; }

    bool isShowCoordinates() const { return showCoordinates_; }
    bool isShowAgentPaths() const { return showAgentPaths_; }
    bool isShowStats() const { return showStats_; }

    bool hasCustomDataDir() const { return !customDataDir_.empty(); }
    const std::string& getCustomDataDir() const { return customDataDir_; }
    bool isForceSettingsDialog() const { return forceSettingsDialog_; }

protected:
    virtual void addOptions([[maybe_unused]] CLI::App &app) {};

private:
    bool showCoordinates_ = false;
    bool showAgentPaths_ = false;
    bool showStats_ = false;
    /*!
     * Set to true to mute the sound and music using CLI param "--nosound".
     */
    bool disableSound_;
    //! Set to true to start game in fullscreen mode using CLI param "-f, --fullscreen".
    bool fullscreen_ = false;
    //! This variable stores the log mask to init log. By default we activate all logs
    std::string logMask_ = "ALL";
    //! The path to the folder containing resynd.conf
    std::string confFolderPath_;

    //! Custom original data dir specified via -d / --data
    std::string customDataDir_;
    //! Force showing settings dialog on startup via --settings
    bool forceSettingsDialog_ = false;

    //! Window scale option string from CLI (-w, --window-scale)
    std::string windowScaleStr_;
    int windowScale_ = 0;
    int customWidth_ = 0;
    int customHeight_ = 0;

    //! Map zoom option from CLI (-z, --map-zoom)
    double mapZoom_ = 0.0;
};

/*!
 * Base class for a ReSyndicated application.
 */
class BaseApp {
public:
    //! Constructor
    BaseApp(MenuFactory *pMenuFactory);
    //! Destructor
    virtual ~BaseApp();

    //! Destroy all components
    virtual void destroy() final;

    int run(int argc, char *argv[]);

    MenuManager &menus() {
        return menus_;
    }

protected:
    //! Initialize application
    virtual bool initialize() final;
    //! running loop
    int run();
    //! Child class implements this method for initialization
    virtual bool doInitialize() { return true; }
    //! Reload all game assets after data directory change at runtime
    void reloadAssets();
    //! Child class implements this method for reloading game session/controller
    virtual bool doReloadAssets() { return true; }
    //! Child class overloads this method for destroying resources
    virtual void doDestroy();
    //! Define the menuid that will be displayed at the application's start
    virtual int getStartMenuId() = 0;
    //! Return true is intro resources must be loaded
    virtual bool isLoadIntroResources();

    virtual rs_eng::CliParam & getCliParam() = 0;

    //! Returns true if the application is running
    bool isRunning() const {
        return running_;
    }

    void waitForKeyPress();

    //! Stop the running loop
    void onQuitHandler(QuitEvent *evt);

    /*! A structure to hold general application information.*/
    std::unique_ptr<AppContext> context_;
    std::unique_ptr<System> system_;

private:
    bool running_;
    AnimationManager animationManager_;
    SoundManager soundManager_;
    MusicManager music_;
    MenuManager menus_;
};

}

#endif // ENGINE_BASEAPP_H
