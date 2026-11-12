// wwe2k16 - achievement list for the launcher (see achievements_summary.cpp).
#pragma once

#include <filesystem>

namespace rex::system {
class KernelState;
}

namespace wwe2k16 {

// Writes userdata\achievements\<TitleId>.json (every achievement with its unlock state) and the icons beside it.
void WriteAchievementSummary(rex::system::KernelState& kernel, const std::filesystem::path& user_data_root);

}  // namespace wwe2k16
