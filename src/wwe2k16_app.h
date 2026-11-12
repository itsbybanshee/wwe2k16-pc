// wwe2k16 - WWE 2K16 (Xbox 360) static recompilation with the ReXGlue SDK v0.10.
//
// Branch native-sdk10 (docs/NATIVE_PORT_PLAN.md). The v0.7.3 version of this file, with Banshee's features, is in
// src_v07/ and is ported piece by piece. The launcher (launcher/) starts the game with command line options and
// WWE2K16_* environment variables; this build accepts all of them, and honours the ones that have an effect so far.

#pragma once

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <thread>
#include <string>
#include <system_error>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <timeapi.h>
#pragma comment(lib, "winmm.lib")
#endif

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/filesystem/devices/host_path_device.h>
#include <rex/filesystem/vfs.h>
#include <rex/logging/macros.h>
#include <rex/input/input_system.h>
#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/system/kernel_state.h>

#include <rex/ui/keybinds.h>
#include "display_refresh_rate.h"
#include "fps_overlay.h"
#include "script_input.h"

#include <imgui.h>

#if defined(SVR_NATIVE_RENDERER)
// Native renderer (wwe2k16_native.exe only, cmake/native_renderer.cmake): re:Blue draws through plume (Vulkan) from the
// game's own D3D calls instead of emulating the Xenos GPU.
#include <rex/system/gpu_plugin.h>
#include "gpu/device.h"
#include "gpu/imgui_overlay_drawer.h"
#include "gpu/shaders/shader_cache.h"
#endif

// Options the launcher always passes. They must exist as cvars or the command line parser rejects them. The ones
// marked "not yet" are accepted and ignored until the matching feature is ported (see docs/NATIVE_PORT_PLAN.md).
// (resolution_scale, anisotropic_override, swap_post_effect and present_effect already exist in the Xenos GPU plugin.)
REXCVAR_DEFINE_BOOL(enable_console, false, "WWE2K16", "Show a console window");
REXCVAR_DEFINE_STRING(xmp_music_dir, "", "WWE2K16", "Folder of the custom soundtrack");  // not yet
REXCVAR_DEFINE_INT32(user_language, 1, "WWE2K16", "Xbox 360 language id");  // not yet
REXCVAR_DEFINE_STRING(pad_remap, "", "WWE2K16", "Controller button remap: FROM=TO pairs");  // not yet
REXCVAR_DEFINE_BOOL(wwe2k16_mods, true, "WWE2K16", "Read replacement files from the mods folder");

#define WWE2K16_KEYBIND(name) \
  REXCVAR_DEFINE_STRING(keybind_##name, "", "WWE2K16/Keybinds", "Keyboard binding for " #name);  // not yet
WWE2K16_KEYBIND(a) WWE2K16_KEYBIND(b) WWE2K16_KEYBIND(x) WWE2K16_KEYBIND(y)
WWE2K16_KEYBIND(left_trigger) WWE2K16_KEYBIND(right_trigger)
WWE2K16_KEYBIND(left_shoulder) WWE2K16_KEYBIND(right_shoulder)
WWE2K16_KEYBIND(lstick_up) WWE2K16_KEYBIND(lstick_down) WWE2K16_KEYBIND(lstick_left) WWE2K16_KEYBIND(lstick_right)
WWE2K16_KEYBIND(lstick_press)
WWE2K16_KEYBIND(rstick_up) WWE2K16_KEYBIND(rstick_down) WWE2K16_KEYBIND(rstick_left) WWE2K16_KEYBIND(rstick_right)
WWE2K16_KEYBIND(rstick_press)
WWE2K16_KEYBIND(dpad_up) WWE2K16_KEYBIND(dpad_down) WWE2K16_KEYBIND(dpad_left) WWE2K16_KEYBIND(dpad_right)
WWE2K16_KEYBIND(back) WWE2K16_KEYBIND(start) WWE2K16_KEYBIND(guide)
#undef WWE2K16_KEYBIND

class Wwe2k16App : public rex::ReXApp {
 public:
  template <typename... Args>
  explicit Wwe2k16App(Args&&... args) : rex::ReXApp(std::forward<Args>(args)...) {
    // The launcher passes the game folder or .iso as the first argument, before the options.
    AddPositionalOption("game_directory");
  }

  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Wwe2k16App>(new Wwe2k16App(ctx, "wwe2k16", PPCImageConfig));
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
#if defined(_WIN32)
    // The GPU command processor waits with Sleep(1 ms); at the default ~15.6 ms timer tick every wait costs a whole tick.
    timeBeginPeriod(1);
#endif
    if (config.gpu_plugin.empty()) {
      config.gpu_plugin = "xenos";
    }
#if defined(SVR_NATIVE_RENDERER)
    // The Xenos plugin's "null" backend keeps the game's D3D command stream moving (fences, interrupts, vblank) without
    // drawing; the re:Blue hooks draw instead.
    if (!config.graphics) {
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin, "null");
      REXLOG_INFO("Native renderer: guest GPU {}", config.graphics ? "null (sync only)" : "MISSING");
    }
#endif
  }

#if defined(SVR_NATIVE_RENDERER)
  // The plume device draws the ImGui overlays at Present time.
  std::unique_ptr<rex::ui::ImmediateDrawer> OnCreateImmediateDrawer() override {
    return std::make_unique<bd::gpu::ImGuiOverlayDrawer>();
  }

  void OnPreLaunchModule() override {
    if (!bd::gpu::Video::CreateHostDevice(window())) {
      // GPU or driver without the Vulkan 1.2 features re:Blue needs (bufferDeviceAddress, descriptor indexing...):
      // device.cpp has logged which. Say so and quit instead of running behind a black window. The launcher looks for
      // "could not create the host device" in the log: keep it.
      REXLOG_ERROR("Native renderer: could not create the host device (Vulkan unavailable or too old)");
      rex::FlushLogging();
#if defined(_WIN32)
      MessageBoxW(nullptr,
                  L"The native renderer could not start: this graphics card or its driver lacks the Vulkan 1.2 "
                  L"features it needs.\n\nUpdate the graphics driver.\n\nLe rendu natif n’a pas pu démarrer : "
                  L"cette carte graphique ou son pilote n’a pas les fonctions Vulkan 1.2 nécessaires.\n\n"
                  L"Mets à jour le pilote graphique.",
                  L"WWE 2K16 PC", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#endif
      std::_Exit(1);
    }
#if defined(WWE2K16_RUNTIME_SHADERS)
    // Without a usable dxcompiler.dll no shader can be converted and the window stays black: fail cleanly.
    if (!Wwe2k16DxcUsable()) {
      REXLOG_ERROR("Native renderer: shader compiler (dxcompiler.dll) missing or unusable");
      rex::FlushLogging();
#if defined(_WIN32)
      MessageBoxW(nullptr,
                  L"The native renderer could not start: dxcompiler.dll is missing or unusable (it must sit next to the "
                  L"program).\n\nLe rendu natif n’a pas pu démarrer : dxcompiler.dll est absent ou "
                  L"inutilisable (il doit être à côté du programme).",
                  L"WWE 2K16 PC", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#endif
      std::_Exit(1);
    }
#endif
    // Present runs on the game thread and ImGui on the UI thread: draw the overlay synchronously on the UI thread into
    // the presented image.
    bd::gpu::Video::SetOverlayDrawHook([this](plume::RenderCommandList* cmd, plume::RenderFramebuffer* fb, uint32_t w,
                                              uint32_t h) {
      if (!imgui_drawer() || !imgui_drawer()->HasDialogs()) return;
      app_context().CallInUIThreadSynchronous([this, cmd, fb, w, h] {
        bd::gpu::ReblueUIDrawContext ctx(w, h, cmd, fb);
        imgui_drawer()->Draw(ctx);
      });
    });
  }

  void OnWindowPixelSizeChanged(uint32_t, uint32_t) override { bd::gpu::Video::RequestResize(); }
#endif

  void OnConfigurePaths(rex::PathConfig& paths) override {
    const auto exe_dir = rex::filesystem::GetExecutableFolder();
    // Keep the graphics and pipeline caches beside the user data when the launcher gave one, else beside the exe.
    if (paths.cache_root.empty()) paths.cache_root = exe_dir / "cache";
    if (paths.user_data_root.empty()) paths.user_data_root = exe_dir / "userdata";
    if (paths.game_data_root.empty()) {
      if (auto game = GetArgument("game_directory"); game && !game->empty()) paths.game_data_root = *game;
    }
    user_data_root_ = paths.user_data_root;
    game_data_root_ = paths.game_data_root;
    update_data_root_ = paths.update_data_root;
    if (paths.game_data_root.empty()) {
      REXLOG_ERROR("No game given: start the game from the launcher, or pass the game folder or .iso as an argument.");
    }
  }

  // With a disc image, default.xexp is not beside default.xex on the disc: the title update folder is laid over the disc
  // (as the mods are) before the executable loads, so the loader finds game:\default.xexp there and applies the update.
  void OnLoadXexImage(std::string& xex_image) override {
    (void)xex_image;
    if (!IsDiscImage(game_data_root_)) return;
    auto* fs = REX_KERNEL_FS();
    if (!fs) return;
    // The launcher's mod overlay comes first: overlays are searched in registration order.
    if (const char* overlay = std::getenv("WWE2K16_GAME_OVERLAY"); overlay && *overlay) {
      AddOverlay(fs, "\\GAMEMODS", overlay);
    }
    // Only default.xexp goes over the disc: the title reads the rest of its update through update:\ (the SDK mounts
    // --update_data_root there) and falls back to the disc itself, so laying the whole update over the drive made it read
    // the update's files in places where it expects the disc's.
    if (!update_data_root_.empty()) {
      std::error_code ec;
      const auto xexp = update_data_root_ / "default.xexp";
      const auto overlay = user_data_root_ / "xexp_overlay";
      if (std::filesystem::is_regular_file(xexp, ec)) {
        std::filesystem::create_directories(overlay, ec);
        std::filesystem::copy_file(xexp, overlay / "default.xexp", std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) AddOverlay(fs, "\\GAMEXEXP", overlay);
      } else {
        REXLOG_ERROR("No default.xexp in {}: the title update cannot be applied", update_data_root_.string());
      }
    }
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {
#if defined(_WIN32)
    SetupWindowMode();
    // Keep the guest update clock steady on high refresh monitors: the game's monitor runs at 60 Hz while it plays.
    if (window()) display_refresh_rate_.SwitchTo60Hz(window()->GetNativeWindowHandle());
#endif
    fps_overlay_.Attach(drawer);
    rex::ui::RegisterBind("bind_toggle_fullscreen", "F11", "Toggle fullscreen / windowed", [this] {
#if defined(_WIN32)
      ToggleWindowMode();
#else
      if (auto* w = window()) w->SetFullscreen(!w->IsFullscreen());
#endif
    });
  }

  void OnShutdown() override {
    rex::ui::UnregisterBind("bind_toggle_fullscreen");
    fps_overlay_.Detach();
#if defined(_WIN32)
    display_refresh_rate_.Restore();
    // A guest thread blocked in a kernel wait can keep the process alive after the window is gone: leave for good.
    std::thread([] {
      Sleep(2000);
      _exit(0);
    }).detach();
#endif
  }
  // The title reads and writes its Xbox HDD cache (cache:\EditEx.cac, cache:\CAR.cac: Create mode and arena data).
  void OnPostSetup() override {
    // Scripted controller for automated tests (script_input.h): WWE2K16_TEST_INPUT=1, commands in logs\test_input.txt.
    if (wwe2k16::ScriptInputDriver::Enabled() && runtime() && runtime()->input_system()) {
      static_cast<rex::input::InputSystem*>(runtime()->input_system())
          ->AddDriver(std::make_unique<wwe2k16::ScriptInputDriver>(nullptr));
      REXLOG_INFO("[wwe2k16] scripted input on (logs/test_input.txt)");
    }
    if (user_data_root_.empty()) return;
    const auto cache_root = user_data_root_ / "xbox_cache";
    std::error_code ec;
    std::filesystem::create_directories(cache_root, ec);
    if (ec) {
      REXLOG_ERROR("Unable to create Xbox cache folder {}: {}", cache_root.string(), ec.message());
      return;
    }
    auto* fs = REX_KERNEL_FS();
    auto device = std::make_unique<rex::filesystem::HostPathDevice>("\\CACHE", cache_root, false);
    if (!fs || !device->Initialize() || !fs->RegisterDevice(std::move(device))) {
      REXLOG_ERROR("Unable to mount Xbox cache folder {}", cache_root.string());
      return;
    }
    fs->RegisterSymbolicLink("cache:", "\\CACHE");
    fs->RegisterSymbolicLink("cache0:", "\\CACHE");
    fs->RegisterSymbolicLink("cache1:", "\\CACHE");
    REXLOG_INFO("Xbox cache mounted at {}", cache_root.string());
  }

 private:
  static bool IsDiscImage(const std::filesystem::path& p) {
    std::error_code ec;
    std::string ext = p.extension().string();
    for (auto& c : ext) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext == ".iso" && std::filesystem::is_regular_file(p, ec);
  }

  static void AddOverlay(rex::filesystem::VirtualFileSystem* fs, const char* name, const std::filesystem::path& root) {
    auto device = std::make_unique<rex::filesystem::HostPathDevice>(name, root, true);
    if (!device->Initialize() || !fs->RegisterDevice(std::move(device))) {
      REXLOG_ERROR("Unable to mount {} as an overlay of the disc image", root.string());
      return;
    }
    fs->RegisterOverlay("\\Device\\Harddisk0\\Partition1", name);
    REXLOG_INFO("Disc image: {} read from {}", name, root.string());
  }

  wwe2k16::FpsOverlay fps_overlay_;
  wwe2k16::DisplayRefreshRate display_refresh_rate_;

#if defined(_WIN32)
  bool is_borderless_ = false;
  RECT pre_borderless_rect_{};
  DWORD pre_borderless_style_ = 0;

  // WWE2K16_WINDOW_MODE=windowed|borderless|fullscreen (the launcher sets it); borderless covers the monitor.
  void SetupWindowMode() {
    HWND hwnd = window() ? static_cast<HWND>(window()->GetNativeWindowHandle()) : nullptr;
    if (!hwnd) return;
    const char* mode_env = std::getenv("WWE2K16_WINDOW_MODE");
    const std::string mode = mode_env ? mode_env : "";
    if (mode == "borderless") EnterBorderless(hwnd);
  }

  void EnterBorderless(HWND hwnd) {
    HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(monitor, &mi)) return;
    pre_borderless_style_ = GetWindowLongW(hwnd, GWL_STYLE);
    GetWindowRect(hwnd, &pre_borderless_rect_);
    DWORD style = pre_borderless_style_;
    style &= ~WS_OVERLAPPEDWINDOW;
    style |= WS_POPUP | WS_VISIBLE;
    SetWindowLongW(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                 mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    is_borderless_ = true;
    REXLOG_INFO("[wwe2k16] Borderless window mode applied");
  }

  void ToggleWindowMode() {
    HWND hwnd = window() ? static_cast<HWND>(window()->GetNativeWindowHandle()) : nullptr;
    if (!hwnd) return;
    if (is_borderless_) {
      SetWindowLongW(hwnd, GWL_STYLE, pre_borderless_style_ ? pre_borderless_style_ : (WS_OVERLAPPEDWINDOW | WS_VISIBLE));
      int w = pre_borderless_rect_.right - pre_borderless_rect_.left;
      int h = pre_borderless_rect_.bottom - pre_borderless_rect_.top;
      if (w <= 0 || h <= 0) { w = 1280; h = 720; }
      SetWindowPos(hwnd, nullptr, pre_borderless_rect_.left, pre_borderless_rect_.top, w, h,
                   SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
      is_borderless_ = false;
      REXLOG_INFO("[wwe2k16] Switched from borderless to windowed");
    } else if (window()->IsFullscreen()) {
      window()->SetFullscreen(false);
    } else {
      EnterBorderless(hwnd);
    }
  }
#endif
  std::filesystem::path user_data_root_;
  std::filesystem::path game_data_root_;
  std::filesystem::path update_data_root_;
};
