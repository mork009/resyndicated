/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2023-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-engine/base_app.h"

#include <filesystem>
#include <format>

#include "tracy/Tracy.hpp"

#include "rs-utils/log/log.h"
#include "rs-utils/io/file.h"

#include "rs-engine/events/event.h"
#include "rs-engine/events/default_events.h"
#include "rs-engine/gfx/animationloader.h"
#include "rs-engine/ui/settings_dialog.h"

#ifdef HAVE_SDL_MIXER

#include "mixer/sdlmixeraudio.h"
#endif // HAVE_SDL_MIXER

namespace rs_eng {

CliParam::CliParam() {
    disableSound_ = false;
}

int CliParam::parseCommandLine(int argc, char *argv[]) {
    CLI::App app;
    app.set_help_flag("-h,--help", "Display command line options and exit.");

    app.add_flag("-n,--nosound", disableSound_, "Disable sound");
    app.add_flag("-f,--fullscreen", fullscreen_, "Start the game in fullscreen mode.");
    app.add_option("-d,--data", customDataDir_, "Specify the path to the original Syndicate data files.")->option_text("<path>");
    app.add_flag("--settings", forceSettingsDialog_, "Open settings dialog to configure data path.");

    std::string desc = "Specify the directory where to find the resynd.conf file. (default: %USERPROFILE%\\.resynd)";
    app.add_option("-u,--conf", confFolderPath_, desc)->option_text("<path>");

#ifdef _DEBUG
    app.add_option("-l,--log", logMask_, "Apply the specified log flags separated by colon.")->option_text("<flag>");
#endif
    app.add_option("-w,--window-scale", windowScaleStr_, "Set initial window scale factor (1, 2, 3, 4) or dimensions (WxH).")->option_text("<scale>");
    app.add_option("-z,--map-zoom", mapZoom_, "Set initial map zoom factor (e.g. 1.0, 1.5, 2.0).")->option_text("<factor>");
    app.add_flag("--show-coords", showCoordinates_, "Display tile coordinates in game viewport.");
    app.add_flag("--show-paths", showAgentPaths_, "Display agent movement path vectors.");
    app.add_flag("--show-stats", showStats_, "Display ped and target health stats overlay.");

    // Add options from subclasses
    addOptions(app);

    try
	{
		app.parse(argc, argv);
	}
	catch (const CLI::ParseError& e)
	{
        std::cout << app.help() << std::endl;
        std::cout.flush();
        fflush(stdout);
        if(e.get_name() == "CallForHelp") {
            if (GetConsoleWindow() == NULL) {
                MessageBoxA(NULL, app.help().c_str(), "ReSyndicated Help", MB_OK | MB_ICONINFORMATION);
            }
            return -1;
        }
		return 1;
	}

    if (!windowScaleStr_.empty()) {
        int w = 0, h = 0;
        if (sscanf(windowScaleStr_.c_str(), "%dx%d", &w, &h) == 2 || sscanf(windowScaleStr_.c_str(), "%dX%d", &w, &h) == 2) {
            if (w == 640 && h == 480) windowScale_ = 1;
            else if (w == 1280 && h == 960) windowScale_ = 2;
            else if (w == 1920 && h == 1440) windowScale_ = 3;
            else if (w == 2560 && h == 1920) windowScale_ = 4;
            else if (w > 0 && h > 0) {
                customWidth_ = w;
                customHeight_ = h;
            }
        } else {
            int scale = atoi(windowScaleStr_.c_str());
            if (scale >= 1 && scale <= 4) {
                windowScale_ = scale;
            } else if (scale > 4) {
                customWidth_ = scale;
                customHeight_ = (scale * 3) / 4;
            }
        }
    }

    return 0;
}

BaseApp::BaseApp(MenuFactory *pMenuFactory)
    : context_(std::make_unique<AppContext>()),
      system_(System::createSystem()),
      animationManager_(),
      soundManager_(),
      music_(),
      menus_(pMenuFactory) {
    running_ = false;
}

BaseApp::~BaseApp() {}

int BaseApp::run(int argc, char *argv[]) {
    //rs_eng::CliParam param;

    int res = getCliParam().parseCommandLine(argc, argv);
    if ( res == -1) { // help
        return 0;
    } else if (res == 1) {
        return 1;
    }

    // Initialize log in the user configuration directory (.resynd or -u/--conf path)
    std::string logFilePath = "game.log";
    if (rs_utl::File::getOrCreateUserConfFolder(getCliParam().getConfDir())) {
        std::filesystem::path confPath;
        rs_utl::File::getResyndConfFullPath(confPath);
        if (!confPath.empty() && !confPath.parent_path().empty()) {
            logFilePath = (confPath.parent_path() / "game.log").string();
        }
    }
    Log::initialize(getCliParam().getLogMask(), logFilePath.c_str());

    LOG(Log::k_FLG_INFO, "Main", "main", ("----- Initializing application..."))
    if (initialize()) {
        LOG(Log::k_FLG_INFO, "Main", "main", ("----- Initializing application completed"))
        LOG(Log::k_FLG_INFO, "Main", "main", ("----- Starting game loop"))
        res = run();
    } else {
        LOG(Log::k_FLG_INFO, "Main", "main", ("----- Initializing application failed"))
    }

    return res;
}

bool BaseApp::initialize() {
    try {
        LOG(Log::k_FLG_INFO, "BaseApp", "initialize", ("App initialization started..."))
        const CliParam& param = getCliParam();
        context_->readConfiguration(param.getConfDir());

        std::string effectiveDataDir;
        if (param.hasCustomDataDir()) {
            effectiveDataDir = param.getCustomDataDir();
            rs_utl::File::setOriginalDataFolder(effectiveDataDir);
        } else {
            effectiveDataDir = rs_utl::File::getOriginalDataFolder().string();
        }

        bool promptSettings = param.isForceSettingsDialog();
        std::string storedSig = context_->getDataSignature();
        bool dataConfigured = false;
        while (!dataConfigured) {
            std::string currentSig = rs_utl::File::computeDataSignature(effectiveDataDir);
            bool fastCheckPassed = !promptSettings && !storedSig.empty() && (storedSig == currentSig);

            if (fastCheckPassed) {
                LOG(Log::k_FLG_INFO, "BaseApp", "initialize",
                    ("Data directory '%s' matches stored signature '%s'. Startup check passed fast.", effectiveDataDir.c_str(), currentSig.c_str()));
                dataConfigured = true;
                break;
            }

            // Size/date check did not pass (or setup dialog forced): run full structural verification
            LOG(Log::k_FLG_INFO, "BaseApp", "initialize",
                ("Running full structural data verification for '%s'...", effectiveDataDir.c_str()));
            auto val = rs_eng::verifyDataDirectories(effectiveDataDir);
            if (!val.isValid || promptSettings) {
                promptSettings = false;
                std::string selectedDir;
                if (rs_eng::showDataDirSetupDialog(nullptr, effectiveDataDir, selectedDir, L"ReSyndicated")) {
                    effectiveDataDir = selectedDir;
                    rs_utl::File::setOriginalDataFolder(effectiveDataDir);
                    storedSig = rs_utl::File::computeDataSignature(effectiveDataDir);
                    rs_eng::saveOriginalDataDirToResyndConf(effectiveDataDir, storedSig);
                    context_->setDataSignature(storedSig);
                    dataConfigured = true;
                    break;
                } else {
                    LOG(Log::k_FLG_INFO, "BaseApp", "initialize", ("Setup dialog cancelled by user. Exiting."));
                    return false;
                }
            } else {
                // Full structural verification passed! Update stored signature for fast subsequent startups
                LOG(Log::k_FLG_INFO, "BaseApp", "initialize",
                    ("Full structural data verification passed for '%s'. Updating stored signature to '%s'.", effectiveDataDir.c_str(), currentSig.c_str()));
                storedSig = currentSig;
                rs_eng::saveOriginalDataDirToResyndConf(effectiveDataDir, storedSig);
                context_->setDataSignature(storedSig);
                dataConfigured = true;
                break;
            }
        }

        if (context_->isTestFiles()) {
            context_->deactivateTestFlag();
        }

        system_->initialize(param.isFullscreen());

        if (!param.isFullscreen()) {
            if (param.hasWindowScale()) {
                int requestedScale = param.getWindowScale();
                if (system_->isScaleAllowed(requestedScale)) {
                    system_->setWindowScale(requestedScale);
                } else {
                    int fallbackScale = 1;
                    for (int s = requestedScale - 1; s >= 1; --s) {
                        if (system_->isScaleAllowed(s)) {
                            fallbackScale = s;
                            break;
                        }
                    }
                    LOG(Log::k_FLG_INFO, "BaseApp", "initialize",
                        ("Requested window scale %dx exceeds display resolution. Clamping to %dx.",
                         requestedScale, fallbackScale))
                    system_->setWindowScale(fallbackScale);
                }
            } else if (param.hasCustomWindowSize()) {
                system_->setWindowDimensions(param.getCustomWidth(), param.getCustomHeight());
            }
        }

        if (param.hasMapZoom()) {
            system_->setMapZoom(param.getMapZoom());
        }

        if (param.isShowCoordinates()) {
            system_->setShowCoordinates(true);
        }
        if (param.isShowAgentPaths()) {
            system_->setShowAgentPaths(true);
        }
        if (param.isShowStats()) {
            system_->setShowStats(true);
        }

        menus_.initialize(isLoadIntroResources());
        menus_.reloadAssetsCallback = [this]() {
            reloadAssets();
        };

        // TODO : add a parameter somewhere to load custom files
        OriginalFilesAnimationLoader loader;
        loader.loadAnimations(animationManager_);
        animationManager_.checkLoadIsOk();

        soundManager_.initialize(system_->getAudio(), param.isSoundDisabled(), isLoadIntroResources());

        music_.initialize(param.isSoundDisabled(), system_->getAudio());

        bool resInit = doInitialize();
        if (resInit) {
            LOG(Log::k_FLG_INFO, "BaseApp", "initialize", ("App initialized with success"))
        }

        EventManager::listen<QuitEvent>(this, &BaseApp::onQuitHandler);

        return resInit;
    } catch (InitializationFailedException &e) {
        g_System.showError(e.what());
        return false;
    }
}

/*!
 * By default we load intro resources 
 * @return 
 */
bool BaseApp::isLoadIntroResources() {
    return context_->isPlayIntro();
}

void BaseApp::destroy() {
    menus_.destroy();

    doDestroy();

    // Close log
    Log::close();
}

void BaseApp::doDestroy() {}

void BaseApp::reloadAssets() {
    try {
        LOG(Log::k_FLG_INFO, "BaseApp", "reloadAssets",
            ("Reloading all game assets from '%s'...",
             rs_utl::File::getOriginalDataFolder().string().c_str()))
        const CliParam& param = getCliParam();

        music_.stopCurrentSong();
        soundManager_.stop();

        menus_.destroy();
        menus_.initialize(isLoadIntroResources());

        animationManager_.reset();
        OriginalFilesAnimationLoader loader;
        loader.loadAnimations(animationManager_);
        animationManager_.checkLoadIsOk();

        soundManager_.initialize(system_->getAudio(), param.isSoundDisabled(), isLoadIntroResources());
        music_.initialize(param.isSoundDisabled(), system_->getAudio());

        doReloadAssets();
    } catch (InitializationFailedException &e) {
        g_System.showError(e.what());
    }
}

/*!
 * This method defines the application loop.
 * \param start_mission Mission id used to start the application in debug mode
 * In standard mode start_mission is always -1.
 */
int BaseApp::run() {

    // Let the concrete app decide what menu to start with
    menus_.gotoMenu(getStartMenuId());

    running_ = true;
    uint32_t lasttick = system_->getTicks();
    while (running_) {
        ZoneScopedN("Main Loop");
        uint32_t curtick = system_->getTicks();
        uint32_t diff_ticks = curtick - lasttick;
        menus_.updtSinceMouseDown(diff_ticks);

        RS_Event fsEvt;
        while(system_->pumpEvents(fsEvt)) {
            menus_.handleEvent(fsEvt);
        }
        if (diff_ticks < 30) {
            system_->delay(30 - diff_ticks);
            continue;
        }
        system_->clearScreen();
        menus_.handleTick(diff_ticks);
        menus_.renderMenu();
        lasttick = curtick;
        system_->updateScreen();
        
        FrameMark;
    }

    return 0;
}

void BaseApp::waitForKeyPress() {

    while (isRunning()) {
        // small pause while waiting for key, also mouse event
        system_->delay(20);
        RS_Event fsEvt;
        while(system_->pumpEvents(fsEvt)) {
            menus_.handleEvent(fsEvt);
        }
    }
}

void BaseApp::onQuitHandler([[maybe_unused]] QuitEvent *evt) {
    LOG(Log::k_FLG_INFO, "BaseApp", "onQuitHandler", ("Received Quit Evt : quitting"))
    context_->updateSoundVolume(soundManager_.getVolume());
    running_ = false;
}

}
