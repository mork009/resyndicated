# The released editor includes the Win32 UI and the matching playtest executable.
add_executable(resynd_editor MACOSX_BUNDLE EXCLUDE_FROM_ALL editor_launcher.mm)
set_target_properties(resynd_editor PROPERTIES
    OUTPUT_NAME "ReSyndicated Editor"
    MACOSX_BUNDLE_BUNDLE_NAME "ReSyndicated Editor"
    MACOSX_BUNDLE_GUI_IDENTIFIER org.resyndicated.editor
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    OBJCXX_STANDARD 20 OBJCXX_STANDARD_REQUIRED ON)
target_compile_options(resynd_editor PRIVATE -fobjc-arc)
target_link_libraries(resynd_editor PRIVATE ReSyndicated::Utils "-framework AppKit")

add_custom_target(resynd_editor_runtime
    COMMAND ${CMAKE_COMMAND}
        "-Ddestination=$<TARGET_BUNDLE_CONTENT_DIR:resynd_editor>/Resources"
        "-Dcache_dir=${CMAKE_BINARY_DIR}/editor-downloads"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/prepare_runtime.cmake"
    COMMENT "Preparing Wine and the ReSyndicated editor"
    VERBATIM)
add_dependencies(resynd_editor resynd_editor_runtime)
