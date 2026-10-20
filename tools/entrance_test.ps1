# Plays the first entrance of a One on One match hidden and prints the frame timing while the game runs at 30 FPS
# (entrance). Extra args go to the game.  tools\entrance_test.ps1 [-Exe wwe2k16_tu] [--some_flag=value]
param([string]$Exe = "wwe2k16_tu")
$Root = Split-Path $PSScriptRoot
Get-Process wwe2k16* -EA SilentlyContinue | Stop-Process -Force; Start-Sleep 1
$env:WWE2K16_OFFSCREEN = "1"
# Title (Start), notices (A), Play, One on One, Normal, pick + ready both sides, start; then no input (entrance plays).
$script = "WAIT 11000`nSTART`nWAIT 2000`nSTART`nWAIT 2000`nSTART`nWAIT 2000`nSTART`nWAIT 2000`nSTART`nWAIT 2000`nSTART`nWAIT 2000`nSTART`nWAIT 2000`nSTART`nWAIT 2000`nA`nWAIT 16000`nA`nWAIT 8000`nA`nWAIT 20000`nA`nWAIT 3000`nA`nWAIT 25000`nA`nWAIT 3000`nA`nWAIT 3000`nA`nWAIT 3000`nA`nWAIT 4000`nA`nWAIT 3000`nA`nWAIT 4000`nA"
Set-Content "$Root\logs\test_input.txt" $script -Encoding ascii
$log = & "$PSScriptRoot\run_hidden.ps1" -Exe $Exe @args
Start-Sleep 200
Get-Process wwe2k16* -EA SilentlyContinue | Stop-Process -Force
$lines = Get-Content $log | Where-Object { $_ -match "wwe2k16-flip\] flips=.*rate=30" }
"$log : $($lines.Count) seconds at 30 FPS"
$lines | ForEach-Object { ($_ -split "flip\] ")[1] } | Select-Object -First 25
