/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2012, 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "menus/minimaprenderer.h"

#include <algorithm>
#include <cmath>

#include "rs-utils/log/log.h"
#include "core/gamecontroller.h"
#include "rs-engine/system/system.h"
#include "rs-engine/events/event.h"
#include "rs-engine/sound/soundmanager.h"
#include "rs-kernel/model/missionbriefing.h"
#include "rs-kernel/model/vehicle.h"
#include "rs-kernel/model/ped.h"
#include "rs-kernel/model/shot.h"

const int MinimapRenderer::kMiniMapSizePx = 128;
const int GamePlayMinimapRenderer::kEvacuationRadius = 15;
const int GamePlayMinimapRenderer::kMinimapTextureSize = 512;
const int GamePlayMinimapRenderer::kCircleTextureWidth = 32;
const int GamePlayMinimapRenderer::kCircleTextureHeight = 16;
const int GamePlayMinimapRenderer::kCircleSpriteSize = 7;

void MinimapRenderer::setZoom(EZoom zoom) {
    zoom_ = zoom;
    pixpertile_ = 10 - zoom_;
    updateRenderingInfos();
}

BriefMinimapRenderer::BriefMinimapRenderer(const Point2D &screenPos) : MinimapRenderer(screenPos), timerMiniMapBlink_(500, false) {
    scroll_step_ = 0;
}

/*!
 * Init the renderer with a new mission, zoom level and draw_enemies params.
 */
void BriefMinimapRenderer::init(rs_knl::MissionBriefing *p_briefing, EZoom zoom, bool draw_enemies) {
    pBriefing_ = p_briefing;
    setZoom(zoom);
    drawEnemies_ = draw_enemies;
    timerMiniMapBlink_.reset();

    // Initialize minimap origin by looking for the position
    // of the first found agent on the map
    initMinimapLocation();
}

/*!
 * Centers the minimap on the starting position of agents
 */
void BriefMinimapRenderer::initMinimapLocation() {
    bool found = false;
    int maxx = pBriefing_->minimap()->max_x();
    int maxy = pBriefing_->minimap()->max_y();

    for (int x = 0; x < maxx && (!found); x++) {
        for (int y = 0; y < maxy && (!found); y++) {
            if (pBriefing_->getMinimapOverlay(x, y) == rs_knl::MiniMap::kOverlayOurAgent) {
                // We found a tile with an agent on it
                // stop searching and memorize position
                world_.tx = x;
                world_.ty = y;
                found = true;
            }
        }
    }

    int halftiles = mm_maxtile_ / 2;
    world_.tx = (world_.tx < halftiles) ? 0 : (world_.tx - halftiles + 1);
    world_.ty = (world_.ty < halftiles) ? 0 : (world_.ty - halftiles + 1);

    clipMinimapToRightAndDown();
}

/*!
 *
 */
void BriefMinimapRenderer::clipMinimapToRightAndDown() {
    if ((world_.tx + mm_maxtile_) >= pBriefing_->minimap()->max_x()) {
        // We assume that map size in tiles (p_mission_->mmax_x_)
        // is bigger than the minimap size (mm_maxtile_)
        world_.tx = pBriefing_->minimap()->max_x() - mm_maxtile_;
    }

    if ((world_.ty + mm_maxtile_) >= pBriefing_->minimap()->max_y()) {
        // We assume that map size in tiles (p_mission_->mmax_y_)
        // is bigger than the minimap size (mm_maxtile_)
        world_.ty = pBriefing_->minimap()->max_y() - mm_maxtile_;
    }
}

void BriefMinimapRenderer::updateRenderingInfos() {
    scroll_step_ = 30 / pixpertile_;
    mm_maxtile_ = 120 / pixpertile_;
}

void BriefMinimapRenderer::zoomOut() {
    switch (zoom_) {
    case ZOOM_X2:
        setZoom(ZOOM_X1);
        break;
    case ZOOM_X3:
        setZoom(ZOOM_X2);
        break;
    case ZOOM_X4:
        setZoom(ZOOM_X3);
        break;
    default:
        break;
    }

    // check if map should be aligned with right and bottom border
    // as when zooming out only displays more tiles but does not
    // move the minimap origin
    clipMinimapToRightAndDown();
}

/*!
 * Update the timer.
 * @param elapsed 
 * @return True if the timer has reached its limit
 */
bool BriefMinimapRenderer::handleTick(uint32_t elapsed) {
    return timerMiniMapBlink_.update(elapsed);
}

/*!
 * Scrolls right using current scroll step. If scroll is too far,
 * clips scrolling to the map's right border.
 */
void BriefMinimapRenderer::scrollRight() {
    world_.tx += scroll_step_;
    clipMinimapToRightAndDown();
}

/*!
 * Scrolls left using current scroll step. If scroll is too far,
 * clips scrolling to the map's left border.
 */
void BriefMinimapRenderer::scrollLeft() {
    // if scroll_step is bigger than world_.tx
    // then world_.tx -= scroll_step_ would be negative
    // but world_.tx is usigned so it would be an error
    if (world_.tx < scroll_step_) {
        world_.tx = 0;
    } else {
        // we know that world_.tx >= scroll_step_
        world_.tx -= scroll_step_;
    }
}

/*!
 * Scrolls up using current scroll step. If scroll is too far,
 * clips scrolling to the map's top border.
 */
void BriefMinimapRenderer::scrollUp() {
    if (world_.ty < scroll_step_) {
        world_.ty = 0;
    } else {
        // we know that world_.ty >= scroll_step_
        world_.ty -= scroll_step_;
    }
}

/*!
 * Scrolls down using current scroll step. If scroll is too far,
 * clips scrolling to the map's bottom border.
 */
void BriefMinimapRenderer::scrollDown() {
    world_.ty += scroll_step_;
    clipMinimapToRightAndDown();
}

/*!
 * Renders the minimap.
 * \param palette the palette for colors
 */
void BriefMinimapRenderer::render(const rs_eng::Palette & palette) {
    for (int tx = world_.tx; tx < (world_.tx + mm_maxtile_); tx++) {
        int xc = topLeftCornerPos_.x + (tx - world_.tx) * pixpertile_;
        for (int ty = world_.ty; ty < (world_.ty + mm_maxtile_); ty++) {
            uint8_t c = pBriefing_->getMinimapOverlay(tx, ty);
            switch (c) {
                case 0: // a basic tile
                    c = pBriefing_->minimap()->getColourAt(tx, ty);
                    break;
                case 1: // our agent
                    c = timerMiniMapBlink_.state() ? rs_eng::kMenuPaletteColorOrange : rs_eng::kMenuPaletteColorWhiteBlue;
                    break;
                case 2: // enemies
                    if (drawEnemies_)
                        c = timerMiniMapBlink_.state() ? rs_eng::kMenuPaletteColorOrange : rs_eng::kMenuPaletteColorBrown;
                    else
                        c = pBriefing_->minimap()->getColourAt(tx, ty);
            }

            Point2D pos {xc, topLeftCornerPos_.y + (ty - world_.ty) * pixpertile_};
            g_System.drawFillRect(pos, pixpertile_, pixpertile_, palette[c]);
        }
    }
}

/*!
 * Default constructor.
 */
GamePlayMinimapRenderer::GamePlayMinimapRenderer(const Point2D &screenPos) : MinimapRenderer(screenPos),
    mm_timer_weap(300, false), mm_timer_ped(260, false),
    mm_timer_signal(250), minimapTexture_(g_System.createTexture()),
    circleTexture_(g_System.createTexture()),
    pCenter_(nullptr), signalRadiusRef_(0.0f),
    signalActive_(false), passedAgent_(false), timeSincePassedAgent_(0.0f) {
    p_mission_ = NULL;
    handleClearSignal();

    EventManager::listen<rs_knl::ObjectiveEndedEvent>(this, &GamePlayMinimapRenderer::onObjectiveEndedEvent);
    EventManager::listen<rs_knl::MissionEndedEvent>(this, &GamePlayMinimapRenderer::onMissionEndedEvent);
    EventManager::listen<rs_knl::EvacuateObjectiveStartedEvent>(this, &GamePlayMinimapRenderer::onEvacuateObjectiveStartedEvent);
    EventManager::listen<rs_knl::TargetObjectiveStartedEvent>(this, &GamePlayMinimapRenderer::onTargetObjectiveStartedEvent);
}

/*!
 * Sets a new mission for rendering the minimap.
 * \param pMission A mission.
 * \param b_scannerEnabled True if scanner is enabled -> changes zoom level.
 */
void GamePlayMinimapRenderer::init(rs_knl::Mission *pMission, bool b_scannerEnabled, const rs_eng::Palette & palette) {
    p_mission_ = pMission;
    p_minimap_ = pMission->getMiniMap();
    setScannerEnabled(b_scannerEnabled);
    initMinimapTexture(palette);
    initCircleTexture(palette);
    world_.reset();
    crossPos_ = topLeftCornerPos_.add(kMiniMapSizePx / 2, kMiniMapSizePx / 2);
    mm_timer_weap.reset();
    mm_timer_signal.reset();
    signalRadiusRef_ = 0.0f;
    signalActive_ = false;
    passedAgent_ = false;
    timeSincePassedAgent_ = 0.0f;
    handleClearSignal();
}

void GamePlayMinimapRenderer::updateRenderingInfos() {
    // mm_maxtile_ can be 17 or 33
    mm_maxtile_ = kMiniMapSizePx / pixpertile_;
}

/*!
 * Setting the scanner on or off will play on the zooming level.
 * \param b_enabled True will set a zoom of X1, else X3.
 */
void GamePlayMinimapRenderer::setScannerEnabled(bool b_enabled) {
    setZoom(b_enabled ? ZOOM_X1 : ZOOM_X3);
}

/*!
 * Create a texture with the representation of the minimap where 
 * each tile is a pixel on the texture. The texture should be larger
 * than the map and remaining pixels are transparent.
 * @param palette 
 */
void GamePlayMinimapRenderer::initMinimapTexture(const rs_eng::Palette & palette) {
    uint8_t minimapBuffer[kMinimapTextureSize*kMinimapTextureSize];

    memset(minimapBuffer, 255, kMinimapTextureSize*kMinimapTextureSize);

    if (p_minimap_->max_x() > kMinimapTextureSize || p_minimap_->max_y() > kMinimapTextureSize) {
        RSERR(Log::k_FLG_GFX, "GamePlayMinimapRenderer", "initMinimapTexture", ("Map size is bigger than texture size : %d, %d", p_minimap_->max_x(), p_minimap_->max_y()));
    }

    for (int y = 0; y < p_minimap_->max_y(); ++y) {
        // pointer to beginning of row
        int yOffset = y * kMinimapTextureSize;
        for (int x = 0; x < p_minimap_->max_x(); x++) {
            uint8_t tileColor = p_mission_->getMiniMap()->getColourAt(x, y);
            minimapBuffer[x + yOffset] = tileColor;
        }
        
    }

    // Then init texture with the buffer
    bool res = minimapTexture_->create8bitsSurfaceFromData(minimapBuffer, 
                                            kMinimapTextureSize, 
                                            kMinimapTextureSize,
                                            255);
    
    if (res) {
        if (minimapTexture_->setPalette(palette)) {
            minimapTexture_->loadTextureFromSurface();
        }
    }
}

/*!
 * This array is used as a mask to draw circle for peds.
 * Values of the mask are :
 * - 0 : use the color of the buffer
 * - 1 : use border color if buffer color is not the fillColor
 * - 2 : use the fillColor
 */
uint8_t g_ped_circle_mask_[] = {
    0,  0,  1,  1,  1,  0,  0,
    0,  1,  2,  2,  2,  1,  0,
    1,  2,  2,  2,  2,  2,  1,
    1,  2,  2,  2,  2,  2,  1,
    1,  2,  2,  2,  2,  2,  1,
    0,  1,  2,  2,  2,  1,  0,
    0,  0,  1,  1,  1,  0,  0
};

/*!
 * This methode creates a texture to store a spritesheet of the different
 * circles used to represent ped on the minimap.
 * For each type of Ped, there are specific colors for the circle. Plus,
 * as the circle is blinking, we store 2 sprites for each circle: one on the
 * first line and the second just below on a second line.
 * There are 4 types of circles.
 * @param palette The palette of colors
 */
void GamePlayMinimapRenderer::initCircleTexture(const rs_eng::Palette & palette) {
    uint8_t circleBuffer[kCircleTextureWidth*kCircleTextureHeight];
    memset(circleBuffer, 255, kCircleTextureWidth*kCircleTextureHeight);
    // This array contains the colors used for each type of circle
    uint8_t colors[] {
        // First is the circle main color, second and third are use for blinking
        rs_eng::kPaletteGameColorYellow, rs_eng::kPaletteGameColorBlack, rs_eng::kPaletteGameColorLightGreen,     // 0 - our agent or persuaded
        rs_eng::kPaletteGameColorLightRed, rs_eng::kPaletteGameColorBlack, rs_eng::kPaletteGameColorDarkRed,      // 1 - enemy agent
        rs_eng::kPaletteGameColorBlue, rs_eng::kPaletteGameColorBlack, rs_eng::kPaletteGameColorBlueGrey,         // 2 - Police
        rs_eng::kPaletteGameColorLightGrey, rs_eng::kPaletteGameColorWhite, rs_eng::kPaletteGameColorBlack        // 3 - Guard
    };
    const int kNumOfCirclePerRow = 4;

    for (int circle=0; circle < kNumOfCirclePerRow; circle++) {
        // start of the sprite pixels in the destination surface (upper left corner of logo)
        int offsetDest1 = circle * (kCircleSpriteSize +1);
        // start of the sprite for the second version of the circle
        int offsetDest2 = circle * (kCircleSpriteSize + 1) + (kCircleTextureWidth*kCircleSpriteSize) + kCircleTextureWidth;

        // Copy pixels
        for (int j=0; j < kCircleSpriteSize; j++) {
            int lineOffsetDest = j * kCircleTextureWidth;
            for (int i=0; i < kCircleSpriteSize; i++) {
                int srcPixel = i + j * kCircleSpriteSize;

                switch(g_ped_circle_mask_[srcPixel]) {
                case 2:
                    circleBuffer[offsetDest1 + lineOffsetDest + i] = colors[circle*3];
                    circleBuffer[offsetDest2 + lineOffsetDest + i] =colors[circle*3];
                    break;
                case 1:
                    circleBuffer[offsetDest1 + lineOffsetDest + i] =colors[circle*3 + 1];
                    circleBuffer[offsetDest2 + lineOffsetDest + i] =colors[circle*3 + 2];
                    break;
                default:
                    circleBuffer[offsetDest1 + lineOffsetDest + i] = 255;
                    circleBuffer[offsetDest2 + lineOffsetDest + i] = 255;
                    break;
                }
                
            }
        }
    }

    // Then init texture with the buffer
    bool res = circleTexture_->create8bitsSurfaceFromData(circleBuffer, 
                                            kCircleTextureWidth, 
                                            kCircleTextureHeight,
                                            255);
    
    if (res) {
        if (circleTexture_->setPalette(palette)) {
            circleTexture_->loadTextureFromSurface();
        }
    }
}

/*!
 * Update the origin of the minimap based on the center position.
 * If the agent is too close from the border, the minimap
 * does not move anymore.
 */
void GamePlayMinimapRenderer::updateWorldPosition() {
    int halfSize = mm_maxtile_ / 2;
    rs_knl::TilePoint tp = pCenter_->position();

    if (tp.tx < halfSize) {
        // we're too close of the top border -> stop moving along X axis
        world_.tx = 0;
        world_.ox = 0;
    } else if ((tp.tx + halfSize) >= p_mission_->mmax_x_) {
        // we're too close of the bottom border -> stop moving along X axis
        world_.tx = p_mission_->mmax_x_ - mm_maxtile_;
        world_.ox = 0;
    } else {
        world_.tx = tp.tx - halfSize;
        world_.ox = tp.ox;
    }

    if (tp.ty < halfSize) {
        world_.ty = 0;
        world_.oy = 0;
    } else if ((tp.ty + halfSize) >= p_mission_->mmax_y_) {
        world_.ty = p_mission_->mmax_y_ - mm_maxtile_;
        world_.oy = 0;
    } else {
        world_.ty = tp.ty - halfSize;
        world_.oy = tp.oy;
    }
}

void GamePlayMinimapRenderer::onObjectiveEndedEvent([[maybe_unused]] rs_knl::ObjectiveEndedEvent *pEvt) {
    p_minimap_->clearTarget();
    handleClearSignal();
}

void GamePlayMinimapRenderer::onMissionEndedEvent([[maybe_unused]] rs_knl::MissionEndedEvent *pEvt) {
    p_minimap_->clearTarget();
    handleClearSignal();
}

/*!
 * @brief React to the Evacuate objective being started.
 * On this event, start the signal centered on the evacuation point
 * @param pEvt The event containing the position where to evacuate
 */
void GamePlayMinimapRenderer::onEvacuateObjectiveStartedEvent(rs_knl::EvacuateObjectiveStartedEvent *pEvt) {
    handleClearSignal();
    pEvt->evacuationPoint.convertToTilePoint(&signalSourceLoc_);

    signalType_ = kEvacuation;
    signalRadiusRef_ = 0.0f;
    signalRadius_ = 0;
    signalActive_ = true;
    passedAgent_ = false;
    timeSincePassedAgent_ = 0.0f;

    // Check if the evacuation point is already visible
    // in this case, we draw the circle in red
    Point2D signalPos = signalLocToMiniMap();
    if (isEvacuationCircleOnMinimap(signalPos.x, signalPos.y)) {
        signalColor_ = rs_eng::kPaletteGameColorDarkRed;
        signalRadius_ = kEvacuationRadius;
    }
}

void GamePlayMinimapRenderer::handleClearSignal() {
    signalRadius_ = 0;
    signalRadiusRef_ = 0.0f;
    signalActive_ = false;
    passedAgent_ = false;
    timeSincePassedAgent_ = 0.0f;
    signalColor_ = rs_eng::kPaletteGameColorWhite;
    signalType_ = kNone;
    signalSourceLoc_.reset();
}

/*!
 * Defines a signal position on the map.
 */
void GamePlayMinimapRenderer::onTargetObjectiveStartedEvent(rs_knl::TargetObjectiveStartedEvent *pEvt) {
    handleClearSignal();
    // get the target current position
    p_minimap_->setTarget(pEvt->pTarget);
    signalSourceLoc_.initFrom(pEvt->pTarget->position());
    signalType_ = kTarget;
    signalRadiusRef_ = 0.0f;
    signalRadius_ = 0;
    signalActive_ = true;
    passedAgent_ = false;
    timeSincePassedAgent_ = 0.0f;
}

bool GamePlayMinimapRenderer::handleTick(uint32_t elapsed) {
    mm_timer_ped.update(elapsed);
    mm_timer_weap.update(elapsed);

    updateWorldPosition();

    if (signalType_ != kNone) {
        Point2D signalPos = signalLocToMiniMap();

        if (signalType_ == kEvacuation && isEvacuationCircleOnMinimap(signalPos.x, signalPos.y)) {
            // the evacuation circle is completely on the map, so it's a red circle with fixed size
            signalColor_ = rs_eng::kPaletteGameColorDarkRed;
            signalRadius_ = kEvacuationRadius;
            signalRadiusRef_ = 0.0f;
            signalActive_ = true;
            passedAgent_ = false;
            timeSincePassedAgent_ = 0.0f;
        } else {
            // reset color to white in case the red circle was displayed
            signalColor_ = rs_eng::kPaletteGameColorWhite;

            // Compute reference position in 4 px/tile coordinate space centered on lead agent (or world)
            float refSignalX = 64.0f;
            float refSignalY = 64.0f;
            if (pCenter_) {
                const rs_knl::TilePoint &agentPos = pCenter_->position();
                float dx = (float)(signalSourceLoc_.tx - agentPos.tx) + (float)(signalSourceLoc_.ox - agentPos.ox) / 256.0f;
                float dy = (float)(signalSourceLoc_.ty - agentPos.ty) + (float)(signalSourceLoc_.oy - agentPos.oy) / 256.0f;
                refSignalX = 64.0f + dx * 4.0f;
                refSignalY = 64.0f + dy * 4.0f;
            } else {
                refSignalX = (float)((signalSourceLoc_.tx - world_.tx) * 4) + (float)(signalSourceLoc_.ox - world_.ox) / 64.0f;
                refSignalY = (float)((signalSourceLoc_.ty - world_.ty) * 4) + (float)(signalSourceLoc_.oy - world_.oy) / 64.0f;
            }

            // Distance from signal source to lead agent (centered at 64, 64) in reference coordinates
            float dxAgent = refSignalX - 64.0f;
            float dyAgent = refSignalY - 64.0f;
            float distToAgent = std::sqrt(dxAgent * dxAgent + dyAgent * dyAgent);

            // The scan line travels 5 tiles (5 * 4 = 20 ref pixels) past the agents and stops
            const float kExtraDistance = 5.0f * 4.0f; // 20.0f ref pixels (5 tiles)
            float stopRadius = distToAgent + kExtraDistance;

            if (signalActive_) {
                // Smooth expansion: at reference zoom (ZOOM_X1, 4 pixels/tile),
                // the radius expands at 16 pixels per 250ms = 0.064 ref-pixels / ms.
                signalRadiusRef_ += elapsed * (16.0f / 250.0f);

                if (!passedAgent_ && signalRadiusRef_ >= distToAgent) {
                    passedAgent_ = true;
                    timeSincePassedAgent_ = 0.0f;
                }

                if (signalRadiusRef_ >= stopRadius) {
                    signalActive_ = false;
                    signalRadius_ = 0;
                } else {
                    // Scale reference radius to current screen zoom level
                    float screenRadius = signalRadiusRef_ * ((float)pixpertile_ / 4.0f);
                    signalRadius_ = (uint16_t)std::round(screenRadius);
                }
            } else {
                signalRadius_ = 0;
            }

            if (passedAgent_) {
                timeSincePassedAgent_ += elapsed;
                // Ping happens about 0.5 second after the scan line passes over the agents
                if (timeSincePassedAgent_ >= 500.0f) {
                    signalRadiusRef_ = 0.0f;
                    signalActive_ = true;
                    passedAgent_ = false;
                    timeSincePassedAgent_ = 0.0f;

                    // Update signal position in case of a moving target
                    if (signalType_ == kTarget && p_minimap_->target()) {
                        signalSourceLoc_.initFrom(p_minimap_->target()->position());
                    }
                    g_SoundMgr.play(rs_eng::TRACKING_PONG);
                }
            }
        }
    }

    return true;
}

Point2D GamePlayMinimapRenderer::mapToScreenPosition(const rs_knl::TilePoint &mapPosition) {
    return 
        {
             ((mapPosition.tx - world_.tx) * pixpertile_) + ((mapPosition.ox - world_.ox) / (256 / pixpertile_)),
             ((mapPosition.ty - world_.ty) * pixpertile_) + ((mapPosition.oy - world_.oy) / (256 / pixpertile_))};
}

/*!
 * \param mm_x coord on the minimap in pixels
 * \param mm_y coord on the minimap in pixels
 */
rs_knl::TilePoint GamePlayMinimapRenderer::minimapToMapPoint(const Point2D & minimapPt) {
    rs_knl::TilePoint pt;
    // I'm not sure this code is correct
    int tx = (minimapPt.x) / pixpertile_;
    int ty = (minimapPt.y) / pixpertile_;
    int ox = (minimapPt.x) % pixpertile_;
    int oy = (minimapPt.y) % pixpertile_;

    pt.tx = tx + world_.tx;
    pt.ty = ty + world_.ty;
    pt.tz = 0;
    pt.ox = ox * (256 / pixpertile_);
    pt.oy = oy * (256 / pixpertile_);

    return pt;
}

/*!
 * Renders the minimap.
 * \param palette the palette for colors
 */
void GamePlayMinimapRenderer::render(const rs_eng::Palette & palette) {
    // Setting the viewport to use clipboarding
    g_System.setViewport(topLeftCornerPos_.x, topLeftCornerPos_.y, kMiniMapSizePx, kMiniMapSizePx);
    // First render the background with the tiles
    // We display 1 tile more horizontaly and vertically to be sure to cover all the viewport
    minimapTexture_->renderStretch({world_.tx, world_.ty}, 
                                    {-world_.ox / (256 / pixpertile_), -world_.oy / (256 / pixpertile_)},
                                    mm_maxtile_ + 1,
                                    mm_maxtile_ + 1,
                                    pixpertile_);

    // Draw the minimap cross
    g_System.drawHLine({0, crossPos_.y}, kMiniMapSizePx, palette[rs_eng::kPaletteGameColorBlack]);
    g_System.drawVLine({crossPos_.x, 0}, kMiniMapSizePx, palette[rs_eng::kPaletteGameColorBlack]);

    // draw all visible elements on the minimap
    drawPedestrians(palette);
    drawWeapons(palette);
    drawVehicles(palette);
    drawAirStrikes(palette);

    if (signalType_ != kNone) {
        Point2D signal = signalLocToMiniMap();
        drawSignalCircle(signal, palette);
    }

    g_System.resetViewport();
}

void GamePlayMinimapRenderer::drawVehicles(const rs_eng::Palette & palette) {
    for (size_t i = 0; i < p_mission_->numVehicles(); i++) {
        rs_knl::Vehicle *p_vehicle = p_mission_->vehicle(i);
        Point2D screenPos = mapToScreenPosition(p_vehicle->position());

        if (isVisible(p_vehicle)) {
            // vehicle is on minimap and must be drawn.
            // if a vehicle contains at least one of our agent
            // we only draw the yellow circle representing the agent
            if (p_vehicle->containsOurAgents()) {
                /* int px = mapToMiniMapX(tx + 1, p_vehicle->offX());
                int py = mapToMiniMapY(ty + 1, p_vehicle->offY());
                uint8_t borderColor = (mm_timer_ped.state()) ? rs_utl::kColorBlack : rs_utl::kColorLightGreen; */
                drawPedCircle(screenPos, 0);

            } else {
                int vehicle_size = (zoom_ == ZOOM_X1) ? 2 : 4;
                /* int px = mapToMiniMapX(tx + 1, p_vehicle->offX()) - vehicle_size / 2;
                int py = mapToMiniMapY(ty + 1, p_vehicle->offY()) - vehicle_size / 2; */

                g_System.drawFillRect(screenPos, vehicle_size, vehicle_size, palette[rs_eng::kPaletteGameColorWhite]);
            }
        }
    }
}

void GamePlayMinimapRenderer::drawWeapons(const rs_eng::Palette & palette) {
    const size_t weapon_size = 2;
    for (size_t i = 0; i < p_mission_->numWeaponsOnGround(); i++)
    {
        rs_knl::WeaponInstance * w = p_mission_->weaponOnGround(i);
        // we draw weapons that have no owner ie that are on the ground
        // and are not destroyed
        if (!w->isDrawable())
            continue;

        if (isVisible(w)) {
            if (mm_timer_weap.state()) {
                Point2D screenPos = mapToScreenPosition(w->position());

                g_System.drawFillRect(screenPos, weapon_size, weapon_size, palette[rs_eng::kPaletteGameColorLightGrey]);
            }
        }
    }
}

void GamePlayMinimapRenderer::drawAirStrikes(const rs_eng::Palette & palette) {
    rs_eng::RSColor white = palette[rs_eng::kPaletteGameColorWhite];
    int worldOriginX = world_.tx * 256 + world_.ox;
    int worldOriginY = world_.ty * 256 + world_.oy;

    auto drawLine = [&](int x0, int y0, int x1, int y1) {
        int dx = std::abs(x1 - x0);
        int dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;
        while (true) {
            if (x0 >= 0 && x0 < 128 && y0 >= 0 && y0 < 128) {
                drawPixel({x0, y0}, white);
            }
            if (x0 == x1 && y0 == y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x0 += sx;
            }
            if (e2 < dx) {
                err += dx;
                y0 += sy;
            }
        }
    };

    auto drawHollowChevron = [&](int cx, int cy, double ux, double uy) {
        double vx = -uy;
        double vy = ux;
        // Tip of the chevron pointing along flight direction
        int tipX = cx + static_cast<int>(std::round(ux * 4.0));
        int tipY = cy + static_cast<int>(std::round(uy * 4.0));
        // Trailing left wingtip
        int leftX = cx + static_cast<int>(std::round(-ux * 3.0 - vx * 3.5));
        int leftY = cy + static_cast<int>(std::round(-uy * 3.0 - vy * 3.5));
        // Trailing right wingtip
        int rightX = cx + static_cast<int>(std::round(-ux * 3.0 + vx * 3.5));
        int rightY = cy + static_cast<int>(std::round(-uy * 3.0 + vy * 3.5));
        // Inner notch / indentation
        int notchX = cx + static_cast<int>(std::round(-ux * 1.5));
        int notchY = cy + static_cast<int>(std::round(-uy * 1.5));

        // Draw hollow chevron outline matching original American Revolt radar display
        drawLine(tipX, tipY, leftX, leftY);
        drawLine(tipX, tipY, rightX, rightY);
        drawLine(leftX, leftY, notchX, notchY);
        drawLine(rightX, rightY, notchX, notchY);
    };

    for (size_t i = 0; i < p_mission_->numProjectileShots(); ++i) {
        rs_knl::ProjectileShot *shot = p_mission_->projectileShot(i);
        if (!shot || !shot->isAirStrike() || shot->isLifeOver()) {
            continue;
        }
        auto *airShot = dynamic_cast<rs_knl::AirStrikeShot *>(shot);
        if (!airShot) continue;

        double ux = airShot->dirX();
        double uy = airShot->dirY();

        std::vector<rs_knl::WorldPoint> planePositions = airShot->getPlanePositions();
        for (const auto &posW : planePositions) {
            int cx = ((posW.x - worldOriginX) * pixpertile_) / 256;
            int cy = ((posW.y - worldOriginY) * pixpertile_) / 256;
            if (cx >= -6 && cx <= 134 && cy >= -6 && cy <= 134) {
                drawHollowChevron(cx, cy, ux, uy);
            }
        }
    }
}

void GamePlayMinimapRenderer::drawPedestrians(const rs_eng::Palette & palette) {
    for (size_t i = 0; i < p_mission_->numPeds(); i++)
    {
        rs_knl::PedInstance *p_ped = p_mission_->ped(i);
        // we are not showing dead or peds inside vehicle
        if (p_ped->isDead() || p_ped->inVehicle())
            continue;

        if (isVisible(p_ped))
        {
            Point2D screenPos = mapToScreenPosition(p_ped->position());

            if (p_ped->isPersuaded()) {
                drawPedCircle(screenPos, 0);
            } else {
                switch (p_ped->type())
                {
                case rs_knl::PedInstance::kPedTypeCivilian:
                case rs_knl::PedInstance::kPedTypeCriminal:
                    {
                        // white rect 2x2 (opaque and transparent blinking)
                        int ped_width = 2;
                        int ped_height = 2;
                        if (mm_timer_ped.state()) {
                            // draw the square
                            g_System.drawFillRect(screenPos, ped_width, ped_height, palette[rs_eng::kPaletteGameColorWhite]);
                        }
                    break;
                    }
                case rs_knl::PedInstance::kPedTypeAgent:
                {
                    if (p_ped->isOurAgent())
                    {
                        drawPedCircle(screenPos, 0);
                        if (p_ped == pCenter_) {
                            // get the cross coordinate
                            crossPos_ = screenPos;
                        }
                    } else {
                        drawPedCircle(screenPos, 1);
                    }
                }
                break;
                case rs_knl::PedInstance::kPedTypePolice:
                    {
                    drawPedCircle(screenPos, 2);
                    }
                    break;
                case rs_knl::PedInstance::kPedTypeGuard:
                    {
                    drawPedCircle(screenPos, 3);
                    }
                    break;
                }
            }
        }
    }
}

/*!
    * Draw a circle with the given colors for fill and border. This is used to represent agents, police and guards.
    * \param screenPos center of the circle on the screen
    * \param type What type of circle : 0 for our agent/persuaded, 1 for enemy, 2 for guard, 3 for police
    */
void GamePlayMinimapRenderer::drawPedCircle(const Point2D &screenPos, int type) {
    if (type < 4) {
        // We add 1 pixel in X and Y because it is the space between each sprite in the texture
        Point2D originTexture {type * (kCircleSpriteSize + 1), 0};
        if (mm_timer_ped.state()) {
            originTexture.y += kCircleSpriteSize + 1;
        }
        circleTexture_->render(originTexture, screenPos.add(-3, -3), kCircleSpriteSize, kCircleSpriteSize);
    }
}

/*!
 * Draw a pixel only if inside the minimap borders
 * @param point Position of the point
 * @param color Color to draw the point
 */
void GamePlayMinimapRenderer::drawPixel (const Point2D &point, rs_eng::RSColor color) {
        if (point.x > 0 && point.x < (kMiniMapSizePx) && 
            point.y > 0 && point.y < (kMiniMapSizePx)) {
            g_System.drawPoint(point, color);
        }
    }

// NOTE: this function is derived from http://cimg.sourceforge.net/
// and is under http://www.cecill.info/licences/Licence_CeCILL_V2-en.txt
// or http://www.cecill.info/licences/Licence_CeCILL-C_V1-en.txt
// licenses
void GamePlayMinimapRenderer::drawSignalCircle(const Point2D &signalPos, const rs_eng::Palette & palette)
{
    rs_eng::RSColor color = palette[signalColor_];
    if (signalRadius_ == 0) {
        return;
    }

    drawPixel(signalPos.add(-signalRadius_, 0), color);
    drawPixel(signalPos.add(signalRadius_, 0), color);
    drawPixel(signalPos.add(0, -signalRadius_), color);
    drawPixel(signalPos.add(0, signalRadius_), color);

    if (signalRadius_ == 1) {
        return;
    }

    for (int f = 1-signalRadius_, ddFx = 0, ddFy = -(signalRadius_<<1), x = 0, y = signalRadius_; x<y; ) {
        if (f>=0) { f+=(ddFy+=2); --y; }
        ++x; ++(f+=(ddFx+=2));
        if (x!=y+1) {
            const int x1 = signalPos.x-y,
                    x2 = signalPos.x+y,
                    y1 = signalPos.y-x,
                    y2 = signalPos.y+x,
                    x3 = signalPos.x-x,
                    x4 = signalPos.x+x,
                    y3 = signalPos.y-y,
                    y4 = signalPos.y+y;
            drawPixel({x1,y1}, color);
            drawPixel({x1,y2}, color);
            drawPixel({x2,y1}, color);
            drawPixel({x2,y2}, color);
            if (x!=y) {
                drawPixel({x3,y3}, color);
                drawPixel({x4,y4}, color);
                drawPixel({x4,y3}, color);
                drawPixel({x3,y4}, color);
            }
        }
    }
}

