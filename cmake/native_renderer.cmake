# WWE 2K16 native renderer, taken from WWE '12 PC (iMrBurn/wwe12-pc, GPL-3.0), itself adapted from re:Blue, BSD-3-Clause, see src/reblue/LICENSE.reblue, through
# nefariousjosiah's SvR 2008-2010 ports, GPL-3.0).
#
# Draws through plume (Vulkan) by observing the game's XDK D3D functions, instead of emulating
# the Xenos GPU. Game shaders come precompiled from generated/native/shader_cache.cpp: the
# shaders extracted from the game files (tools/find_shaders.py --bpe) converted with the patched
# reblue-XenosRecomp (see the README).
#
# The renderer's sources go straight into the executable: its REX_HOOK overrides are strong
# definitions nothing else references, so a static library would drop them at link time.

enable_language(C)  # zstd and xxHash are C

set(WWE2K16_NATIVE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
# plume and reblue-XenosRecomp, cloned at the commits WWE '12 PC pins and patched with its patches
# (see docs/NATIVE_PORT_PLAN.md). Only needed for the native executable.
set(WWE2K16_NATIVE_DEPS "${CMAKE_CURRENT_SOURCE_DIR}/native-deps" CACHE PATH
    "Folder holding plume and reblue-XenosRecomp for the native renderer")
set(WWE2K16_RECOMP_DIR "${WWE2K16_NATIVE_DEPS}")
set(WWE2K16_XENOS_DIR "${WWE2K16_RECOMP_DIR}/reblue-XenosRecomp")
set(WWE2K16_PLUME_DIR "${WWE2K16_RECOMP_DIR}/plume")
set(WWE2K16_NATIVE_GEN "${CMAKE_BINARY_DIR}/native_gen")

# Rendering hardware interface: Vulkan. plume's D3D12 backend stays off, as in the SvR ports.
set(PLUME_D3D12_ENABLED OFF CACHE BOOL "" FORCE)
add_subdirectory("${WWE2K16_PLUME_DIR}" "${CMAKE_BINARY_DIR}/plume" EXCLUDE_FROM_ALL)

# Small third-party pieces the renderer uses.
add_library(wwe2k16_native_thirdparty STATIC
    "${WWE2K16_NATIVE_ROOT}/third_party/reblue_thirdparty/zstd/zstddeclib.c"
    "${WWE2K16_XENOS_DIR}/thirdparty/xxHash/xxhash.c"
    "${WWE2K16_XENOS_DIR}/thirdparty/smol-v/source/smolv.cpp")
target_include_directories(wwe2k16_native_thirdparty PUBLIC
    "${WWE2K16_NATIVE_ROOT}/third_party/reblue_thirdparty/zstd"
    "${WWE2K16_XENOS_DIR}/thirdparty/xxHash"
    "${WWE2K16_XENOS_DIR}/thirdparty/smol-v/source")

# Game shaders are converted on the player's PC (option chosen on 2026-10-27: no game-derived data
# in any package). XenosRecomp's translator (MIT) is built into the executable; DXC's
# dxcompiler.dll (shipped beside the exe) turns its HLSL into SPIR-V; the result is kept in
# cache/native_shaders.bin next to the exe (src/native/wwe2k16_shader_cache.cpp).
add_library(wwe2k16_xenosrecomp STATIC
    "${WWE2K16_XENOS_DIR}/XenosRecomp/shader_recompiler.cpp"
    "${WWE2K16_NATIVE_ROOT}/src/native_xr/xenos_translate.cpp")
# XenosRecomp's own fmt (header-only) stays private: the SDK's spdlog links another fmt build.
target_include_directories(wwe2k16_xenosrecomp PRIVATE
    "${WWE2K16_XENOS_DIR}/XenosRecomp"
    "${WWE2K16_XENOS_DIR}/thirdparty/fmt/include"
    "${WWE2K16_XENOS_DIR}/thirdparty/zstd/lib"
    "${WWE2K16_XENOS_DIR}/thirdparty/xxHash")
target_include_directories(wwe2k16_xenosrecomp PUBLIC
    "${WWE2K16_NATIVE_ROOT}/src/native_xr"
    "${WWE2K16_XENOS_DIR}/thirdparty/dxc-bin/inc"
    "${WWE2K16_XENOS_DIR}/thirdparty/smol-v/source")
target_compile_definitions(wwe2k16_xenosrecomp PRIVATE REBLUE_RECOMP FMT_HEADER_ONLY _CRT_SECURE_NO_WARNINGS)
if(MSVC)
    target_compile_options(wwe2k16_xenosrecomp PRIVATE "/FI${WWE2K16_XENOS_DIR}/XenosRecomp/pch.h"
        /clang:-Wno-switch /clang:-Wno-unused-variable /clang:-Wno-null-arithmetic)
else()
    target_compile_options(wwe2k16_xenosrecomp PRIVATE -include "${WWE2K16_XENOS_DIR}/XenosRecomp/pch.h"
        -Wno-switch -Wno-unused-variable -Wno-null-arithmetic -fms-extensions)
endif()
# The HLSL prelude XenosRecomp prepends to every shader (shader_common.h), embedded as bytes.
file(READ "${WWE2K16_XENOS_DIR}/XenosRecomp/shader_common.h" _common_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _common_bytes "${_common_hex}")
file(WRITE "${WWE2K16_NATIVE_GEN}/shader_common_text.inc" "${_common_bytes}")
set(WWE2K16_DXCOMPILER_DLL "${WWE2K16_XENOS_DIR}/thirdparty/dxc-bin/bin/x64/dxcompiler.dll")

# Host shaders (copy, ImGui overlay, MSAA resolve, gamma) compiled to SPIR-V headers with the
# DXC shipped in reblue-XenosRecomp (the Windows SDK's dxc has no SPIR-V backend).
set(WWE2K16_DXC "${WWE2K16_XENOS_DIR}/thirdparty/dxc-bin/bin/x64/dxc.exe")
set(WWE2K16_HLSL_DIR "${WWE2K16_NATIVE_ROOT}/src/reblue/gpu/shaders/hlsl")
set(WWE2K16_HOST_SHADER_HEADERS "")
function(wwe2k16_host_shader STEM PROFILE)
    file(GLOB hlsl_includes "${WWE2K16_HLSL_DIR}/*.hlsli")
    set(args -spirv -fvk-use-dx-layout)
    if(PROFILE MATCHES "^vs")
        list(APPEND args -fvk-invert-y)
    endif()
    set(out "${WWE2K16_NATIVE_GEN}/src/gpu/shaders/hlsl/${STEM}.hlsl.spirv.h")
    add_custom_command(
        OUTPUT "${out}"
        COMMAND "${WWE2K16_DXC}" -T ${PROFILE} -HV 2021 -all-resources-bound -Wno-ignored-attributes
                ${args} -I "${WWE2K16_NATIVE_ROOT}" -Fh "${out}" "${WWE2K16_HLSL_DIR}/${STEM}.hlsl"
                -Vn g_${STEM}_spirv
        DEPENDS "${WWE2K16_HLSL_DIR}/${STEM}.hlsl" ${hlsl_includes}
        COMMENT "Compiling ${STEM}.hlsl (${PROFILE}, SPIR-V)"
        VERBATIM)
    set(WWE2K16_HOST_SHADER_HEADERS ${WWE2K16_HOST_SHADER_HEADERS} "${out}" PARENT_SCOPE)
endfunction()
foreach(shader IN ITEMS copy_vs imgui_vs)
    wwe2k16_host_shader(${shader} vs_6_0)
endforeach()
foreach(shader IN ITEMS copy_color_ps copy_depth_ps gamma_correction_ps pfx_occlusion_count_ps
        imgui_ps resolve_msaa_color_2x resolve_msaa_color_4x resolve_msaa_color_8x
        resolve_msaa_depth_2x resolve_msaa_depth_4x resolve_msaa_depth_8x)
    wwe2k16_host_shader(${shader} ps_6_0)
endforeach()

file(GLOB_RECURSE WWE2K16_NATIVE_SOURCES CONFIGURE_DEPENDS
    "${WWE2K16_NATIVE_ROOT}/src/reblue/*.cpp" "${WWE2K16_NATIVE_ROOT}/src/reblue/*.h"
    "${WWE2K16_NATIVE_ROOT}/src/native/*.cpp" "${WWE2K16_NATIVE_ROOT}/src/native/*.h")

function(wwe2k16_add_native_renderer target)
    # No generated/native/shader_cache.cpp here: it would put game-derived shaders in the exe.
    target_sources(${target} PRIVATE
        ${WWE2K16_NATIVE_SOURCES}
        ${WWE2K16_HOST_SHADER_HEADERS})
    target_compile_definitions(${target} PRIVATE WWE2K16_RUNTIME_SHADERS=1)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WWE2K16_DXCOMPILER_DLL}"
            "$<TARGET_FILE_DIR:${target}>/dxcompiler.dll"
        VERBATIM)
    target_include_directories(${target} PRIVATE
        "${WWE2K16_NATIVE_ROOT}/src/reblue"
        "${WWE2K16_NATIVE_ROOT}/src/native"
        "${WWE2K16_NATIVE_GEN}"
        "${WWE2K16_PLUME_DIR}"
        "${WWE2K16_XENOS_DIR}/XenosRecomp"
    )
    target_compile_definitions(${target} PRIVATE SVR_NATIVE_RENDERER=1)
    # WWE 2K16's addresses for the D3D functions re:Blue's hooks name (D3DDevice_Clear -> sub_...).
    if(MSVC)
        set(_names_include "/FI${WWE2K16_NATIVE_ROOT}/src/native/wwe2k16_d3d_names.h")
    else()
        set(_names_include "-include;${WWE2K16_NATIVE_ROOT}/src/native/wwe2k16_d3d_names.h")
    endif()
    set_source_files_properties(${WWE2K16_NATIVE_SOURCES} PROPERTIES COMPILE_OPTIONS "${_names_include}")
    target_link_libraries(${target} PRIVATE plume wwe2k16_native_thirdparty wwe2k16_xenosrecomp rex::o1heap)
    # Linker map: names recompiled functions in crash stacks.
    if(MSVC)
        target_link_options(${target} PRIVATE "/MAP:${CMAKE_BINARY_DIR}/wwe2k16_native.map")
    elseif(WIN32)
        target_link_options(${target} PRIVATE "-Wl,/MAP:${CMAKE_BINARY_DIR}/wwe2k16_native.map")
    else()
        target_link_options(${target} PRIVATE "-Wl,-Map=${CMAKE_BINARY_DIR}/wwe2k16_native.map")
    endif()
endfunction()
