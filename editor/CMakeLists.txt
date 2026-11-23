#
#  ReSyndicated - a modernized game engine and full map/mission editor for the
#  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
#  project, FreeSynd.
#
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

set(WIN_EDITOR_SOURCES
    main.cpp
    resource.rc
    core/editor_commands.cpp
    core/editor_config.cpp
    core/mission_validator.cpp
    core/playtest_manager.cpp
    dialogs/dialog_theme.cpp
    dialogs/editor_settings_dialog.cpp
    dialogs/mission_chooser.cpp
    dialogs/mission_properties_dialog.cpp
    dialogs/new_mission_dialog.cpp
    dialogs/objective_dialog.cpp
    dialogs/playtest_loadout_dialog.cpp
    dialogs/save_mission_as_dialog.cpp
    dialogs/ai_tactics_dialog.cpp
    graphics/icon_manager.cpp
    graphics/sprite_rasterizer.cpp
    graphics/tile_rasterizer.cpp
    views/art_panel.cpp
    views/main_window.cpp
    views/map_viewport.cpp
)

set(WIN_EDITOR_HEADERS
    resource.h
    core/editor_commands.h
    core/editor_config.h
    core/mission_validator.h
    core/playtest_manager.h
    dialogs/dialog_theme.h
    dialogs/editor_settings_dialog.h
    dialogs/mission_chooser.h
    dialogs/mission_properties_dialog.h
    dialogs/new_mission_dialog.h
    dialogs/objective_dialog.h
    dialogs/playtest_loadout_dialog.h
    dialogs/save_mission_as_dialog.h
    dialogs/ai_tactics_dialog.h
    graphics/icon_manager.h
    graphics/sprite_rasterizer.h
    graphics/tile_rasterizer.h
    views/art_panel.h
    views/main_window.h
    views/map_viewport.h
)

add_executable(resynd_editor WIN32
    ${WIN_EDITOR_SOURCES}
    ${WIN_EDITOR_HEADERS}
)
add_dependencies(resynd_editor bundle_resynd_data)

set_source_files_properties(resource.rc PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons_master_48.dat;${CMAKE_CURRENT_SOURCE_DIR}/resources/toolbar_icons_24.bmp;${CMAKE_CURRENT_SOURCE_DIR}/resources/toolbar_icons_48.bmp;${CMAKE_CURRENT_SOURCE_DIR}/resources/tab_icons_16.bmp;${CMAKE_CURRENT_SOURCE_DIR}/resources/tab_icons_32.bmp;${CMAKE_CURRENT_SOURCE_DIR}/resources/logo.ico"
)

target_compile_definitions(resynd_editor PRIVATE NOMINMAX UNICODE _UNICODE)

target_include_directories(resynd_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/views
    ${CMAKE_SOURCE_DIR}/engine/include
    ${CMAKE_SOURCE_DIR}/kernel/include
    ${CMAKE_SOURCE_DIR}/utils/include
)

target_link_libraries(resynd_editor PRIVATE
    resynd_warnings
    ReSyndicated::Utils
    ReSyndicated::Engine
    ReSyndicated::Kernel
    comctl32
    user32
    gdi32
    comdlg32
    shell32
    shlwapi
    dwmapi
    ole32
    uxtheme
    msimg32
)

install(TARGETS resynd_editor DESTINATION .)
