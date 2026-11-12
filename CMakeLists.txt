# wwe2k16 - WWE 2K16 (Xbox 360) static recompilation with the ReXGlue SDK v0.10 (branch native-sdk10).
# SDK boilerplate lives in generated_v10/rexglue.cmake (rendered by `rexglue init`, see docs/NATIVE_PORT_PLAN.md).
# The v0.7.3 build of Banshee's version is on the main branch.

cmake_minimum_required(VERSION 3.25)
project(wwe2k16 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated_v10/rexglue.cmake)

set(WWE2K16_SOURCES
    src/main.cpp
    src/exit_menu.cpp
    src/shadows.cpp
    src/framerate_hook.cpp
    src/test_input.cpp
    src/fps_overlay.cpp
    src/display_refresh_rate.cpp
    src/achievement_sound.cpp
    src/achievements_summary.cpp
    third_party/stb_vorbis/stb_vorbis.c
)

if(WIN32)
    enable_language(RC)
endif()

if(WIN32)
    add_executable(wwe2k16 WIN32 ${WWE2K16_SOURCES})
else()
    add_executable(wwe2k16 ${WWE2K16_SOURCES})
endif()


# Native renderer (docs/NATIVE_PORT_PLAN.md): a second executable, wwe2k16_native.exe, that draws the game's D3D calls
# through re:Blue + plume (Vulkan), taken from WWE 2K15 PC / WWE '12 PC. It links the same recompiled objects; wwe2k16.exe
# (Xenos GPU emulation) is unchanged. Needs plume and reblue-XenosRecomp in native-deps/ (WWE2K16_NATIVE_DEPS).
# Added before the SDK libraries so plume's VMA wins over rexruntime's (see WWE '12 PC).
option(WWE2K16_NATIVE_RENDERER "Also build wwe2k16_native.exe (native Vulkan renderer)" OFF)
if(WWE2K16_NATIVE_RENDERER)
    include(cmake/native_renderer.cmake)
    add_executable(wwe2k16_native WIN32 ${WWE2K16_SOURCES} res/wwe2k16.rc)
    target_compile_definitions(wwe2k16_native PRIVATE WWE2K16_TU=1)
    wwe2k16_add_native_renderer(wwe2k16_native)
endif()
target_compile_definitions(wwe2k16 PRIVATE WWE2K16_TU=1)
rexglue_setup_target(wwe2k16 GPU_PLUGINS xenos)

if(WWE2K16_NATIVE_RENDERER)
    # wwe2k16_recomp only exists once codegen has written generated_v10/ (a fresh checkout's first configure doesn't).
    if(TARGET wwe2k16_recomp)
        target_sources(wwe2k16_native PRIVATE $<TARGET_OBJECTS:wwe2k16_recomp>)
    endif()
    target_include_directories(wwe2k16_native PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${REXGLUE_ENTRYPOINT_INCLUDE_DIR})
    target_link_libraries(wwe2k16_native PRIVATE rex::runtime)
    add_dependencies(wwe2k16_native wwe2k16_codegen)
    rexglue_apply_target_settings(wwe2k16_native)
    rexglue_configure_target(wwe2k16_native GPU_PLUGINS xenos)
    if(MSVC)
        target_link_options(wwe2k16_native PRIVATE /DEBUG /OPT:REF /OPT:ICF)
    endif()
endif()

if(WIN32)
    enable_language(RC)
    target_sources(wwe2k16 PRIVATE res/wwe2k16.rc)
    set_source_files_properties(res/wwe2k16.rc PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/wwe2k16.ico)
    if(MSVC)
        target_link_options(wwe2k16 PRIVATE /DEBUG /OPT:REF /OPT:ICF)
    endif()
endif()
