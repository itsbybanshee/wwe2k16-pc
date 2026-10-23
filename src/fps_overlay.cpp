#include "fps_overlay.h"

#include <cfloat>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <imgui.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>

namespace wwe2k16 {

namespace {

std::atomic<uint64_t> g_game_frame_count{0};

bool Enabled() {
  const char* v = std::getenv("WWE2K16_FPS_COUNTER");
  return v && v[0] != '0';
}

class FpsOverlayDialog final : public rex::ui::ImGuiDialog {
 public:
  explicit FpsOverlayDialog(rex::ui::ImGuiDrawer* drawer) : ImGuiDialog(drawer) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    if (!Enabled()) {
      return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (last_sample_.time_since_epoch().count() == 0) {
      last_sample_ = now;
      last_frame_count_ = GameFrameCount();
    } else if (now - last_sample_ >= std::chrono::milliseconds(500)) {
      const uint64_t frames = GameFrameCount();
      const double seconds = std::chrono::duration<double>(now - last_sample_).count();
      game_fps_ = seconds > 0.0 ? float(double(frames - last_frame_count_) / seconds) : 0.0f;
      last_frame_count_ = frames;
      last_sample_ = now;
    }
    char text[40];
    std::snprintf(text, sizeof(text), "%.0f FPS", game_fps_);

    ImFont* font = ImGui::GetFont();
    const float font_size = 18.0f;
    const ImVec2 text_size = font ? font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, text) : ImVec2(105, 20);

    const float pad_x = 10.0f;
    const float pad_y = 5.0f;
    const float pill_w = text_size.x + pad_x * 2.0f;
    const float pill_h = text_size.y + pad_y * 2.0f;
    const float margin = 16.0f;

    // Top-right corner of the window
    const ImVec2 p0(io.DisplaySize.x - pill_w - margin, margin);
    const ImVec2 p1(p0.x + pill_w, p0.y + pill_h);
    const float rounding = 6.0f;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    // Subtle dark translucent background pill
    dl->AddRectFilled(p0, p1, ImGui::GetColorU32(ImVec4(0.08f, 0.09f, 0.11f, 0.82f)), rounding);
    dl->AddRect(p0, p1, ImGui::GetColorU32(ImVec4(0.80f, 0.82f, 0.85f, 0.35f)), rounding, 0, 1.0f);

    // Color: clean bright white with slight tint
    ImVec4 text_color(0.95f, 0.96f, 0.98f, 0.95f);
    if (game_fps_ >= 58.0f) {
      text_color = ImVec4(0.40f, 0.95f, 0.45f, 0.95f);  // Subtle green for ~60 FPS
    } else if (game_fps_ >= 28.0f && game_fps_ <= 32.0f) {
      text_color = ImVec4(0.35f, 0.85f, 1.0f, 0.95f);   // Cyan for locked 30 FPS
    }

    const ImVec2 text_pos(p0.x + pad_x, p0.y + pad_y);
    dl->AddText(font, font_size, text_pos, ImGui::GetColorU32(text_color), text);
  }

 private:
  std::chrono::steady_clock::time_point last_sample_{};
  uint64_t last_frame_count_ = 0;
  float game_fps_ = 0.0f;
};

}  // namespace

void RecordGameFrame() { g_game_frame_count.fetch_add(1, std::memory_order_relaxed); }

uint64_t GameFrameCount() { return g_game_frame_count.load(std::memory_order_relaxed); }

void FpsOverlay::Attach(rex::ui::ImGuiDrawer* drawer) {
  drawer_ = drawer;
  if (!Enabled()) {
    return;
  }
  if (drawer_ && !active_) {
    active_ = true;
    new FpsOverlayDialog(drawer_);
  }
}

void FpsOverlay::Detach() {
  active_ = false;
  drawer_ = nullptr;
}

}  // namespace wwe2k16
