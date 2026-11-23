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

#include "rs-engine/gfx/animationmanager.h"

#include "rs-utils/io/file.h"
#include "rs-utils/log/log.h"
#include "rs-engine/enginecommon.h"
#include "rs-engine/system/system.h"

namespace rs_eng {

const size_t AnimationManager::kAnimSpritesTextureWidth = 1024;

AnimationManager::AnimationManager() : spritesManager_(false, kAnimSpritesTextureWidth)
{
}

AnimationManager::~AnimationManager()
{
}

/**
 * Loads game sprites.
 * \return bool return true if everything is ok.
 *
 */
void AnimationManager::checkLoadIsOk() {
    if (!spritesManager_.loaded()) {
        LOG(Log::k_FLG_GFX, "AnimationManager", "load", ("Loading game sprites ..."))
        rs_eng::Palette emptyPalette;
        spritesManager_.loadSprites("hspr-0.tab", "hspr-0.dat", emptyPalette);
    }

    // Check consistency of resources
    for (auto &element : elements_) {
        if (element.nextElement >= elements_.size()) {
            throw InitializationFailedException(
                "GameSpriteFrameElement references an element out of limits");
        }
    }

    for (auto &frame : frames_) {
        if (frame.nextFrame >= frames_.size()) {
            throw InitializationFailedException(
                "GameSpriteFrame references a frame out of limits");
        }

        if (frame.firstElement >= elements_.size()) {
            throw InitializationFailedException(
                "GameSpriteFrame references an element out of limits");
        }
    }

    for (auto &index : firstFramePerAnim_) {
        if (index >= frames_.size()) {
            throw InitializationFailedException(
                "Animation references a frame out of limits");
        }
    }

    LOG(Log::k_FLG_SND, "AnimationManager", "load", ("loaded %i frame elements", elements_.size()))
    LOG(Log::k_FLG_SND, "AnimationManager", "load", ("loaded %i frames", frames_.size()))
    LOG(Log::k_FLG_SND, "AnimationManager", "load", ("index contains %i animations", firstFramePerAnim_.size()))
}

void AnimationManager::reset() {
    spritesManager_.clear();
    firstFramePerAnim_.clear();
    frames_.clear();
    elements_.clear();
    headSpritesLoaded_ = false;
    factionHeadsTexture_.reset();
}

void AnimationManager::addFrameElement(GameSpriteFrameElement element) {
    elements_.push_back(element);
}

/*!
 * All elements must have been added first
 * @param frame GameSpriteFrame to add
 */
void AnimationManager::addFrame(GameSpriteFrame frame) {
    assert(frame.firstElement < elements_.size());
    frames_.push_back(frame);
}

void AnimationManager::addFramesAnimation(uint16_t firstFrameIndex) {
    assert(firstFrameIndex < frames_.size());
    firstFramePerAnim_.push_back(firstFrameIndex);
}

void AnimationManager::loadHeadSprites() {
    if (headSpritesLoaded_) return;
    size_t tabSz = 0, datSz = 0;
    uint8_t *tabD = rs_utl::File::loadOriginalFile("hspr-0.tab", tabSz);
    uint8_t *datD = rs_utl::File::loadOriginalFile("hspr-0.dat", datSz);
    if (!tabD || !datD) {
        if (tabD) delete[] tabD;
        if (datD) delete[] datD;
        return;
    }
    for (int s = 41; s <= 46; ++s) {
        uint32_t off = rs_utl::READ_LE_UINT32(tabD + s * 6);
        int w = tabD[s * 6 + 4];
        int h = tabD[s * 6 + 5];
        int stride = (w + 7) / 8 * 8;
        headSprites_[s - 41].w = w;
        headSprites_[s - 41].h = h;
        headSprites_[s - 41].stride = stride;
        headSprites_[s - 41].pixels.assign(stride * h, 255);
        const uint8_t *srcB = datD + off;
        for (int row = 0; row < h; ++row) {
            uint8_t *dstR = &headSprites_[s - 41].pixels[row * stride];
            for (int col = 0; col < w; col += 8) {
                for (int px = 0; px < 8; ++px) {
                    if (!rs_utl::bitSet(srcB[0], 7 - px)) {
                        dstR[col + px] =
                            static_cast<uint8_t>((rs_utl::bitValue(srcB[1], 7 - px) << 0) & 0xff) |
                            static_cast<uint8_t>((rs_utl::bitValue(srcB[2], 7 - px) << 1) & 0xff) |
                            static_cast<uint8_t>((rs_utl::bitValue(srcB[3], 7 - px) << 2) & 0xff) |
                            static_cast<uint8_t>((rs_utl::bitValue(srcB[4], 7 - px) << 3) & 0xff);
                    }
                }
                srcB += 5;
            }
        }
    }
    delete[] tabD;
    delete[] datD;
    headSpritesLoaded_ = true;
}

bool AnimationManager::setPalette(const rs_eng::Palette &missionPalette) {
    currentMissionPalette_ = missionPalette;
    bool res = spritesManager_.setPalette(missionPalette);
    loadHeadSprites();
    if (headSpritesLoaded_ && rs_eng::System::singletonPtr() != nullptr) {
        uint8_t atlas[128 * 128];
        memset(atlas, 255, sizeof(atlas));
        uint8_t kFactionPaletteIndex[8] = { playerFactionColor_, 7, 14, 11, 11, 12, 13, 15 };
        for (int f = 0; f < 8; ++f) {
            uint8_t remappedColor = kFactionPaletteIndex[f];
            for (int s = 0; s < 6; ++s) {
                const auto &spr = headSprites_[s];
                int cellX = s * 16;
                int cellY = f * 16;
                for (int y = 0; y < spr.h; ++y) {
                    for (int x = 0; x < spr.w; ++x) {
                        uint8_t pix = spr.pixels[y * spr.stride + x];
                        if (pix == 6) {
                            pix = remappedColor;
                        }
                        atlas[(cellY + y) * 128 + (cellX + x)] = pix;
                    }
                }
            }
        }
        if (!factionHeadsTexture_) {
            factionHeadsTexture_ = g_System.createTexture();
        }
        if (factionHeadsTexture_) {
            factionHeadsTexture_->create8bitsSurfaceFromData(atlas, 128, 128, 255);
            factionHeadsTexture_->setPalette(missionPalette);
            factionHeadsTexture_->loadTextureFromSurface();
        }
    }
    return res;
}

void AnimationManager::setPlayerFactionColor(uint8_t color) {
    if (playerFactionColor_ == color) return;
    playerFactionColor_ = color;
    if (factionHeadsTexture_ && headSpritesLoaded_) {
        setPalette(currentMissionPalette_);
    }
}

void AnimationManager::drawFactionHeadSprite(int spriteId, int x, int y, bool flipped, int factionId) {
    if (!factionHeadsTexture_ || !headSpritesLoaded_ || spriteId < 41 || spriteId > 46) {
        spritesManager_.drawSprite(spriteId, x, y, flipped);
        return;
    }
    int s = spriteId - 41;
    int f = factionId & 7;
    const auto &spr = headSprites_[s];
    Point2D src = { s * 16, f * 16 };
    Point2D dst = { x, y };
    factionHeadsTexture_->renderExtended(src, dst, spr.w, spr.h, 1, flipped);
}

/*!
 * Convienient method to draw a single sprite 
 * @param spriteId Id of the sprite
 * @param screenPos Position on the screen
 */
void AnimationManager::drawSprite(int spriteId, const Point2D &screenPos) {
    spritesManager_.drawSprite(spriteId, screenPos.x, screenPos.y);
}

/*!
 * Draw a given frame that's within the given animation at given screen position.
 * @param animId The animation id
 * @param frameId The frame id in the animation 
 * @param screenPos The position on the screen
 * @return 
 */
bool AnimationManager::drawFrame(uint16_t animId, int frameId, const Point2D &screenPos, int factionId)
{
    if (animId >= firstFramePerAnim_.size()) {
        return false;
    }
    uint16_t first = firstFramePerAnim_[animId];
    if (first >= frames_.size()) {
        return false;
    }

    GameSpriteFrame *f = &frames_[first];

    size_t frameCount = 0;
    while (frameId && frameCount++ < frames_.size()) {
        if (f->nextFrame >= frames_.size()) {
            break;
        }
        f = &frames_[f->nextFrame];
        frameId--;
    }

    if (f->firstElement >= elements_.size()) {
        return false;
    }

    GameSpriteFrameElement *e = &elements_[f->firstElement];
    size_t elemCount = 0;
    while (elemCount++ < elements_.size()) {
        if (factionId >= 0 && e->sprite >= 41 && e->sprite <= 46) {
            drawFactionHeadSprite(e->sprite, screenPos.x + e->off_x, screenPos.y + e->off_y, e->flipped, factionId);
        } else {
            spritesManager_.drawSprite(e->sprite, screenPos.x + e->off_x, screenPos.y + e->off_y, e->flipped);
        }
        if (e->nextElement == 0 || e->nextElement >= elements_.size())
            break;
        e = &elements_[e->nextElement];
    }

    return f->nextFrame == first;
}

/*!
 * Tells if the frame is the last in the given animation
 * @param animId The animation id
 * @param frameId The id of the frame
 * @return True if the frame is the last one.
 */
bool AnimationManager::lastFrame(uint16_t animId, int frameId)
{
    if (animId >= firstFramePerAnim_.size()) {
        return true;
    }
    uint16_t first = firstFramePerAnim_[animId];
    if (first >= frames_.size()) {
        return true;
    }

    GameSpriteFrame *f = &frames_[first];
    while (frameId) {
        if (f->nextFrame >= frames_.size()) break;
        f = &frames_[f->nextFrame];
        frameId--;
    }

    return f->nextFrame == first;
}

uint8_t AnimationManager::lastFrame(uint16_t animId)
{
    uint8_t frameId = 0;
    if (animId >= firstFramePerAnim_.size()) {
        return 0;
    }
    uint16_t first = firstFramePerAnim_[animId];
    if (first >= frames_.size()) {
        return 0;
    }

    GameSpriteFrame *f = &frames_[first];
    while (f->nextFrame != first && f->nextFrame < frames_.size()) {
        f = &frames_[f->nextFrame];
        frameId++;
    }
    return frameId;
}

int AnimationManager::getFrameFromFrameIndx(uint16_t frameIndx)
{
    int frameId = 0;
    if (frameIndx >= frames_.size()) {
        return 0;
    }

    GameSpriteFrame *f = &frames_[frameIndx];
    size_t count = 0;
    while (count++ < frames_.size()) {
        if (f->nextFrame >= frames_.size()) break;
        f = &frames_[f->nextFrame];
        if (f->flags == 0x0100)
            break;
    }

    count = 0;
    while (count++ < frames_.size()) {
        if (f->nextFrame >= frames_.size()) break;
        f = &frames_[f->nextFrame];
        if (f->nextFrame == frameIndx)
            return frameId;
        frameId++;
    }
    return frameId;
}

}
