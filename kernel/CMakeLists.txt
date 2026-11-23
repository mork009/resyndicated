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
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/mod.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/modowner.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/position.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/path.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/pathsurfaces.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/leveldata.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/agent.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/ipastim.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/mapobject.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/missionbriefing.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/research.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/sfxobject.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/shot.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/static.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/damage.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/map.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/mission.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/objectivedesc.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/pedactionstate.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/ped.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/train.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/vehicle.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/squad.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/time.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/weapon.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/model/weaponholder.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/ia/actions.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/ia/behaviour.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/mgr/agentmanager.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/mgr/pedmanager.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/mgr/mapmanager.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/mgr/modmanager.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/mgr/researchmanager.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/mgr/weaponmanager.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/mgr/missionmanager.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/mgr/mission_triad.h"
    "${ReSyndicated_SOURCE_DIR}/kernel/include/rs-kernel/mgr/vehiclemanager.h"
    )

set(SOURCE_LIST
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/agent.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/ipastim.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/mod.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/map.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/mapobject.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/missionbriefing.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/mission.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/objectivedesc.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/ped.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/pedactions.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/pedpathfinding.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/research.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/static.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/sfxobject.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/shot.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/squad.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/time.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/train.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/vehicle.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/weapon.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/model/weaponholder.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/ia/actions.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/ia/behaviour.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/mgr/agentmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/mgr/pedmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/mgr/mapmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/mgr/modmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/mgr/researchmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/mgr/weaponmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/mgr/missionmanager.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/mgr/mission_triad.cpp"
    "${ReSyndicated_SOURCE_DIR}/kernel/src/mgr/vehiclemanager.cpp"
    )

# Make an automatic library - will be static or dynamic based on user setting
add_library(rs_kernel ${SOURCE_LIST} ${HEADER_LIST})
add_library(ReSyndicated::Kernel ALIAS rs_kernel)

# We need this directory, and users of our library will need it too
target_include_directories(rs_kernel PUBLIC include)

target_link_libraries(rs_kernel PRIVATE resynd_warnings ReSyndicated::Utils ReSyndicated::Engine )

target_link_libraries(rs_kernel PRIVATE Tracy::TracyClient)
target_compile_definitions(rs_kernel PUBLIC TRACY_ENABLE)

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/kernel/include"
  FILES ${HEADER_LIST})

if(BUILD_TESTING)
    add_subdirectory(test)
endif()
