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

#ifndef MAPMENU_H
#define MAPMENU_H

#include "rs-engine/menus/menu.h"
#include "rs-utils/misc/timer.h"
#include "rs-engine/gfx/rstexture.h"

//! Displays the mission selection map.
/*!
 * This menu displays a world map on which the player
 * selects the next mission to play.
 */
class MapMenu : public rs_eng::Menu {
public:
    MapMenu(rs_eng::MenuManager *m);
    ~MapMenu();

    bool handleTick(uint32_t elapsed) override;
    bool handleBeforeShow() override;
    void handleAction(const ActionDesc &action) override;

protected:
    void handleRender() override;
    bool handleUnMappedKey(const rs_eng::RS_Key key) override;
    bool handleMouseDown(Point2D point, int button) override;
    //! Utility method to draw the mission selector
    void drawSelector();
    //! Draw the segments on the line to simulate a dotted line
    void drawDottedline(Point2D start, Point2D end);
    //! Utility method to update mission informations
    void handleBlockSelected();
    //! Update the game time display
    void updateClock();
    
private:
    //! Read the countries data and init the texture
    void initCountriesTexture();
    //! Copies countries data to a buffer
    void copyCountriesPixelsToBuffer(const uint8_t *mapblkData, uint8_t *countriesBuffer);
    //! Used for rendering optimization
    void buildMapOfCountriesPerColor();

protected:
    //! The width of the array that stores the color of a country
    static const int kCountrySpriteWidth;
    //! The height of the array that stores the color of a country
    static const int kCountrySpriteHeight;
    //! All sprites are store in a square texture
    static const int kCountryTextureSize;
    //! How many sprites are on a line in the texture
    static const int kCountrySpritePerRow;
    //! Size of the segment for drawing the line connecting the block to the logo
    static const int kSegmentSize;
    //! The size of the interval between 2 segments
    static const int kIntervalSize;

    /*! Contains the images of the differents blocks.*/
    uint8_t *mapblk_data_;
    //! A texture that stores the masks for drawing each country
    std::unique_ptr<rs_eng::RSTexture> countriesTexture_;
    //! An array of positions for each country in the texture
    Point2D **countrySpritePositions_;
    /*! A counter for the blinking line of the selector.*/
    rs_utl::Timer timerBlinkLine_;
    //! Used for blinking of the line of the selector.
    int offsetLine_;
    /*! A counter for the blinking available blocks. */
    rs_utl::BoolTimer timerBlinkCountry_;

    /*! Id of the text widget for time.*/
    int txtTimeId_;
    /*! Id of the text widget for Own label status.*/
    int txtOwnLblId_;
    /*! Id of the text widget for Own status.*/
    int txtOwnId_;
    /*! Id of the text widget for Country name.*/
    int txtCountryId_;
    /*! Id of the text widget for population.*/
    int txtPopId_;
    /*! Id of the text widget for tax value.*/
    int txtTaxValueId_;
    /*! Id of the text widget for tax percentage.*/
    int txtTaxPctId_;

    /*! Id of the increment tax percentage button.*/
    int incrTaxButId_;
    /*! Id of the decrement tax percentage button.*/
    int decrTaxButId_;

    /*! Id of the briefing button.*/
    int briefButId_;

    /*! This is a repartition of countries per owner
     * so we can optimized rendering.*/
    std::map<rs_eng::RSColor, std::list<int>> countriesPerColor_;
};

#endif
