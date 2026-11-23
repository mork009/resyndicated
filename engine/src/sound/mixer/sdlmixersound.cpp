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

#include "sdlmixersound.h"

#include <algorithm>
#include <cstring>
#include <vector>
#include "rs-utils/log/log.h"
#include "sdlmixeraudio.h"

namespace rs_eng {

namespace {

/*!
 * \brief Parse Creative Voice (.VOC) data into raw linear PCM samples.
 */
bool parseVoc(const uint8_t *data, uint32_t size, SDL_AudioSpec &specOut, std::vector<uint8_t> &pcmOut) {
    constexpr const char kVocHeader[] = "Creative Voice File\x1A";
    constexpr size_t kVocHeaderLen = 20;

    if (!data || size < 26) {
        return false;
    }
    if (std::memcmp(data, kVocHeader, kVocHeaderLen) != 0) {
        return false;
    }

    uint16_t headerLen = data[20] | (static_cast<uint16_t>(data[21]) << 8);
    if (headerLen >= size) {
        return false;
    }

    int sampleRate = 0;
    SDL_AudioFormat format = SDL_AUDIO_U8;
    int channels = 1;

    uint32_t pos = headerLen;
    while (pos < size) {
        uint8_t blockType = data[pos];
        if (blockType == 0) { // VOC_TERM: end of sound blocks
            break;
        }
        if (pos + 4 > size) {
            break;
        }

        uint32_t blockLen = data[pos + 1] |
                            (static_cast<uint32_t>(data[pos + 2]) << 8) |
                            (static_cast<uint32_t>(data[pos + 3]) << 16);
        pos += 4;
        if (pos + blockLen > size) {
            blockLen = size - pos;
        }

        switch (blockType) {
            case 0x01: { // Voice Data
                if (blockLen >= 2) {
                    uint8_t timeConstant = data[pos];
                    uint8_t codec = data[pos + 1];
                    if (timeConstant != 0) {
                        sampleRate = 1000000 / (256 - timeConstant);
                    }
                    if (codec == 0) { // 8-bit unsigned PCM
                        const uint8_t *samples = data + pos + 2;
                        size_t sampleCount = blockLen - 2;
                        pcmOut.insert(pcmOut.end(), samples, samples + sampleCount);
                    }
                }
                break;
            }
            case 0x02: { // Voice Continuation
                const uint8_t *samples = data + pos;
                pcmOut.insert(pcmOut.end(), samples, samples + blockLen);
                break;
            }
            case 0x03: { // Silence
                if (blockLen >= 3) {
                    uint16_t count = (data[pos] | (static_cast<uint16_t>(data[pos + 1]) << 8)) + 1;
                    uint8_t timeConstant = data[pos + 2];
                    if (sampleRate == 0 && timeConstant != 0) {
                        sampleRate = 1000000 / (256 - timeConstant);
                    }
                    pcmOut.insert(pcmOut.end(), count, 128); // 128 is silence for unsigned 8-bit
                }
                break;
            }
            case 0x08: { // Extended attributes
                if (blockLen >= 4) {
                    uint16_t timeConstant = data[pos] | (static_cast<uint16_t>(data[pos + 1]) << 8);
                    uint8_t mode = data[pos + 3];
                    channels = (mode == 1) ? 2 : 1;
                    if (timeConstant != 0) {
                        sampleRate = 256000000 / (channels * (65536 - timeConstant));
                    }
                }
                break;
            }
            case 0x09: { // New format (VOC 1.20+)
                if (blockLen >= 12) {
                    sampleRate = data[pos] | (data[pos + 1] << 8) | (data[pos + 2] << 16) | (data[pos + 3] << 24);
                    uint8_t bits = data[pos + 4];
                    channels = data[pos + 5];
                    if (bits == 8) {
                        format = SDL_AUDIO_U8;
                    } else if (bits == 16) {
                        format = SDL_AUDIO_S16LE;
                    }
                    const uint8_t *samples = data + pos + 12;
                    size_t sampleCount = blockLen - 12;
                    pcmOut.insert(pcmOut.end(), samples, samples + sampleCount);
                }
                break;
            }
            default:
                // Skip markers, loops, text
                break;
        }

        pos += blockLen;
    }

    if (pcmOut.empty()) {
        return false;
    }

    if (sampleRate <= 0) {
        sampleRate = 10000;
    }

    specOut.format = format;
    specOut.channels = channels;
    specOut.freq = sampleRate;
    return true;
}

} // anonymous namespace

SdlMixerSound::SdlMixerSound(SdlMixerAudio *pAudio)
    : pAudio_(pAudio),
      soundData_(nullptr),
      soundLen_(0),
      volume_(128) {
}

SdlMixerSound::~SdlMixerSound() {
    if (soundData_) {
        SDL_free(soundData_);
        soundData_ = nullptr;
    }
}

void SdlMixerSound::play(int channel, int loops) const {
    if (!soundData_ || soundLen_ <= 0 || !pAudio_) {
        return;
    }
    pAudio_->playSoundOnChannel(channel, soundData_, soundLen_, loops);
}

void SdlMixerSound::stop(int channel) const {
    if (pAudio_) {
        pAudio_->stopSound(channel);
    }
}

bool SdlMixerSound::setVolume(int volume) {
    volume_ = std::clamp(volume, 0, 128);
    return true;
}

bool SdlMixerSound::loadSound(uint8_t *soundData, uint32_t size) {
    if (!soundData || size == 0) {
        return false;
    }

    if (soundData_) {
        SDL_free(soundData_);
        soundData_ = nullptr;
        soundLen_ = 0;
    }

    SDL_AudioSpec srcSpec;
    const uint8_t *srcBuf = nullptr;
    int srcLen = 0;
    Uint8 *allocatedWavBuf = nullptr;
    std::vector<uint8_t> vocPcm;

    constexpr const char kVocHeader[] = "Creative Voice File\x1A";
    if (size >= 26 && std::memcmp(soundData, kVocHeader, 20) == 0) {
        if (!parseVoc(soundData, size, srcSpec, vocPcm)) {
            RSERR(Log::k_FLG_SND, "SdlMixerSound", "loadSound", ("Failed parsing VOC sound data"));
            return false;
        }
        srcBuf = vocPcm.data();
        srcLen = static_cast<int>(vocPcm.size());
    } else {
        SDL_IOStream *io = SDL_IOFromMem(soundData, size);
        if (!io) {
            RSERR(Log::k_FLG_SND, "SdlMixerSound", "loadSound", ("Failed creating SDL_IO buffer from memory"));
            return false;
        }

        Uint32 wavLen = 0;
        if (!SDL_LoadWAV_IO(io, true, &srcSpec, &allocatedWavBuf, &wavLen)) {
            RSERR(Log::k_FLG_SND, "SdlMixerSound", "loadSound", ("Failed loading WAV from SDL_IO buffer: %s", SDL_GetError()));
            return false;
        }
        srcBuf = allocatedWavBuf;
        srcLen = static_cast<int>(wavLen);
    }

    if (!srcBuf || srcLen <= 0) {
        if (allocatedWavBuf) SDL_free(allocatedWavBuf);
        return false;
    }

    if (pAudio_) {
        const SDL_AudioSpec &dstSpec = pAudio_->getAudioSpec();
        Uint8 *convertedBuf = nullptr;
        int convertedLen = 0;
        if (SDL_ConvertAudioSamples(&srcSpec, srcBuf, srcLen, &dstSpec, &convertedBuf, &convertedLen)) {
            soundData_ = convertedBuf;
            soundLen_ = convertedLen;
            if (allocatedWavBuf) SDL_free(allocatedWavBuf);
            return true;
        }
    }

    soundData_ = static_cast<Uint8*>(SDL_malloc(srcLen));
    if (soundData_) {
        std::memcpy(soundData_, srcBuf, srcLen);
        soundLen_ = srcLen;
    }
    if (allocatedWavBuf) SDL_free(allocatedWavBuf);
    return soundData_ != nullptr;
}

}
