/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2012  Ryan Cocks <ryan@ryancocks.net>
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

#include "rs-kernel/model/ipastim.h"

#include <assert.h>

#ifdef _DEBUG
#include <stdio.h>

const char * IPAStim::IPANames[3] = {
    "Adrenaline",
    "Perception",
    "Intelligence"
};
#endif

IPAStim::IPAStim(IPAType ipa_type, uint8_t amount, uint8_t dependency)
:ipa_type_(ipa_type), effect_(50), effect_timer_(1000), dependency_timer_(4923),
 effect_remainder_ms_(0.0), dep_remainder_ms_(0.0)
{
    assert(ipa_type_ <= 3);
    setLevels(amount, dependency);
}

int IPAStim::getMagnitude() const
{
    return dependency_ < amount_ ? amount_ - dependency_: dependency_ - amount_;
}


float IPAStim::getMultiplier() const
{
    // In the original game, hormone multipliers depend directly on the current drug amount
    // (bar level), ranging from 0.5 (at 0%) to 1.0 (at 50% neutral midpoint) to 2.0 (at 100% max).
    // It is independent of the dependency difference magnitude.
    if (amount_ >= 50) {
        // Map 50 -> 1.0f, 100 -> 2.0f
        return 1.0f + (static_cast<float>(amount_ - 50) / 50.0f);
    } else {
        // Map 0 -> 0.5f, 50 -> 1.0f
        return 0.5f + (static_cast<float>(amount_) / 100.0f);
    }
}

void IPAStim::setLevels(uint8_t amount, uint8_t dependency, uint8_t effect)
{
    amount_ = amount;
    dependency_ = dependency;

    effect_ = effect;
    effect_remainder_ms_ = 0.0;
    dep_remainder_ms_ = 0.0;

    //printf("%s: A: %d, D: %d, E: %d\n", getName(), amount, dependency, effect_);
}

void IPAStim::processTicks(uint32_t elapsed, int mod_version)
{
    // From observation of the original Syndicate:
    // * Effect moves about once a second.
    // * Dependency bar moves once for every 5 or 6 moves of Effect
    // * Effect is independent of amount! If you flick amount to the
    //   other side effect will stay where it was.
    // * there appear to be 50 'positions' on the bar so it looks
    //   like the levels move in notches, 1% at a time.

    // Effect consumption and dependency regeneration replicate MAIN.EXE's 8-bit (0..255)
    // tick counter modulo [4, 8, 16, 40] and [20, 28, 40, 128] wraparound behavior:
    // - Effect:     [64, 32, 16, 7] hits per 256 ticks -> [1.0x, 2.0x, 4.0x, 64/7 (~9.14x)]
    // - Dependency: [13, 10,  7, 2] hits per 256 ticks -> [1.0x, 1.3x, 13/7 (~1.86x), 6.5x]
    //   relative to V0 dependency base (4923ms = 1000ms * 64/13).
    double effect_multiplier = 1.0;
    double dep_multiplier = 1.0;
    if (mod_version == 0) { // MOD_V1
        effect_multiplier = 2.0;
        dep_multiplier = 1.3;
    } else if (mod_version == 1) { // MOD_V2
        effect_multiplier = 4.0;
        dep_multiplier = 13.0 / 7.0;
    } else if (mod_version == 2) { // MOD_V3
        effect_multiplier = 64.0 / 7.0;
        dep_multiplier = 6.5;
    }

    // Preserve sub-millisecond remainders across per-frame ticks (e.g. 30-33ms frames / 10.0 or 6.4)
    // so V1-V3 retention does not stall or drift slower due to integer truncation.
    effect_remainder_ms_ += static_cast<double>(elapsed) / effect_multiplier;
    uint32_t scaled_effect_elapsed = static_cast<uint32_t>(effect_remainder_ms_);
    effect_remainder_ms_ -= static_cast<double>(scaled_effect_elapsed);

    dep_remainder_ms_ += static_cast<double>(elapsed) / dep_multiplier;
    uint32_t scaled_dep_elapsed = static_cast<uint32_t>(dep_remainder_ms_);
    dep_remainder_ms_ -= static_cast<double>(scaled_dep_elapsed);

    if(effect_timer_.update(scaled_effect_elapsed))
    {
        if(effect_ > amount_)
        {
            --effect_;
        }
        else if(effect_ < amount_)
        {
            ++effect_;
        }
        else // equal
        {
            // So once effect has 'caught up' to amount then they
            // both start moving towards the value of dependency
            // together
            if(amount_ > dependency_)
            {
                effect_ = --amount_;
            }
            else if(amount_ < dependency_)
            {
                effect_ = ++amount_;
            }
        }
        assert(effect_ >= 0 && effect_ <= 100);
    }

    // The dependency indicator always creaps towards amount
    if(dependency_timer_.update(scaled_dep_elapsed))
    {
        if (dependency_ > amount_) {
            --dependency_;
        } else if (dependency_ < amount_) {
            ++dependency_;
        } else {
            // equal
            if (amount_ < 50) {
                ++amount_;
                ++dependency_;
            } else if (amount_ > 50) {
                --amount_;
                --dependency_;
            }
        }
        assert(dependency_ >= 0 && dependency_ <= 100);

    }
}
