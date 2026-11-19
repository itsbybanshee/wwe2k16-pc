#include "achievement_toast.h"

#include <algorithm>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include <imgui.h>
#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ui/imgui_dialog.h>
#include <rex/ui/imgui_drawer.h>
#include <rex/ui/immediate_drawer.h>
#include <rex/ui/windowed_app_context.h>

#ifdef _WIN32
#include <windows.h>
#include <gdiplus.h>
#include <objbase.h>
#include <wincodec.h>
#endif

#include "achievement_sound.h"

namespace wwe2k16 {
namespace {

// Decodes a PNG to R8G8B8A8 (what ImmediateDrawer::CreateTexture takes).
bool DecodePng(const std::string& path, uint32_t& width, uint32_t& height, std::vector<uint8_t>& rgba) {
#ifdef _WIN32
  if (path.empty())
    return false;
  const HRESULT co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  bool ok = false;
  IWICImagingFactory* factory = nullptr;
  IWICBitmapDecoder* decoder = nullptr;
  IWICBitmapFrameDecode* frame = nullptr;
  IWICFormatConverter* converter = nullptr;
  const std::wstring wide = std::filesystem::u8path(path).wstring();
  do {
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory))))
      break;
    if (FAILED(factory->CreateDecoderFromFilename(wide.c_str(), nullptr, GENERIC_READ,
                                                  WICDecodeMetadataCacheOnDemand, &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)) || FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom)))
      break;
    UINT w = 0, h = 0;
    if (FAILED(converter->GetSize(&w, &h)) || !w || !h || w > 1024 || h > 1024)
      break;
    rgba.resize(size_t(w) * h * 4);
    if (FAILED(converter->CopyPixels(nullptr, w * 4, UINT(rgba.size()), rgba.data())))
      break;
    width = w;
    height = h;
    ok = true;
  } while (false);
  if (converter) converter->Release();
  if (frame) frame->Release();
  if (decoder) decoder->Release();
  if (factory) factory->Release();
  if (SUCCEEDED(co)) CoUninitialize();
  return ok;
#else
  (void)path, (void)width, (void)height, (void)rgba;
  return false;
#endif
}

ImU32 Color(float r, float g, float b, float a) { return ImGui::GetColorU32(ImVec4(r, g, b, a)); }

// Splits text into lines that fit `wrap_width` at `size` (word wrap), so each line can be drawn centred.
std::vector<std::string_view> WrapLines(ImFont* font, float size, std::string_view text, float wrap_width) {
  std::vector<std::string_view> lines;
  const char* p = text.data();
  const char* end = text.data() + text.size();
  while (p < end) {
    const char* line_end = font->CalcWordWrapPosition(size, p, end, wrap_width);
    if (line_end == p)  // a single word wider than the line: take at least one character
      ++line_end;
    std::string_view line(p, size_t(line_end - p));
    while (!line.empty() && (line.back() == ' ' || line.back() == '\n'))
      line.remove_suffix(1);
    lines.push_back(line);
    p = line_end;
    while (p < end && (*p == ' ' || *p == '\n'))
      ++p;
  }
  return lines;
}

}  // namespace

#ifdef _WIN32
struct AchievementToasts::NativeState {
  explicit NativeState(AchievementToasts* owner, HWND game_window) : owner(owner), game_window(game_window) {}
  ~NativeState() {
    if (window && IsWindow(window)) DestroyWindow(window);
    if (gdiplus_token) Gdiplus::GdiplusShutdown(gdiplus_token);
  }

  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* self = reinterpret_cast<NativeState*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
      auto* create = reinterpret_cast<CREATESTRUCTW*>(lparam);
      self = static_cast<NativeState*>(create->lpCreateParams);
      SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (!self) return DefWindowProcW(hwnd, message, wparam, lparam);
    if (message == WM_TIMER && wparam == 1) {
      self->OnTimer();
      return 0;
    }
    if (message == WM_NCHITTEST) return HTTRANSPARENT;
    if (message == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    return DefWindowProcW(hwnd, message, wparam, lparam);
  }

  bool Initialize() {
    Gdiplus::GdiplusStartupInput startup;
    if (Gdiplus::GdiplusStartup(&gdiplus_token, &startup, nullptr) != Gdiplus::Ok) return false;
    static constexpr wchar_t kClassName[] = L"WWE2K16AchievementToastOverlay";
    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kClassName;
    RegisterClassExW(&wc);
    window = CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
                             kClassName, L"", WS_POPUP, 0, 0, 1, 1, game_window, nullptr,
                             GetModuleHandleW(nullptr), this);
    if (!window) return false;
    SetTimer(window, 1, 40, nullptr);
    return true;
  }

  void StartNext() {
    if (current) return;
    {
      std::lock_guard lock(owner->mutex_);
      if (owner->queue_.empty()) {
        owner->dialog_active_ = false;
        ShowWindow(window, SW_HIDE);
        return;
      }
      current = std::move(owner->queue_.front());
      owner->queue_.pop_front();
    }
    if (!current->icon_path.empty())
      icon.reset(Gdiplus::Bitmap::FromFile(std::filesystem::u8path(current->icon_path).wstring().c_str(), FALSE));
    if (icon && icon->GetLastStatus() != Gdiplus::Ok) icon.reset();
    shown_at = std::chrono::steady_clock::now();
    PlayWavAsync(owner->sound_);
    Render(0.0f);
    PositionAndShow();
  }

  void OnTimer() {
    if (!IsWindow(game_window) || IsIconic(game_window) || !IsWindowVisible(game_window)) {
      ShowWindow(window, SW_HIDE);
      return;
    }
    if (!current) {
      StartNext();
      return;
    }
    const float elapsed = std::chrono::duration<float>(std::chrono::steady_clock::now() - shown_at).count();
    constexpr float duration = 5.0f;
    if (elapsed >= duration) {
      current.reset();
      icon.reset();
      StartNext();
      return;
    }
    const float alpha = std::clamp(std::min(elapsed, duration - elapsed) / 0.3f, 0.0f, 1.0f);
    if (std::abs(alpha - last_alpha) > 0.03f) {
      Render(alpha);
      last_alpha = alpha;
    }
    PositionAndShow();
  }

  void PositionAndShow() {
    if (!current || !window) return;
    RECT client{};
    if (!GetClientRect(game_window, &client)) return;
    POINT origin{client.left, client.top};
    ClientToScreen(game_window, &origin);
    const int client_h = client.bottom - client.top;
    if (client_h <= 0) return;
    const float scale = std::max(0.5f, client_h / 1080.0f);
    const int width = int(std::lround(720.0f * scale));
    const int height = int(std::lround(100.0f * scale));
    const int x = origin.x + ((client.right - client.left) - width) / 2;
    const int y = origin.y + client_h - height - int(std::lround(56.0f * scale));
    RECT current{};
    GetWindowRect(window, &current);
    if (current.left != x || current.top != y || current.right - current.left != width ||
        current.bottom - current.top != height || !IsWindowVisible(window)) {
      SetWindowPos(window, HWND_TOP, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    }
  }

  void Render(float alpha) {
    if (!current || !window) return;
    RECT client{};
    GetClientRect(game_window, &client);
    const float scale = std::max(0.5f, (client.bottom - client.top) / 1080.0f);
    const int width = std::max(1, int(std::lround(720.0f * scale)));
    const int height = std::max(1, int(std::lround(100.0f * scale)));
    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);
    void* pixels = nullptr;
    HBITMAP dib = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &pixels, nullptr, 0);
    HGDIOBJ previous = SelectObject(memory, dib);
    Gdiplus::Bitmap bitmap(width, height, width * 4, PixelFormat32bppPARGB, static_cast<BYTE*>(pixels));
    Gdiplus::Graphics g(&bitmap);
    g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    g.Clear(Gdiplus::Color(0, 0, 0, 0));
    const int a = int(std::lround(255.0f * alpha));
    Gdiplus::SolidBrush panel(Gdiplus::Color(BYTE(a * 0.96f), 19, 20, 23));
    Gdiplus::Pen border(Gdiplus::Color(BYTE(a * 0.55f), 210, 214, 224), std::max(1.0f, 1.2f * scale));
    Gdiplus::RectF box(0.6f * scale, 0.6f * scale, width - 1.2f * scale, height - 1.2f * scale);
    Gdiplus::GraphicsPath path;
    const float radius = 10.0f * scale;
    path.AddArc(box.X, box.Y, radius * 2, radius * 2, 180, 90);
    path.AddArc(box.GetRight() - radius * 2, box.Y, radius * 2, radius * 2, 270, 90);
    path.AddArc(box.GetRight() - radius * 2, box.GetBottom() - radius * 2, radius * 2, radius * 2, 0, 90);
    path.AddArc(box.X, box.GetBottom() - radius * 2, radius * 2, radius * 2, 90, 90);
    path.CloseFigure();
    g.FillPath(&panel, &path);
    g.DrawPath(&border, &path);

    const float pad = 7.0f * scale;
    const float icon_size = 62.0f * scale;
    const float icon_x = pad;
    const float icon_y = (height - icon_size) * 0.5f;
    if (icon) {
      Gdiplus::RectF ir(icon_x, icon_y, icon_size, icon_size);
      Gdiplus::GraphicsPath ip;
      ip.AddArc(ir.X, ir.Y, 12 * scale, 12 * scale, 180, 90);
      ip.AddArc(ir.GetRight() - 12 * scale, ir.Y, 12 * scale, 12 * scale, 270, 90);
      ip.AddArc(ir.GetRight() - 12 * scale, ir.GetBottom() - 12 * scale, 12 * scale, 12 * scale, 0, 90);
      ip.AddArc(ir.X, ir.GetBottom() - 12 * scale, 12 * scale, 12 * scale, 90, 90);
      ip.CloseFigure();
      g.SetClip(&ip);
      Gdiplus::ColorMatrix matrix{};
      matrix.m[0][0] = matrix.m[1][1] = matrix.m[2][2] = matrix.m[4][4] = 1.0f;
      matrix.m[3][3] = alpha;
      Gdiplus::ImageAttributes attributes;
      attributes.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
      g.DrawImage(icon.get(), ir, 0, 0, icon->GetWidth(), icon->GetHeight(), Gdiplus::UnitPixel, &attributes);
      g.ResetClip();
    } else {
      Gdiplus::SolidBrush placeholder(Gdiplus::Color(BYTE(a), 51, 54, 61));
      g.FillRectangle(&placeholder, icon_x, icon_y, icon_size, icon_size);
    }

    const std::wstring prefix = LocalizedPrefix();
    const std::wstring title = Utf8ToWide(current->name);
    const std::wstring description = Utf8ToWide(current->description);
    const float text_x = 88.0f * scale;
    const float text_w = width - text_x - 14.0f * scale;
    Gdiplus::Font title_font(L"Segoe UI", 19.0f * scale, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::Font title_bold(L"Segoe UI", 19.0f * scale, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::Font desc_font(L"Segoe UI", 17.0f * scale, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    Gdiplus::SolidBrush white(Gdiplus::Color(BYTE(a), 255, 255, 255));
    Gdiplus::SolidBrush gold(Gdiplus::Color(BYTE(a), 255, 217, 26));
    Gdiplus::RectF prefix_bounds, name_bounds;
    g.MeasureString(prefix.c_str(), -1, &title_font, Gdiplus::PointF(0, 0), &prefix_bounds);
    g.MeasureString(title.c_str(), -1, &title_bold, Gdiplus::PointF(0, 0), &name_bounds);
    const float title_width = std::min(text_w, prefix_bounds.Width + name_bounds.Width);
    const float title_x = text_x + (text_w - title_width) * 0.5f;
    const float prefix_width = std::min(prefix_bounds.Width, title_width);
    Gdiplus::RectF prefix_rect(title_x, 8.0f * scale, prefix_width, 27.0f * scale);
    Gdiplus::RectF name_rect(title_x + prefix_width, 8.0f * scale,
                             std::max(0.0f, title_width - prefix_width), 27.0f * scale);
    Gdiplus::StringFormat title_format;
    title_format.SetTrimming(Gdiplus::StringTrimmingEllipsisCharacter);
    g.DrawString(prefix.c_str(), -1, &title_font, prefix_rect, &title_format, &white);
    g.DrawString(title.c_str(), -1, &title_bold, name_rect, &title_format, &gold);
    Gdiplus::StringFormat desc_format;
    desc_format.SetAlignment(Gdiplus::StringAlignmentCenter);
    desc_format.SetLineAlignment(Gdiplus::StringAlignmentFar);
    desc_format.SetTrimming(Gdiplus::StringTrimmingWord);
    Gdiplus::RectF desc_rect(text_x, 35.0f * scale, text_w, 57.0f * scale);
    g.DrawString(description.c_str(), -1, &desc_font, desc_rect, &desc_format, &white);

    POINT dst{client.left, client.top};
    ClientToScreen(game_window, &dst);
    dst.x += ((client.right - client.left) - width) / 2;
    dst.y += client.bottom - client.top - height - int(std::lround(56.0f * scale));
    POINT src{};
    SIZE size{width, height};
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(window, screen, &dst, &size, memory, &src, 0, &blend, ULW_ALPHA);
    SelectObject(memory, previous);
    DeleteObject(dib);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);
  }

  static std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) return {};
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
    if (length <= 0) return {};
    std::wstring wide(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), wide.data(), length);
    return wide;
  }

  std::wstring LocalizedPrefix() const {
    const auto lang = rex::cvar::GetFlagByName("user_language");
    if (lang == "3") return L"Erfolg freigeschaltet: ";
    if (lang == "4") return L"Succès déverrouillé : ";
    if (lang == "5") return L"Logro desbloqueado: ";
    if (lang == "6") return L"Obiettivo sbloccato: ";
    return L"Achievement unlocked: ";
  }

  AchievementToasts* owner;
  HWND game_window = nullptr;
  HWND window = nullptr;
  ULONG_PTR gdiplus_token = 0;
  std::optional<Toast> current;
  std::unique_ptr<Gdiplus::Bitmap> icon;
  std::chrono::steady_clock::time_point shown_at{};
  float last_alpha = -1.0f;
};
#else
struct AchievementToasts::NativeState {};
#endif

AchievementToasts::~AchievementToasts() = default;

class AchievementToastDialog final : public rex::ui::ImGuiDialog {
 public:
  AchievementToastDialog(rex::ui::ImGuiDrawer* drawer, AchievementToasts* owner)
      : ImGuiDialog(drawer), drawer_(drawer), owner_(owner) {}

 protected:
  void OnDraw(ImGuiIO& io) override {
    using clock = std::chrono::steady_clock;
    constexpr float kShowSeconds = 5.0f;
    constexpr float kFadeSeconds = 0.3f;

    if (!current_) {
      {
        std::lock_guard lock(owner_->mutex_);
        if (owner_->queue_.empty()) {
          owner_->dialog_active_ = false;
          Close();
          return;
        }
        current_ = std::move(owner_->queue_.front());
        owner_->queue_.pop_front();
      }
      icon_.reset();
      uint32_t w = 0, h = 0;
      std::vector<uint8_t> rgba;
      if (drawer_->immediate_drawer() && DecodePng(current_->icon_path, w, h, rgba)) {
        icon_ = drawer_->immediate_drawer()->CreateTexture(w, h, rex::ui::ImmediateTextureFilter::kLinear, false,
                                                           rgba.data());
      }
      shown_at_ = clock::now();
      PlayWavAsync(owner_->sound_);  // one chime per pop-up, as it appears
    }
    const float t = std::chrono::duration<float>(clock::now() - shown_at_).count();
    if (t >= kShowSeconds) {
      current_.reset();  // next one (or close) on the following frame
      icon_.reset();
      return;
    }
    const float alpha = std::clamp(std::min(t, kShowSeconds - t) / kFadeSeconds, 0.0f, 1.0f);
    ImFont* regular = owner_->font_regular_ ? owner_->font_regular_ : ImGui::GetFont();
    ImFont* bold = owner_->font_bold_ ? owner_->font_bold_ : regular;

    // Proportions of the design at 1080p (about 470 x 76 px); everything scales with the window height.
    const float s = std::max(0.5f, io.DisplaySize.y / 1080.0f);
    const float pad = 7.0f * s, icon = 62.0f * s, gap = 12.0f * s, rounding = 10.0f * s;
    const float title_size = 19.0f * s, desc_size = 17.0f * s, desc_line = 19.0f * s, min_gap = 6.0f * s;
    const char* kPrefix = "Achievement unlocked: ";
    const std::string lang_str = rex::cvar::GetFlagByName("user_language");
    if (lang_str == "3") {
      kPrefix = "Erfolg freigeschaltet: ";
    } else if (lang_str == "4") {
      kPrefix = "Succ\xc3\xa8s d\xc3\xa9verrouill\xc3\xa9 : ";
    } else if (lang_str == "5") {
      kPrefix = "Logro desbloqueado: ";
    } else if (lang_str == "6") {
      kPrefix = "Obiettivo sbloccato: ";
    }

    const float prefix_w = regular->CalcTextSizeA(title_size, FLT_MAX, 0.0f, kPrefix).x;
    const float name_w = bold->CalcTextSizeA(title_size, FLT_MAX, 0.0f, current_->name.c_str()).x;
    const float text_w = std::clamp(prefix_w + name_w + 16.0f * s, 370.0f * s, 640.0f * s);
    const auto lines = WrapLines(regular, desc_size, current_->description, text_w - 8.0f * s);

    // The title starts at the icon's top edge and the description ends at its bottom edge; the space
    // between them is the separation. A description too long for that grows the panel instead.
    const float desc_h = lines.size() * desc_line;
    const float content_h = std::max(icon, title_size + (lines.empty() ? 0.0f : min_gap + desc_h));
    const float box_w = pad + icon + gap + text_w + gap;
    const float box_h = content_h + 2.0f * pad;
    const ImVec2 p0(std::floor((io.DisplaySize.x - box_w) * 0.5f), std::floor(io.DisplaySize.y - box_h - 56.0f * s));
    const ImVec2 p1(p0.x + box_w, p0.y + box_h);

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    // Panel: flat dark, with a thin light border.
    dl->AddRectFilled(p0, p1, Color(0.075f, 0.08f, 0.09f, 0.96f * alpha), rounding);
    dl->AddRect(p0, p1, Color(0.82f, 0.84f, 0.88f, 0.55f * alpha), rounding, 0, std::max(1.0f, 1.2f * s));

    // Icon, vertically centred on the left.
    const float content_top = p0.y + pad;
    const ImVec2 i0(p0.x + pad, content_top + (content_h - icon) * 0.5f);
    const ImVec2 i1(i0.x + icon, i0.y + icon);
    if (icon_) {
      dl->AddImageRounded(reinterpret_cast<ImTextureID>(icon_.get()), i0, i1, ImVec2(0, 0), ImVec2(1, 1),
                          Color(1, 1, 1, alpha), 6.0f * s);
    } else {
      dl->AddRectFilled(i0, i1, Color(0.2f, 0.21f, 0.24f, alpha), 6.0f * s);
    }
    dl->AddRect(i0, i1, Color(1, 1, 1, 0.18f * alpha), 6.0f * s);

    // Text, centred horizontally in the area right of the icon: title at the top, description at the bottom.
    const float area_x = i1.x + gap;
    const float centre_x = area_x + text_w * 0.5f;
    float y = content_top;
    const float title_x = std::floor(centre_x - (prefix_w + name_w) * 0.5f);
    dl->AddText(regular, title_size, ImVec2(title_x, y), Color(1, 1, 1, alpha), kPrefix);
    dl->AddText(bold, title_size, ImVec2(title_x + prefix_w, y), Color(1.0f, 0.85f, 0.1f, alpha),
                current_->name.c_str());
    y = content_top + content_h - desc_h;
    for (const auto& line : lines) {
      const float w = regular->CalcTextSizeA(desc_size, FLT_MAX, 0.0f, line.data(), line.data() + line.size()).x;
      dl->AddText(regular, desc_size, ImVec2(std::floor(centre_x - w * 0.5f), y), Color(1, 1, 1, 0.92f * alpha),
                  line.data(), line.data() + line.size());
      y += desc_line;
    }
  }

  void OnClose() override { icon_.reset(); }

 private:
  rex::ui::ImGuiDrawer* drawer_;
  AchievementToasts* owner_;
  std::optional<AchievementToasts::Toast> current_;
  std::unique_ptr<rex::ui::ImmediateTexture> icon_;
  std::chrono::steady_clock::time_point shown_at_;
};

void AchievementToasts::Attach(rex::ui::WindowedAppContext* app_context, rex::ui::ImGuiDrawer* drawer,
                               void* native_window_handle) {
  app_context_ = app_context;
  drawer_ = drawer;
#ifdef _WIN32
  if (native_window_handle) {
    native_state_ = std::make_shared<NativeState>(this, static_cast<HWND>(native_window_handle));
    if (!native_state_->Initialize()) {
      REXLOG_WARN("[wwe2k16] Native achievement notification overlay unavailable; using the game UI overlay");
      native_state_.reset();
    }
  }
  if (native_state_) return;
#endif
  if (!drawer_)
    return;
  // Segoe UI, baked large so it stays smooth when drawn smaller; the default ImGui font stays first.
  ImFontAtlas* fonts = drawer_->GetIO().Fonts;
  if (fonts->Fonts.empty())
    fonts->AddFontDefault();
  const std::string windows_fonts = "C:\\Windows\\Fonts\\";
  if (std::filesystem::exists(windows_fonts + "segoeui.ttf") && std::filesystem::exists(windows_fonts + "segoeuib.ttf")) {
    font_regular_ = fonts->AddFontFromFileTTF((windows_fonts + "segoeui.ttf").c_str(), 40.0f);
    font_bold_ = fonts->AddFontFromFileTTF((windows_fonts + "segoeuib.ttf").c_str(), 40.0f);
    drawer_->RebuildFontTexture();
  } else {
    REXLOG_WARN("[wwe2k16] Segoe UI not found; achievement pop-ups use the default font");
  }
}

void AchievementToasts::Detach() {
#ifdef _WIN32
  native_state_.reset();
#endif
  std::lock_guard lock(mutex_);
  queue_.clear();
  app_context_ = nullptr;
  drawer_ = nullptr;
}

void AchievementToasts::Push(std::string name, std::string description, std::string icon_path) {
  rex::ui::WindowedAppContext* context;
  {
    std::lock_guard lock(mutex_);
    if (!app_context_)
      return;
    queue_.push_back({std::move(name), std::move(description), std::move(icon_path)});
    context = app_context_;
  }
  context->CallInUIThread([this] { ShowFromUIThread(); });
}

void AchievementToasts::ShowFromUIThread() {
  if (dialog_active_)
    return;
#ifdef _WIN32
  if (native_state_) {
    dialog_active_ = true;
    native_state_->StartNext();
    return;
  }
#endif
  if (!drawer_) return;
  dialog_active_ = true;
  new AchievementToastDialog(drawer_, this);  // registers itself; deletes itself when closed
}

}  // namespace wwe2k16
