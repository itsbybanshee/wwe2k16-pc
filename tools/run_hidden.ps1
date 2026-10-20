# Test launch that never shows on screen: starts the game minimized with scripted input and timer vblank, then moves
# its window off-screen (still rendering, so tools\capture_window.ps1 works). Prints the log path.
#   tools\run_hidden.ps1 [-Exe wwe2k16] [extra game args]
param([string]$Exe = "wwe2k16_tu")
$Root = Split-Path $PSScriptRoot
$env:WWE2K16_TEST_INPUT = "1"; if (-not $env:WWE2K16_HOST_VSYNC) { $env:WWE2K16_HOST_VSYNC = "0" }; $env:WWE2K16_WINDOW_MODE = "windowed"
$log = "$Root\logs\hidden-$(Get-Date -Format yyyyMMdd-HHmmss).log"
$a = @("`"$env:USERPROFILE\Downloads\xiso\wwe2k16`"", "--user_data_root=$Root\userdata",
  "--update_data_root=$Root\userdata\title_update", "--no-enable_console", "--fullscreen=false",
  "--window_width=1600", "--window_height=900", "--log_file=$log") + $args
# The retail build must not load the update (its code is the unpatched game).
if ($Exe -eq "wwe2k16") { $a = $a | Where-Object { $_ -notlike "--update_data_root=*" } }
Start-Process "$Root\out\build\win-amd64\RelWithDebInfo\$Exe.exe" -ArgumentList $a -WorkingDirectory $Root -WindowStyle Minimized
for ($i = 0; $i -lt 40; $i++) {
  Start-Sleep -Milliseconds 250
  if (Get-Process $Exe -EA SilentlyContinue | Where-Object MainWindowHandle -ne 0) { break }
}
if ($env:WWE2K16_OFFSCREEN -eq "1") { & "$PSScriptRoot\offscreen.ps1" -Process $Exe | Out-Null }
$log
