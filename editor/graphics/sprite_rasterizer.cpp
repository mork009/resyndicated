/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
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

#include "sprite_rasterizer.h"
#include "tile_rasterizer.h"

#include <cmath>
#include <algorithm>
#include <cassert>
#include <cstring>

#include "rs-utils/io/file.h"
#include "rs-utils/log/log.h"
#include "rs-engine/gfx/animationmanager.h"
#include "rs-engine/gfx/base_weapon_sprites.h"
#include "rs-kernel/model/mapobject.h"
#include "rs-kernel/model/weapon.h"
#include "rs-kernel/model/ped.h"

namespace rs_win_editor {

SpriteRasterizer& SpriteRasterizer::instance() {
    static SpriteRasterizer s_instance;
    return s_instance;
}

bool SpriteRasterizer::load(bool forceReload) {
    if (isLoaded_ && !forceReload) return true;
    isLoaded_ = false;

    size_t tabSize = 0;
    uint8_t *tabData = rs_utl::File::loadOriginalFile("hspr-0.tab", tabSize);
    if (!tabData) {
        RSERR(Log::k_FLG_IO, "SpriteRasterizer", "load", ("Failed to load hspr-0.tab"));
        return false;
    }

    size_t datSize = 0;
    uint8_t *datData = rs_utl::File::loadOriginalFile("hspr-0.dat", datSize);
    if (!datData) {
        delete[] tabData;
        RSERR(Log::k_FLG_IO, "SpriteRasterizer", "load", ("Failed to load hspr-0.dat"));
        return false;
    }


    rs_eng::patchWeaponSprites(false, tabData, tabSize, datData, datSize);
    size_t count = tabSize / 6;

    sprites_.clear();
    sprites_.resize(count);

    for (size_t i = 0; i < count; ++i) {
        uint32_t offset = rs_utl::READ_LE_UINT32(tabData + i * 6);
        uint8_t w = tabData[i * 6 + 4];
        uint8_t h = tabData[i * 6 + 5];

        WinSprite &spr = sprites_[i];
        spr.width = w;
        spr.height = h;

        if (w == 0 || h == 0 || offset >= datSize) {
            continue;
        }

        spr.stride = (w + 7) / 8 * 8;
        spr.pixels.assign(spr.stride * h, 255); // 255 = transparent

        const uint8_t *srcBlocks = datData + offset;
        for (int row = 0; row < h; ++row) {
            uint8_t *dstRow = &spr.pixels[row * spr.stride];
            for (int col = 0; col < w; col += 8) {
                if (srcBlocks + 5 > datData + datSize) break;

                // Unpack 5-byte Bullfrog block (1 transparency bit + 4 color bitplanes for 8 pixels)
                for (int px = 0; px < 8; ++px) {
                    if (rs_utl::bitSet(srcBlocks[0], 7 - px)) {
                        dstRow[col + px] = 255; // transparent
                    } else {
                        dstRow[col + px] =
                            static_cast<uint8_t>((rs_utl::bitValue(srcBlocks[1], 7 - px) << 0) & 0xff) |
                            static_cast<uint8_t>((rs_utl::bitValue(srcBlocks[2], 7 - px) << 1) & 0xff) |
                            static_cast<uint8_t>((rs_utl::bitValue(srcBlocks[3], 7 - px) << 2) & 0xff) |
                            static_cast<uint8_t>((rs_utl::bitValue(srcBlocks[4], 7 - px) << 3) & 0xff);
                    }
                }
                srcBlocks += 5;
            }
        }
    }

    delete[] tabData;
    delete[] datData;

    isLoaded_ = true;
    LOG(Log::k_FLG_GFX, "SpriteRasterizer", "load", ("Successfully loaded %zu sprites from hspr-0", count));
    return true;
}

void SpriteRasterizer::drawSprite(uint32_t *dst, int dstW, int dstH, int spriteId,
                                  int x, int y, float scale, bool flipped, uint8_t alpha, int factionId) const {
    if (spriteId < 0 || spriteId >= static_cast<int>(sprites_.size())) return;

    const WinSprite &spr = sprites_[spriteId];
    if (spr.width <= 0 || spr.height <= 0) return;

    const auto &palette = TileRasterizer::instance().palette();

    if (std::abs(scale - 1.0f) < 0.01f) {
        int w = spr.width;
        int h = spr.height;
        if (x + w <= 0 || x >= dstW || y + h <= 0 || y >= dstH) return;

        int sxStart = (x < 0) ? -x : 0;
        int sxEnd   = (x + w > dstW) ? (dstW - x) : w;
        int syStart = (y < 0) ? -y : 0;
        int syEnd   = (y + h > dstH) ? (dstH - y) : h;

        for (int sy = syStart; sy < syEnd; ++sy) {
            const uint8_t *srcRow = &spr.pixels[sy * spr.stride];
            uint32_t *dstRow = &dst[(y + sy) * dstW + x];

            for (int sx = sxStart; sx < sxEnd; ++sx) {
                int srcX = flipped ? (w - 1 - sx) : sx;
                uint8_t pix = srcRow[srcX];
                if (pix == 255) continue;
                if (factionId >= 0 && spriteId >= 41 && spriteId <= 46 && pix == 6) {
                    uint8_t kFactionPaletteIndex[8] = { s_playerFactionColor_, 7, 14, 11, 11, 12, 13, 15 };
                    pix = kFactionPaletteIndex[factionId & 7];
                }

                uint32_t color = palette[pix];
                if (alpha == 255) {
                    dstRow[sx] = color;
                } else {
                    uint32_t bg = dstRow[sx];
                    uint32_t rb = (color & 0x00FF00FF) * alpha + (bg & 0x00FF00FF) * (256 - alpha);
                    uint32_t g  = (color & 0x0000FF00) * alpha + (bg & 0x0000FF00) * (256 - alpha);
                    dstRow[sx] = 0xFF000000 | ((rb >> 8) & 0x00FF00FF) | ((g >> 8) & 0x0000FF00);
                }
            }
        }
    } else {
        int scaledW = static_cast<int>(spr.width * scale);
        int scaledH = static_cast<int>(spr.height * scale);
        if (x + scaledW <= 0 || x >= dstW || y + scaledH <= 0 || y >= dstH) return;

        int dxStart = std::max(0, -x);
        int dxEnd   = std::min(scaledW, dstW - x);
        int dyStart = std::max(0, -y);
        int dyEnd   = std::min(scaledH, dstH - y);

        float invScale = 1.0f / scale;

        for (int dy = dyStart; dy < dyEnd; ++dy) {
            int sy = std::min(spr.height - 1, static_cast<int>(dy * invScale));
            const uint8_t *srcRow = &spr.pixels[sy * spr.stride];
            uint32_t *dstRow = &dst[(y + dy) * dstW + x];

            for (int dx = dxStart; dx < dxEnd; ++dx) {
                int sx = std::min(spr.width - 1, static_cast<int>(dx * invScale));
                int srcX = flipped ? (spr.width - 1 - sx) : sx;
                uint8_t pix = srcRow[srcX];
                if (pix == 255) continue;
                if (factionId >= 0 && spriteId >= 41 && spriteId <= 46 && pix == 6) {
                    uint8_t kFactionPaletteIndex[8] = { s_playerFactionColor_, 7, 14, 11, 11, 12, 13, 15 };
                    pix = kFactionPaletteIndex[factionId & 7];
                }

                uint32_t color = palette[pix];
                if (alpha == 255) {
                    dstRow[dx] = color;
                } else {
                    uint32_t bg = dstRow[dx];
                    uint32_t rb = (color & 0x00FF00FF) * alpha + (bg & 0x00FF00FF) * (256 - alpha);
                    uint32_t g  = (color & 0x0000FF00) * alpha + (bg & 0x0000FF00) * (256 - alpha);
                    dstRow[dx] = 0xFF000000 | ((rb >> 8) & 0x00FF00FF) | ((g >> 8) & 0x0000FF00);
                }
            }
        }
    }
}

bool SpriteRasterizer::drawAnimFrame(uint32_t *dst, int dstW, int dstH,
                                     uint16_t animId, int frameId,
                                     int anchorX, int anchorY,
                                     float scale, uint8_t alpha, int factionId) const {
    auto *animMgr = rs_eng::AnimationManager::singletonPtr();
    if (!animMgr) return false;

    return animMgr->forEachFrameElement(animId, frameId, [&](const rs_eng::GameSpriteFrameElement &e) {
        int elemX = anchorX + static_cast<int>(e.off_x * scale);
        int elemY = anchorY + static_cast<int>(e.off_y * scale);
        drawSprite(dst, dstW, dstH, e.sprite, elemX, elemY, scale, e.flipped, alpha, factionId);
    });
}

bool SpriteRasterizer::drawEntity(uint32_t *dst, int dstW, int dstH,
                                 const rs_knl::MapObject *obj,
                                 int anchorX, int anchorY,
                                 float scale, uint8_t alpha) const {
    if (!obj) return false;
    uint16_t animId = obj->getDrawAnimation();
    int frameId = obj->getDrawFrame();
    if (animId == 0) return false;
    int factionId = 0;
    const auto *pPed = dynamic_cast<const rs_knl::PedInstance*>(obj);
    if (pPed) {
        factionId = pPed->factionId();
    }
    return drawAnimFrame(dst, dstW, dstH, animId, frameId, anchorX, anchorY, scale, alpha, factionId);
}

bool SpriteRasterizer::getAnimFrameBounds(uint16_t animId, int frameId,
                                         int &minX, int &minY, int &maxX, int &maxY) const {
    auto *animMgr = rs_eng::AnimationManager::singletonPtr();
    if (!animMgr) return false;
    bool found = false;
    minX = 10000; minY = 10000;
    maxX = -10000; maxY = -10000;
    animMgr->forEachFrameElement(animId, frameId, [&](const rs_eng::GameSpriteFrameElement &e) {
        if (e.sprite >= 0 && e.sprite < static_cast<int>(sprites_.size())) {
            const auto &spr = sprites_[e.sprite];
            if (spr.width > 0 && spr.height > 0) {
                found = true;
                int left = e.off_x;
                int top = e.off_y;
                int right = left + spr.width;
                int bottom = top + spr.height;
                if (left < minX) minX = left;
                if (top < minY) minY = top;
                if (right > maxX) maxX = right;
                if (bottom > maxY) maxY = bottom;
            }
        }
    });
    return found;
}

bool SpriteRasterizer::loadMenuSprites(bool forceReload) {
    if (forceReload) {
        isLogosLoaded_ = false;
        logosData_.clear();
    }
    if (isMenuLoaded_ && !forceReload) return true;
    isMenuLoaded_ = false;

    // Load menu palette
    size_t palSize = 0;
    uint8_t *palData = rs_utl::File::loadOriginalFile("mselect.pal", palSize);
    if (!palData) {
        RSERR(Log::k_FLG_IO, "SpriteRasterizer", "loadMenuSprites", ("Failed to load mselect.pal"));
        return false;
    }

    menuPaletteBgra_.assign(256, 0x00000000);
    for (size_t i = 0; i < 256; ++i) {
        if (i * 3 + 2 < palSize) {
            uint8_t r = palData[i * 3 + 0];
            uint8_t g = palData[i * 3 + 1];
            uint8_t b = palData[i * 3 + 2];
            uint8_t r8 = static_cast<uint8_t>((r << 2) | (r >> 4));
            uint8_t g8 = static_cast<uint8_t>((g << 2) | (g >> 4));
            uint8_t b8 = static_cast<uint8_t>((b << 2) | (b >> 4));
            menuPaletteBgra_[i] = 0xFF000000 | (static_cast<uint32_t>(r8) << 16) | (static_cast<uint32_t>(g8) << 8) | static_cast<uint32_t>(b8);
        }
    }
    delete[] palData;

    // Load tab file
    size_t tabSize = 0;
    uint8_t *tabData = rs_utl::File::loadOriginalFile("mspr-0.tab", tabSize);
    if (!tabData) {
        RSERR(Log::k_FLG_IO, "SpriteRasterizer", "loadMenuSprites", ("Failed to load mspr-0.tab"));
        return false;
    }

    // Load dat file
    size_t datSize = 0;
    uint8_t *datData = rs_utl::File::loadOriginalFile("mspr-0.dat", datSize);
    if (!datData) {
        delete[] tabData;
        RSERR(Log::k_FLG_IO, "SpriteRasterizer", "loadMenuSprites", ("Failed to load mspr-0.dat"));
        return false;
    }


    rs_eng::patchWeaponSprites(true, tabData, tabSize, datData, datSize);
    size_t count = tabSize / 6;

    menuSprites_.clear();
    menuSprites_.resize(count);

    for (size_t i = 0; i < count; ++i) {
        uint32_t offset = rs_utl::READ_LE_UINT32(tabData + i * 6);
        uint8_t w = tabData[i * 6 + 4];
        uint8_t h = tabData[i * 6 + 5];

        WinSprite &spr = menuSprites_[i];
        spr.width = w;
        spr.height = h;

        if (w == 0 || h == 0 || offset >= datSize) {
            continue;
        }

        spr.stride = (w + 7) / 8 * 8;
        spr.pixels.assign(spr.stride * h, 255); // 255 = transparent

        const uint8_t *spriteBlocks = datData + offset;
        const uint8_t *datEnd = datData + datSize;

        for (int row = 0; row < h; ++row) {
            uint8_t *currentPixel = &spr.pixels[row * spr.stride];
            uint8_t *rowEnd = currentPixel + spr.stride;

            if (spriteBlocks >= datEnd) break;
            uint8_t b = *spriteBlocks++;
            int runLength = b < 128 ? b : -(256 - b);
            while (runLength != 0) {
                if (runLength > 0) {
                    // Pixel run
                    int toCopy = runLength;
                    if (currentPixel + toCopy > rowEnd) {
                        toCopy = static_cast<int>(rowEnd - currentPixel);
                    }
                    if (spriteBlocks + runLength > datEnd) break;
                    for (int j = 0; j < toCopy; ++j) {
                        *currentPixel++ = *spriteBlocks++;
                    }
                    if (runLength > toCopy) {
                        spriteBlocks += (runLength - toCopy);
                    }
                } else {
                    // Transparent run (runLength < 0)
                    int clearLen = -runLength;
                    if (currentPixel + clearLen > rowEnd) {
                        clearLen = static_cast<int>(rowEnd - currentPixel);
                    }
                    std::memset(currentPixel, 255, clearLen);
                    currentPixel += clearLen;
                }

                if (spriteBlocks >= datEnd) break;
                b = *spriteBlocks++;
                runLength = b < 128 ? b : -(256 - b);
            }
        }
    }

    delete[] tabData;
    delete[] datData;

    isMenuLoaded_ = true;
    LOG(Log::k_FLG_GFX, "SpriteRasterizer", "loadMenuSprites", ("Successfully loaded %zu menu sprites from mspr-0", count));
    return true;
}

HBITMAP SpriteRasterizer::createWeaponIconBitmap(int weaponType, int targetW, int targetH) const {
    if (!isLoaded_) {
        const_cast<SpriteRasterizer*>(this)->load();
    }
    if (sprites_.empty()) return nullptr;

    const auto &palette = TileRasterizer::instance().palette();
    if (palette[1] == 0) {
        TileRasterizer::instance().load(1);
    }

    int spriteId = -1;
    switch (weaponType) {
        case rs_knl::Weapon::Pistol:       spriteId = 1603; break;
        case rs_knl::Weapon::GaussGun:     spriteId = 1604; break;
        case rs_knl::Weapon::Shotgun:      spriteId = 1605; break;
        case rs_knl::Weapon::Uzi:          spriteId = 1606; break;
        case rs_knl::Weapon::Minigun:      spriteId = 1607; break;
        case rs_knl::Weapon::Laser:        spriteId = 1608; break;
        case rs_knl::Weapon::Flamer:       spriteId = 1609; break;
        case rs_knl::Weapon::LongRange:    spriteId = 1610; break;
        case rs_knl::Weapon::Scanner:      spriteId = 1611; break;
        case rs_knl::Weapon::MediKit:      spriteId = 1612; break;
        case rs_knl::Weapon::TimeBomb:     spriteId = 1613; break;
        case rs_knl::Weapon::AccessCard:   spriteId = 1614; break;
        case rs_knl::Weapon::EnergyShield: spriteId = 1618; break;
        case rs_knl::Weapon::Persuadatron: spriteId = 1602; break;
        case rs_knl::Weapon::AirStrike:    spriteId = 1615; break;
        case rs_knl::Weapon::CloneShield:  spriteId = 1617; break;

        default:
            return nullptr;
    }

    if (spriteId < 0 || spriteId >= static_cast<int>(sprites_.size())) return nullptr;
    const WinSprite &spr = sprites_[spriteId];
    if (spr.width <= 1 || spr.height <= 1) return nullptr;

    std::vector<uint32_t> iconPixels(32 * 32, 0x00000000);
    for (int y = 0; y < 32 && y < spr.height; ++y) {
        const uint8_t *srcRow = &spr.pixels[y * spr.stride];
        for (int x = 0; x < 32 && x < spr.width; ++x) {
            uint8_t pix = srcRow[x];
            if (pix != 255 && pix < palette.size()) {
                iconPixels[y * 32 + x] = palette[pix];
            }
        }
    }

    if (false && weaponType == rs_knl::Weapon::AirStrike) {
        static const std::pair<int, int> kJet[] = {
            {16, 10},
            {15, 11}, {16, 11}, {17, 11},
            {15, 12}, {16, 12}, {17, 12},
            {14, 13}, {15, 13}, {16, 13}, {17, 13}, {18, 13},
            {14, 14}, {15, 14}, {16, 14}, {17, 14}, {18, 14},
            {13, 15}, {14, 15}, {15, 15}, {16, 15}, {17, 15}, {18, 15}, {19, 15},
            {12, 16}, {13, 16}, {14, 16}, {15, 16}, {16, 16}, {17, 16}, {18, 16}, {19, 16}, {20, 16},
            {11, 17}, {12, 17}, {13, 17}, {14, 17}, {15, 17}, {16, 17}, {17, 17}, {18, 17}, {19, 17}, {20, 17}, {21, 17},
            {10, 18}, {11, 18}, {12, 18}, {13, 18}, {14, 18}, {15, 18}, {16, 18}, {17, 18}, {18, 18}, {19, 18}, {20, 18}, {21, 18}, {22, 18},
            {9, 19}, {10, 19}, {11, 19}, {12, 19}, {13, 19}, {14, 19}, {15, 19}, {16, 19}, {17, 19}, {18, 19}, {19, 19}, {20, 19}, {21, 19}, {22, 19}, {23, 19},
            {8, 20}, {9, 20}, {10, 20}, {11, 20}, {12, 20}, {13, 20}, {14, 20}, {15, 20}, {16, 20}, {17, 20}, {18, 20}, {19, 20}, {20, 20}, {21, 20}, {22, 20}, {23, 20}, {24, 20},
            {7, 21}, {8, 21}, {9, 21}, {10, 21}, {11, 21}, {12, 21}, {13, 21}, {19, 21}, {20, 21}, {21, 21}, {22, 21}, {23, 21}, {24, 21}, {25, 21},
            {6, 22}, {7, 22}, {8, 22}, {9, 22}, {10, 22}, {22, 22}, {23, 22}, {24, 22}, {25, 22}, {26, 22},
            {15, 22}, {16, 22}, {17, 22},
            {16, 23},
            {16, 25}, {15, 26}, {16, 26}, {17, 26}, {16, 27}
        };
        auto inJet = [](int x, int y) {
            for (const auto &p : kJet) {
                if (p.first == x && p.second == y) return true;
            }
            return false;
        };
        for (const auto &p : kJet) {
            int x = p.first;
            int y = p.second;
            if (x < 0 || x >= 32 || y < 0 || y >= 32) continue;
            uint32_t col = palette[6]; // C_IN
            if (!inJet(x - 1, y) || !inJet(x, y - 1)) {
                col = palette[1]; // C_HI
            } else if (!inJet(x + 1, y) || !inJet(x, y + 1)) {
                col = palette[3]; // C_SH
            }
            iconPixels[y * 32 + x] = col;
        }
    } else if (false && weaponType == rs_knl::Weapon::CloneShield) {
        static const std::pair<int, int> kClone[] = {
            // head
            {15, 14}, {16, 14},
            {15, 15}, {16, 15},
            // neck
            {15, 16}, {16, 16},
            // shoulders & torso
            {13, 17}, {14, 17}, {15, 17}, {16, 17}, {17, 17}, {18, 17},
            {13, 18}, {14, 18}, {15, 18}, {16, 18}, {17, 18}, {18, 18},
            {14, 19}, {15, 19}, {16, 19}, {17, 19},
            {14, 20}, {15, 20}, {16, 20}, {17, 20},
            {14, 21}, {15, 21}, {16, 21}, {17, 21},
            // legs
            {14, 22}, {15, 22}, {16, 22}, {17, 22},
            {14, 23}, {15, 23}, {16, 23}, {17, 23},
            {14, 24}, {15, 24}, {16, 24}, {17, 24},
            {13, 25}, {14, 25}, {17, 25}, {18, 25}
        };
        auto inClone = [](int x, int y) {
            for (const auto &p : kClone) {
                if (p.first == x && p.second == y) return true;
            }
            return false;
        };
        for (const auto &p : kClone) {
            int x = p.first;
            int y = p.second;
            if (x < 0 || x >= 32 || y < 0 || y >= 32) continue;
            uint32_t col = palette[6]; // C_IN
            if (!inClone(x - 1, y) || !inClone(x, y - 1)) {
                col = palette[1]; // C_HI
            } else if (!inClone(x + 1, y) || !inClone(x, y + 1)) {
                col = palette[3]; // C_SH
            }
            iconPixels[y * 32 + x] = col;
        }
    }

    int outW = (targetW > 0) ? targetW : 32;
    int outH = (targetH > 0) ? targetH : 32;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = outW;
    bmi.bmiHeader.biHeight = -outH; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(nullptr);
    uint32_t *pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);

    if (!hBmp || !pBits) return nullptr;

    if (outW == 32 && outH == 32) {
        std::memcpy(pBits, iconPixels.data(), 32 * 32 * sizeof(uint32_t));
    } else {
        std::fill(pBits, pBits + (outW * outH), 0x00000000);
        float scale = std::min(static_cast<float>(outW) / 32.0f, static_cast<float>(outH) / 32.0f);
        int drawW = std::clamp(static_cast<int>(32 * scale), 1, outW);
        int drawH = std::clamp(static_cast<int>(32 * scale), 1, outH);
        int startX = (outW - drawW) / 2;
        int startY = (outH - drawH) / 2;
        float invScale = 1.0f / scale;
        for (int dy = 0; dy < drawH; ++dy) {
            int sy = std::min(31, static_cast<int>(dy * invScale));
            const uint32_t *srcRow = &iconPixels[sy * 32];
            uint32_t *dstRow = &pBits[(startY + dy) * outW + startX];
            for (int dx = 0; dx < drawW; ++dx) {
                int sx = std::min(31, static_cast<int>(dx * invScale));
                dstRow[dx] = srcRow[sx];
            }
        }
    }

    return hBmp;
}

bool SpriteRasterizer::loadLogos() {
    if (isLogosLoaded_) return true;
    size_t size = 0;
    uint8_t *data = rs_utl::File::loadOriginalFile("mlogos.dat", size);
    if (!data) {
        RSERR(Log::k_FLG_IO, "SpriteRasterizer", "loadLogos", ("Failed to load mlogos.dat"));
        return false;
    }
    logosData_.assign(data, data + size);
    delete[] data;
    isLogosLoaded_ = true;
    return true;
}

HBITMAP SpriteRasterizer::createLogoBitmap(int logoId, int colorIdx, int targetW, int targetH) {
    if (!isLogosLoaded_) {
        loadLogos();
    }
    if (!isMenuLoaded_) {
        loadMenuSprites();
    }
    if (logosData_.empty() || menuPaletteBgra_.empty()) {
        return nullptr;
    }

    if (logoId < 0 || logoId >= 40) logoId = 0;
    colorIdx = std::clamp(colorIdx, 0, 7);

    static constexpr uint8_t kColours[8] = { 6, 7, 14, 3, 11, 12, 13, 15 };
    uint8_t palIdx = kColours[colorIdx];
    uint32_t bgColor = (palIdx < menuPaletteBgra_.size()) ? menuPaletteBgra_[palIdx] : 0xFF880000;

    int outW = (targetW > 0) ? targetW : 64;
    int outH = (targetH > 0) ? targetH : 64;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = outW;
    bmi.bmiHeader.biHeight = -outH; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC hdcScreen = GetDC(nullptr);
    uint32_t *pBits = nullptr;
    HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);

    if (!hBmp || !pBits) return nullptr;

    // Fill background with company color
    std::fill(pBits, pBits + (outW * outH), bgColor);

    // Each logo is 32x32 = 1024 bytes
    const uint8_t *logoSrc = logosData_.data() + (logoId * 1024);
    if ((logoId + 1) * 1024 > static_cast<int>(logosData_.size())) {
        return hBmp;
    }

    float scaleX = static_cast<float>(outW) / 32.0f;
    float scaleY = static_cast<float>(outH) / 32.0f;

    for (int y = 0; y < outH; ++y) {
        int srcY = std::min(31, static_cast<int>(y / scaleY));
        const uint8_t *srcRow = &logoSrc[srcY * 32];
        uint32_t *dstRow = &pBits[y * outW];

        for (int x = 0; x < outW; ++x) {
            int srcX = std::min(31, static_cast<int>(x / scaleX));
            uint8_t pix = srcRow[srcX];
            if (pix != 254 && pix < menuPaletteBgra_.size()) {
                dstRow[x] = menuPaletteBgra_[pix];
            }
        }
    }

    return hBmp;
}

} // namespace rs_win_editor
