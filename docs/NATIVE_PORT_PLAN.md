# WWE 2K16: native renderer port plan

Branch `native-sdk10`. Started 2026-11-12 by iMrBurn, with Banshee's agreement to take the project over.

## Why

Measured on a RTX 5070 Laptop (see the session notes): at 1440p the Vulkan/Xenos path keeps the GPU at 99 % during
victory scenes (about 50 late frames per second, 27 images actually shown), independent of shadows, AA and
anisotropy. The cost is the emulation itself (EDRAM, resolves, per-draw shader work). WWE '12, WWE 2K14 and WWE 2K15
avoid it with a native renderer: re:Blue + plume draw the game's own XDK Direct3D calls straight on the GPU. On
2K15 that holds 60 fps on 2 cores and on integrated GPUs where the Xenos path drops to 30.

## Decision (2026-11-12)

Rebuild 2K16 on the same stack as WWE 2K15 / WWE '12: ReXGlue v0.10.0 (`c94f5eb`) with the WWE 2K15 SDK patches,
instead of Banshee's older SDK (a v0.7.3 fork whose hook API is `PPC_FUNC`, not `REX_HOOK`). The native renderer
code of WWE 2K15 then drops in, and fixes made on one title reach the others.

**Banshee's launcher stays.** It is a separate WebView2 app (`launcher/`) that only starts the game with command
line options and `WWE2K16_*` environment variables, so it does not depend on the SDK. The new executables must keep
accepting that contract (options the native renderer has no use for are accepted and ignored):

* arguments: game path, `--user_data_root`, `--update_data_root`, `--log_file`, `--resolution_scale`,
  `--window_width`, `--window_height`, `--fullscreen` / `--no-fullscreen`, `--no-enable_console`,
  `--xmp_music_dir`, `--anisotropic_override`, `--user_language`, `--swap_post_effect`, `--present_effect`,
  `--keybind_*`, `--pad_remap`;
* environment: `WWE2K16_VSYNC`, `_KEEP_60`, `_SHADOWS`, `_FPS_COUNTER`, `_WINDOW_MODE`, `_RESOLUTION`,
  `_WINDOW_WIDTH`, `_WINDOW_HEIGHT`, `_ANISO`, `_AA`, `_UPSCALE`, `_GAME_OVERLAY`.

Banshee's in-game features (60 fps entrances and victories, shadows switch, FPS overlay, achievements, exit menu,
screenshots, key bindings, pad remap, display refresh rate) live in `src/*.cpp` as hooks on the Xenos path and are
ported file by file to the new hook API.

## Stages

1. **D3D map** (done): `docs/d3d_functions.md`. 22 functions for the base game and the title update.
2. **SDK**: `sdk-v10/` is a copy of the local WWE '12 SDK checkout reset to `c94f5eb` with the 32 WWE 2K15 SDK
   patches applied (all but `xctd-compression-info` and the 2K15/2K13-specific `wwe2k15-memory-range`). Build and
   install to `sdk-v10/out/install/ninja-clangcl`. No download was needed.
3. **Boot on the new stack, Xenos path**: codegen of 2K16 with the v0.10 `rexglue` (config and function lists from
   `config/`, `config_tu/`), CMake ported, `src/*.cpp` hooks ported. Goal: same behaviour as today before touching
   the renderer.
4. **Shaders**: extract the 2K16 shader containers from the player's files, XenosRecomp + DXC, generated cache.
   plume and reblue-XenosRecomp come from the local WWE '12 checkout (`recomp/`), no download.
5. **Renderer shell**: `cmake/native_renderer.cmake`, hooks from `docs/d3d_functions.md`, present and clear, menus.
6. **3D**: declarations, textures, render targets, resolves, depth, tiling.
7. **Game features**: created attire readback, videos, shadows, post effects, resolution scale, the launcher options.

## Licence

WWE '12 PC and WWE 2K15 PC (the source of the renderer) are GPL-3.0. This repository has no licence yet: before
publishing a native build, settle it with Banshee in writing and state the licence of the combined work.
