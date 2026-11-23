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

#ifndef EDITOR_SPRITE_RASTERIZER_H_
#define EDITOR_SPRITE_RASTERIZER_H_

#include <windows.h>
#include <cstdint>
#include <vector>

namespace rs_knl {
class MapObject;
}

namespace rs_win_editor {

struct WinSprite {
    int width = 0;
    int height = 0;
    int stride = 0;
    std::vector<uint8_t> pixels; // 0..15 color indices into mission palette, 255 = transparent
};

class SpriteRasterizer {
public:
    static SpriteRasterizer& instance();
    ~SpriteRasterizer() = default;

    bool load(bool forceReload = false);
    bool isLoaded() const { return isLoaded_; }

    const WinSprite* getSprite(int id) const {
        if (id < 0 || id >= static_cast<int>(sprites_.size())) return nullptr;
        return &sprites_[id];
    }

    size_t spriteCount() const { return sprites_.size(); }

    //! Draw a single sprite into a 32-bit BGRA buffer
    void drawSprite(uint32_t *dst, int dstW, int dstH, int spriteId,
                    int x, int y, float scale, bool flipped, uint8_t alpha = 255, int factionId = 0) const;

    //! Draw an animation frame composed of elements into a 32-bit BGRA buffer
    bool drawAnimFrame(uint32_t *dst, int dstW, int dstH,
                       uint16_t animId, int frameId,
                       int anchorX, int anchorY,
                       float scale, uint8_t alpha = 255, int factionId = 0) const;

    //! Draw any MapObject (Ped, Vehicle, Static) at its screen anchor point
    bool drawEntity(uint32_t *dst, int dstW, int dstH,
                    const rs_knl::MapObject *obj,
                    int anchorX, int anchorY,
                    float scale, uint8_t alpha = 255) const;

    //! Calculate bounding box of an animation frame relative to anchor (0, 0)
    bool getAnimFrameBounds(uint16_t animId, int frameId,
                            int &minX, int &minY, int &maxX, int &maxY) const;

    //! Menu sprites (mspr-0) and weapon icon rendering
    bool loadMenuSprites(bool forceReload = false);
    bool hasMenuSprites() const { return !menuSprites_.empty(); }
    const WinSprite* getMenuSprite(int id) const {
        if (id < 0 || id >= static_cast<int>(menuSprites_.size())) return nullptr;
        return &menuSprites_[id];
    }
    size_t menuSpriteCount() const { return menuSprites_.size(); }

    //! Creates a 32-bit ARGB HBITMAP for a weapon's small icon from mspr-0
    HBITMAP createWeaponIconBitmap(int weaponType, int targetW, int targetH) const;

    static void setPlayerFactionColor(uint8_t color) { s_playerFactionColor_ = color; }
    static uint8_t playerFactionColor() { return s_playerFactionColor_; }

    //! Logo sprites (mlogos.dat) management
    bool loadLogos();
    bool hasLogos() const { return isLogosLoaded_; }
    HBITMAP createLogoBitmap(int logoId, int colorIdx, int targetW = 64, int targetH = 64);

private:
    SpriteRasterizer() = default;

private:
    std::vector<WinSprite> sprites_;
    bool isLoaded_ = false;

    std::vector<WinSprite> menuSprites_;
    std::vector<uint32_t> menuPaletteBgra_;
    bool isMenuLoaded_ = false;

    std::vector<uint8_t> logosData_;
    bool isLogosLoaded_ = false;
    static inline uint8_t s_playerFactionColor_ = 6;
};

} // namespace rs_win_editor

#endif // EDITOR_SPRITE_RASTERIZER_H_
