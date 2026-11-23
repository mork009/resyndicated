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

#include "rs-engine/gfx/animationplayer.h"

#include <cassert>

#include "rs-utils/log/log.h"
#include "rs-engine/gfx/animationmanager.h"

namespace rs_eng {

AnimationPlayer::AnimationPlayer() : spentTime_(0) {
    reset();
}

AnimationPlayer::~AnimationPlayer() {}

/*!
 * Adds a new animation.
 * @param animation 
 * @return position of the animation in the list of animation
 */
uint16_t AnimationPlayer::addAnimation(MapObjectAnimation animation) {
    assert(animation.framePerSec != 0);
    
    animations_.push_back(animation);
    return static_cast<uint16_t> (animations_.size() - 1);
}

/*!
 * Builds an animation and adds it to the list
 * @param framesAnimation 
 * @param mode 
 * @param framePerSec 
 * @param maxPlayTime 
 * @return position of the animation in the list of animation
 */
uint16_t AnimationPlayer::addAnimation(uint16_t framesAnimation, AnimationMode mode, uint8_t framePerSec, uint32_t maxPlayTime ) {
    MapObjectAnimation animation {
        .framesAnimation = framesAnimation,
        .framePerSec = framePerSec,
        .mode = mode,
        .maxPlayTime = maxPlayTime
    };
    return addAnimation(animation);
}

/*!
 * Play a given animation. By defaut animation starts at frame 0,
 * but we can specify the starting frame.
 * @param mapObjectAnimationId Id of registered animation
 * @param startFrame Starting frame
 * @param newMaxPlayTime If non zero, overrides default maxPlayTime of the animation
 * @return True if animation of given id has been found
 */
bool AnimationPlayer::play(uint16_t mapObjectAnimationId, uint8_t startFrame, uint32_t newMaxPlayTime) {
    if (mapObjectAnimationId < animations_.size()) {
        if (currentAnimationId_ == mapObjectAnimationId && isPlaying_ && startFrame == 0 && newMaxPlayTime == 0) {
            return true;
        }
        loadAnimation(mapObjectAnimationId, newMaxPlayTime);
        currentAnimationId_ = mapObjectAnimationId;
        frame_ = startFrame;
        if (rs_eng::AnimationManager::singletonPtr() != nullptr) {
            lastFrame_ = g_AnimMgr.lastFrame(currentAnimation_.framesAnimation);
        } else {
            lastFrame_ = 0;
        }
        isPlaying_ = true;
        elapsedCarry_ = 0;
        if (currentAnimation_.maxPlayTime) {
            spentTime_.reset(currentAnimation_.maxPlayTime);
        }
        return true;
    } else {
        // We use the default animation
        reset();
        return false;
    }
}

void AnimationPlayer::loadAnimation(const uint16_t mapObjectAnimationId, uint32_t newMaxPlayTime) {
    currentAnimation_ = animations_[mapObjectAnimationId];
    if (newMaxPlayTime != 0) {
        // We can override maxPlayTime at run time.
        // This allows for randomizing this value
        currentAnimation_.maxPlayTime = newMaxPlayTime;
    }
}

/*!
 * Updates the animation
 * @param elapsed Time in milliseconds since the last execution
 * @return True if current animation has finished
 */
bool AnimationPlayer::handleTick(uint32_t elapsed) {
    if (isPlaying_) {
        if (currentAnimation_.framePerSec == 0) {
            return false;
        }
        bool maxTimeReached = (currentAnimation_.maxPlayTime != 0) ?
                                spentTime_.update(elapsed) : false;
        // This is the time to display one frame in milliseconds
        uint32_t frameTics = 1000 / currentAnimation_.framePerSec;
        if (frameTics == 0) frameTics = 1;
        // Adding carry and elapsed to get total elapsed time
        uint32_t totalElapsed = elapsed + elapsedCarry_;
        // Get new value for elapsedCarry to store remaining time to 
        // get to next frame.
        elapsedCarry_ = totalElapsed % frameTics;
        
        // Compute the new frame based on the number of periods
        // of frameTics contained in totalElapsed
        // Number of frames is added to current frame rank
        frame_ += (uint8_t) (totalElapsed / frameTics);

        // Check if we reached end of animation
        if (frame_ > lastFrame_) {
            if (currentAnimation_.mode == kAnimationModeSingle) {
                frame_ = lastFrame_;
                if (currentAnimation_.maxPlayTime == 0) {
                    isPlaying_ = false;
                    return true;
                }
            } else if (currentAnimation_.mode == kAnimationModeLoop) {
                // just looping through animation
                frame_ %= static_cast< uint8_t >(lastFrame_ + 1);
            }
            
        }
        
        if (maxTimeReached) {
            isPlaying_ = false;
            return true;
        }
    }
    // We have not reached the end of animation
    return false;
}

/*!
 * Used to rewind an already loaded animation to first frame.
 * Animation is stopped and must be started again to be animated
 */
void AnimationPlayer::resetAnimation() {
    lastFrame_ = 0;
    frame_ = 0;
    elapsedCarry_ = 0;
    isPlaying_ = false;
}

/*!
 * @brief 
 */
void AnimationPlayer::reset() {
    resetAnimation();
    currentAnimationId_ = 0;
    currentAnimation_.framesAnimation = 0;
    currentAnimation_.mode = kAnimationModeSingle;
    currentAnimation_.maxPlayTime = 0;
    currentAnimation_.framePerSec = 0;
}

/*!
 * Draw the current animation.
 * @param screenPos Position on the screen
 * @param animOffset An offset to apply to current animationId to get animation to draw
 */
void AnimationPlayer::draw(const Point2D &screenPos, uint16_t animOffset, int factionId) {
    if (animations_.empty() || currentAnimationId_ >= animations_.size()) {
        return;
    }
    if (rs_eng::AnimationManager::singletonPtr() != nullptr) {
        g_AnimMgr.drawFrame(currentAnimation_.framesAnimation + animOffset, frame_, screenPos, factionId);
    }
}
}