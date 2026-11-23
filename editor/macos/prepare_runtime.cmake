cmake_minimum_required(VERSION 3.28)
file(MAKE_DIRECTORY "${cache_dir}" "${destination}")

function(stage_archive url hash entry directory)
    get_filename_component(archive "${url}" NAME)
    set(cached "${cache_dir}/${archive}")
    file(LOCK "${cached}.lock" GUARD FUNCTION TIMEOUT 300)
    set(stamp "${destination}/${directory}/.version")
    if(EXISTS "${stamp}" AND EXISTS "${destination}/${directory}/${entry}")
        file(READ "${stamp}" installed)
        if(installed STREQUAL hash)
            return()
        endif()
    endif()

    file(DOWNLOAD "${url}" "${cached}" EXPECTED_HASH "SHA256=${hash}" TLS_VERIFY ON SHOW_PROGRESS)
    # Extract away from the live runtime so a failed download or unpack can be retried.
    set(staging "${destination}/${directory}.staging")
    file(REMOVE_RECURSE "${staging}")
    file(MAKE_DIRECTORY "${staging}")
    execute_process(COMMAND /usr/bin/tar -xf "${cached}" -C "${staging}" --strip-components 1
        COMMAND_ERROR_IS_FATAL ANY)
    if(NOT EXISTS "${staging}/${entry}")
        message(FATAL_ERROR "Missing ${entry} in ${archive}")
    endif()
    file(WRITE "${staging}/.version" "${hash}")
    file(REMOVE_RECURSE "${destination}/${directory}")
    file(RENAME "${staging}" "${destination}/${directory}")
endfunction()

stage_archive("https://github.com/mork009/resyndicated/releases/download/v1.0.0/ReSyndicated-1.0.0-x64.zip"
    "45909d47d69b0649fbbc0bfeb9130093285d8bb79ba5a6a48da6fb6e1db296f8"
    "resynd_editor.exe" editor)
stage_archive("https://github.com/Gcenx/macOS_Wine_builds/releases/download/11.21/wine-devel-11.21-osx64.tar.xz"
    "9909b486657e10ab66c7279371ae1f990345b596b3b1b3a0cf21f28af24fa0ce"
    "Contents/Resources/wine/bin/wine" "Wine Devel.app")
