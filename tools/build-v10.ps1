# Build the v0.10 (native-sdk10 branch) game: codegen from game_tu\ (tools\build-tu.ps1 prepares it), then wwe2k16.exe.
# The SDK must be built first (sdk-v10\, see docs/NATIVE_PORT_PLAN.md). Output: out\build\v10\wwe2k16.exe with the SDK DLLs.
#   tools\build-v10.ps1 [-Config Release|RelWithDebInfo]
param([ValidateSet("Release", "RelWithDebInfo")] [string]$Config = "Release", [switch]$Native)
$ErrorActionPreference = "Stop"
$Root = Split-Path $PSScriptRoot
. (Join-Path $PSScriptRoot "env.ps1")
$install = (Join-Path $Root "sdk-v10\out\install\ninja-clangcl") -replace '\\', '/'
$build = Join-Path $Root "out\build\v10"
Push-Location $Root
try {
    # Explicit flags: a configure run where clang-cl detection is incomplete leaves the release flags empty (2-3x slower).
    Invoke-Checked cmake -S . -B $build -G Ninja "-DCMAKE_BUILD_TYPE=$Config" "-DWWE2K16_NATIVE_RENDERER=$(if ($Native) { "ON" } else { "OFF" })" `
        -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl "-DCMAKE_PREFIX_PATH=$install" "-DSDL3_DIR=$install/cmake" `
        "-DCMAKE_C_FLAGS=/clang:-march=x86-64-v2 /clang:-Wno-everything" `
        "-DCMAKE_CXX_FLAGS=/EHsc /Zc:char8_t- /clang:-march=x86-64-v2 /clang:-Wno-everything" `
        "-DCMAKE_C_FLAGS_RELEASE=/O2 /Ob2 /DNDEBUG" "-DCMAKE_CXX_FLAGS_RELEASE=/O2 /Ob2 /DNDEBUG" `
        "-DCMAKE_C_FLAGS_RELWITHDEBINFO=/O2 /Ob2 /DNDEBUG /Z7" "-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=/O2 /Ob2 /DNDEBUG /Z7"
    Invoke-Checked cmake --build $build
    $bin = Join-Path $install "bin"
    Copy-Item (Join-Path $bin "rexruntime.dll"), (Join-Path $bin "rexgpu-xenos.dll") $build -Force
    # The SDK reads <exe name>.toml beside the executable: Vulkan on the discrete GPU, shader and readback settings.
    Copy-Item (Join-Path $Root "wwe2k16_runtime.toml") (Join-Path $build "wwe2k16.toml") -Force
} finally { Pop-Location }
