#include "display_refresh_rate.h"

#include <rex/logging.h>

namespace wwe2k16 {

DisplayRefreshRate::~DisplayRefreshRate() { Restore(); }

bool DisplayRefreshRate::SwitchTo60Hz(void* game_window) {
#if defined(_WIN32)
  if (switched_ || !game_window) return switched_;

  const HWND hwnd = static_cast<HWND>(game_window);
  const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  MONITORINFOEXW monitor_info{};
  monitor_info.cbSize = sizeof(monitor_info);
  if (!GetMonitorInfoW(monitor, &monitor_info)) {
    REXLOG_WARN("[wwe2k16] Could not identify the game display; leaving its refresh rate unchanged");
    return false;
  }
  device_name_ = monitor_info.szDevice;

  original_mode_ = {};
  original_mode_.dmSize = sizeof(original_mode_);
  if (!EnumDisplaySettingsExW(device_name_.c_str(), ENUM_CURRENT_SETTINGS, &original_mode_, 0)) {
    REXLOG_WARN("[wwe2k16] Could not read the current display mode; leaving refresh unchanged");
    device_name_.clear();
    return false;
  }
  const DWORD current_hz = original_mode_.dmDisplayFrequency;
  if (current_hz <= 60) {
    REXLOG_INFO("[wwe2k16] Display refresh is already {} Hz; no mode change needed", current_hz);
    device_name_.clear();
    return false;
  }

  DEVMODEW mode_60{};
  bool found_60hz = false;
  DEVMODEW near_60hz_mode{};
  bool found_near_60hz = false;
  for (DWORD index = 0;; ++index) {
    DEVMODEW candidate{};
    candidate.dmSize = sizeof(candidate);
    if (!EnumDisplaySettingsExW(device_name_.c_str(), index, &candidate, 0)) break;
    if (candidate.dmPelsWidth == original_mode_.dmPelsWidth &&
        candidate.dmPelsHeight == original_mode_.dmPelsHeight &&
        candidate.dmBitsPerPel == original_mode_.dmBitsPerPel &&
        candidate.dmDisplayOrientation == original_mode_.dmDisplayOrientation &&
        candidate.dmDisplayFrequency >= 59 && candidate.dmDisplayFrequency <= 60) {
      if (candidate.dmDisplayFrequency == 60) {
        mode_60 = candidate;
        found_60hz = true;
        break;
      }
      near_60hz_mode = candidate;
      found_near_60hz = true;
    }
  }
  if (!found_60hz && found_near_60hz) {
    // Some Windows drivers report the common 59.94 Hz mode as 59.
    mode_60 = near_60hz_mode;
    found_60hz = true;
  }
  if (!found_60hz) {
    REXLOG_WARN("[wwe2k16] {} Hz is active, but this display has no 59/60 Hz mode at {}x{} and {} bpp; "
                "leaving its refresh rate unchanged",
                current_hz, original_mode_.dmPelsWidth, original_mode_.dmPelsHeight,
                original_mode_.dmBitsPerPel);
    device_name_.clear();
    return false;
  }

  mode_60.dmFields |= DM_DISPLAYFREQUENCY;
  const LONG test_result = ChangeDisplaySettingsExW(device_name_.c_str(), &mode_60, nullptr,
                                                    CDS_TEST | CDS_FULLSCREEN, nullptr);
  if (test_result != DISP_CHANGE_SUCCESSFUL) {
    REXLOG_WARN("[wwe2k16] Windows rejected the 60 Hz mode (status {}); leaving it unchanged",
                test_result);
    device_name_.clear();
    return false;
  }

  const LONG result = ChangeDisplaySettingsExW(device_name_.c_str(), &mode_60, nullptr,
                                               CDS_FULLSCREEN, nullptr);
  if (result != DISP_CHANGE_SUCCESSFUL) {
    REXLOG_WARN("[wwe2k16] Could not switch from {} Hz to 60 Hz (status {})", current_hz, result);
    device_name_.clear();
    return false;
  }

  game_mode_ = mode_60;
  switched_ = true;
  REXLOG_INFO("[wwe2k16] Temporarily changed the display from {} Hz to 60 Hz at {}x{}",
              current_hz, game_mode_.dmPelsWidth,
              game_mode_.dmPelsHeight);
  return true;
#else
  (void)game_window;
  return false;
#endif
}

void DisplayRefreshRate::Restore() {
#if defined(_WIN32)
  if (!switched_) return;

  // Respect any display mode the user deliberately changed while the game was open.
  DEVMODEW current{};
  current.dmSize = sizeof(current);
  const bool still_in_game_mode =
      EnumDisplaySettingsExW(device_name_.c_str(), ENUM_CURRENT_SETTINGS, &current, 0) &&
      current.dmPelsWidth == game_mode_.dmPelsWidth && current.dmPelsHeight == game_mode_.dmPelsHeight &&
      current.dmBitsPerPel == game_mode_.dmBitsPerPel &&
      current.dmDisplayFrequency == game_mode_.dmDisplayFrequency;
  if (still_in_game_mode) {
    original_mode_.dmFields |= DM_DISPLAYFREQUENCY;
    const LONG result = ChangeDisplaySettingsExW(device_name_.c_str(), &original_mode_, nullptr,
                                                 CDS_FULLSCREEN, nullptr);
    if (result == DISP_CHANGE_SUCCESSFUL) {
      REXLOG_INFO("[wwe2k16] Restored the display to {} Hz", original_mode_.dmDisplayFrequency);
    } else {
      REXLOG_ERROR("[wwe2k16] Could not restore the display to {} Hz (status {})",
                   original_mode_.dmDisplayFrequency, result);
    }
  } else {
    REXLOG_INFO("[wwe2k16] Display mode changed while the game was open; leaving the user's current mode intact");
  }
  switched_ = false;
  device_name_.clear();
#endif
}

}  // namespace wwe2k16
