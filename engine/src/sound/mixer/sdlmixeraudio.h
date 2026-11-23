/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2023-2025  Benoit Blancard <benblan@users.sourceforge.net>
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

#ifndef RS_SDLMIXER_AUDIO_H
#define RS_SDLMIXER_AUDIO_H

#include "rs-engine/sound/audio.h"

#include <SDL3/SDL.h>
#include <array>
#include <string>

struct ADL_MIDIPlayer;

namespace rs_eng {

class SdlMixerAudio : public Audio {
public:
    SdlMixerAudio();
    ~SdlMixerAudio();

    bool init(EFrequency frequency = FRQ_DEFAULT,
              EFormat format = FMT_DEFAULT,
              EChannel channels = CHN_DEFAULT,
              int chunksize = 1024) override;

    bool isInitialized() override { return initialized_; }
    bool quit() override;

    int getMaxVolume() override;

    void setMusicVolume(int volume) override;
    int getMusicVolume() override;

    void setSoundVolume(int volume, int channel) override;
    int getSoundVolume(int channel) override;
    void stopSound(int channel) override;

    std::unique_ptr<Sound> createSound() override;

    bool openFile(std::string path) override;
    void playMusic(int songNb, bool loop) override;
    void stopMusic() override;
    void pauseMusic() override;
    void resumeMusic() override;

    const SDL_AudioSpec& getAudioSpec() const { return deviceSpec_; }
    void playSoundOnChannel(int channel, const Uint8 *data, int len, int loops);

private:
    void initObtainedAudioFormat();

    bool initialized_;
    SDL_AudioDeviceID devId_;
    SDL_AudioSpec deviceSpec_;
    std::array<SDL_AudioStream *, 32> channelStreams_;
    std::array<int, 32> channelVolumes_;
    SDL_AudioStream *musicStream_;
    int musicVolume_;
    struct ADL_MIDIPlayer *pMidiPlayer_;
};

}

#endif // RS_SDLMIXER_AUDIO_H
