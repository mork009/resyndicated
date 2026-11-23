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

#ifndef EDITOR_TILE_RASTERIZER_H_
#define EDITOR_TILE_RASTERIZER_H_

#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <array>

namespace rs_win_editor {

enum class TileCategory {
    All = 0,
    Ground,
    Roads,
    Walls,
    Roofs,
    Stairs,
    Specials
};

struct WinTile {
    uint8_t id = 0;
    uint8_t type = 0;
    bool notAlpha = false;
    std::array<uint8_t, 64 * 48> pixels; // 0..15 palette indices, 255 = transparent
    std::array<uint32_t, 64 * 48> bgra;   // 32-bit BGRA (0x00000000 = transparent)
    HBITMAP hThumb = nullptr;            // GDI 64x48 thumbnail for Art Panel
};

class TileRasterizer {
public:
    static TileRasterizer& instance();
    ~TileRasterizer();

    bool load(int paletteId = 1);
    bool setPalette(int paletteId);

    const WinTile& getTile(uint8_t id) const { return tiles_[id]; }
    WinTile& getTile(uint8_t id) { return tiles_[id]; }

    int currentPaletteId() const { return currentPaletteId_; }
    const std::array<uint32_t, 256>& palette() const { return paletteBgra_; }

    void drawTile(uint32_t *dst, int dstW, int dstH, uint8_t tileId, int x, int y) const;
    void drawTileScaled(uint32_t *dst, int dstW, int dstH, uint8_t tileId, int x, int y, float scale) const;
    void drawTileScaledAlpha(uint32_t *dst, int dstW, int dstH, uint8_t tileId, int x, int y, float scale, uint8_t alpha = 160) const;

    TileCategory getTileCategory(uint8_t id) const;
    std::vector<uint8_t> getTilesForCategory(TileCategory cat) const;
    std::string getTileTypeName(uint8_t type) const;
    bool isTileWalkableSurface(uint8_t id) const;

private:
    TileRasterizer() = default;
    void rebuildThumbnails();

private:
    std::array<WinTile, 256> tiles_;
    std::array<uint32_t, 256> paletteBgra_;
    int currentPaletteId_ = 1;
    bool isLoaded_ = false;
};

} // namespace rs_win_editor

#endif // EDITOR_TILE_RASTERIZER_H_
