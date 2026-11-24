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
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/common.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/log/log.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/io/dernc.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/io/file.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/io/configfile.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/io/resyndconfig.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/io/portablefile.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/io/formatversion.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/misc/singleton.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/misc/seqmodel.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/misc/timer.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/misc/win_console.h"
    "${ReSyndicated_SOURCE_DIR}/utils/include/rs-utils/version.h"
    )

set(SOURCE_LIST
    "${ReSyndicated_SOURCE_DIR}/utils/src/log.cpp"
    "${ReSyndicated_SOURCE_DIR}/utils/src/file.cpp"
    "${ReSyndicated_SOURCE_DIR}/utils/src/configfile.cpp"
    "${ReSyndicated_SOURCE_DIR}/utils/src/resyndconfig.cpp"
    "${ReSyndicated_SOURCE_DIR}/utils/src/portablefile.cpp"
    "${ReSyndicated_SOURCE_DIR}/utils/src/dernc.cpp"
    "${ReSyndicated_SOURCE_DIR}/utils/src/rnc_pack.cpp"
    "${ReSyndicated_SOURCE_DIR}/utils/src/seqmodel.cpp"
    "${ReSyndicated_SOURCE_DIR}/utils/src/win_console.cpp"
    )

# Definition of the rs_utils library - will be static or dynamic based on user setting
add_library(rs_utils ${SOURCE_LIST} ${HEADER_LIST})
add_library(ReSyndicated::Utils ALIAS rs_utils)

target_link_libraries(rs_utils PRIVATE resynd_warnings)

# We need this directory, and users of our library will need it too (ie PUBLIC)
target_include_directories(rs_utils PUBLIC include)

# Set standard definitions for all platforms.
target_compile_definitions(rs_utils PRIVATE RS_DATA_DIR=\"${RS_DATA_DIR}\")

# IDEs should put the headers in a nice place
source_group(
  TREE "${PROJECT_SOURCE_DIR}/utils/include"
  FILES ${HEADER_LIST})

