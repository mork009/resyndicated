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

#include "sdlmixeraudio.h"

#include <SDL3/SDL.h>
#include "adlmidi.h"

#include <algorithm>
#include <vector>

#include "rs-utils/log/log.h"
#include "sdlmixersound.h"

static const int kMaxVolume = 128;
static Uint32 is_playing = 0;
static struct ADLMIDI_AudioFormat s_audioFormat;
static std::vector<Uint8> temp_buffer;

static void SDLCALL rs_audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    (void)total_amount;
    if (!is_playing || additional_amount <= 0 || !userdata) {
        return;
    }

    struct ADL_MIDIPlayer *p = static_cast<struct ADL_MIDIPlayer *>(userdata);

    if (static_cast<int>(temp_buffer.size()) < additional_amount) {
        temp_buffer.resize(additional_amount);
    }

    int samples_count = additional_amount / s_audioFormat.containerSize;
    samples_count = adl_playFormat(p, samples_count,
                                   temp_buffer.data(),
                                   temp_buffer.data() + s_audioFormat.containerSize,
                                   &s_audioFormat);

    if (samples_count <= 0) {
        is_playing = 0;
    } else {
        int bytes = samples_count * s_audioFormat.containerSize;
        SDL_PutAudioStreamData(stream, temp_buffer.data(), bytes);
    }
}

namespace rs_eng {

SdlMixerAudio::SdlMixerAudio()
    : initialized_(false),
      devId_(0),
      musicStream_(nullptr),
      musicVolume_(kMaxVolume),
      pMidiPlayer_(nullptr) {
    channelStreams_.fill(nullptr);
    channelVolumes_.fill(kMaxVolume);
    SDL_zero(deviceSpec_);
}

SdlMixerAudio::~SdlMixerAudio() {
    quit();
}

bool SdlMixerAudio::init(EFrequency freq, [[maybe_unused]] EFormat fmt, EChannel chan, [[maybe_unused]] int chunksize) {
    LOG(Log::k_FLG_SND, "SdlMixerAudio", "init", ("Initializing sound system with SDL3 audio streams..."));

    if (initialized_) {
        RSERR(Log::k_FLG_SND, "SdlMixerAudio", "init", ("SDL Audio has already been initialized."));
        return true;
    }

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        RSERR(Log::k_FLG_SND, "SdlMixerAudio", "init", ("Failed initialize SDL Audio module: %s", SDL_GetError()));
        return false;
    }

    int frequency = 44100;
    switch (freq) {
        case FRQ_11025: frequency = 11025; break;
        case FRQ_22050: frequency = 22050; break;
        case FRQ_44100: frequency = 44100; break;
        default:        frequency = 44100; break;
    }

    int channels = (chan == MONO) ? 1 : 2;

    SDL_AudioSpec requestedSpec;
    SDL_zero(requestedSpec);
    requestedSpec.freq = frequency;
    requestedSpec.format = SDL_AUDIO_S16LE;
    requestedSpec.channels = channels;

    devId_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &requestedSpec);
    if (devId_ == 0) {
        RSERR(Log::k_FLG_SND, "SdlMixerAudio", "init", ("Failed to open SDL3 audio device: %s", SDL_GetError()));
        return false;
    }

    if (!SDL_GetAudioDeviceFormat(devId_, &deviceSpec_, nullptr)) {
        deviceSpec_ = requestedSpec;
    }

    // Initialize the 32 sound channels
    for (size_t i = 0; i < channelStreams_.size(); ++i) {
        channelStreams_[i] = SDL_CreateAudioStream(&deviceSpec_, &deviceSpec_);
        if (channelStreams_[i]) {
            SDL_BindAudioStream(devId_, channelStreams_[i]);
            channelVolumes_[i] = kMaxVolume;
            SDL_SetAudioStreamGain(channelStreams_[i], 1.0f);
        }
    }

    // Initialize libADLMIDI
    pMidiPlayer_ = adl_init(deviceSpec_.freq);
    if (!pMidiPlayer_) {
        RSERR(Log::k_FLG_SND, "SdlMixerAudio", "init", ("Couldn't initialize ADLMIDI: %s\n", adl_errorString()));
        quit();
        return false;
    }

    adl_switchEmulator(pMidiPlayer_, ADLMIDI_EMU_NUKED);
    adl_setNumChips(pMidiPlayer_, 1);
    adl_setBank(pMidiPlayer_, 19);

    initObtainedAudioFormat();

    // Create music stream
    musicStream_ = SDL_CreateAudioStream(&deviceSpec_, &deviceSpec_);
    if (musicStream_) {
        SDL_BindAudioStream(devId_, musicStream_);
        SDL_SetAudioStreamGain(musicStream_, musicVolume_ / static_cast<float>(kMaxVolume));
        SDL_SetAudioStreamGetCallback(musicStream_, rs_audio_callback, pMidiPlayer_);
    }

    SDL_ResumeAudioDevice(devId_);

    LOG(Log::k_FLG_SND, "Audio", "init", ("SDL3 Audio system initialized"));
    initialized_ = true;
    return true;
}

void SdlMixerAudio::initObtainedAudioFormat() {
    switch (deviceSpec_.format) {
        case SDL_AUDIO_S8:
            s_audioFormat.type = ADLMIDI_SampleType_S8;
            s_audioFormat.containerSize = sizeof(int8_t);
            s_audioFormat.sampleOffset = sizeof(int8_t) * 2;
            break;
        case SDL_AUDIO_U8:
            s_audioFormat.type = ADLMIDI_SampleType_U8;
            s_audioFormat.containerSize = sizeof(uint8_t);
            s_audioFormat.sampleOffset = sizeof(uint8_t) * 2;
            break;
        case SDL_AUDIO_S16LE:
        case SDL_AUDIO_S16BE:
            s_audioFormat.type = ADLMIDI_SampleType_S16;
            s_audioFormat.containerSize = sizeof(int16_t);
            s_audioFormat.sampleOffset = sizeof(int16_t) * 2;
            break;
        case SDL_AUDIO_S32LE:
        case SDL_AUDIO_S32BE:
            s_audioFormat.type = ADLMIDI_SampleType_S32;
            s_audioFormat.containerSize = sizeof(int32_t);
            s_audioFormat.sampleOffset = sizeof(int32_t) * 2;
            break;
        case SDL_AUDIO_F32LE:
        case SDL_AUDIO_F32BE:
            s_audioFormat.type = ADLMIDI_SampleType_F32;
            s_audioFormat.containerSize = sizeof(float);
            s_audioFormat.sampleOffset = sizeof(float) * 2;
            break;
        default:
            s_audioFormat.type = ADLMIDI_SampleType_S16;
            s_audioFormat.containerSize = sizeof(int16_t);
            s_audioFormat.sampleOffset = sizeof(int16_t) * 2;
            break;
    }
}

bool SdlMixerAudio::quit() {
    if (initialized_) {
        is_playing = 0;
        for (auto &stream : channelStreams_) {
            if (stream) {
                SDL_DestroyAudioStream(stream);
                stream = nullptr;
            }
        }
        if (musicStream_) {
            SDL_DestroyAudioStream(musicStream_);
            musicStream_ = nullptr;
        }
        if (devId_) {
            SDL_CloseAudioDevice(devId_);
            devId_ = 0;
        }
        if (pMidiPlayer_) {
            adl_close(pMidiPlayer_);
            pMidiPlayer_ = nullptr;
        }
        initialized_ = false;
        return true;
    }
    return false;
}

void SdlMixerAudio::setMusicVolume(int volume) {
    musicVolume_ = std::clamp(volume, 0, kMaxVolume);
    if (musicStream_) {
        SDL_SetAudioStreamGain(musicStream_, musicVolume_ / static_cast<float>(kMaxVolume));
    }
}

int SdlMixerAudio::getMusicVolume() {
    return initialized_ ? musicVolume_ : -1;
}

void SdlMixerAudio::setSoundVolume(int volume, int channel) {
    int vol = std::clamp(volume, 0, kMaxVolume);
    if (channel < 0) {
        for (size_t i = 0; i < channelStreams_.size(); ++i) {
            channelVolumes_[i] = vol;
            if (channelStreams_[i]) {
                SDL_SetAudioStreamGain(channelStreams_[i], vol / static_cast<float>(kMaxVolume));
            }
        }
    } else if (channel < static_cast<int>(channelStreams_.size())) {
        channelVolumes_[channel] = vol;
        if (channelStreams_[channel]) {
            SDL_SetAudioStreamGain(channelStreams_[channel], vol / static_cast<float>(kMaxVolume));
        }
    }
}

int SdlMixerAudio::getSoundVolume(int channel) {
    if (!initialized_) return -1;
    if (channel < 0) return channelVolumes_[0];
    if (channel < static_cast<int>(channelVolumes_.size())) {
        return channelVolumes_[channel];
    }
    return -1;
}

void SdlMixerAudio::stopSound(int channel) {
    if (!initialized_) return;
    if (channel < 0) {
        for (auto stream : channelStreams_) {
            if (stream) SDL_ClearAudioStream(stream);
        }
    } else if (channel < static_cast<int>(channelStreams_.size())) {
        if (channelStreams_[channel]) {
            SDL_ClearAudioStream(channelStreams_[channel]);
        }
    }
}

void SdlMixerAudio::playSoundOnChannel(int channel, const Uint8 *data, int len, int loops) {
    if (!initialized_ || !data || len <= 0) return;

    int targetChan = channel;
    if (targetChan < 0 || targetChan >= static_cast<int>(channelStreams_.size())) {
        // Find an idle channel
        targetChan = 0;
        for (size_t i = 0; i < channelStreams_.size(); ++i) {
            if (channelStreams_[i] && SDL_GetAudioStreamQueued(channelStreams_[i]) == 0) {
                targetChan = static_cast<int>(i);
                break;
            }
        }
    }

    SDL_AudioStream *stream = channelStreams_[targetChan];
    if (!stream) return;

    SDL_ClearAudioStream(stream);

    int count = (loops < 0) ? 1 : (loops + 1);
    for (int l = 0; l < count; ++l) {
        SDL_PutAudioStreamData(stream, data, len);
    }
}

int SdlMixerAudio::getMaxVolume() {
    return kMaxVolume;
}

std::unique_ptr<Sound> SdlMixerAudio::createSound() {
    return std::make_unique<SdlMixerSound>(this);
}

bool SdlMixerAudio::openFile(std::string path) {
    if (!pMidiPlayer_) return false;

    if (adl_openFile(pMidiPlayer_, path.c_str()) < 0) {
        RSERR(Log::k_FLG_SND, "SdlMixerAudio", "openFile", ("Couldn't open music file: %s\n", adl_errorInfo(pMidiPlayer_)));
        return false;
    }
    is_playing = 0;
    return true;
}

void SdlMixerAudio::playMusic(int songNb, bool loop) {
    if (!pMidiPlayer_) return;
    pauseMusic();
    adl_setLoopEnabled(pMidiPlayer_, loop ? 1 : 0);
    adl_selectSongNum(pMidiPlayer_, songNb);
    resumeMusic();
}

void SdlMixerAudio::pauseMusic() {
    is_playing = 0;
    if (musicStream_) {
        SDL_ClearAudioStream(musicStream_);
    }
}

void SdlMixerAudio::resumeMusic() {
    is_playing = 1;
}

void SdlMixerAudio::stopMusic() {
    is_playing = 0;
    if (pMidiPlayer_) {
        adl_reset(pMidiPlayer_);
    }
    if (musicStream_) {
        SDL_ClearAudioStream(musicStream_);
    }
}

}
