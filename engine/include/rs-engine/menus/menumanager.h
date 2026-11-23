/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
 *   Copyright (C) 2010, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef MENUMANAGER_H
#define MENUMANAGER_H

#include <string>
#include <map>
#include <array>
#include <memory>
#include <functional>

#include "rs-utils/common.h"
#include "rs-utils/io/configfile.h"
#include "rs-engine/enginecommon.h"
#include "rs-engine/system/system.h"
#include "rs-engine/menus/menu.h"
#include "rs-engine/gfx/dirtylist.h"
#include "rs-engine/gfx/spritemanager.h"
#include "rs-engine/gfx/logomanager.h"

namespace rs_eng {

class MenuManager;
class FliMenu;

/*!
 * This abstract class is responsible for instanciating menus from a given id.
 */
class MenuFactory {
public:
    virtual ~MenuFactory() {}
    //! Create an instance of Menu corresponding to the given id 
    Menu * createMenu(const int menuId);

    //! Return true if there is an animation for the given menu
    virtual bool hasLeaveAnimation(int menuId);
    //! Return true if there is an animation for the given menu
    virtual bool hasShowAnimation(int menuId);
    //! Return the animation file for the show of the menu
    virtual const char* getShowAnimation(int menuId);
    //! Return the animation file for the leave of the menu
    virtual const char* getLeaveAnimation(int menuId);
    
    void setMenuManager(MenuManager *pManager) { pManager_ = pManager; }

protected:
    //! Create a menu for the component
    virtual Menu * createCustomMenu(const int menuId) = 0;

protected:
    MenuManager *pManager_;
};

/*!
 * Menu manager class.
 */
class MenuManager {
public:
    std::function<void(int)> cheatCallback;
    std::function<void(double)> zoomCallback;
    std::function<void()> reloadAssetsCallback;
    Menu* getCurrentMenu() { return current_; }

    MenuManager(MenuFactory *pFactory);
    ~MenuManager();

    void initialize(bool loadIntroFont);

    //! Destroy all menus and resources
    void destroy();

    //! Return the menu sprites manager
    SpriteManager &menuSprites() {
        return menuSprites_;
    }

    FontManager &fonts() {
        return fonts_;
    }

    //! Dispatches event based on its type
    void handleEvent(const RS_Event& evt);

    void handleTick(uint32_t elapsed);

    // Change the menu
    void gotoMenu(int menuId);

    //! Return the color at given index for the current palette
    void getColorFromMenuPalette(const int colorIndex, rs_eng::RSColor &color);
    //! Returns the palette for menus
    rs_eng::Palette & getMenuPalette() { return menuPalette_; }

    //! Displays the current menu
    void renderMenu();

    //! Called before we save background for rendering menu
    void preSaveBackground();
    //! Called after background is saved
    void postSaveBackground();
    //! Copy a portion of the background texture to the screen
    void copyFromBackground(Point2D pos, int width, int height);

    //! Returns true if a menu is being displayed
    bool showingMenu() { return current_ != NULL; }

    void resetSinceMouseDown() {
        since_mouse_down_ = 0;
        mouseup_was_ = false;
    }
    bool simpleMouseDown();
    bool isMouseDragged() { return !(simpleMouseDown() && mouseup_was_ ); }
    void updtSinceMouseDown(uint32_t elapsed) {
        if (!mouseup_was_)
            since_mouse_down_ += elapsed;
    }

    //! Common colors for Menus : Dark green
    rs_eng::RSColor kMenuColorDarkGreen;
    //! Common colors for Menus : Light green
    rs_eng::RSColor kMenuColorLightGreen;
    //! Common colors for Menus : white
    rs_eng::RSColor kMenuColorWhite;
    //! Common colors for Menus : grey
    rs_eng::RSColor kMenuColorGrey;
    //! Common colors for Menus : whiteblue
    rs_eng::RSColor kMenuColorWhiteBlue;
    //! Common colors for Menus : black
    rs_eng::RSColor kMenuColorBlack;
    //! Common colors for Menus : yellow
    rs_eng::RSColor kMenuColorYellow;

protected:
    //! Returns a menu with the given id
    Menu * getMenu(int menuId);
    //! Returns the FliMenu used for transition
    FliMenu *getFliTransitionMenu();
    //! Show the next menu
    void showNextMenu();
    //! Close current menu 
    void leaveCurrentMenu();
    //! Switch from menu and plays the transition animations.
    void changeCurrentMenu();

private:
    //! Loads the palette used in menu sprites
    void loadMenuPalette();

private:
    /** The menu factory.*/
    MenuFactory *pFactory_;
    /** The list of currently loaded menus.*/
    std::map<int, Menu *> menus_;
    /** The current menu being displayed.*/
    Menu *current_;
    /** The id of the next menu. -1 by default.*/
    int nextMenuId_;
    /** This flag prevents the input events from being processed.*/
    bool drop_events_;

    /*! Sprite manager for menu sprites.*/
    SpriteManager menuSprites_;
    //! This is the palette for menu sprites
    Palette menuPalette_;
    /*! Font manager.*/
    FontManager fonts_;
    //SoundManager *pGameSounds_;
    LogoManager logoManager_;

    /*! Time since last mouse down event without mouseup*/
    uint32_t since_mouse_down_;
    //! Will not update since_mouse_down_ after this event
    bool mouseup_was_;
    //! This texture is used to draw the background for menus
    std::unique_ptr<RSTexture> pBackgroundTexture_;
};
}

#endif
