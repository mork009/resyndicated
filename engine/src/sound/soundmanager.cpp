/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2005  Stuart Binge  <skbinge@gmail.com>
 *   Copyright (C) 2005  Joost Peters  <joostp@users.sourceforge.net>
 *   Copyright (C) 2006  Trent Waddington <qg@biodome.org>
 *   Copyright (C) 2006  Tarjei Knapstad <tarjei.knapstad@gmail.com>
 *   Copyright (C) 2010, 2024-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

#include "rs-engine/sound/soundmanager.h"

#include <memory>

#include "rs-engine/sound/audio.h"
#include "rs-utils/io/file.h"
#include "rs-utils/log/log.h"
#include "rs-engine/appcontext.h"

namespace rs_eng {

SoundManager::SoundManager():tabentry_startoffset_(58), tabentry_offset_(32)
{
    volumeBeforeMute_ = -1;
    disabled_ = false;
    audio_ = nullptr;
}

SoundManager::~SoundManager() {}

Sound *SoundManager::soundFromInGame(size_t sample)
{
    if (sample >= sounds_.size() || !sounds_.at(sample)) {
        if (sample == AIRSTRIKE) {
            return soundFromInGame(EXPLOSION);
        }
        return nullptr;
    }
    return sounds_.at(sample).get();
}

Sound *SoundManager::soundFromIntro(size_t sample)
{
    if (sample >= introSounds_.size())
        return nullptr;
    return introSounds_.at(sample).get();
}

void SoundManager::initialize(Audio* audio, bool disabled, bool doLoadIntroSounds) {
    audio_ = audio;
    audio_ = audio;
    disabled_ = disabled;
    sounds_.clear();
    introSounds_.clear();
    lastPlayedTime_.clear();

    LOG(Log::k_FLG_INFO, "SoundManager", "initialize", ("Loading sounds..."))

    if (disabled_) {
        RSINFO(Log::k_FLG_SND, "SoundManager", "initialize", ("Sound will be disabled\n"))
    } else {
        // Always load ingame sounds
        loadSounds(SAMPLES_GAME);
        if (doLoadIntroSounds) {
            loadSounds(SAMPLES_INTRO);
        }
    }

    audio_->setSoundVolume(g_Ctx.getSoundVolume());
}

void SoundManager::loadSounds(SampleSet set)
{
    switch (set) {
    case SAMPLES_INTRO:
        {
            LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("Loading intro sounds 1/2"))
            loadSounds("ISNDS-0.TAB", "ISNDS-0.DAT", introSounds_);

            LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("Loading intro sounds 2/2"))
            loadSounds("ISNDS-1.TAB", "ISNDS-1.DAT", introSounds_);
        }
        break;
    case SAMPLES_GAME:
        {
            LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("Loading game sounds 1/2"))
            std::unique_ptr<Sound> airStrikeSound;
            loadSounds("SOUND-0.TAB", "SOUND-0.DAT", sounds_, &airStrikeSound);

            LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("Loading game sounds 2/2"))
            loadSounds("SOUND-1.TAB", "SOUND-1.DAT", sounds_);

            if (airStrikeSound) {
                LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("American Revolt Air Strike sound loaded, assigned to AIRSTRIKE"))
                sounds_.push_back(std::move(airStrikeSound));
            }
        }
        break;
    default:
        break;
    }
}

namespace {
/*!
 * \brief Returns the exact size of a VOC stream up to its VOC_TERM block.
 *
 * Bullfrog packed VOC sound assets with alignment padding in DAT files.
 * When SDL_mixer's VOC decoder encounters trailing bytes after the VOC_TERM
 * block (0x00), it attempts to parse them as additional block headers, which
 * misinterprets padding bytes as huge silence blocks and corrupts the heap.
 * Trimming the buffer to the exact VOC_TERM terminator ensures clean parsing.
 */
uint32_t getVocActualSize(const uint8_t *data, uint32_t size) {
    constexpr const char kVocHeader[] = "Creative Voice File\x1A";
    constexpr size_t kVocHeaderLen = 20;

    if (size < 26) {
        return size;
    }
    if (std::memcmp(data, kVocHeader, kVocHeaderLen) != 0) {
        return size;
    }

    uint16_t dataOffset = data[20] | (static_cast<uint16_t>(data[21]) << 8);
    if (dataOffset >= size) {
        return size;
    }

    uint32_t pos = dataOffset;
    while (pos < size) {
        uint8_t blockType = data[pos];
        if (blockType == 0) { // VOC_TERM: marks the exact end of the VOC stream
            return pos + 1;
        }
        if (pos + 4 > size) {
            break;
        }
        uint32_t blockLen = data[pos + 1] |
                            (static_cast<uint32_t>(data[pos + 2]) << 8) |
                            (static_cast<uint32_t>(data[pos + 3]) << 16);
        pos += 4 + blockLen;
    }
    return size;
}
} // anonymous namespace

void SoundManager::loadSounds(const std::string &tabFile, const std::string &datFile, std::vector<std::unique_ptr<Sound>> &sounds, std::unique_ptr<Sound> *pAirStrikeOut)
{
    size_t tabSize = 0, datSize = 0;

    uint8_t *tabDataOri = rs_utl::File::loadOriginalFile(tabFile, tabSize);
    uint8_t *soundDataOri = rs_utl::File::loadOriginalFile(datFile, datSize);

    if (!tabDataOri || !soundDataOri) {
        delete[] tabDataOri;
        delete[] soundDataOri;
        return;
    }

    uint8_t *tabData = tabDataOri;
    uint8_t *soundData = soundDataOri;

    tabData += tabentry_startoffset_;
    uint32_t offset = 0;

    for (int i = 0; i < int(tabSize) - tabentry_offset_;
        i += tabentry_offset_)
    {
        uint32_t soundsize = rs_utl::READ_LE_UINT32(tabData);

        // Samples with size < 144 are bogus
        if (soundsize > 144) {
            // Check if this is the American Revolt Air Strike sound (entry 17 at loop offset 544 in SOUND-D / SOUND-0)
            bool isAirStrikeEntry = (pAirStrikeOut != nullptr && i == 544);

            if (offset + soundsize <= datSize) {
                // Find exact VOC termination point to trim Bullfrog chunk alignment padding
                uint32_t actualSize = getVocActualSize(soundData, soundsize);

                uint8_t *sample = new uint8_t[actualSize];
                memcpy(sample, soundData, actualSize);

                if (isAirStrikeEntry) {
                    *pAirStrikeOut = audio_->createSound();
                    if ((*pAirStrikeOut)->loadSound(sample, actualSize)) {
                        LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("Air Strike sound loaded at offset %d (size %u -> %u)", i, soundsize, actualSize))
                    } else {
                        LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("Failed loading Air Strike sound at offset %d", i))
                        pAirStrikeOut->reset();
                    }
                } else {
                    sounds.push_back(audio_->createSound());

                    // patching wrong sample rate for original Syndicate assets
                    if (tabFile.find("SOUND-0") != std::string::npos || tabFile.find("sound-0") != std::string::npos) {
                        if (actualSize > 0x1e && sounds.size() == 13)
                            sample[0x1e] = 0x9c;
                    } else if (tabFile.find("SOUND-1") != std::string::npos || tabFile.find("sound-1") != std::string::npos) {
                        if (actualSize > 0x1e) {
                            if (sounds.size() == 24)
                                sample[0x1e] = 0x9c;
                            else if (sounds.size() == 25)
                                sample[0x1e] = 0x38;
                        }
                    }

                    if (sounds.back()->loadSound(sample, actualSize)) {
                        LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("Sound loaded at offset %d (size %u -> %u)", i, soundsize, actualSize))
                    } else {
                        LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("Failed loading sound at offset %d", i))
                    }
                }
                delete []sample;
            } else {
                LOG(Log::k_FLG_SND, "SoundManager", "loadSounds", ("Sound at offset %d exceeds datSize (%u + %u > %zu)", i, offset, soundsize, datSize))
            }
        }
        soundData += soundsize;
        offset += soundsize;

        tabData += tabentry_offset_;
    }

    delete[] tabDataOri;
    delete[] soundDataOri;
}

bool SoundManager::canUseAudio() {
    return !disabled_ &&
            audio_ != NULL &&
            audio_->isInitialized();
}

/** \brief Play a sound from in game library
 *
 * \param sample InGameSample
 * \param channel int
 * \param loops int
 * \return void
 *
 */
void SoundManager::play(size_t sample, int channel, int loops) {
    if (canUseAudio()) {
        if (channel == -1 && loops == 0) {
            auto now = std::chrono::steady_clock::now();
            auto it = lastPlayedTime_.find(sample);
            if (it != lastPlayedTime_.end()) {
                auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second).count();
                if (elapsedMs < 40) {
                    return;
                }
            }
            lastPlayedTime_[sample] = now;
        }
        Sound *pSound = soundFromInGame(sample);

        if (pSound) {
            // Sound is played on first available channel (value -1)
            pSound->play(channel, loops);
        }
    }
}

/** \brief Play a sound from in intro library
 *
 * \param sample sample to play
 * \param channel int
 * \param loops int
 * \return void
 *
 */
void SoundManager::playIntro(size_t sample, int channel, int loops) {
    if (canUseAudio()) {
        Sound *pSound = soundFromIntro(sample);

        if (pSound) {
            // Sound is played on first available channel (value -1)
            pSound->play(channel, loops);
        }
    }
}


/**
 *
 * \param channel channel to stop
 * \return void
 *
 */
void SoundManager::stop(int channel) {
    if (canUseAudio()) {
        audio_->stopSound(channel);
    }
}

void SoundManager::setVolume(int volume) {
    if (canUseAudio()) {
        audio_->setSoundVolume(volume);
    }
}

int SoundManager::getVolume() {
    if (canUseAudio()) {
        return audio_->getSoundVolume();
    }
    return 0;
}


/*!
 * Increase volume with the given percentage (0-100).
 * Cannot be higher the max volume.
 * @param percentage 
 * @return the new volume
 */
int SoundManager::increaseVolume(int percentage) {
    int incr = percentage * audio_->getMaxVolume() / 100;
    setVolume(getVolume() + incr);
    return getVolume();
}


/*!
 * Decrease volume with the given percentage (0-100).
 * Cannot be lower than zero
 * @param percentage 
 * @return the new volume
 */
int SoundManager::decreaseVolume(int percentage) {
    int decr = percentage * audio_->getMaxVolume() / 100;
    if (decr > getVolume()) {
        setVolume(0);
    } else {
        setVolume(getVolume() - decr);
    }
    
    return getVolume();
}

/*!
 * Turns the music on/off.
 * The method watches for the value of field volumeBeforeMute_ :
 *  - if it's value is -1 : then music volume is on. So the method
 *    saves the level into volumeBeforeMute_ and sets the volume to zero
 *  - else it means volume is mute so the methods restores the original
 *    volume level.
 */
void SoundManager::toggleSound() {
    if (canUseAudio()) {
        if (volumeBeforeMute_ == -1) {
            volumeBeforeMute_ = audio_->getSoundVolume();
            LOG(Log::k_FLG_SND, "SoundManager", "toggleSound", ("Turning sound off : %d", volumeBeforeMute_))
            audio_->setSoundVolume(0);
        } else {
            LOG(Log::k_FLG_SND, "SoundManager", "toggleSound", ("Turning sound on : %d", volumeBeforeMute_))
            audio_->setSoundVolume(volumeBeforeMute_);
            volumeBeforeMute_ = -1;
        }
    }
}
}