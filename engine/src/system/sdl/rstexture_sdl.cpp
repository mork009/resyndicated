/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
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

#include "rstexture_sdl.h"

#include <format>

#include "rs-utils/log/log.h"

namespace rs_eng {

/*!
 * @brief 
 * @param pRenderer 
 */
RSTextureSDL::RSTextureSDL(SDL_Renderer *pRenderer) {
    pRenderer_ = pRenderer;
    pSurface_ = nullptr;
    pTexture_ = nullptr;
    width_ = 0;
    height_ = 0;
}

/*!
 * @brief 
 */
RSTextureSDL::~RSTextureSDL() {
    freeSurface();
    freeTexture();
    pRenderer_ = nullptr;
}

/*!
 * @brief 
 */
void RSTextureSDL::freeSurface() {
    if (pSurface_ != nullptr) {
        SDL_DestroySurface(pSurface_);
        pSurface_ = nullptr;
        width_ = 0;
        height_ = 0;
        hasColorKey_ = false;
        colorKey_ = 0;
    }
}

/*!
 * @brief 
 */
void RSTextureSDL::freeTexture() {
    if (pTexture_ != nullptr) {
        SDL_DestroyTexture(pTexture_);
        pTexture_ = nullptr;
    }
}

/*!
 * @brief 
 * @param src 
 * @param dst 
 * @param width 
 * @param height 
 */
void RSTextureSDL::render(Point2D src, Point2D dst, int width, int height) {
    // Set rendering space and render to screen
    SDL_FRect renderQuad = { (float)dst.x, (float)dst.y, (float)width, (float)height };
    SDL_FRect tileQuad = { (float)src.x, (float)src.y, (float)width, (float)height };
    SDL_RenderTexture(pRenderer_, pTexture_, &tileQuad, &renderQuad);
}

void RSTextureSDL::renderStretch(Point2D src, Point2D dst, int width, int height, int ratio) {
    // Set rendering space and render to screen
    SDL_FRect renderQuad = { (float)dst.x, (float)dst.y, (float)(width * ratio), (float)(height * ratio) };
    SDL_FRect tileQuad = { (float)src.x, (float)src.y, (float)width, (float)height };
    SDL_RenderTexture(pRenderer_, pTexture_, &tileQuad, &renderQuad);
}

/*!
 * Render a texture clip to a destination rect and position with possible ratio and horizontal flip.
 * @param src Origin of the clip in source texture
 * @param dst Position on the screen to copy the texture
 * @param destWidth Width of destination
 * @param destHeight Height of destination
 * @param ratio A multiplication factor that change the size of destination rect
 * @param flipped True means to flip the texture in horizontal
 */
void RSTextureSDL::renderExtended(Point2D src, Point2D dst, int width, int height, int ratio, bool flipped) {
    // Set rendering space and render to screen
    SDL_FRect renderQuad = { (float)dst.x, (float)dst.y, (float)(width * ratio), (float)(height * ratio) };
    SDL_FRect tileQuad = { (float)src.x, (float)src.y, (float)width, (float)height };
    SDL_FlipMode flip = flipped ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_RenderTextureRotated(pRenderer_, pTexture_, &tileQuad, &renderQuad, 0.0, NULL, flip);
}

/*!
 * Render this texture as a whole to the target at position (0,0) and with the given size.
 * @param destWidth Width of destination
 * @param destHeight Height of destination
 */
void RSTextureSDL::renderFullTextureStrech(int destWidth, int destHeight) {
    // Set rendering space and render to screen
    SDL_FRect renderQuad = { 0.0f, 0.0f, (float)destWidth, (float)destHeight };
    SDL_FRect tileQuad = { 0.0f, 0.0f, (float)width_, (float)height_ };
    SDL_RenderTexture(pRenderer_, pTexture_, &tileQuad, &renderQuad);
}

void RSTextureSDL::renderStretchedDest(Point2D dst, int destWidth, int destHeight) {
    SDL_FRect renderQuad = { (float)dst.x, (float)dst.y, (float)destWidth, (float)destHeight };
    SDL_FRect tileQuad = { 0.0f, 0.0f, (float)width_, (float)height_ };
    SDL_RenderTexture(pRenderer_, pTexture_, &tileQuad, &renderQuad);
}

/*!
 * Create a texture with pixel format of RGBA8888 for a stream access.
 * @param width Width of the new texture
 * @param height Height of the new texture
 * @return true if creation is ok
 */
bool RSTextureSDL::createStreamingTexture(int width, int height) {
    freeTexture();
    
    pTexture_ = SDL_CreateTexture(pRenderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (pTexture_ == nullptr) {
        RSERR(Log::k_FLG_GFX, "RSTextureSDL", "createStreamingTexture", ("Critical error, Could create texture! SDL Error : %s", SDL_GetError()))
    } else {
        SDL_SetTextureScaleMode(pTexture_, SDL_SCALEMODE_NEAREST);
    }

    width_ = width;
    height_ = height;

    return pTexture_ != nullptr;
}

/*!
 * Update the whole texture with the given pixels.
 * Pixels are expected to be index in the given color palette.
 * @param pixels 
 * @param colorPalette 
 * @return 
 */
bool RSTextureSDL::updateStreamingTexture(const uint8_t *pixels, const rs_eng::Palette &colorPalette) {
    int pitch = 0;
    void *destVoid = nullptr;
    if (!SDL_LockTexture(pTexture_, nullptr, &destVoid, &pitch)) {
        RSERR(Log::k_FLG_GFX, "RSTextureSDL", "updateStreamingTexture", ("Critical error, Could not lock texture! SDL Error : %s", SDL_GetError()))
        return false;
    }

    Uint32 *destPixels = static_cast<Uint32*>(destVoid);
    const uint8_t *srcPixels = pixels;
    const SDL_PixelFormatDetails *details = SDL_GetPixelFormatDetails(SDL_PIXELFORMAT_RGBA8888);
    int pitchPixels = pitch / sizeof(Uint32);

    for (int y = 0; y < height_; y++) {
        const uint8_t *srcRow = srcPixels + y * width_;
        Uint32 *dstRow = destPixels + y * pitchPixels;
        for (int x = 0; x < width_; x++) {
            uint8_t index = srcRow[x];
            rs_eng::RSColor color = colorPalette[index];
            dstRow[x] = SDL_MapRGBA(details, nullptr, color.r, color.g, color.b, color.a);
        }
    }

    SDL_UnlockTexture(pTexture_);

    return true;
}

/*!
 * @brief 
 * @param width 
 * @param height 
 * @return 
 */
void RSTextureSDL::createRenderTargetTexture(int width, int height) {
    freeTexture();

    pTexture_ = SDL_CreateTexture(pRenderer_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, width, height);
    if (pTexture_ == nullptr) {
        throw InitializationFailedException(std::format("Critical error, Could not create texture! SDL Error : %s", SDL_GetError()));
    }
    SDL_SetTextureScaleMode(pTexture_, SDL_SCALEMODE_NEAREST);

    width_ = width;
    height_ = height;
}

/*!
 * Set this texture to be the target
 * @return true if operation succeeded
 */
bool RSTextureSDL::setAsRenderTarget() {
    if (!SDL_SetRenderTarget(pRenderer_, pTexture_)) {
        RSERR(Log::k_FLG_GFX, "RSTextureSDL", "setAsRenderTarget", ("Critical error, Could not set target texture! SDL Error : %s", SDL_GetError()))
        return false;
    }

    return true;
}

/*!
 * Copy an array of pixel in the surface stored inside this class.
 * The surface is an 8 bits pixel so an empty palette is created.
 * @param srcPixels Array of pixel. Must be the same size as width and height
 * @param width Width of the surface
 * @param height Height of the surface
 * @param colorKey The index in the palette of the color key to use
 * @return True if import is ok
 */
bool RSTextureSDL::create8bitsSurfaceFromData(const uint8_t *srcPixels, int width, int height, uint8_t colorKey) {
    LOG(Log::k_FLG_GFX, "RSTextureSDL", "importSurface", ("Importing pixels in surface..."))
    bool res = true;
    // Initialize an indexed surface
    freeSurface();
    pSurface_ = SDL_CreateSurface(width, height, SDL_PIXELFORMAT_INDEX8);
    if (pSurface_ == nullptr) {
        RSERR(Log::k_FLG_GFX, "RSTextureSDL", "importSurface", ("Critical error, Tileset surface could not be created! SDL Error : %s", SDL_GetError()))
        return false;
    }
    width_ = width;
    height_ = height;
    colorKey_ = colorKey;
    hasColorKey_ = true;

    if (SDL_MUSTLOCK(pSurface_) && !SDL_LockSurface(pSurface_)) {
        res = false;
        RSERR(Log::k_FLG_GFX, "RSTextureSDL", "importSurface", ("Critical error, Could not lock surface! SDL Error : %s", SDL_GetError()))
    }

    if (res) {
        Uint8 *dstPixels = static_cast<Uint8*>(pSurface_->pixels);
        int pitch = pSurface_->pitch;
        for (int y = 0; y < height; y++) {
            std::memcpy(dstPixels + y * pitch, srcPixels + y * width, width);
        }

        if (SDL_MUSTLOCK(pSurface_)) {
            SDL_UnlockSurface(pSurface_);
        }
    }
    // Set the color at given index in the palette as a transparent color
    SDL_SetSurfaceColorKey(pSurface_, true, colorKey);
    
    return res;
}

/*!
 * Sets a new palette. This texture must have been created with create8bitsSurfaceFromData().
 * @param palette A palette of color
 * @return True if everything is ok
 */
bool RSTextureSDL::setPalette(const rs_eng::Palette &palette) {
    SDL_Color sdlPalette[rs_eng::kPaletteMaxColor];
    size_t i = 0;

    for (const auto& color : palette) {
        sdlPalette[i].r = color.r;
        sdlPalette[i].g = color.g;
        sdlPalette[i].b = color.b;
        sdlPalette[i].a = color.a;
        i++;
    }

    if (hasColorKey_ && colorKey_ < rs_eng::kPaletteMaxColor) {
        sdlPalette[colorKey_].r = 0;
        sdlPalette[colorKey_].g = 0;
        sdlPalette[colorKey_].b = 0;
        sdlPalette[colorKey_].a = 0;
    }

    SDL_Palette *surfPalette = SDL_GetSurfacePalette(pSurface_);
    if (!surfPalette) {
        surfPalette = SDL_CreateSurfacePalette(pSurface_);
    }

    if (!surfPalette || !SDL_SetPaletteColors(surfPalette, sdlPalette, 0, rs_eng::kPaletteMaxColor)) {
        RSERR(Log::k_FLG_GFX, "RSTextureSDL", "setPalette", ("Could not set palette with %i colors! SDL Error : %s", rs_eng::kPaletteMaxColor, SDL_GetError()))
        return false;
    }

    if (hasColorKey_) {
        SDL_SetSurfaceColorKey(pSurface_, true, colorKey_);
    }

    return true;
}

void RSTextureSDL::setColorInPalette(int index, rs_eng::RSColor color) {
    SDL_Color newColor {color.r, color.g, color.b, color.a};

    SDL_Palette *surfPalette = SDL_GetSurfacePalette(pSurface_);
    if (!surfPalette) {
        surfPalette = SDL_CreateSurfacePalette(pSurface_);
    }

    if (!surfPalette || !SDL_SetPaletteColors(surfPalette, &newColor, index, 1)) {
        RSERR(Log::k_FLG_GFX, "RSTextureSDL", "setPalette", ("Could not set color in palette! SDL Error : %s", SDL_GetError()))
    }
}

/*!
 * @brief 
 * @return 
 */
bool RSTextureSDL::loadTextureFromSurface() {
    freeTexture();
    if (pSurface_) {
        pTexture_ = SDL_CreateTextureFromSurface(pRenderer_, pSurface_);
        if (pTexture_ == nullptr) {
            RSERR(Log::k_FLG_GFX, "RSTextureSDL", "loadTextureFromSurface", ("Critical error, Could create texture from surface! SDL Error : %s", SDL_GetError()))
        } else {
            SDL_SetTextureScaleMode(pTexture_, SDL_SCALEMODE_NEAREST);
        }
    }
    return pTexture_ != nullptr;
}

/*!
 * Returns the color in the palette at the given index
 * @param index Index of the color in the palette
 * @param color Resulting color
 * @return True if color was found
 */
bool RSTextureSDL::getColorFromPalette(const int index, rs_eng::RSColor& color) {
    SDL_Palette *surfPalette = SDL_GetSurfacePalette(pSurface_);
    if (!surfPalette || index < 0 || index >= surfPalette->ncolors) {
        return false;
    }

    SDL_Color sdlColor = surfPalette->colors[index];
    color.r = sdlColor.r;
    color.g = sdlColor.g;
    color.b = sdlColor.b;
    color.a = sdlColor.a;
    
    return true;
}

/*!
 * Set the color factor for color modulation
 * @param color The color factor
 */
void RSTextureSDL::setColorModulation( rs_eng::RSColor color ) {
    // Modulate texture
    SDL_SetTextureColorMod(pTexture_, color.r, color.g, color.b);
}

}
