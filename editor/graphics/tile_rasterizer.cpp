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

#include "tile_rasterizer.h"

#include <cstring>
#include <format>
#include <algorithm>

#include "rs-utils/io/file.h"
#include "rs-utils/common.h"

namespace rs_win_editor {

static void unpackBlocks4(const uint8_t *data, uint8_t *pixels) {
    for (int j = 0; j < 4; ++j) {
        for (int i = 0; i < 8; ++i) {
            if (rs_utl::bitSet(data[j], 7 - i)) {
                pixels[j * 8 + i] = 255; // transparent
            } else {
                pixels[j * 8 + i] =
                    static_cast<uint8_t>(((rs_utl::bitValue(data[4 + j], 7 - i) << 0) & 0xff) |
                                         ((rs_utl::bitValue(data[8 + j], 7 - i) << 1) & 0xff) |
                                         ((rs_utl::bitValue(data[12 + j], 7 - i) << 2) & 0xff) |
                                         ((rs_utl::bitValue(data[16 + j], 7 - i) << 3) & 0xff));
            }
        }
    }
}

TileRasterizer& TileRasterizer::instance() {
    static TileRasterizer s_instance;
    return s_instance;
}

TileRasterizer::~TileRasterizer() {
    for (auto &t : tiles_) {
        if (t.hThumb) {
            DeleteObject(t.hThumb);
            t.hThumb = nullptr;
        }
    }
}

bool TileRasterizer::setPalette(int paletteId) {
    if (paletteId < 1 || paletteId > 5) {
        paletteId = 1;
    }
    currentPaletteId_ = paletteId;

    uint8_t basePaletteData[256 * 3];
    std::memset(basePaletteData, 0, sizeof(basePaletteData));

    // Base shared palette hpal04.dat for indices 16..255
    size_t baseSize = 0;
    uint8_t *loadedBase = rs_utl::File::loadOriginalFile("hpal04.dat", baseSize);
    if (loadedBase) {
        size_t copyLen = std::min(baseSize, sizeof(basePaletteData));
        std::memcpy(basePaletteData, loadedBase, copyLen);
        delete[] loadedBase;
    }

    std::string fname = std::format("hpal0{}.dat", paletteId);
    size_t size = 0;
    uint8_t *paletteData = rs_utl::File::loadOriginalFile(fname, size);
    if (!paletteData) {
        return false;
    }

    for (size_t i = 0; i < 256; ++i) {
        uint8_t r = 0, g = 0, b = 0;
        if (i >= 16 && (paletteId >= 1 && paletteId <= 3)) {
            r = basePaletteData[i * 3 + 0];
            g = basePaletteData[i * 3 + 1];
            b = basePaletteData[i * 3 + 2];
        } else if (i * 3 + 2 < size) {
            r = paletteData[i * 3 + 0];
            g = paletteData[i * 3 + 1];
            b = paletteData[i * 3 + 2];
        }

        // Convert 6-bit (0..63) to 8-bit (0..255)
        uint8_t r8 = (r << 2) | (r >> 4);
        uint8_t g8 = (g << 2) | (g >> 4);
        uint8_t b8 = (b << 2) | (b >> 4);

        // Win32 32-bit DIB pixel format: 0xAARRGGBB (where AA=0xFF for opaque)
        paletteBgra_[i] = 0xFF000000 | (static_cast<uint32_t>(r8) << 16) | (static_cast<uint32_t>(g8) << 8) | b8;
    }

    delete[] paletteData;

    // Refresh 32-bit BGRA array for all tiles
    for (auto &tile : tiles_) {
        for (int i = 0; i < 64 * 48; ++i) {
            uint8_t idx = tile.pixels[i];
            if (idx == 255) {
                tile.bgra[i] = 0x00000000; // Transparent
            } else {
                tile.bgra[i] = paletteBgra_[idx];
            }
        }
    }

    rebuildThumbnails();
    return true;
}

bool TileRasterizer::load(int paletteId) {
    size_t blkSize = 0;
    uint8_t *tilesData = rs_utl::File::loadOriginalFile("hblk01.dat", blkSize);
    if (!tilesData) {
        return false;
    }

    size_t colSize = 0;
    uint8_t *typesData = rs_utl::File::loadOriginalFile("col01.dat", colSize);

    const int kSubTileWidth = 32;
    const int kSubTileHeight = 16;
    const int kSubTilePerWidth = 2;
    const int kSubTilePerHeight = 3;
    const int kTileIndexSize = 24;
    const int kSubTileRowLength = 20;

    for (int id = 0; id < 256; ++id) {
        WinTile &tile = tiles_[id];
        tile.id = static_cast<uint8_t>(id);
        tile.type = (typesData && id < static_cast<int>(colSize)) ? typesData[id] : 0;
        tile.pixels.fill(255);

        int offset = id * kTileIndexSize;
        for (int i = 0; i < kSubTilePerWidth; ++i) {
            for (int j = 0; j < kSubTilePerHeight; ++j) {
                int subTileOffset = rs_utl::READ_LE_INT32(tilesData + offset + (i * kSubTilePerHeight + j) * 4);
                if (subTileOffset < 6144) continue;

                const uint8_t *subData = tilesData + subTileOffset;
                int startY = j * kSubTileHeight;
                int startX = i * kSubTileWidth;

                for (int row = 0; row < kSubTileHeight; ++row) {
                    int pixelY = startY + row;
                    unpackBlocks4(subData, &tile.pixels[pixelY * 64 + startX]);
                    subData += kSubTileRowLength;
                }
            }
        }

        // Determine if tile has any non-transparent pixel
        tile.notAlpha = false;
        for (int i = 0; i < 64 * 48; ++i) {
            if (tile.pixels[i] != 255) {
                tile.notAlpha = true;
                break;
            }
        }
    }

    delete[] tilesData;
    if (typesData) delete[] typesData;

    setPalette(paletteId);
    isLoaded_ = true;
    return true;
}

void TileRasterizer::rebuildThumbnails() {
    HDC hdcScreen = GetDC(nullptr);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = 64;
    bmi.bmiHeader.biHeight = -48; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    for (auto &tile : tiles_) {
        if (tile.hThumb) {
            DeleteObject(tile.hThumb);
            tile.hThumb = nullptr;
        }

        uint32_t *pBits = nullptr;
        HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pBits), nullptr, 0);
        if (hBmp && pBits) {
            // Render thumbnail with dark background for transparent areas
            for (int i = 0; i < 64 * 48; ++i) {
                uint32_t c = tile.bgra[i];
                if ((c & 0xFF000000) != 0) {
                    pBits[i] = c;
                } else {
                    // Checkered dark grey background for transparent pixels
                    int x = i % 64;
                    int y = i / 64;
                    bool check = ((x / 8) + (y / 8)) % 2 == 0;
                    pBits[i] = check ? 0xFF181C18 : 0xFF222822;
                }
            }
            tile.hThumb = hBmp;
        }
    }

    ReleaseDC(nullptr, hdcScreen);
}

void TileRasterizer::drawTile(uint32_t *dst, int dstW, int dstH, uint8_t tileId, int x, int y) const {
    if (x <= -64 || x >= dstW || y <= -48 || y >= dstH) return;

    const WinTile &tile = tiles_[tileId];
    if (!tile.notAlpha) return;

    int sxStart = (x < 0) ? -x : 0;
    int sxEnd   = (x + 64 > dstW) ? (dstW - x) : 64;
    int syStart = (y < 0) ? -y : 0;
    int syEnd   = (y + 48 > dstH) ? (dstH - y) : 48;

    for (int sy = syStart; sy < syEnd; ++sy) {
        const uint32_t *srcRow = &tile.bgra[sy * 64];
        uint32_t *dstRow = &dst[(y + sy) * dstW + x];

        for (int sx = sxStart; sx < sxEnd; ++sx) {
            uint32_t pix = srcRow[sx];
            if ((pix & 0xFF000000) != 0) {
                dstRow[sx] = pix;
            }
        }
    }
}

void TileRasterizer::drawTileScaled(uint32_t *dst, int dstW, int dstH, uint8_t tileId, int x, int y, float scale) const {
    if (std::abs(scale - 1.0f) < 0.01f) {
        drawTile(dst, dstW, dstH, tileId, x, y);
        return;
    }

    int scaledW = static_cast<int>(64 * scale);
    int scaledH = static_cast<int>(48 * scale);
    if (x <= -scaledW || x >= dstW || y <= -scaledH || y >= dstH) return;

    const WinTile &tile = tiles_[tileId];
    if (!tile.notAlpha) return;

    int dxStart = std::max(0, -x);
    int dxEnd   = std::min(scaledW, dstW - x);
    int dyStart = std::max(0, -y);
    int dyEnd   = std::min(scaledH, dstH - y);

    float invScale = 1.0f / scale;

    for (int dy = dyStart; dy < dyEnd; ++dy) {
        int sy = std::min(47, static_cast<int>(dy * invScale));
        const uint32_t *srcRow = &tile.bgra[sy * 64];
        uint32_t *dstRow = &dst[(y + dy) * dstW + x];

        for (int dx = dxStart; dx < dxEnd; ++dx) {
            int sx = std::min(63, static_cast<int>(dx * invScale));
            uint32_t pix = srcRow[sx];
            if ((pix & 0xFF000000) != 0) {
                dstRow[dx] = pix;
            }
        }
    }
}

void TileRasterizer::drawTileScaledAlpha(uint32_t *dst, int dstW, int dstH, uint8_t tileId, int x, int y, float scale, uint8_t alpha) const {
    int scaledW = static_cast<int>(64 * scale);
    int scaledH = static_cast<int>(48 * scale);
    if (x <= -scaledW || x >= dstW || y <= -scaledH || y >= dstH) return;

    const WinTile &tile = tiles_[tileId];
    if (!tile.notAlpha) return;

    int dxStart = std::max(0, -x);
    int dxEnd   = std::min(scaledW, dstW - x);
    int dyStart = std::max(0, -y);
    int dyEnd   = std::min(scaledH, dstH - y);

    float invScale = 1.0f / scale;
    uint32_t invAlpha = 255 - alpha;

    for (int dy = dyStart; dy < dyEnd; ++dy) {
        int sy = std::min(47, static_cast<int>(dy * invScale));
        const uint32_t *srcRow = &tile.bgra[sy * 64];
        uint32_t *dstRow = &dst[(y + dy) * dstW + x];

        for (int dx = dxStart; dx < dxEnd; ++dx) {
            int sx = std::min(63, static_cast<int>(dx * invScale));
            uint32_t sPix = srcRow[sx];
            if ((sPix & 0xFF000000) != 0) {
                uint32_t dPix = dstRow[dx];
                uint32_t sb = (sPix & 0xFF);
                uint32_t sg = ((sPix >> 8) & 0xFF);
                uint32_t sr = ((sPix >> 16) & 0xFF);

                uint32_t db = (dPix & 0xFF);
                uint32_t dg = ((dPix >> 8) & 0xFF);
                uint32_t dr = ((dPix >> 16) & 0xFF);

                uint32_t rb = (sb * alpha + db * invAlpha) >> 8;
                uint32_t rg = (sg * alpha + dg * invAlpha) >> 8;
                uint32_t rr = (sr * alpha + dr * invAlpha) >> 8;

                dstRow[dx] = 0xFF000000 | (rr << 16) | (rg << 8) | rb;
            }
        }
    }
}

TileCategory TileRasterizer::getTileCategory(uint8_t id) const {
    uint8_t type = tiles_[id].type;
    switch (type) {
        case 0x05: // Ground
            return TileCategory::Ground;
        case 0x06: // RoadSideEW
        case 0x07: // RoadSideWE
        case 0x08: // RoadSideSN
        case 0x09: // RoadSideNS
        case 0x0B: // RoadCurve
        case 0x0E: // RoadPedCross
        case 0x0F: // RoadMark
            return TileCategory::Roads;
        case 0x0A: // Wall
            return TileCategory::Walls;
        case 0x0D: // Roof
            return TileCategory::Roofs;
        case 0x01: // SlopeSN
        case 0x02: // SlopeNS
        case 0x03: // SlopeEW
        case 0x04: // SlopeWE
            return TileCategory::Stairs;
        case 0x0C: // Handrail
        case 0x10: // TrainStop
        case 0x00: // Empty / None
        default:
            return TileCategory::Specials;
    }
}

std::vector<uint8_t> TileRasterizer::getTilesForCategory(TileCategory cat) const {
    std::vector<uint8_t> result;
    for (int i = 0; i < 256; ++i) {
        uint8_t tid = static_cast<uint8_t>(i);
        if (cat == TileCategory::All || getTileCategory(tid) == cat) {
            result.push_back(tid);
        }
    }
    if (result.empty()) result.push_back(0);
    return result;
}

std::string TileRasterizer::getTileTypeName(uint8_t type) const {
    switch (type) {
        case 0x01: return "SlopeSN";
        case 0x02: return "SlopeNS";
        case 0x03: return "SlopeEW";
        case 0x04: return "SlopeWE";
        case 0x05: return "Ground";
        case 0x06: return "RoadSideEW";
        case 0x07: return "RoadSideWE";
        case 0x08: return "RoadSideSN";
        case 0x09: return "RoadSideNS";
        case 0x0A: return "Wall";
        case 0x0B: return "RoadCurve";
        case 0x0C: return "Handrail";
        case 0x0D: return "Roof";
        case 0x0E: return "RoadPedCross";
        case 0x0F: return "RoadMark";
        case 0x10: return "TrainStop";
        default:   return "Empty";
    }
}

bool TileRasterizer::isTileWalkableSurface(uint8_t id) const {
    if (id == 0x80 || id == 0x81) return true; // Train platforms
    if (id == 0x8F || id == 0x93) return false;
    uint8_t type = tiles_[id].type;
    switch (type) {
        case 0x01: // SlopeSN
        case 0x02: // SlopeNS
        case 0x03: // SlopeEW
        case 0x04: // SlopeWE
        case 0x05: // Ground
        case 0x06: // RoadSideEW
        case 0x07: // RoadSideWE
        case 0x08: // RoadSideSN
        case 0x09: // RoadSideNS
        case 0x0B: // RoadCurve
        case 0x0D: // Roof
        case 0x0E: // RoadPedCross
        case 0x0F: // RoadMark
            return true;
        default:
            return false;
    }
}

} // namespace rs_win_editor
