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

add_custom_target(bundle_resynd_data ALL
    COMMENT "Bundling ReSyndicated data files into build directories"
)

if(WIN32)
    set(data_destinations game/data editor/data)
endif()
if(BUILD_TESTING)
    list(APPEND data_destinations kernel/test/data)
endif()

foreach(subdir IN LISTS RESYND_DATA_SUBDIRS)
    foreach(dest IN LISTS data_destinations)
        add_custom_command(TARGET bundle_resynd_data PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}" "${CMAKE_BINARY_DIR}/${dest}/${subdir}"
            VERBATIM)
    endforeach()
    if(NOT APPLE)
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}" DESTINATION data)
    endif()
endforeach()
