# wwe2k16 - WWE 2K16 (Xbox 360) static recompilation with the ReXGlue SDK.
# SDK boilerplate lives in generated/rexglue.cmake (rendered by tools/codegen.ps1 via `rexglue init`).

cmake_minimum_required(VERSION 3.25)
project(wwe2k16 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated/rexglue.cmake)

set(WWE2K16_SOURCES
    src/main.cpp
    src/disc_image.cpp
    src/test_input.cpp
    src/resolve_coherency.cpp
    src/frame_rate.cpp
    src/launcher_settings.cpp
    src/screenshot.cpp
    src/achievement_toast.cpp
    src/achievement_sound.cpp
    src/fps_overlay.cpp
    src/shadows.cpp
    src/cp_profiler.cpp
    src/exit_menu.cpp
    src/display_refresh_rate.cpp
)

if(WIN32)
    add_executable(wwe2k16 WIN32 ${WWE2K16_SOURCES})
else()
    add_executable(wwe2k16 ${WWE2K16_SOURCES})
endif()

rexglue_setup_target(wwe2k16)

# Title update 3 build: wwe2k16_tu.exe from generated_tu\ (tools/build-tu.ps1). Same app code; WWE2K16_TU selects the
# title update's function addresses where the app hooks the game.
set(WWE2K16_TARGETS wwe2k16)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated_tu/sources.cmake")
    set(WWE2K16_RETAIL_SOURCES ${GENERATED_SOURCES})
    include(generated_tu/sources.cmake)
    set(WWE2K16_TU_SOURCES ${GENERATED_SOURCES})
    set(GENERATED_SOURCES ${WWE2K16_RETAIL_SOURCES})
    if(WIN32)
        add_executable(wwe2k16_tu WIN32 ${WWE2K16_SOURCES} ${WWE2K16_TU_SOURCES})
    else()
        add_executable(wwe2k16_tu ${WWE2K16_SOURCES} ${WWE2K16_TU_SOURCES})
    endif()
    target_include_directories(wwe2k16_tu PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/generated_tu)
    target_link_libraries(wwe2k16_tu PRIVATE rex::core rex::system rex::kernel rex::graphics rex::ui)
    rexglue_configure_target(wwe2k16_tu)
    target_compile_definitions(wwe2k16_tu PRIVATE WWE2K16_TU=1)
    list(APPEND WWE2K16_TARGETS wwe2k16_tu)
endif()

foreach(target IN LISTS WWE2K16_TARGETS)
if(WIN32)
    enable_language(RC)
    target_sources(${target} PRIVATE res/wwe2k16.rc)
    target_link_libraries(${target} PRIVATE windowscodecs ole32 shell32 winmm gdiplus)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/res/achievement.ogg"
                "$<TARGET_FILE_DIR:${target}>/achievement.ogg")
    target_link_options(${target} PRIVATE "LINKER:/map:$<TARGET_FILE_DIR:${target}>/${target}.map")
endif()
endforeach()
