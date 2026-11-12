#!/usr/bin/env python3
"""Package the native-renderer build (branch native-sdk10) as a zip: launcher, game executables, runtime DLLs, shader
compiler, pipeline seed, licences. Reuses the launcher / UI packing of package_release.py (Banshee's v0.7.3 packer).

    python -I tools/package_v10.py            # needs out/build/v10 (tools/build-v10.ps1 -Native)

Layout: "WWE 2K16 PC.exe" and bin/ at the top level of the zip, like Banshee's releases. The native renderer executable is
shipped as bin/wwe2k16_tu.exe because the launcher starts that name when the title update is installed (this build needs
it); bin/wwe2k16.exe is the Xenos GPU emulation build of the same code, kept as a fallback for GPUs without Vulkan 1.2
features. No game data, generated code or shaders converted from the game are included.
"""

import shutil
import sys
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import package_release as pr  # noqa: E402

ROOT = pr.ROOT
BUILD = ROOT / "out" / "build" / "v10"
VERSION = (ROOT / "VERSION").read_text().strip() if (ROOT / "VERSION").exists() else "dev"
ZIP_OUT = pr.DIST / f"WWE 2K16 PC Native {VERSION}.zip"

README = """WWE 2K16 PC - native renderer build
====================================

Development build of the WWE 2K16 PC recompilation with a native renderer (re:Blue + plume on Vulkan) instead of the
Xbox 360 GPU emulation. Based on Banshee's WWE 2K16 PC (launcher and in-game features), ReXGlue SDK v0.10.

You need your own copy of WWE 2K16 for Xbox 360 (a .iso or an extracted folder) AND its Title Update 3: this version only
runs with the update (use the Title update button of the launcher). No game data is included.

1. Unzip into a folder of its own (not Program Files). 2. Open "WWE 2K16 PC.exe". 3. Choose your game, install the title
update, press PLAY.

The first start converts the game's shaders on your PC (a few seconds, kept in bin\\cache). A graphics card with Vulkan 1.2
is required for the native renderer; the older emulation build is bin\\wwe2k16.exe.

Keys: F2 FPS counter (also in the launcher), F11 window mode, F12 screenshot (Documents\\wwe2k16\\Screenshots).
Fan project, not affiliated with WWE, 2K or Yuke's. See the licences folder.
"""


def main() -> None:
    pr.build_single_file_launcher()
    staging = pr.DIST / "staging_v10"
    if staging.exists():
        shutil.rmtree(staging)
    bin_dir = staging / "bin"
    bin_dir.mkdir(parents=True)

    launcher = staging / "WWE 2K16 PC.exe"
    shutil.copy2(pr.SINGLE_DIR / "WWE 2K16 PC.exe", launcher)
    pr.pack_encrypted_ui(launcher, ROOT / "launcher" / "ui", bin_dir / "ui.dat")

    files = {
        "wwe2k16_tu.exe": BUILD / "wwe2k16_native.exe",
        "wwe2k16.exe": BUILD / "wwe2k16.exe",
        "rexruntime.dll": BUILD / "rexruntime.dll",
        "rexgpu-xenos.dll": BUILD / "rexgpu-xenos.dll",
        "dxcompiler.dll": BUILD / "dxcompiler.dll",
        "wwe2k16.toml": BUILD / "wwe2k16.toml",
        "achievement.ogg": ROOT / "res" / "achievement.ogg",
        "pipelines_seed.w12p": ROOT / "res" / "pipelines_seed.w12p",
    }
    for name, src in files.items():
        if not src.exists():
            raise SystemExit(f"missing {src}")
        shutil.copy2(src, bin_dir / name)
        print(f"  {name} <- {src}")
    for exe in ("wwe2k16_tu.exe", "wwe2k16.exe"):
        pr.sanitize_pe_pdb_path(bin_dir / exe, exe.replace(".exe", ".pdb").encode())
    for dll in ("rexruntime.dll", "rexgpu-xenos.dll"):
        pr.sanitize_pe_pdb_path(bin_dir / dll, dll.replace(".dll", ".pdb").encode())

    lic = staging / "licenses"
    lic.mkdir()
    for f in (ROOT / "third_party" / "licenses").glob("*"):
        shutil.copy2(f, lic / f.name)
    for f in ("THIRD_PARTY_NOTICES.md",):
        shutil.copy2(ROOT / f, lic / f)
    reblue = ROOT / "src" / "reblue" / "LICENSE.reblue"
    if reblue.exists():
        shutil.copy2(reblue, lic / "re-Blue-LICENSE.txt")
    (staging / "ReadMe!.txt").write_text(README, encoding="utf-8")

    ZIP_OUT.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(ZIP_OUT, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as z:
        for path in sorted(staging.rglob("*")):
            if path.is_file():
                z.write(path, path.relative_to(staging).as_posix())
    print(f"wrote {ZIP_OUT} ({ZIP_OUT.stat().st_size / 1e6:.1f} MB)")


if __name__ == "__main__":
    main()
