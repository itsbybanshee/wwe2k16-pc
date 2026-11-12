# wwe2k16 - WWE 2K16 (Xbox 360) static recompilation with the ReXGlue SDK v0.10 (branch native-sdk10).
# SDK boilerplate lives in generated_v10/rexglue.cmake (rendered by `rexglue init`, see docs/NATIVE_PORT_PLAN.md).
# The v0.7.3 build of Banshee's version is on the main branch.

cmake_minimum_required(VERSION 3.25)
project(wwe2k16 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(generated_v10/rexglue.cmake)

set(WWE2K16_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(wwe2k16 WIN32 ${WWE2K16_SOURCES})
else()
    add_executable(wwe2k16 ${WWE2K16_SOURCES})
endif()

rexglue_setup_target(wwe2k16 GPU_PLUGINS xenos)

if(WIN32)
    enable_language(RC)
    target_sources(wwe2k16 PRIVATE res/wwe2k16.rc)
    set_source_files_properties(res/wwe2k16.rc PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/wwe2k16.ico)
    if(MSVC)
        target_link_options(wwe2k16 PRIVATE /DEBUG /OPT:REF /OPT:ICF)
    endif()
endif()
