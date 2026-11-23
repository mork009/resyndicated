/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
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

#ifndef RS_SDLMIXER_SOUND_H
#define RS_SDLMIXER_SOUND_H

#include "rs-engine/sound/sound.h"

#include <SDL3/SDL.h>

namespace rs_eng {

class SdlMixerAudio;

class SdlMixerSound : public Sound {
public:
    explicit SdlMixerSound(SdlMixerAudio *pAudio);
    ~SdlMixerSound();

    void play(int channel = -1, int loops = 0) const override;
    void stop(int channel = -1) const override;
    bool setVolume(int volume) override;
    bool loadSound(uint8_t *soundData, uint32_t size) override;

private:
    SdlMixerAudio *pAudio_;
    Uint8 *soundData_;
    int soundLen_;
    int volume_;
};

}

#endif // RS_SDLMIXER_SOUND_H
