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

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

find_package(SDL3 REQUIRED CONFIG)

set (SOURCES
	app.cpp
	core/gamesession.cpp
	core/gamecontroller.cpp
	resynd.cpp
	menus/agentselectorrenderer.cpp
	menus/maprenderer.cpp
	menus/minimaprenderer.cpp
	menus/briefmenu.cpp
	menus/confmenu.cpp
	menus/debriefmenu.cpp
	menus/gameplaymenu.cpp
	menus/loadingmenu.cpp
	menus/loadsavemenu.cpp
	menus/mainmenu.cpp
	menus/mapmenu.cpp
	menus/gamemenufactory.cpp
	menus/researchmenu.cpp
	menus/selectmenu.cpp
	menus/squadselection.cpp
)

set(HEADERS
    version.h
	app.h
	core/gamesession.h
	core/gamecontroller.h
	menus/agentselectorrenderer.h
	menus/maprenderer.h
	menus/minimaprenderer.h
	menus/briefmenu.h
	menus/confmenu.h
	menus/debriefmenu.h
	menus/gameplaymenu.h
	menus/loadingmenu.h
	menus/loadsavemenu.h
	menus/mainmenu.h
	menus/mapmenu.h
	menus/gamemenufactory.h
	menus/gamemenuid.h
	menus/researchmenu.h
	menus/selectmenu.h
	menus/squadselection.h
)

source_group(Menus REGULAR_EXPRESSION menus/*)
source_group(Core REGULAR_EXPRESSION core/*)

set(WIN_RESOURCES "${CMAKE_CURRENT_SOURCE_DIR}/resynd.rc")
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/resynd.rc" PROPERTIES OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/logo.ico"
)
add_executable (resynd_game WIN32 ${SOURCES} ${HEADERS} ${WIN_RESOURCES})

target_link_libraries (resynd_game PRIVATE resynd_warnings ReSyndicated::Utils ReSyndicated::Engine ReSyndicated::Kernel SDL3::SDL3)
add_dependencies (resynd_game bundle_resynd_data)

target_link_libraries(resynd_game PRIVATE Tracy::TracyClient)
target_compile_definitions(resynd_game PRIVATE TRACY_ENABLE)

install (TARGETS resynd_game DESTINATION .)