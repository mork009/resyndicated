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

set(CPACK_PACKAGE_DESCRIPTION "This package will install the ReSyndicated application.")
set(CPACK_PACKAGE_CHECKSUM SHA256)
if(NOT RESYND_ARCH)
    set(RESYND_ARCH "x64")
endif()
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${RESYND_ARCH}")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.md")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

install(FILES "${CMAKE_SOURCE_DIR}/changelog.txt" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/README.md" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/LICENSE.md" DESTINATION .)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/extern/sdl3/lib/${RESYND_ARCH}/" DESTINATION .
        FILES_MATCHING PATTERN "*.dll")

install(DIRECTORY "${CMAKE_SOURCE_DIR}/extern/SDL3_image/lib/${RESYND_ARCH}/" DESTINATION .
        FILES_MATCHING PATTERN "*.dll"
        PATTERN "optional" EXCLUDE)

# Resulting package will be generated in this directory
set(CPACK_PACKAGE_DIRECTORY "PackageOutput")
set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})

set(CPACK_STRIP_FILES YES)

# Source packaging
set(CPACK_SOURCE_GENERATOR "ZIP")
set(CPACK_SOURCE_IGNORE_FILES
    /build.*
    /\\\\.git/
    /\\\\.gitignore
    game/version.h)

# Binary package generator
set(CPACK_GENERATOR ZIP)

include(CPack)