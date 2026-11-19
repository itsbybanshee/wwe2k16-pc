#pragma once

#include <cstdint>

namespace rex::ui {
class ImGuiDrawer;
}

namespace wwe2k16 {

// Count frames submitted by the game so the overlay doesn't mistake the UI's
// monitor-paced render rate for the game's frame rate.
void RecordGameFrame();
uint64_t GameFrameCount();

class FpsOverlay {
 public:
  void Attach(rex::ui::ImGuiDrawer* drawer);
  void Detach();

 private:
  rex::ui::ImGuiDrawer* drawer_ = nullptr;
  bool active_ = false;
};

}  // namespace wwe2k16
