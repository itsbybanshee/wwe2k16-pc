# WWE 2K16 PC Recomp

> **v2.0.0 (fork by [iMrBurn](https://github.com/iMrBurn))** - continuation of [Banshee's WWE 2K16 PC](https://github.com/itsbybanshee/wwe2k16-pc), who no longer works on his recomps. This version runs on ReXGlue SDK v0.10 with a **native Vulkan renderer** (re:Blue + plume) instead of Xbox 360 GPU emulation: far lower GPU load, steady 60 FPS, up to 4K. Banshee's launcher and in-game features are kept.
> Download: [Releases](https://github.com/iMrBurn/wwe2k16-pc/releases/latest). Needs **WWE 2K16 Title Update 3** (installed from the launcher) and a Vulkan 1.2 GPU. The older GPU-emulation build is `bin\wwe2k16.exe` in the zip.
>
> New in v2.0.0: native renderer, upscaler (the launcher's "fsr" option is a Lanczos-based upscaler, not AMD FSR), automatic graphics profile for weak GPUs, pipeline seed against first-run stutter, ported achievement pop-up.
> Known limits: screenshots are window-sized; rare crashes may remain (please open an issue with your log). Build notes: `docs/NATIVE_PORT_PLAN.md`, `tools/build-v10.ps1`.
> Licence: GPL-3.0 (see `LICENSE`). Banshee's written permission, in the credits of [his README](https://github.com/itsbybanshee/wwe2k16-pc#credits): "I allow MrBurn to take over my code and distribute it with his own modifications."
> Original text below is Banshee's (v1.x); parts about FSR refer to his Xenos build.

WWE 2K16 PC recompilation with a modern launcher, graphics options, non-destructive mod support, custom soundtrack, achievements and more.

- No game data included. You need your own legally acquired WWE 2K16 (Xbox 360) files: a disc image (`.iso`) or the
  extracted game folder.

## Socials

If you want to support me, here's my [Patreon](https://www.patreon.com/c/Banshee734).

To keep up with updates, follow me on [Twitter](https://x.com/itsbybanshee).

## Features

- **Resolution**: 720p, 1080p, 1440p or 4K
- **AMD FSR**
- **NVIDIA FXAA 3.11 Quality** anti-aliasing (FXAA / Max)
- **Smooth 60 FPS gameplay** with 60 FPS entrances and victory motions (toggle in Settings)
- **Keyboard gameplay** with a modern WWE 2K-style layout
- **Button mapper** for controller and keyboard, with export / import of control schemes
- **Languages**: English, French, German, Italian or Spanish (game text and launcher)
- **Runs straight from your `.iso`**, or from an extracted game folder
- **Mods** with automatic ARCH container catalog patching (`plist360.arc`) for characters, arenas & more
- **DLC support** (all 6 official DLC packages)
- **Title update support**
- **Achievements** with unlock notifications and progress tracker
- **Custom soundtrack** in the game's menus and match entrances (MP3, WAV, FLAC, OGG)
- **Screenshots** with F12.

## Installation

1. Unzip the download into a folder of its own (e.g. `C:\Games\WWE 2K16 PC`).
2. Run `WWE 2K16 PC.exe`.
3. Under **Game files**, click **Choose ISO** and pick your WWE 2K16 `.iso` (or **Choose folder** for an extracted copy).
4. Pick your settings and press **Play**.

**Requirements:** Windows 10/11 64-bit, a Vulkan graphics card with an up-to-date driver, controller or keyboard, and the [Microsoft Edge WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/) (already on Windows 11 and most Windows 10 PCs).

Saves, settings, music and DLC are kept in `userdata` next to the launcher, mods in `mods`, logs in `logs`. To update, unzip the new version over the old one: `userdata` and `mods` are kept.

## Controls

An Xbox controller (or any controller Windows sees as one) works like on the Xbox 360. Keyboard works too; the full layout is under **Controls** in the launcher, and **Controls > Custom** lets you remap any button or key.

## Troubleshooting

- **Launcher doesn't open**: unzip everything first, and check your antivirus didn't block `WWE 2K16 PC.exe`.
- **Launcher is blank / black**: install the WebView2 Runtime (link above).
- **"Not an Xbox 360 disc image"**: the `.iso` has to be the Xbox 360 version; PS3 isos don't work.
- **Game doesn't start or closes right away**: update your graphics driver, check your antivirus didn't remove anything from the `bin` folder, and try the game folder instead of the `.iso` (or the other way round).
- **Low framerate**: lower the resolution, turn on FSR, turn off shadows, or set anti-aliasing to Off.
- **A mod crashes the game**: turn it off in the Mods tab.
- When reporting a problem, include what happened, your graphics card and Windows version, and the newest file from `logs`.

Online features of the original game are not available. This is an early release, so expect some bugs.

## Credits

- **ReXGlue SDK** by Tom Clay (BSD 3-Clause)
- The libraries listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md); the SDK patches are under the SDK's licence ([patches/sdk/LICENSE](patches/sdk/LICENSE))
- **Banshee** ([@itsbybanshee](https://x.com/itsbybanshee)): WWE 2K16 PC port and launcher
- [MrBurn](https://github.com/iMrBurn) for help with texture rendering issues
- [Bryan Caminos](https://github.com/BryanCaminos) for troubleshooting & testing

## Licences

- I allow [MrBurn](https://github.com/iMrBurn) to take over my code and distribute it with his own modifications.

Fan project, not affiliated with WWE, 2K or Yuke's.
