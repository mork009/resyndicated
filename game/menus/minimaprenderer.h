/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2012, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef MENUS_MINIMAPRENDERER_H_
#define MENUS_MINIMAPRENDERER_H_

#include <map>

#include "rs-utils/common.h"
#include "rs-utils/misc/timer.h"
#include "rs-engine/enginecommon.h"
#include "rs-engine/gfx/rstexture.h"
#include "rs-kernel/model/map.h"
#include "rs-kernel/model/pathsurfaces.h"
#include "rs-kernel/model/objectivedesc.h"
#include "rs-kernel/model/mission.h"
#include "rs-kernel/model/missionbriefing.h"

/*!
 * Base class Renderer for minimap.
 * A minimap renderer can display a minimap at a given position on the screen.
 * The minimap is rendered with a resolution that depends on the zoom level.
 * The resolution is the number of pixel used to render one tile.
 * The renderer also stores the coords for the top left corner of the minimap
 * in the map coordinates. Moving the minimap depends on the subclass.
 */
class MinimapRenderer {
 public:
    /*!
     * Enumeration for the available zoom levels.
     * The higher is the zoom, the more pixels is used to
     * render a tile.
     */
    enum EZoom {
        ZOOM_X1 = 6,
        ZOOM_X2 = 4,
        ZOOM_X3 = 2,
        ZOOM_X4 = 0
    };

    MinimapRenderer(const Point2D &screenPos) : topLeftCornerPos_(screenPos) {}
    virtual ~MinimapRenderer() {}

    //! update the class with elapsed time
    virtual bool handleTick(uint32_t elapsed) = 0;

    //! Render the minimap at the given point
    virtual void render(const rs_eng::Palette & palette) = 0;
 protected:
    void setZoom(EZoom zoom);
     //! called when zoom changes
    virtual void updateRenderingInfos() = 0;
 protected:
    /*! The size in pixel of the minimap. Minimap is a square.*/
    static const int kMiniMapSizePx;

    /*! Number of pixel used to draw a map tile.*/
    int pixpertile_;
    /*!
     * Total number of tiles displayed in the minimap.
     * same for width and height.
     * The number of displayable tiles depends on the presence of a radar 
     * in the inventory.
     */
    int mm_maxtile_;
    /*! Current zoom level.*/
    EZoom zoom_;
    /*! Tile coord on the map for the top left corner of the minimap.*/
    rs_knl::TilePoint world_;
    //! Top left corner of the minimap on the screen
    Point2D topLeftCornerPos_;
};

/*!
 * Renderer for minimap.
 * This class is used to display a minimap in the briefing menu.
 */
class BriefMinimapRenderer : public MinimapRenderer {
 public:

    //! Class Constructor.
    BriefMinimapRenderer(const Point2D &screenPos);
    //! Reset the class with a new mission
    void init(rs_knl::MissionBriefing *p_briefing, EZoom zoom, bool drawEnemies);

    //! update the class with elapsed time
    bool handleTick(uint32_t elapsed) override;

    //! Render the minimap at the given point
    void render(const rs_eng::Palette & palette) override;

    //! Sets all parameters that depend on zooming level
    void zoomOut();
    //! Sets the flag whether to draw enemies on the minimap or not
    void setDrawEnemies(bool draw) { drawEnemies_ = draw; }

    //! Scrolls the minimap to the right
    void scrollRight();
    //! Scrolls the minimap to the left
    void scrollLeft();
    //! Scrolls the minimap to the top
    void scrollUp();
    //! Scrolls the minimap to the bottom
    void scrollDown();

 protected:
    //! Finds the minimap location
    void initMinimapLocation();
    void updateRenderingInfos() override;
    //! If minimap is too far right or down, align with right or down border
    void clipMinimapToRightAndDown();

 protected:
    /*! A timer to control the blinking on the minimap.*/
    rs_utl::BoolTimer timerMiniMapBlink_;

    /*! The scrolling step : depends on the zoom level.*/
    int scroll_step_;
    /*! The MissionBriefing that contains the minimap.*/
    rs_knl::MissionBriefing *pBriefing_;
    /*! IF true, enemies are drawn on the minimap.*/
    bool drawEnemies_;
};


/*!
 * Renderer for minimap.
 * This class is used to display a minimap in the gameplay menu.
 */
class GamePlayMinimapRenderer : public MinimapRenderer {
public:
    //! Constructor for the class
    GamePlayMinimapRenderer(const Point2D &screenPos);

    //! Reset the class with a new mission
    void init(rs_knl::Mission *pMission, bool b_scannerEnabled, const rs_eng::Palette & palette);

    /*!
     * Sets the minimap center on the given object.
     * This object should be the Agent leader.
     * \param pObject The MapObject on which to center the minimap.
     */
    void centerOn(rs_knl::MapObject *pObject) { pCenter_ = pObject; }

    //! Tells whether scanner is on or off.
    void setScannerEnabled(bool b_enabled);

    //! update the class with elapsed time
    bool handleTick(uint32_t elapsed) override;

    //! Render the minimap
    void render(const rs_eng::Palette & palette) override;

    //! Returns the map coordinates of the point on the minimap.
    rs_knl::TilePoint minimapToMapPoint(const Point2D & minimapPt);

protected:

    /*!
     * This enumeration lists the type of signal on the minimap.
     */
    enum ESignalType {
        //! No signal is emitted
        kNone,
        //! A signal for a target is emitted
        kTarget,
        //! A signal for an evacuation is emitted
        kEvacuation
    };
    //! called when zoom changes
    void updateRenderingInfos() override;
    //! Create the texture using the minimap information
    void initMinimapTexture(const rs_eng::Palette & palette);
    //! Create the texture for circle
    void initCircleTexture(const rs_eng::Palette & palette);

    //! Update minimap position based on the object location that the minimap is tracking
    void updateWorldPosition();

    //! Draw all visible cars
    void drawVehicles(const rs_eng::Palette & palette);
    //! Draw all visible dropped weapons
    void drawWeapons(const rs_eng::Palette & palette);
    //! Draw active Air Strike bomber planes as white triangles
    void drawAirStrikes(const rs_eng::Palette & palette);
    //! Draw visible peds
    void drawPedestrians(const rs_eng::Palette & palette);
    //! Draw a circle to represent agents, police and guards.
    void drawPedCircle(const Point2D &screenPos, int type);
    //! Draw a circle on the minimap for the signal
    void drawSignalCircle(const Point2D &signalPos, const rs_eng::Palette & palette);
    //! Draw a pixel on the minimap
    void drawPixel (const Point2D &point, rs_eng::RSColor color);

    /*!
     * Returns true if object is visible on the minimap
     * \param object to verify
     */
    bool isVisible(rs_knl::MapObject *pObject) {
        const rs_knl::TilePoint pt = pObject->position();
        return (pt.tx >= world_.tx &&
            pt.tx < (world_.tx + mm_maxtile_) &&
            pt.ty >= world_.ty &&
            pt.ty < (world_.ty + mm_maxtile_));
    }
    /*!
     * Returns coord of the given tile relatively
     * to the top letf corner of the minimap in pixel.
     */
    Point2D mapToScreenPosition(const rs_knl::TilePoint &mapPosition);

    /*!
     * Returns coord of the current real world coordinate of the signal relatively
     * to the top letf corner of the minimap in pixel.
     */
    Point2D signalLocToMiniMap() {
        return mapToScreenPosition(signalSourceLoc_);
    }

    /*!
     * Return true if the evacuation circle is completly on the minimap.
     */
    bool isEvacuationCircleOnMinimap(int signal_px, int signal_py) {
        int maxPx = mm_maxtile_ * pixpertile_;
        return (signal_px + kEvacuationRadius < maxPx &&
                signal_px - kEvacuationRadius > 0 &&
                signal_py + kEvacuationRadius < maxPx &&
                signal_py - kEvacuationRadius > 0);
    }

    //! Clear all signals on map
    void handleClearSignal();
    void onTargetObjectiveStartedEvent(rs_knl::TargetObjectiveStartedEvent *pEvt);
    void onEvacuateObjectiveStartedEvent(rs_knl::EvacuateObjectiveStartedEvent *pEvt);
    void onObjectiveEndedEvent(rs_knl::ObjectiveEndedEvent *pEvt);
    void onMissionEndedEvent(rs_knl::MissionEndedEvent *pEvt);
private:
     /*! Radius of the red evacuation circle.*/
    static const int kEvacuationRadius;
    /*! size of the texture.*/
    static const int kMinimapTextureSize;
    //! Width of the texture to store circles
    static const int kCircleTextureWidth;
    //! Height of the texture to store circles
    static const int kCircleTextureHeight;
    //! Width and height of a circle sprite in the texture
    static const int kCircleSpriteSize;

    /*! The mission that contains the minimap.*/
    rs_knl::Mission *p_mission_;
    /*! The minimap to display.*/
    rs_knl::MiniMap *p_minimap_;
    //! The minimap is always centered on the lead agent
    rs_knl::MapObject *pCenter_;
    /*! Coords in pixels of the cross.*/
    Point2D crossPos_;
    /*! Coords on the world map of the signal source.*/
    rs_knl::TilePoint signalSourceLoc_;
    /*! Type of emitted signal. If NONE, no signal is emitted.*/
    ESignalType signalType_;
    /*! Radius for the signal circle.*/
    uint16_t signalRadius_;
    /*! Smooth reference radius for the expanding signal wave.*/
    float signalRadiusRef_;
    /*! True if the scan wave is currently expanding and visible.*/
    bool signalActive_;
    /*! True if the current scan wave has crossed the agents.*/
    bool passedAgent_;
    /*! Milliseconds elapsed since the scan wave crossed the agents.*/
    float timeSincePassedAgent_;
    /*! Current signal color.*/
    uint8_t signalColor_;
    /*! A timer to control the blinking of weapons.*/
    rs_utl::BoolTimer mm_timer_weap;
    /*! A timer to control the blinking of pedestrians.*/
    rs_utl::BoolTimer mm_timer_ped;
    /*! Timer for the signal.*/
    rs_utl::Timer mm_timer_signal;
    /*!
     * A texture that store the minimap but only floor tiles.
     * The texture is draw at different size depending on the zoom.
     */
    std::unique_ptr<rs_eng::RSTexture> minimapTexture_;
    /*!
     * This texture holds circles that represent peds on the minimap.
     */
    std::unique_ptr<rs_eng::RSTexture> circleTexture_;
};

#endif  // MENUS_MINIMAPRENDERER_H_
