/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
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

#ifndef SYSTEM_SDL_H
#define SYSTEM_SDL_H

#include <SDL3/SDL.h>
#include <queue>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "rs-engine/system/system.h"
#include "rs-engine/io/keys.h"

namespace rs_eng {

/*! \brief Implementation of the System interface for SDL.
 *
 * This class implements the System interface based on the SDL3 library.
 *  - Display
 *    Every object in the game is drawn on a simple uint8 array in the Screen
 *    class. Then in SystemSDL::updateScreen(), we copy this array into a
 *    Uint32 array that matches the display format and this array is then copied
 *    to an SDL Texture. This texture is then copied on the back buffer before
 *    presenting the scree.
 *  - Mouse Cursor
 *    In order to display colorfull cursors, the SDL cursor display is disabled
 *    and is manually managed by this class with a SDL_Texture.
 *    A surface is loaded with a collection of sprites, and every time the
 *    mouse moves, the corresponding sprite is blit on screen at the
 *    mouse coordinates.\n
 *    If the SDLSystem fails to load the cursor surface, the default SDL cursor
 *    will be used, but not change will affect the cursor, except hide/show.
 */
class SystemSDL : public System {
public:
    SystemSDL();
    ~SystemSDL();

    //! Initialize the SDL resources
    void initialize(bool fullscreen) override;
    //! @copydoc System::showError()
    void showError(const char *errorMsg) override;
    //! @copydoc System::browseFile()
    bool browseFile(const char *title, const char *filterDesc, const char *filterPattern, std::filesystem::path &outPath) override;
    bool clearScreen() override;
    //! Render back buffer to the screen
    void updateScreen() override;
    //! @copydoc System::resetRenderTarget()
    bool resetRenderTarget() override;
    //! @copydoc System::setViewport()
    bool setViewport(int x, int y, int width, int height) override;
    //! @copydoc System::resetViewport()
    bool resetViewport() override;
    
    void setWindowScale(int scale) override;
    bool isScaleAllowed(int scale) const override;
    void setWindowDimensions(int width, int height) override;
    void setMapZoom(double zoomFactor) override;
#ifdef _WIN32
    void updateWindowMenuScales();
#endif

    //! Pumps an event from the event queue
    bool pumpEvents(RS_Event &evtOut) override;
    void handleMenuCommand(int command);

    void delay(uint32_t msec) override;
    uint32_t getTicks() override;

    void drawPoint (Point2D start, rs_eng::RSColor color) override;
    void drawVLine(Point2D start, int length, rs_eng::RSColor color) override;
    void drawHLine(Point2D start, int length, rs_eng::RSColor color) override;
    void drawDashedVLine(Point2D start, int length, int dashLength, int dashOffset, rs_eng::RSColor color) override;
    void drawDashedHLine(Point2D start, int length, int dashLength, int dashOffset, rs_eng::RSColor color) override;

    void drawLine(Point2D start, Point2D end, rs_eng::RSColor color) override;
    void drawRect(Point2D pos, int width, int height, rs_eng::RSColor color) override;
    void drawFillRect(Point2D pos, int width, int height, rs_eng::RSColor color) override;

    //! Returns the mouse pointer coordinates
    uint32_t getMousePos(Point2D &point) override;

    void hideCursor() override;
    void showCursor() override;
    void useMenuCursor() override;
    void usePointerCursor() override;
    void usePointerYellowCursor() override;
    void useTargetCursor() override;
    void useTargetRedCursor() override;
    void usePickupCursor() override;
    int getKeyModState() override {
        return keyModState_;
    }

    bool isKeyModStatePressed(const int keyMod) override {
        return (keyModState_ & keyMod) != 0;
    }

    bool isKeyPressed(const RS_KeyCode keyCode) override;

    //! Call this method to activate the text event
    void startReceiveText() override;
    //! Call this method to deactivate the text event
    void stopReceiveText() override;

    std::unique_ptr<RSTexture> createTexture() override;

    //! @copydoc System::getLanguageFromSystem()
    rs_eng::RS_Lang getLanguageFromSystem() override;

    bool getShowCoordinates() const override { return showCoordinates_; }
    void setShowCoordinates(bool show) override;

    bool getShowAgentPaths() const override { return showAgentPaths_; }
    void setShowAgentPaths(bool show) override;

    bool getShowStats() const override { return showStats_; }
    void setShowStats(bool show) override;

protected:
    //! Creates a SDL window either for fullscreen or not
    SDL_Window * createWindow(bool fullscreen);
    //! Loads the graphic file that contains the cursor sprites.
    bool loadCursorSprites();

    //! Sets the key arguments with some key codes
    void fillKeyEvent(const SDL_KeyboardEvent &keyEvt, RS_Event &evtOut);

#if defined(_WIN32)
    static bool SDLCALL win32MessageHook(void *userdata, MSG *msg);
#endif

protected:
    /*! A constant that holds the cursor icon width and height.*/
    static const int kCursorWidth;
    /*! Cursor visibility.*/
    bool cursor_visible_;
    /*! Cursor screen coordinates. */
    int32_t cursor_x_;
    /*! Cursor screen coordinates. */
    int32_t cursor_y_;
    /*! Current cursor hotspot.*/
    int cursor_hs_x_;
    /*! Current cursor hotspot.*/
    int cursor_hs_y_;

    //! The main window
    SDL_Window *pWindow_;
    //! The renderer is necessary to manipulate SDL_Texture and use graphic acceleration
    SDL_Renderer *pRenderer_;
#ifdef _WIN32
    void *hWindowMenu_;
    void *hMapZoomMenu_;
    void *hViewMenu_;
    void *hDebugMenu_;
#elif defined(__APPLE__)
    void *macMenuController_ = nullptr;
#endif

    /*!
     * A texture that holds all cursors images.
     */
    SDL_Texture *pCursorTexture_;
    /*! A rect that identify the part of
     * the cursor surface for the current cursor.*/
    SDL_Rect cursor_rect_;
    /*! A flag that tells that cursor must be updated because
     the mouse has moved or the cursor has changed.*/
    bool update_cursor_;
    bool mouse_in_window_;
    /*!
     * This field is a bit buffer storing the state of modifier buttons.
     * When a bit is set, that means a button is pressed.
     * See KeyMod enumeration to know all modifier buttons.
     */
    int keyModState_;
    void toggleFullscreen();
    bool showCoordinates_;
    bool showAgentPaths_;
    bool showStats_;
    std::queue<RS_Event> pendingEvents_;
};

}

#endif
