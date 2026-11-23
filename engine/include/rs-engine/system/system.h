/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
 *   Copyright (C) 2010, 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef SYSTEM_H
#define SYSTEM_H

#include <memory>
#include <filesystem>

#include "rs-utils/common.h"
#include "rs-utils/misc/singleton.h"
#include "rs-engine/enginecommon.h"
#include "rs-engine/io/keys.h"
#include "rs-engine/sound/audio.h"
#include "rs-engine/gfx/rstexture.h"

class Sprite;

namespace rs_eng {

enum RS_EventType {
    EVT_NONE = 0,        // No event in the queue
    EVT_QUIT = 1,        // Quit event
    EVT_MSE_MOTION = 2,    // Mouse motion
    EVT_MSE_UP = 3,
    EVT_MSE_DOWN = 4,
    EVT_KEY_DOWN = 5,
    EVT_CHEAT = 6,
    EVT_ZOOM = 7,
    EVT_RELOAD_ASSETS = 8
};

/** The "quit requested" event */
struct RS_QuitEvent {
    RS_EventType type;
};

struct RS_MouseMotionEvent {
    RS_EventType type;    /**< SDL_MOUSEMOTION */
    uint32_t state;    /**< The current button state */
    int32_t x, y;    /**< The X/Y coordinates of the mouse */
    int keyMods;
};

struct RS_MouseButtonEvent {
    RS_EventType type;    /**< SDL_MOUSEMOTION */
    uint8_t button;    /**< The current button state */
    int32_t x, y;    /**< The X/Y coordinates of the mouse */
    int keyMods;
};

struct RS_KeyEvent {
    RS_EventType type;    /**< EVT_KEY_DOWN */
    RS_Key key;
    int keyMods;
};

struct RS_CheatEvent {
    RS_EventType type;
    int cheatId;
};

struct RS_ZoomEvent {
    RS_EventType type;
    double zoomFactor;
};

/** General event structure */
union RS_Event {
    RS_EventType type;

    RS_QuitEvent quit;
    RS_MouseMotionEvent motion;
    RS_MouseButtonEvent button;
    RS_KeyEvent key;
    RS_CheatEvent cheat;
    RS_ZoomEvent zoom;
};

/*!
 * Abstract interface that all systems/ports should implement.
 */
class System : public Singleton<System> {
public:
    static std::unique_ptr<System> createSystem();

    virtual ~System() {}

    virtual void initialize(bool fullscreen) = 0;

    // Display an error message in a visible way for the user
    virtual void showError(const char *errorMsg) = 0;
    //! Opens a native file browse dialog. Returns true if a file was selected.
    virtual bool browseFile(const char *title, const char *filterDesc, const char *filterPattern, std::filesystem::path &outPath) = 0;

    virtual bool clearScreen() = 0;
    virtual void updateScreen() = 0;
    // Set the render target to be the default one
    virtual bool resetRenderTarget() = 0;
    //! Set the viewport for the drawing area
    virtual bool setViewport(int x, int y, int width, int height) = 0;
    // Reset to default viewport
    virtual bool resetViewport() = 0;

    //! Set window scale factor (1, 2, 3, 4)
    virtual void setWindowScale([[maybe_unused]] int scale) {}
    //! Returns true if the given window scale factor (1, 2, 3, 4) fits within display bounds
    virtual bool isScaleAllowed([[maybe_unused]] int scale) const { return true; }
    //! Set window custom dimensions (width x height)
    virtual void setWindowDimensions([[maybe_unused]] int width, [[maybe_unused]] int height) {}
    //! Set map zoom factor (1.0, 1.5, 2.0, etc.)
    virtual void setMapZoom([[maybe_unused]] double zoomFactor) {}

    //! Pumps an event from the event queue
    virtual bool pumpEvents(RS_Event &evtOut) = 0;
    virtual void delay(uint32_t msec) = 0;
    virtual uint32_t getTicks() = 0;

    //! Draw a point on the screen at given position and color
    virtual void drawPoint (Point2D start, rs_eng::RSColor color) = 0;
    /**
     * Draw a vertical line with the given color
     */
    virtual void drawVLine(Point2D start, int length, rs_eng::RSColor color) = 0;
    //! Draw a vertical dashed line with the given color
    virtual void drawDashedVLine(Point2D start, int length, int dashLength, int dashOffset, rs_eng::RSColor color) = 0;
    /**
     * Draw a horizontal line with the given color
     */
    virtual void drawHLine(Point2D start, int length, rs_eng::RSColor color) = 0;
    //! Draw a horizontal dashed line with the given color
    virtual void drawDashedHLine(Point2D start, int length, int dashLength, int dashOffset, rs_eng::RSColor color) = 0;
    /**
     * Draw a line with the given color
     */
    virtual void drawLine(Point2D start, Point2D end, rs_eng::RSColor color) = 0;
    /**
     * Draw a rectangle with the given color
     */
    virtual void drawRect(Point2D pos, int width, int height, rs_eng::RSColor color) = 0;
    /*!
     * @brief Draw a rect filled with the given color
     * @param pos Upper left corner position of the rect
     * @param width Width of the rectangle
     * @param height Height of the rectangle
     * @param color Color to fill the rectangle
     */
    virtual void drawFillRect(Point2D pos, int width, int height, rs_eng::RSColor color) = 0;

    //! Returns the mouse pointer coordinates
    virtual uint32_t getMousePos(Point2D &point) = 0;
    //! Hides the mouse cursor.
    virtual void hideCursor() = 0;
    //! Shows the mouse cursor.
    virtual void showCursor() = 0;
    //! Use this cursor for menu screens
    virtual void useMenuCursor() = 0;
    virtual void usePointerCursor() = 0;
    virtual void usePointerYellowCursor() = 0;
    virtual void useTargetCursor() = 0;
    virtual void useTargetRedCursor() = 0;
    virtual void usePickupCursor() = 0;

    virtual int getKeyModState() = 0;

    virtual bool isKeyModStatePressed(const int keyMod) = 0;

    /*!
     * Returns true if the specified key is currently held down.
     */
    virtual bool isKeyPressed(const RS_KeyCode keyCode) = 0;

    //! Call this method to activate the text event
    virtual void startReceiveText() = 0;
    //! Call this method to deactivate the text event
    virtual void stopReceiveText() = 0;

    //! Return a pointer to the Audio system instance
    rs_eng::Audio* getAudio() {return audio_.get();}

    //! Create a RSTexture with the current system
    virtual std::unique_ptr<RSTexture> createTexture() = 0;

    /*!
     * Return the language based on the system preferences.
     * @return English is the default
     */
    virtual rs_eng::RS_Lang getLanguageFromSystem() = 0;

    virtual bool getShowCoordinates() const = 0;
    virtual void setShowCoordinates(bool show) = 0;

    virtual bool getShowAgentPaths() const = 0;
    virtual void setShowAgentPaths(bool show) = 0;

    virtual bool getShowStats() const = 0;
    virtual void setShowStats(bool show) = 0;

protected:
    std::unique_ptr<rs_eng::Audio> audio_;
};
}

#define g_System    rs_eng::System::singleton()

#endif
