#
#  ReSyndicated - a modernized game engine and full map/mission editor for the
#  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
#  project, FreeSynd.
#
#   Copyright (C) 2023-2025  Benoit Blancard <benblan@users.sourceforge.net>
#   Copyright (C) 2026  mork009 <mork009resynd@gmail.com>
#
#   This program is free software: you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation, either version 3 of the
#  License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#  See the GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#  along with this program. If not, see <https://www.gnu.org/licenses/>.
#

set(HEADER_LIST
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/enginecommon.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/base_app.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/appcontext.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/config.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/system/system.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/cp437.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/dirtylist.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/font.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/fontmanager.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/logomanager.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/sprite.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/spritemanager.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/base_weapon_sprites.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/animationloader.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/animationmanager.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/tile.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/tilemanager.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/rstexture.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/gfx/animationplayer.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/sound/audio.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/sound/musicmanager.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/sound/sound.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/sound/soundmanager.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/io/keys.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/io/resources.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/menus/menu.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/menus/menumanager.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/menus/widget.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/menus/fliplayer.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/menus/flimenu.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/menus/logoutmenu.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/events/event.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/events/default_events.h"
    "${ReSyndicated_SOURCE_DIR}/engine/include/rs-engine/ui/settings_dialog.h"
    )

set(SOURCE_LIST
    "${ReSyndicated_SOURCE_DIR}/engine/src/base_app.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/appcontext.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/ui/settings_dialog.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/system/system.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/dirtylist.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/font.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/fontmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/sprite.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/spritemanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/animationloader.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/animationmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/animationplayer.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/logomanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/tile.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/gfx/tilemanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/sound/audio.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/sound/musicmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/sound/soundmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/menus/menu.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/menus/menumanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/menus/widget.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/menus/fliplayer.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/menus/flimenu.cpp"
    "${ReSyndicated_SOURCE_DIR}/engine/src/menus/logoutmenu.cpp"
    )


find_package (PNG REQUIRED)
find_package (CLI11 REQUIRED)
find_package (utf8cpp REQUIRED)

# This is the main library for this directory
add_library(rs_engine ${SOURCE_LIST} ${HEADER_LIST})
add_library(ReSyndicated::Engine ALIAS rs_engine)

# We need this directory, and users of our library will need it too
target_include_directories(rs_engine PUBLIC include)

target_link_libraries(rs_engine PRIVATE resynd_warnings ReSyndicated::Utils PNG::PNG CLI11::CLI11 utf8cpp comctl32 dwmapi ole32 shell32 uxtheme msimg32)
target_link_libraries(rs_engine INTERFACE CLI11::CLI11 utf8cpp)

target_link_libraries(rs_engine PRIVATE Tracy::TracyClient)
target_compile_definitions(rs_engine PUBLIC TRACY_ENABLE)

# The USE_SYSTEM_SDL option enables the creation of the rs_engine_sdl
# library. If not on, there will be an error as there is not other implementation
option(USE_SYSTEM_SDL "Use SDL Library" ON)

if (USE_SYSTEM_SDL)
    target_compile_definitions(rs_engine PRIVATE "SYSTEM_SDL")

    # Find the SDL3 library
    find_package (SDL3 REQUIRED CONFIG)
    find_package (SDL3_image REQUIRED CONFIG)

    add_library(rs_engine_sdl STATIC
                  "${ReSyndicated_SOURCE_DIR}/engine/src/system/sdl/system_sdl.cpp"
                  "${ReSyndicated_SOURCE_DIR}/engine/src/system/sdl/rstexture_sdl.cpp"

                  "${ReSyndicated_SOURCE_DIR}/engine/src/system/sdl/system_sdl.h"
                  "${ReSyndicated_SOURCE_DIR}/engine/src/system/sdl/rstexture_sdl.h"
                )
    add_library(ReSyndicated::Engine::Sdl ALIAS rs_engine_sdl)

    target_include_directories(rs_engine_sdl PUBLIC src/system)
    target_include_directories(rs_engine_sdl PRIVATE include)

    target_link_libraries(rs_engine_sdl PUBLIC SDL3::SDL3 SDL3_image::SDL3_image ReSyndicated::Utils dwmapi)
    target_link_libraries(rs_engine_sdl PRIVATE resynd_warnings utf8cpp)

    target_link_libraries(rs_engine PRIVATE ReSyndicated::Engine::Sdl)
endif()

# The USE_SDL_MIXER option enables the creation of the rs_engine_sdlmixer
# library. If not on, there will be the default Audio implementation
option(USE_SDL_MIXER "Use SDL Audio library" ON)

if (USE_SDL_MIXER)

    add_subdirectory(extern)

    add_library(rs_engine_sdlmixer STATIC
                  "${ReSyndicated_SOURCE_DIR}/engine/src/sound/mixer/sdlmixeraudio.cpp"
                  "${ReSyndicated_SOURCE_DIR}/engine/src/sound/mixer/sdlmixersound.cpp"

                  "${ReSyndicated_SOURCE_DIR}/engine/src/sound/mixer/sdlmixeraudio.h"
                  "${ReSyndicated_SOURCE_DIR}/engine/src/sound/mixer/sdlmixersound.h"
                )
    add_library(ReSyndicated::Engine::SdlMixer ALIAS rs_engine_sdlmixer)

    target_include_directories(rs_engine_sdlmixer PUBLIC src/sound)
    target_include_directories(rs_engine_sdlmixer PRIVATE include)
    target_link_libraries(rs_engine_sdlmixer PRIVATE resynd_warnings ReSyndicated::Utils SDL3::SDL3 ADLMIDI)

    target_compile_definitions(rs_engine_sdl PRIVATE "HAVE_SDL_MIXER")

    target_link_libraries(rs_engine_sdl PUBLIC ReSyndicated::Engine::SdlMixer)
endif()


# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/engine/include"
  FILES ${HEADER_LIST})

if(BUILD_TESTING)
  add_subdirectory(test)
endif()