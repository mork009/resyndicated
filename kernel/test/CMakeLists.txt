#
#  ReSyndicated - a modernized game engine and full map/mission editor for the
#  classic Bullfrog game, Syndicate. ReSyndicated is forked off the open source
#  project, FreeSynd.
#
#   Copyright (C) 2023-2026  Benoit Blancard <benblan@users.sourceforge.net>
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

add_executable(test-kernel
    testcase.cpp
    mapobjecttest.cpp
    weaponholdertest.cpp
    weapontest.cpp
    squadtest.cpp
    vehicletest.cpp
    objectivedesctest.cpp
    pedtest.cpp
    pedactiontest.cpp
    missiontest.cpp
    timetest.cpp
    ipatest.cpp
    maptest.cpp
    rnctest.cpp
    triadtest.cpp
    editorconfigtest.cpp
    missionvalidatortest.cpp
    aitacticstest.cpp
    editorcommandstest.cpp
    ../../editor/core/editor_config.cpp
    ../../editor/core/mission_validator.cpp
    ../../editor/core/editor_commands.cpp
    ../../editor/dialogs/objective_dialog.cpp)

target_include_directories(test-kernel PRIVATE
    ${CMAKE_SOURCE_DIR}/editor
    ${CMAKE_SOURCE_DIR}/editor/core
    ${CMAKE_SOURCE_DIR}/editor/dialogs
    ${CMAKE_SOURCE_DIR}/editor/graphics
    ${CMAKE_SOURCE_DIR}/editor/views
)

target_link_libraries(test-kernel PRIVATE Catch2::Catch2WithMain ReSyndicated::Kernel ReSyndicated::Utils ReSyndicated::Engine)
add_dependencies(test-kernel bundle_resynd_data)

target_compile_definitions(test-kernel PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    GAME_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
    RS_TEST_KERNEL=1)

if (MSVC)
    target_compile_options(test-kernel PRIVATE /EHsc)
endif ()

# catch_discover_tests(test-kernel)
add_test(test-kernel test-kernel)
set_tests_properties(test-kernel PROPERTIES TIMEOUT 120)