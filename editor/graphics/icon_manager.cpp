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

#include "icon_manager.h"
#include "resource.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace rs_win_editor {

// Helpers to plot anti-aliased or blended pixels into 32-bit BGRA buffer
static inline void setPixel(uint32_t *buf, int w, int h, int x, int y, uint32_t color) {
    if (x >= 0 && x < w && y >= 0 && y < h) {
        buf[y * w + x] = color;
    }
}

static inline void blendPixel(uint32_t *buf, int w, int h, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x < 0 || x >= w || y < 0 || y >= h || a == 0) return;
    uint32_t &dst = buf[y * w + x];
    uint8_t da = (dst >> 24) & 0xFF;
    uint8_t dr = (dst >> 16) & 0xFF;
    uint8_t dg = (dst >> 8) & 0xFF;
    uint8_t db = dst & 0xFF;

    float alpha = a / 255.0f;
    float invAlpha = 1.0f - alpha;

    uint8_t outR = static_cast<uint8_t>(r * alpha + dr * invAlpha);
    uint8_t outG = static_cast<uint8_t>(g * alpha + dg * invAlpha);
    uint8_t outB = static_cast<uint8_t>(b * alpha + db * invAlpha);
    uint8_t outA = static_cast<uint8_t>(std::max<int>(da, a));

    dst = (static_cast<uint32_t>(outA) << 24) |
          (static_cast<uint32_t>(outR) << 16) |
          (static_cast<uint32_t>(outG) << 8) |
          static_cast<uint32_t>(outB);
}

static void fillCircle(uint32_t *buf, int w, int h, float cx, float cy, float r, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) {
    int minX = std::max(0, static_cast<int>(cx - r - 1));
    int maxX = std::min(w - 1, static_cast<int>(cx + r + 1));
    int minY = std::max(0, static_cast<int>(cy - r - 1));
    int maxY = std::min(h - 1, static_cast<int>(cy + r + 1));

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            float dist = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));
            if (dist <= r - 0.5f) {
                blendPixel(buf, w, h, x, y, red, green, blue, alpha);
            } else if (dist <= r + 0.5f) {
                float f = (r + 0.5f - dist);
                blendPixel(buf, w, h, x, y, red, green, blue, static_cast<uint8_t>(alpha * f));
            }
        }
    }
}

static void fillRect(uint32_t *buf, int w, int h, int rx, int ry, int rw, int rh, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) {
    for (int y = ry; y < ry + rh; ++y) {
        for (int x = rx; x < rx + rw; ++x) {
            blendPixel(buf, w, h, x, y, red, green, blue, alpha);
        }
    }
}

static void drawLine(uint32_t *buf, int w, int h, int x0, int y0, int x1, int y1, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        blendPixel(buf, w, h, x0, y0, red, green, blue, alpha);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

IconManager& IconManager::instance() {
    static IconManager s_mgr;
    return s_mgr;
}

IconManager::~IconManager() {
    if (hToolbarImageList_) {
        ImageList_Destroy(hToolbarImageList_);
        hToolbarImageList_ = nullptr;
    }
    if (hTabsImageList_) {
        ImageList_Destroy(hTabsImageList_);
        hTabsImageList_ = nullptr;
    }
}

bool IconManager::loadMasterResource() {
    if (pMasterIcons_) return true;
    HINSTANCE hInst = GetModuleHandle(nullptr);
    HRSRC hResInfo = FindResourceW(hInst, MAKEINTRESOURCEW(IDR_ICONS_MASTER), RT_RCDATA);
    if (!hResInfo) return false;
    HGLOBAL hRes = LoadResource(hInst, hResInfo);
    if (!hRes) return false;
    DWORD resSize = SizeofResource(hInst, hResInfo);
    if (resSize < static_cast<DWORD>(kMasterIconSize * kMasterIconSize * sizeof(uint32_t))) return false;
    pMasterIcons_ = reinterpret_cast<const uint32_t*>(LockResource(hRes));
    masterIconCount_ = static_cast<int>(resSize / (kMasterIconSize * kMasterIconSize * sizeof(uint32_t)));
    return (pMasterIcons_ != nullptr && masterIconCount_ > 0);
}

bool IconManager::renderMasterIcon(uint32_t *dst, int size, IconId id, uint32_t bgPixel) {
    if (!loadMasterResource()) return false;
    int idx = static_cast<int>(id);
    if (idx < 0 || idx >= masterIconCount_) return false;

    const uint32_t *srcIcon = pMasterIcons_ + idx * (kMasterIconSize * kMasterIconSize);

    if (size == 48) {
        std::memcpy(dst, srcIcon, 48 * 48 * sizeof(uint32_t));
    } else if (size == 24) {
        for (int y = 0; y < 24; ++y) {
            for (int x = 0; x < 24; ++x) {
                const uint32_t p00 = srcIcon[(y * 2) * 48 + (x * 2)];
                const uint32_t p10 = srcIcon[(y * 2) * 48 + (x * 2 + 1)];
                const uint32_t p01 = srcIcon[(y * 2 + 1) * 48 + (x * 2)];
                const uint32_t p11 = srcIcon[(y * 2 + 1) * 48 + (x * 2 + 1)];

                uint32_t b = (((p00) & 0xFF) + ((p10) & 0xFF) + ((p01) & 0xFF) + ((p11) & 0xFF) + 2) >> 2;
                uint32_t g = (((p00 >> 8) & 0xFF) + ((p10 >> 8) & 0xFF) + ((p01 >> 8) & 0xFF) + ((p11 >> 8) & 0xFF) + 2) >> 2;
                uint32_t r = (((p00 >> 16) & 0xFF) + ((p10 >> 16) & 0xFF) + ((p01 >> 16) & 0xFF) + ((p11 >> 16) & 0xFF) + 2) >> 2;
                uint32_t a = (((p00 >> 24) & 0xFF) + ((p10 >> 24) & 0xFF) + ((p01 >> 24) & 0xFF) + ((p11 >> 24) & 0xFF) + 2) >> 2;

                dst[y * 24 + x] = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
    } else if (size == 16) {
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                uint32_t sumB = 0, sumG = 0, sumR = 0, sumA = 0;
                for (int sy = 0; sy < 3; ++sy) {
                    for (int sx = 0; sx < 3; ++sx) {
                        uint32_t p = srcIcon[(y * 3 + sy) * 48 + (x * 3 + sx)];
                        sumB += (p & 0xFF);
                        sumG += ((p >> 8) & 0xFF);
                        sumR += ((p >> 16) & 0xFF);
                        sumA += ((p >> 24) & 0xFF);
                    }
                }
                uint32_t b = (sumB + 4) / 9;
                uint32_t g = (sumG + 4) / 9;
                uint32_t r = (sumR + 4) / 9;
                uint32_t a = (sumA + 4) / 9;

                dst[y * 16 + x] = (a << 24) | (r << 16) | (g << 8) | b;
            }
        }
    } else {
        // General area-averaging resampler for any other size (e.g. 20x20 in art panel buttons)
        float scale = 48.0f / size;
        for (int y = 0; y < size; ++y) {
            float sy0 = y * scale;
            float sy1 = (y + 1) * scale;
            int isy0 = static_cast<int>(sy0);
            int isy1 = std::min(47, static_cast<int>(std::ceil(sy1)));

            for (int x = 0; x < size; ++x) {
                float sx0 = x * scale;
                float sx1 = (x + 1) * scale;
                int isx0 = static_cast<int>(sx0);
                int isx1 = std::min(47, static_cast<int>(std::ceil(sx1)));

                float totalWeight = 0.0f;
                float sumB = 0.0f, sumG = 0.0f, sumR = 0.0f, sumA = 0.0f;

                for (int sy = isy0; sy <= isy1; ++sy) {
                    float wy = 1.0f;
                    if (sy < sy0) wy -= (sy0 - sy);
                    if (sy + 1 > sy1) wy -= (sy + 1 - sy1);
                    wy = std::max(0.0f, wy);

                    for (int sx = isx0; sx <= isx1; ++sx) {
                        float wx = 1.0f;
                        if (sx < sx0) wx -= (sx0 - sx);
                        if (sx + 1 > sx1) wx -= (sx + 1 - sx1);
                        wx = std::max(0.0f, wx);

                        float w = wx * wy;
                        if (w > 0.0f) {
                            uint32_t p = srcIcon[sy * 48 + sx];
                            sumB += (p & 0xFF) * w;
                            sumG += ((p >> 8) & 0xFF) * w;
                            sumR += ((p >> 16) & 0xFF) * w;
                            sumA += ((p >> 24) & 0xFF) * w;
                            totalWeight += w;
                        }
                    }
                }

                if (totalWeight > 0.0001f) {
                    uint32_t b = static_cast<uint32_t>(std::clamp(sumB / totalWeight + 0.5f, 0.0f, 255.0f));
                    uint32_t g = static_cast<uint32_t>(std::clamp(sumG / totalWeight + 0.5f, 0.0f, 255.0f));
                    uint32_t r = static_cast<uint32_t>(std::clamp(sumR / totalWeight + 0.5f, 0.0f, 255.0f));
                    uint32_t a = static_cast<uint32_t>(std::clamp(sumA / totalWeight + 0.5f, 0.0f, 255.0f));
                    dst[y * size + x] = (a << 24) | (r << 16) | (g << 8) | b;
                } else {
                    dst[y * size + x] = 0;
                }
            }
        }
    }

    if ((bgPixel >> 24) != 0) {
        uint8_t bgB = bgPixel & 0xFF;
        uint8_t bgG = (bgPixel >> 8) & 0xFF;
        uint8_t bgR = (bgPixel >> 16) & 0xFF;
        for (int i = 0; i < size * size; ++i) {
            uint32_t p = dst[i];
            uint8_t a = (p >> 24) & 0xFF;
            if (a == 255) continue;
            uint8_t r = (p >> 16) & 0xFF;
            uint8_t g = (p >> 8) & 0xFF;
            uint8_t b = p & 0xFF;

            uint8_t invA = 255 - a;
            uint8_t outB = static_cast<uint8_t>(std::min(255, b + ((bgB * invA + 127) / 255)));
            uint8_t outG = static_cast<uint8_t>(std::min(255, g + ((bgG * invA + 127) / 255)));
            uint8_t outR = static_cast<uint8_t>(std::min(255, r + ((bgR * invA + 127) / 255)));
            dst[i] = (0xFF << 24) | (outR << 16) | (outG << 8) | outB;
        }
    }

    return true;
}

void IconManager::initialize(int iconSize) {
    iconSize_ = iconSize;
    if (hToolbarImageList_) ImageList_Destroy(hToolbarImageList_);
    if (hTabsImageList_) ImageList_Destroy(hTabsImageList_);

    loadMasterResource();

    hToolbarImageList_ = ImageList_Create(iconSize_, iconSize_, ILC_COLOR32 | ILC_MASK, static_cast<int>(IconId::Count), 4);
    hTabsImageList_ = ImageList_Create(24, 24, ILC_COLOR32 | ILC_MASK, 4, 2);

    for (int i = 0; i < static_cast<int>(IconId::Count); ++i) {
        HBITMAP hBmp = createIconBitmap(static_cast<IconId>(i), iconSize_);
        ImageList_Add(hToolbarImageList_, hBmp, nullptr);
        DeleteObject(hBmp);
    }

    // Add Tab icons (24x24 matching toolbar buttons)
    IconId tabIds[] = {IconId::TabTiles, IconId::TabEntities, IconId::TabObjectives, IconId::TabProperties};
    for (IconId tid : tabIds) {
        HBITMAP hBmp = createIconBitmap(tid, 24);
        ImageList_Add(hTabsImageList_, hBmp, nullptr);
        DeleteObject(hBmp);
    }
}

HBITMAP IconManager::createIconBitmap(IconId id, int size, uint32_t bgPixel) {
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = size;
    bmi.bmiHeader.biHeight = -size; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    uint32_t *pPixels = nullptr;
    HDC hdcScreen = GetDC(nullptr);
    HBITMAP hBitmap = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&pPixels), nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);

    if (pPixels) {
        if (!renderMasterIcon(pPixels, size, id, bgPixel)) {
            std::fill(pPixels, pPixels + size * size, bgPixel);
            drawIcon(pPixels, size, id);
        }
    }

    return hBitmap;
}



void IconManager::drawIcon(uint32_t *p, int s, IconId id) {
    float sc = s / 24.0f; // Baseline scale factor relative to 24px

    switch (id) {
        case IconId::New: {
            // White document with folded top-right corner
            fillRect(p, s, s, static_cast<int>(5 * sc), static_cast<int>(3 * sc), static_cast<int>(14 * sc), static_cast<int>(18 * sc), 230, 235, 240);
            fillRect(p, s, s, static_cast<int>(6 * sc), static_cast<int>(4 * sc), static_cast<int>(12 * sc), static_cast<int>(16 * sc), 255, 255, 255);
            // Document lines
            fillRect(p, s, s, static_cast<int>(8 * sc), static_cast<int>(8 * sc), static_cast<int>(8 * sc), static_cast<int>(1.5f * sc), 70, 140, 220);
            fillRect(p, s, s, static_cast<int>(8 * sc), static_cast<int>(11 * sc), static_cast<int>(8 * sc), static_cast<int>(1.5f * sc), 140, 160, 180);
            fillRect(p, s, s, static_cast<int>(8 * sc), static_cast<int>(14 * sc), static_cast<int>(6 * sc), static_cast<int>(1.5f * sc), 140, 160, 180);
            break;
        }

        case IconId::Open: {
            // Warm folder
            fillRect(p, s, s, static_cast<int>(3 * sc), static_cast<int>(6 * sc), static_cast<int>(8 * sc), static_cast<int>(3 * sc), 240, 180, 50);
            fillRect(p, s, s, static_cast<int>(3 * sc), static_cast<int>(8 * sc), static_cast<int>(18 * sc), static_cast<int>(12 * sc), 255, 200, 60);
            fillRect(p, s, s, static_cast<int>(5 * sc), static_cast<int>(10 * sc), static_cast<int>(15 * sc), static_cast<int>(9 * sc), 255, 225, 110);
            break;
        }

        case IconId::Save: {
            // Blue/slate floppy disk body
            fillRect(p, s, s, static_cast<int>(4 * sc), static_cast<int>(4 * sc), static_cast<int>(16 * sc), static_cast<int>(16 * sc), 30, 90, 160);
            // White label at top (upright floppy disk)
            fillRect(p, s, s, static_cast<int>(6 * sc), static_cast<int>(5 * sc), static_cast<int>(12 * sc), static_cast<int>(7 * sc), 250, 250, 250);
            // Label marker lines
            fillRect(p, s, s, static_cast<int>(8 * sc), static_cast<int>(7 * sc), static_cast<int>(8 * sc), static_cast<int>(1.5f * sc), 100, 140, 200);
            fillRect(p, s, s, static_cast<int>(8 * sc), static_cast<int>(9.5f * sc), static_cast<int>(6 * sc), static_cast<int>(1.0f * sc), 100, 140, 200);
            // Silver metal shutter slider at bottom
            fillRect(p, s, s, static_cast<int>(7 * sc), static_cast<int>(14 * sc), static_cast<int>(10 * sc), static_cast<int>(6 * sc), 210, 215, 225);
            // Shutter hole/window
            fillRect(p, s, s, static_cast<int>(9 * sc), static_cast<int>(15 * sc), static_cast<int>(2.5f * sc), static_cast<int>(4 * sc), 40, 40, 40);
            break;
        }

        case IconId::Undo: {
            // Crisp curved arrow pointing left
            // Arrowhead at left: apex (4, 10), base at x=8 from y=6 to y=14
            for (int i = 0; i <= 4; ++i) {
                int halfH = i;
                drawLine(p, s, s, static_cast<int>((4 + i) * sc), static_cast<int>((10 - halfH) * sc),
                         static_cast<int>((4 + i) * sc), static_cast<int>((10 + halfH) * sc), 40, 190, 240);
            }
            // Curved body sweeping up and over to the right
            for (float t = 0; t <= 1.0f; t += 0.05f) {
                // Bezier arc from (8, 10) through control (15, 4) to (17, 16)
                float u = 1.0f - t;
                float px = u * u * 8.0f + 2.0f * u * t * 16.0f + t * t * 17.0f;
                float py = u * u * 10.0f + 2.0f * u * t * 4.0f + t * t * 16.0f;
                fillRect(p, s, s, static_cast<int>((px - 1.0f) * sc), static_cast<int>((py - 1.0f) * sc),
                         static_cast<int>(2.6f * sc), static_cast<int>(2.6f * sc), 40, 190, 240);
            }
            break;
        }

        case IconId::Redo: {
            // Crisp curved arrow pointing right
            // Arrowhead at right: apex (20, 10), base at x=16 from y=6 to y=14
            for (int i = 0; i <= 4; ++i) {
                int halfH = i;
                drawLine(p, s, s, static_cast<int>((20 - i) * sc), static_cast<int>((10 - halfH) * sc),
                         static_cast<int>((20 - i) * sc), static_cast<int>((10 + halfH) * sc), 40, 190, 240);
            }
            // Curved body sweeping up and over to the left
            for (float t = 0; t <= 1.0f; t += 0.05f) {
                float u = 1.0f - t;
                float px = u * u * 16.0f + 2.0f * u * t * 8.0f + t * t * 7.0f;
                float py = u * u * 10.0f + 2.0f * u * t * 4.0f + t * t * 16.0f;
                fillRect(p, s, s, static_cast<int>((px - 1.0f) * sc), static_cast<int>((py - 1.0f) * sc),
                         static_cast<int>(2.6f * sc), static_cast<int>(2.6f * sc), 40, 190, 240);
            }
            break;
        }

        case IconId::Pencil: {
            // Angled yellow pencil
            for (int i = 0; i < 11; ++i) {
                int x = static_cast<int>((7 + i) * sc);
                int y = static_cast<int>((17 - i) * sc);
                fillRect(p, s, s, x, y, static_cast<int>(4 * sc), static_cast<int>(4 * sc), 245, 190, 30);
            }
            // Dark graphite tip
            fillRect(p, s, s, static_cast<int>(5 * sc), static_cast<int>(18 * sc), static_cast<int>(3 * sc), static_cast<int>(3 * sc), 50, 50, 50);
            // Pink eraser
            fillRect(p, s, s, static_cast<int>(17 * sc), static_cast<int>(6 * sc), static_cast<int>(4 * sc), static_cast<int>(4 * sc), 235, 110, 130);
            break;
        }

        case IconId::BoxFill: {
            // Cyan dashed/bordered box with semi-transparent fill
            fillRect(p, s, s, static_cast<int>(4 * sc), static_cast<int>(4 * sc), static_cast<int>(16 * sc), static_cast<int>(16 * sc), 40, 200, 240, 50);
            for (int i = 0; i < 16; i += 3) {
                fillRect(p, s, s, static_cast<int>((4 + i) * sc), static_cast<int>(4 * sc), static_cast<int>(2 * sc), static_cast<int>(2 * sc), 40, 220, 255);
                fillRect(p, s, s, static_cast<int>((4 + i) * sc), static_cast<int>(18 * sc), static_cast<int>(2 * sc), static_cast<int>(2 * sc), 40, 220, 255);
                fillRect(p, s, s, static_cast<int>(4 * sc), static_cast<int>((4 + i) * sc), static_cast<int>(2 * sc), static_cast<int>(2 * sc), 40, 220, 255);
                fillRect(p, s, s, static_cast<int>(18 * sc), static_cast<int>((4 + i) * sc), static_cast<int>(2 * sc), static_cast<int>(2 * sc), 40, 220, 255);
            }
            break;
        }

        case IconId::Select: {
            // Pointer cursor
            for (int i = 0; i < 12; ++i) {
                drawLine(p, s, s, static_cast<int>(6 * sc), static_cast<int>((5 + i) * sc),
                         static_cast<int>((6 + i * 0.7f) * sc), static_cast<int>((5 + i) * sc), 240, 245, 250);
            }
            // Border
            drawLine(p, s, s, static_cast<int>(5 * sc), static_cast<int>(4 * sc), static_cast<int>(5 * sc), static_cast<int>(18 * sc), 20, 30, 40);
            drawLine(p, s, s, static_cast<int>(5 * sc), static_cast<int>(4 * sc), static_cast<int>(16 * sc), static_cast<int>(13 * sc), 20, 30, 40);
            drawLine(p, s, s, static_cast<int>(5 * sc), static_cast<int>(18 * sc), static_cast<int>(16 * sc), static_cast<int>(13 * sc), 20, 30, 40);
            break;
        }

        case IconId::Slice: {
            // Stacked isometric layers with center one glowing
            auto drawLayer = [&](int yOff, uint8_t r, uint8_t g, uint8_t b) {
                drawLine(p, s, s, static_cast<int>(12 * sc), static_cast<int>((yOff) * sc), static_cast<int>(19 * sc), static_cast<int>((yOff + 3.5f) * sc), r, g, b);
                drawLine(p, s, s, static_cast<int>(19 * sc), static_cast<int>((yOff + 3.5f) * sc), static_cast<int>(12 * sc), static_cast<int>((yOff + 7) * sc), r, g, b);
                drawLine(p, s, s, static_cast<int>(12 * sc), static_cast<int>((yOff + 7) * sc), static_cast<int>(5 * sc), static_cast<int>((yOff + 3.5f) * sc), r, g, b);
                drawLine(p, s, s, static_cast<int>(5 * sc), static_cast<int>((yOff + 3.5f) * sc), static_cast<int>(12 * sc), static_cast<int>((yOff) * sc), r, g, b);
            };
            drawLayer(4, 90, 110, 130);
            drawLayer(9, 30, 240, 120); // Active layer glowing green!
            drawLayer(14, 90, 110, 130);
            break;
        }

        case IconId::LayerDown: {
            // Down arrow over isometric tile
            fillRect(p, s, s, static_cast<int>(11 * sc), static_cast<int>(5 * sc), static_cast<int>(3 * sc), static_cast<int>(8 * sc), 50, 210, 240);
            for (int i = 0; i < 6; ++i) {
                drawLine(p, s, s, static_cast<int>((12.5f - i) * sc), static_cast<int>((12 + i) * sc),
                         static_cast<int>((12.5f + i) * sc), static_cast<int>((12 + i) * sc), 50, 210, 240);
            }
            break;
        }

        case IconId::LayerUp: {
            // Up arrow over isometric tile
            fillRect(p, s, s, static_cast<int>(11 * sc), static_cast<int>(11 * sc), static_cast<int>(3 * sc), static_cast<int>(8 * sc), 50, 210, 240);
            for (int i = 0; i < 6; ++i) {
                drawLine(p, s, s, static_cast<int>((12.5f - i) * sc), static_cast<int>((11 - i) * sc),
                         static_cast<int>((12.5f + i) * sc), static_cast<int>((11 - i) * sc), 50, 210, 240);
            }
            break;
        }

        case IconId::Spawns: {
            // Reticle with 4 colored agent diamonds
            fillCircle(p, s, s, 12 * sc, 12 * sc, 6 * sc, 60, 80, 100, 100);
            // 4 diamonds: Red (top), Blue (right), Yellow (bottom), Green (left)
            fillCircle(p, s, s, 12 * sc, 6 * sc, 2.5f * sc, 240, 50, 50);    // Agent 1 Red
            fillCircle(p, s, s, 18 * sc, 12 * sc, 2.5f * sc, 50, 120, 255);  // Agent 2 Blue
            fillCircle(p, s, s, 12 * sc, 18 * sc, 2.5f * sc, 255, 220, 40);  // Agent 3 Yellow
            fillCircle(p, s, s, 6 * sc, 12 * sc, 2.5f * sc, 40, 220, 60);   // Agent 4 Green
            break;
        }

        case IconId::Playtest: {
            // Glowing lime-green Play triangle pointing right (>)
            for (int x = 0; x < 12; ++x) {
                int halfH = static_cast<int>((11 - x) * 0.7f);
                drawLine(p, s, s, static_cast<int>((7 + x) * sc), static_cast<int>((12 - halfH) * sc),
                         static_cast<int>((7 + x) * sc), static_cast<int>((12 + halfH) * sc), 20, 235, 60);
            }
            break;
        }

        case IconId::ZoomIn: {
            fillCircle(p, s, s, 10 * sc, 10 * sc, 6 * sc, 60, 180, 240);
            fillCircle(p, s, s, 10 * sc, 10 * sc, 4 * sc, 20, 30, 40);
            // Handle
            for (int i = 0; i < 6; ++i) {
                fillRect(p, s, s, static_cast<int>((14 + i) * sc), static_cast<int>((14 + i) * sc), static_cast<int>(2.5f * sc), static_cast<int>(2.5f * sc), 140, 160, 180);
            }
            // Plus
            fillRect(p, s, s, static_cast<int>(8 * sc), static_cast<int>(9.5f * sc), static_cast<int>(5 * sc), static_cast<int>(1.5f * sc), 255, 255, 255);
            fillRect(p, s, s, static_cast<int>(9.5f * sc), static_cast<int>(8 * sc), static_cast<int>(1.5f * sc), static_cast<int>(5 * sc), 255, 255, 255);
            break;
        }

        case IconId::ZoomOut: {
            fillCircle(p, s, s, 10 * sc, 10 * sc, 6 * sc, 60, 180, 240);
            fillCircle(p, s, s, 10 * sc, 10 * sc, 4 * sc, 20, 30, 40);
            // Handle
            for (int i = 0; i < 6; ++i) {
                fillRect(p, s, s, static_cast<int>((14 + i) * sc), static_cast<int>((14 + i) * sc), static_cast<int>(2.5f * sc), static_cast<int>(2.5f * sc), 140, 160, 180);
            }
            // Minus
            fillRect(p, s, s, static_cast<int>(8 * sc), static_cast<int>(9.5f * sc), static_cast<int>(5 * sc), static_cast<int>(1.5f * sc), 255, 255, 255);
            break;
        }

        case IconId::Zoom100: {
            fillCircle(p, s, s, 10 * sc, 10 * sc, 6 * sc, 60, 180, 240);
            fillCircle(p, s, s, 10 * sc, 10 * sc, 4 * sc, 20, 30, 40);
            for (int i = 0; i < 6; ++i) {
                fillRect(p, s, s, static_cast<int>((14 + i) * sc), static_cast<int>((14 + i) * sc), static_cast<int>(2.5f * sc), static_cast<int>(2.5f * sc), 140, 160, 180);
            }
            fillRect(p, s, s, static_cast<int>(9.5f * sc), static_cast<int>(8 * sc), static_cast<int>(1.5f * sc), static_cast<int>(5 * sc), 255, 255, 255);
            break;
        }

        case IconId::TabTiles: {
            // 4-tile checkered pattern
            fillRect(p, s, s, static_cast<int>(3 * sc), static_cast<int>(3 * sc), static_cast<int>(4 * sc), static_cast<int>(4 * sc), 50, 200, 240);
            fillRect(p, s, s, static_cast<int>(9 * sc), static_cast<int>(3 * sc), static_cast<int>(4 * sc), static_cast<int>(4 * sc), 40, 120, 180);
            fillRect(p, s, s, static_cast<int>(3 * sc), static_cast<int>(9 * sc), static_cast<int>(4 * sc), static_cast<int>(4 * sc), 40, 120, 180);
            fillRect(p, s, s, static_cast<int>(9 * sc), static_cast<int>(9 * sc), static_cast<int>(4 * sc), static_cast<int>(4 * sc), 50, 200, 240);
            break;
        }

        case IconId::TabEntities: {
            // Person silhouette
            fillCircle(p, s, s, 8 * sc, 5 * sc, 2.5f * sc, 255, 210, 60);
            fillRect(p, s, s, static_cast<int>(5 * sc), static_cast<int>(8 * sc), static_cast<int>(6 * sc), static_cast<int>(6 * sc), 50, 180, 240);
            break;
        }

        case IconId::TabObjectives: {
            // Target bullseye
            fillCircle(p, s, s, 8 * sc, 8 * sc, 6 * sc, 240, 70, 70);
            fillCircle(p, s, s, 8 * sc, 8 * sc, 4 * sc, 255, 255, 255);
            fillCircle(p, s, s, 8 * sc, 8 * sc, 2 * sc, 240, 70, 70);
            break;
        }

        case IconId::TabProperties: {
            // Info / Gear badge
            fillCircle(p, s, s, 8 * sc, 8 * sc, 6 * sc, 70, 160, 240);
            fillRect(p, s, s, static_cast<int>(7 * sc), static_cast<int>(4 * sc), static_cast<int>(2 * sc), static_cast<int>(2 * sc), 255, 255, 255);
            fillRect(p, s, s, static_cast<int>(7 * sc), static_cast<int>(7 * sc), static_cast<int>(2 * sc), static_cast<int>(5 * sc), 255, 255, 255);
            break;
        }

        case IconId::Search: {
            // Magnifying glass in cyberpunk cyan & amber style
            float cx = 10.0f * sc;
            float cy = 10.0f * sc;
            float rOuter = 5.6f * sc;
            float rInner = 3.4f * sc;
            // Translucent cyan glass fill
            fillCircle(p, s, s, cx, cy, rInner, 30, 180, 220, 60);
            // Glass reflection highlight
            fillCircle(p, s, s, cx - 1.4f * sc, cy - 1.4f * sc, 1.2f * sc, 230, 245, 255, 180);
            // Cyan metallic rim
            for (int y = static_cast<int>(cy - rOuter - 1); y <= static_cast<int>(cy + rOuter + 1); ++y) {
                for (int x = static_cast<int>(cx - rOuter - 1); x <= static_cast<int>(cx + rOuter + 1); ++x) {
                    float d = std::sqrt((x - cx)*(x - cx) + (y - cy)*(y - cy));
                    if (d >= rInner && d <= rOuter) {
                        blendPixel(p, s, s, x, y, 0, 229, 255, 240);
                    }
                }
            }
            // Amber diagonal handle from (13.5, 13.5) extending down-right to (19.5, 19.5)
            for (float t = 0.0f; t <= 6.5f; t += 0.5f) {
                float hx = (13.5f + t) * sc;
                float hy = (13.5f + t) * sc;
                fillRect(p, s, s, static_cast<int>(hx - 1.0f * sc), static_cast<int>(hy - 1.0f * sc),
                         static_cast<int>(2.5f * sc), static_cast<int>(2.5f * sc), 245, 190, 30);
            }
            break;
        }

        default:
            break;
    }
}

} // namespace rs_win_editor
