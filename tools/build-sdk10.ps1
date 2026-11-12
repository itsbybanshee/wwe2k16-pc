# Build and install the ReXGlue v0.10.0 SDK (with the WWE 2K15 patch set) from sdk-v10 into sdk-v10\out\install\ninja-clangcl.
$ErrorActionPreference = "Stop"
$Root = "C:\Users\melfo\Desktop\WWE 2K16 Recomp\wwe2k16-pc-src"
. "$Root\tools\env.ps1"
$Sdk = "$Root\sdk-v10"
$install = Join-Path $Sdk "out\install\ninja-clangcl"
Push-Location $Sdk
try {
  if (-not (Test-Path "out\build\win-amd64\CMakeCache.txt")) {
    Invoke-Checked cmake --preset win-amd64 -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl `
      -DREXGLUE_USE_VULKAN=ON -DREXGLUE_ENABLE_TRACY=OFF "-DCMAKE_INSTALL_PREFIX=$install" `
      "-DCMAKE_C_FLAGS=/clang:-march=x86-64-v2 /clang:-Wno-everything /Z7" `
      "-DCMAKE_CXX_FLAGS=/EHsc /Zc:char8_t- /clang:-march=x86-64-v2 /clang:-Wno-everything /Z7" `
      "-DCMAKE_SHARED_LINKER_FLAGS=/machine:x64 /DEBUG /OPT:REF /OPT:ICF"
  }
  Invoke-Checked cmake --build out/build/win-amd64 --config Release
  Invoke-Checked cmake --install out/build/win-amd64 --config Release
  "SDK10 DONE"
} finally { Pop-Location }
