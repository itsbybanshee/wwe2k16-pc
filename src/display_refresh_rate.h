#pragma once

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <string>
#endif

namespace wwe2k16 {

// Temporarily use 60 Hz on the game's monitor to keep the title's simulation cadence stable.
class DisplayRefreshRate {
 public:
  ~DisplayRefreshRate();

  bool SwitchTo60Hz(void* game_window);
  void Restore();

 private:
#if defined(_WIN32)
  std::wstring device_name_;
  DEVMODEW original_mode_{};
  DEVMODEW game_mode_{};
  bool switched_ = false;
#endif
};

}  // namespace wwe2k16
