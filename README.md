# WWE 2K16 PC Recomp (By Banshee)

WWE 2K16 PC recompilation with a modern launcher, graphics options, non-destructive mod support, custom soundtrack, achievements and more.

- No game data included. You need your own legally acquired WWE 2K16 (Xbox 360) files: a disc image (`.iso`) or the
  extracted game folder.

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

Fan project, not affiliated with WWE, 2K or Yuke's.
