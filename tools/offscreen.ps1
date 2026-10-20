# Moves the game window off-screen without activating it (it keeps rendering, so capture_window.ps1 works, but it
# isn't visible). Use instead of minimized windows when screenshots are needed.
param([string[]]$Process = @("wwe2k16_tu", "wwe2k16"))
Add-Type -AssemblyName System.Windows.Forms
Add-Type @"
using System; using System.Runtime.InteropServices;
public static class Off {
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int c);
  [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int w, int hh, uint f);
}
"@
$p = Get-Process $Process -EA SilentlyContinue | Where-Object MainWindowHandle -ne 0 | Select-Object -First 1
if (-not $p) { "window not found"; exit 1 }
[Off]::ShowWindow($p.MainWindowHandle, 4) | Out-Null        # SW_SHOWNOACTIVATE
[Off]::SetWindowPos($p.MainWindowHandle, [IntPtr]::Zero, ([System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Right - 2), ([System.Windows.Forms.Screen]::PrimaryScreen.Bounds.Bottom - 2), 1600, 900, 0x0014) | Out-Null   # NOZORDER|NOACTIVATE
"moved $($p.ProcessName) off-screen"
