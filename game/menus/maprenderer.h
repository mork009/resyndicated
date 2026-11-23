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

#ifndef MENUS_MAPRENDERER_H_
#define MENUS_MAPRENDERER_H_

#include <list>
#include <vector>
#include <set>
#include <map>

#include "rs-utils/common.h"
#include "rs-engine/enginecommon.h"
#include "rs-utils/log/log.h"
#include "rs-kernel/model/position.h"
#include "rs-kernel/model/mission.h"
#include "squadselection.h"

namespace rs_eng {
class Texture;
}

class PoolableResource {
public:
    PoolableResource() { free_ = true; }
    virtual ~PoolableResource() {}

    bool isFree() { return free_; }
    void setFree(bool free) { free_ = free; }

    virtual void handleRelease() {}

private:
    bool free_;
};

class ObjectToDraw : public PoolableResource {
public:
    ObjectToDraw() {
        handleRelease();
    }

    void handleRelease() {
        pObject_ = NULL;
        pNext_ = NULL;
    }

    rs_knl::MapObject * getObject() { return pObject_; }
    ObjectToDraw * getNext() { return pNext_; }

    void setObject(rs_knl::MapObject *pObj) { pObject_ = pObj; }
    void setNext(ObjectToDraw *pNext) {
        pNext_ = pNext;
    }
    void insertNext(ObjectToDraw *pNext) {
        pNext->setNext(pNext_);
        pNext_ = pNext;
    }


private:
    rs_knl::MapObject *pObject_;
    ObjectToDraw *pNext_;
};

template<class T>
class Pool {
public:
    Pool(int nbInitialRes) {
        totalResourceCreated_ = 0;
        for (int nbRes = 0; nbRes < nbInitialRes; nbRes++) {
            pool_.push_back(new T());
            totalResourceCreated_++;
        }
    }
    ~Pool() {
        for (typename std::list < T * >::iterator it = pool_.begin();
         it != pool_.end(); it++) {
             T * pRes = (*it);
            delete pRes;
            totalResourceCreated_--;
        }

        pool_.clear();

        if (totalResourceCreated_ > 0) {
            RSERR(Log::k_FLG_MEM, "Pool", "~Pool", ("There %d unfreed resource(s)", totalResourceCreated_));
        }
    };

    T * getResource() {
        T *pResource = NULL;

        if (pool_.empty()) {
            pResource = new T();
            totalResourceCreated_++;
        } else {
            pResource = pool_.front();
            pool_.pop_front();
        }

        pResource->setFree(false);
        return pResource;
    }

    void releaseResource(T * pResource) {
        if (!pResource->isFree()) {
            pResource->handleRelease();
            pool_.push_back(pResource);
            pResource->setFree(true);
        }
    }

private:
    std::list<T *> pool_;
    int totalResourceCreated_;
};

class MapRenderer {
public:
    MapRenderer() : pool_(10) {}

    //! Initialize the renderer
    void init(rs_knl::Mission *pMission, SquadSelection *pSelection);

    void render();

    //! Scroll the map horizontally.
    void scrollOnX(int scrollAmount);
    //! Scroll the map vertically.
    void scrollOnY(int scrollAmount);

    //! Centers the map on the given ped
    void centerMapOnPed(rs_knl::PedInstance *pPed);
    //! Centers the map on the given tile point
    void centerMapOnTile(const rs_knl::TilePoint &tilePt);

    //! Return true if the mouse is over given object
    bool isMouseHovering(const Point2D &mousePt, const rs_knl::MapObject &mapObject, const Point2D &padTopLeft, const Point2D &padBotRight);
    //! Return the tilepoint under mouse
    rs_knl::TilePoint getTilePointFromMouse(const Point2D &mousePt);

    const Point2D & getViewportOriginPt() const { return viewportOriginPt_; }

private:
    //! The width of the left control panel in Gameplaymenu
    static const int kGameplayPanelWidth;

    /**
     * Return a integer which is a hash for identifying easily
     * a tile instead of use the 3 usual coordinates.
     * \param tilePos
     * \return int
     */
    static int tileHashKey(const rs_knl::TilePoint & tilePos) {
        return tilePos.tx | (tilePos.ty << 8) | (tilePos.tz << 16);
    }

    //! Get the hashkey of the tile for the given object
    static int tileHashKey(rs_knl::MapObject * m);

    void listObjectsToDraw(const Point2D &viewport);
    bool isObjectInsideDrawingArea(rs_knl::MapObject *pObject, const Point2D &viewport);
    static bool shouldDrawBefore(rs_knl::MapObject *a, rs_knl::MapObject *b);
    int drawObjectsOnTile(const rs_knl::TilePoint & tilePos, const Point2D &screenPos);
    int drawWallStaticsOnTile(const rs_knl::TilePoint & tilePos, const Point2D &screenPos);
    int drawWallStaticsForTile(const rs_knl::TilePoint & tilePos, const Point2D &screenPos);
    void drawAgentMarkersOverlay(int cmx);
    void addObjectToDraw(rs_knl::MapObject *pObject);
    void freeUnreleasedResources();

public:
    ~MapRenderer();

private:
    rs_knl::Mission *pMission_;
    rs_knl::Map *pMap_;
    SquadSelection *pSelection_;

    Pool<ObjectToDraw> pool_;
    /*! This map contains for each tile the list of dynamic/freestanding objects to draw.*/
    std::map<int, ObjectToDraw *> objectsByTile_;
    /*! This map contains for each wall tile the list of wall-mounted statics (windows, ads, signs, doors) to draw.*/
    std::map<int, ObjectToDraw *> wallStaticsByTile_;

    /*! This is a projection in 2D of a point on a the map. This point represents the top
     * left corner of the game viewport and it moves inside the map's borders.
     */
    Point2D viewportOriginPt_;

    std::unique_ptr<rs_eng::RSTexture> pZoomTexture_;
    int zoomTextureWidth_ = 0;
    int zoomTextureHeight_ = 0;
    double lastZoom_ = 1.0;
};

#endif  // MENUS_MAPRENDERER_H_
