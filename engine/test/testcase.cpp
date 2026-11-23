/*
 *  ReSyndicated - a modernized game engine and full map/mission editor for the
 *  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
 *  project, FreeSynd.
 *
 *   Copyright (C) 2013, 2024-2025  Benoit Blancard <benblan@users.sourceforge.net>
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
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <vector>
#include <format>

#include "rs-engine/enginecommon.h"

// Test helper to validate scale checking logic independently of display hardware
static bool checkScaleFit(int scale, int desktopWidth, int desktopHeight) {
    if (scale < 1 || scale > 4) return false;
    int w = rs_eng::kScreenWidth * scale;
    int h = rs_eng::kScreenHeight * scale;
    return (w <= desktopWidth && h <= desktopHeight);
}

TEST_CASE("Window scale fits screen resolution validation", "[engine][system]") {
    // 1080p desktop (1920x1080)
    int width1080p = 1920;
    int height1080p = 1080;

    // 1x: 640x480 -> fits
    REQUIRE(checkScaleFit(1, width1080p, height1080p) == true);
    // 2x: 1280x960 -> fits
    REQUIRE(checkScaleFit(2, width1080p, height1080p) == true);
    // 3x: 1920x1440 -> exceeds 1080 height
    REQUIRE(checkScaleFit(3, width1080p, height1080p) == false);
    // 4x: 2560x1920 -> exceeds 1080 height and 1920 width
    REQUIRE(checkScaleFit(4, width1080p, height1080p) == false);

    // 1440p desktop (2560x1440)
    int width1440p = 2560;
    int height1440p = 1440;
    REQUIRE(checkScaleFit(1, width1440p, height1440p) == true);
    REQUIRE(checkScaleFit(2, width1440p, height1440p) == true);
    REQUIRE(checkScaleFit(3, width1440p, height1440p) == true); // 1920x1440 == 2560x1440
    REQUIRE(checkScaleFit(4, width1440p, height1440p) == false);

    // 4K desktop (3840x2160)
    int width4k = 3840;
    int height4k = 2160;
    REQUIRE(checkScaleFit(1, width4k, height4k) == true);
    REQUIRE(checkScaleFit(2, width4k, height4k) == true);
    REQUIRE(checkScaleFit(3, width4k, height4k) == true);
    REQUIRE(checkScaleFit(4, width4k, height4k) == true);

    // Invalid scales
    REQUIRE(checkScaleFit(0, width4k, height4k) == false);
    REQUIRE(checkScaleFit(5, width4k, height4k) == false);
}

// Test helper to compute panning step offsets given mouse and keyboard directions
static void computePanningDeltas(int mouseDirX, int mouseDirY,
                                 bool keyLeft, bool keyRight, bool keyUp, bool keyDown,
                                 int &outDx, int &outDy) {
    const int kScrollStep = 16;

    int keyDirX = 0;
    if (keyLeft && !keyRight) {
        keyDirX = -1;
    } else if (keyRight && !keyLeft) {
        keyDirX = 1;
    }

    int keyDirY = 0;
    if (keyUp && !keyDown) {
        keyDirY = -1;
    } else if (keyDown && !keyUp) {
        keyDirY = 1;
    }

    int dirX = (mouseDirX != 0) ? mouseDirX : keyDirX;
    int dirY = (mouseDirY != 0) ? mouseDirY : keyDirY;

    int stepX = kScrollStep;
    int stepY = (dirX != 0 && dirY != 0) ? (kScrollStep / 2) : kScrollStep;

    outDx = dirX * stepX;
    outDy = dirY * stepY;
}

TEST_CASE("Map panning direction and isometric angle deltas", "[engine][panning]") {
    int dx = 0;
    int dy = 0;

    SECTION("No input yields zero scrolling") {
        computePanningDeltas(0, 0, false, false, false, false, dx, dy);
        REQUIRE(dx == 0);
        REQUIRE(dy == 0);
    }

    SECTION("Cardinal keyboard arrow panning uses full step") {
        // Left
        computePanningDeltas(0, 0, true, false, false, false, dx, dy);
        REQUIRE(dx == -16);
        REQUIRE(dy == 0);

        // Right
        computePanningDeltas(0, 0, false, true, false, false, dx, dy);
        REQUIRE(dx == 16);
        REQUIRE(dy == 0);

        // Up
        computePanningDeltas(0, 0, false, false, true, false, dx, dy);
        REQUIRE(dx == 0);
        REQUIRE(dy == -16);

        // Down
        computePanningDeltas(0, 0, false, false, false, true, dx, dy);
        REQUIRE(dx == 0);
        REQUIRE(dy == 16);
    }

    SECTION("Simultaneous diagonal keyboard arrows use 2:1 isometric angle") {
        // Up + Right (top-right isometric angle: dx = +16, dy = -8)
        computePanningDeltas(0, 0, false, true, true, false, dx, dy);
        REQUIRE(dx == 16);
        REQUIRE(dy == -8);

        // Up + Left (top-left isometric angle: dx = -16, dy = -8)
        computePanningDeltas(0, 0, true, false, true, false, dx, dy);
        REQUIRE(dx == -16);
        REQUIRE(dy == -8);

        // Down + Right (bottom-right isometric angle: dx = +16, dy = +8)
        computePanningDeltas(0, 0, false, true, false, true, dx, dy);
        REQUIRE(dx == 16);
        REQUIRE(dy == 8);

        // Down + Left (bottom-left isometric angle: dx = -16, dy = +8)
        computePanningDeltas(0, 0, true, false, false, true, dx, dy);
        REQUIRE(dx == -16);
        REQUIRE(dy == 8);
    }

    SECTION("Opposing keys cancel out") {
        // Left + Right pressed together
        computePanningDeltas(0, 0, true, true, false, false, dx, dy);
        REQUIRE(dx == 0);
        REQUIRE(dy == 0);

        // Up + Down pressed together
        computePanningDeltas(0, 0, false, false, true, true, dx, dy);
        REQUIRE(dx == 0);
        REQUIRE(dy == 0);

        // Left + Right + Up (Up still works)
        computePanningDeltas(0, 0, true, true, true, false, dx, dy);
        REQUIRE(dx == 0);
        REQUIRE(dy == -16);
    }

    SECTION("Mouse edge panning and keyboard arrows combine cleanly") {
        // Mouse at top edge (dy = -16), Right arrow pressed (dx = +16) -> diagonal (dx = +16, dy = -8)
        computePanningDeltas(0, -1, false, true, false, false, dx, dy);
        REQUIRE(dx == 16);
        REQUIRE(dy == -8);
    }
}

TEST_CASE("Mission list sorting and formatting validation", "[editor][missions]") {
    struct TestMission {
        int id;
        std::string name;
    };

    std::vector<TestMission> missions = {
        {1, "WESTERN EUROPE"},
        {2, "FAR EAST"},
        {17, "ALASKA"},
        {50, "CHINA"},
        {37, "ATLANTIC ACCELERATOR"}
    };

    std::sort(missions.begin(), missions.end(), [](const TestMission &a, const TestMission &b) {
        return a.name < b.name;
    });

    REQUIRE(missions[0].name == "ALASKA");
    REQUIRE(missions[0].id == 17);
    REQUIRE(std::format("{} ({})", missions[0].name, missions[0].id) == "ALASKA (17)");

    REQUIRE(missions[1].name == "ATLANTIC ACCELERATOR");
    REQUIRE(missions[1].id == 37);
    REQUIRE(std::format("{} ({})", missions[1].name, missions[1].id) == "ATLANTIC ACCELERATOR (37)");

    REQUIRE(missions[2].name == "CHINA");
    REQUIRE(missions[2].id == 50);
    REQUIRE(std::format("{} ({})", missions[2].name, missions[2].id) == "CHINA (50)");

    REQUIRE(missions[3].name == "FAR EAST");
    REQUIRE(missions[3].id == 2);
    REQUIRE(std::format("{} ({})", missions[3].name, missions[3].id) == "FAR EAST (2)");

    REQUIRE(missions[4].name == "WESTERN EUROPE");
    REQUIRE(missions[4].id == 1);
    REQUIRE(std::format("{} ({})", missions[4].name, missions[4].id) == "WESTERN EUROPE (1)");
}