#pragma once

#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

struct ImFont;

namespace rex::ui {
class ImGuiDrawer;
class WindowedAppContext;
}  // namespace rex::ui

namespace wwe2k16 {

// Achievement notifications are shown in a separate, click-through host window so they don't divert game-frame
// presentation through the UI thread.
class AchievementToasts {
 public:
  ~AchievementToasts();

  // UI thread. native_window_handle is the game's HWND on Windows.
  void Attach(rex::ui::WindowedAppContext* app_context, rex::ui::ImGuiDrawer* drawer,
              void* native_window_handle = nullptr);
  void Detach();

  // Any thread (unlocks arrive on guest threads). icon_path: PNG of the achievement icon, may be empty.
  void Push(std::string name, std::string description, std::string icon_path);

  // In-memory WAV played as each pop-up appears (achievement_sound.h). Empty = silent.
  void SetSound(std::vector<uint8_t> wav) { sound_ = std::move(wav); }

 private:
  friend class AchievementToastDialog;
  struct NativeState;

  struct Toast {
    std::string name;
    std::string description;
    std::string icon_path;
  };

  void ShowFromUIThread();

  rex::ui::WindowedAppContext* app_context_ = nullptr;
  rex::ui::ImGuiDrawer* drawer_ = nullptr;
  std::shared_ptr<NativeState> native_state_;
  ImFont* font_regular_ = nullptr;
  ImFont* font_bold_ = nullptr;
  std::mutex mutex_;
  std::deque<Toast> queue_;
  std::vector<uint8_t> sound_;
  bool dialog_active_ = false;  // UI thread only
};

}  // namespace wwe2k16
