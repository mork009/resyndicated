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

set(RESYND_DATA_SUBDIRS cursors lang ref resources)

add_custom_target(bundle_resynd_data ALL
    COMMENT "Bundling ReSyndicated data files into build directories"
)

foreach(SUBDIR ${RESYND_DATA_SUBDIRS})
    add_custom_command(TARGET bundle_resynd_data PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/game/data/${SUBDIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}"
            "${CMAKE_BINARY_DIR}/game/data/${SUBDIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/editor/data/${SUBDIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}"
            "${CMAKE_BINARY_DIR}/editor/data/${SUBDIR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/kernel/test/data/${SUBDIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}"
            "${CMAKE_BINARY_DIR}/kernel/test/data/${SUBDIR}"
    )
endforeach()

# Install ReSyndicated bundled data directory for distribution packaging
foreach(SUBDIR ${RESYND_DATA_SUBDIRS})
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${SUBDIR}" DESTINATION "data")
endforeach()