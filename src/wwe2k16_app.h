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
#include <rex/rex_app.h>
#include <rex/system/kernel_state.h>

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
  }

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
    if (!update_data_root_.empty()) AddOverlay(fs, "\\GAMEUPDATE", update_data_root_);
  }

  // The title reads and writes its Xbox HDD cache (cache:\EditEx.cac, cache:\CAR.cac: Create mode and arena data).
  void OnPostSetup() override {
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

  std::filesystem::path user_data_root_;
  std::filesystem::path game_data_root_;
  std::filesystem::path update_data_root_;
};
