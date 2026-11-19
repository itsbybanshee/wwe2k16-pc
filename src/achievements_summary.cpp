// wwe2k16 - achievement list for the launcher.
//
// The SDK (v0.10) keeps the title's achievements and the unlock state (userdata\achievements\<TitleId>.toml) and shows its
// own notifications. The launcher's Achievements panel reads userdata\achievements\<TitleId>.json instead: every
// achievement with its unlock state, plus the icons (<TitleId>\<image id>.png) taken from the title's own data. This file
// writes both, once the module is set up and again after each unlock. Format and field names are those of Banshee's
// v0.7.3 SDK patch (src_v07/ and patches/sdk/0001, achievements.cpp).

#include "achievements_summary.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

#include <fmt/format.h>
#include <rex/logging.h>
#include <rex/system/kernel_state.h>
#include <rex/system/util/xdbf_utils.h>

namespace wwe2k16 {
namespace {

constexpr uint64_t kUnixEpochAsFileTime = 116444736000000000ull;  // 1601 -> 1970, 100 ns units

std::string JsonString(const std::string& s) {
  std::string out = "\"";
  for (unsigned char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20)
          out += fmt::format("\\u{:04x}", c);
        else
          out += char(c);
    }
  }
  return out + "\"";
}

// Saves each achievement icon (PNG in the title's XDBF image section) once; returns the image ids that exist on disk.
std::map<uint32_t, bool> ExtractIcons(rex::system::KernelState& kernel, const std::filesystem::path& icon_folder,
                                      const std::vector<rex::system::AchievementInfo>& list) {
  std::map<uint32_t, bool> present;
  const rex::system::util::XdbfGameData db = kernel.title_xdbf();
  if (!db.is_valid()) return present;
  std::error_code ec;
  std::filesystem::create_directories(icon_folder, ec);
  for (const auto& a : list) {
    if (present.count(a.image_id)) continue;
    const auto path = icon_folder / fmt::format("{}.png", a.image_id);
    if (std::filesystem::exists(path, ec)) {
      present[a.image_id] = true;
      continue;
    }
    const rex::system::util::XdbfBlock image = db.GetEntry(rex::system::util::XdbfSection::kImage, a.image_id);
    if (!image || image.size < 8) continue;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(reinterpret_cast<const char*>(image.buffer), std::streamsize(image.size));
    present[a.image_id] = bool(out);
  }
  return present;
}

}  // namespace

void WriteAchievementSummary(rex::system::KernelState& kernel, const std::filesystem::path& user_data_root) {
  if (user_data_root.empty()) return;
  const auto list = kernel.loaded_achievements();
  if (list.empty()) return;
  const uint32_t title_id = kernel.title_id();
  const auto folder = user_data_root / "achievements";
  const std::string icon_dir = fmt::format("{:08X}", title_id);
  const auto icons = ExtractIcons(kernel, folder / icon_dir, list);

  std::ostringstream json;
  json << "{\"titleId\":\"" << icon_dir << "\",\"achievements\":[";
  for (size_t i = 0; i < list.size(); ++i) {
    const auto& a = list[i];
    const bool unlocked = kernel.IsAchievementUnlocked(a.id);
    const uint64_t filetime = unlocked ? kernel.GetAchievementUnlockTime(a.id) : 0;
    // Unlock time as Unix milliseconds, convenient for JavaScript/.NET.
    const long long unix_ms = filetime > kUnixEpochAsFileTime ? (long long)((filetime - kUnixEpochAsFileTime) / 10000) : 0;
    json << (i ? "," : "") << "{\"id\":" << a.id << ",\"name\":" << JsonString(a.label)
         << ",\"description\":" << JsonString(a.description) << ",\"gamerscore\":" << a.gamerscore
         << ",\"unlocked\":" << (unlocked ? "true" : "false") << ",\"unlockedAt\":" << unix_ms << ",\"image\":"
         << (icons.count(a.image_id) ? JsonString(fmt::format("{}/{}.png", icon_dir, a.image_id)) : "null") << "}";
  }
  json << "]}";

  std::error_code ec;
  std::filesystem::create_directories(folder, ec);
  const auto path = folder / fmt::format("{:08X}.json", title_id);
  const auto temp = folder / fmt::format("{:08X}.json.tmp", title_id);
  {
    std::ofstream out(temp, std::ios::trunc | std::ios::binary);
    out << json.str();
    if (!out) return;
  }
  std::filesystem::rename(temp, path, ec);  // atomic replace: a crash never leaves half a file
  REXLOG_INFO("[achievements] wrote {} ({} achievements)", rex::path_to_utf8(path), list.size());
}

}  // namespace wwe2k16
