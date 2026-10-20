// wwe2k16 - Main Menu EXIT option, menu layout reordering, system string branding, and quit confirmation prompt.
//
// Changes made:
//   1. Main Menu Layout (replace WWE Shop with Exit at bottom, suppress Xbox Live item):
//      - The original Xbox 360 Main Menu order:
//          PLAY
//          2K SHOWCASE
//          WWE UNIVERSE
//          WWE CREATIONS
//          WWE ONLINE / Xbox Live  <--- REMOVED COMPLETELY (suppressed via GetMenuItemDesc & CheckMenuItemVisible)
//          MY WWE
//          WWE OPTIONS
//          WWE SHOP                <--- REPLACED WITH "EXIT" (string 35178 patched in-place to "EXIT")
//      - We hook `GetMenuItemDesc` (`sub_8281B388` in TU3 / `sub_82816938` in Vanilla):
//          * When queried for string `35243` ("Xbox Live" / WWE Online), it returns the menu object's invalid
//            sentinel descriptor (`menu_obj + 11472`), which causes the entire menu system (visible item collector,
//            slot layout, cursor navigation, and rendering) to treat the entry as completely non-existent.
//      - We also hook `CheckMenuItemVisible` (`sub_8281C6B0` in TU3 / `sub_82817C60` in Vanilla):
//          * Returns -1 for string `35243` to ensure it is never collected into visible items.
//          * Forces string `35178` ("WWE SHOP" -> "EXIT") visible at the bottom of the Main Menu.
//      - Only string `35178` ("WWE SHOP") in `pac\string.pac` is patched in-place to "EXIT" across all languages.
//        String `35243` is left untouched so no duplicate "EXIT" string exists.
//      - In `SetupMenuSlot` (`sub_8281F718` in TU3 / `sub_8281ACC8` in Vanilla), brightness is forced to 1.0f (white).
//
//   2. Native "Are you sure you want to quit the game?" confirmation prompt with Yes (A) and No (B):
//      - When the user selects "EXIT" at the bottom of the menu (`ConfirmCheckAndPopup`):
//        * Instantiates DialogManager singleton if needed (`EnsureDialogManager`).
//        * Allocates guest request memory and configures a type 2 (Yes/No) modal dialog with prompt text.
//        * Sets `player_idx` to `0xFFFFFFFF` (-1) so any controller can respond.
//        * Sets default button to 0 (Yes / A).
//        * Displays the dialog via `ShowDialog` (`sub_828F3DA0` in TU3 / `sub_828EE7A0` in Vanilla).
//      - In `DialogUpdate` (`sub_828F4FE0` in TU3 / `sub_828EF9E0` in Vanilla):
//        * Runs every frame while the modal dialog is up.
//        * Right after `IMP_DialogUpdate` processes controller inputs, we poll `PollDialogResult` (`sub_828F4EF8` in TU3 /
//          `sub_828EF8F8` in Vanilla).
//        * Returns -2 while still waiting for user input.
//        * When the user presses **Yes (A)** (status == 1):
//            Immediately dismisses the dialog via `ResetDialog` and calls `RequestCleanClose()`, cleanly restoring monitor
//            refresh rate and terminating the process on the exact frame button A is pressed.
//        * When the user presses **No (B)** (status == 0):
//            Dismisses the dialog via `ResetDialog` and returns the player to the Main Menu on "EXIT".
//      - `ResetDialog` (`sub_828F35C0` in TU3 / `sub_828EDF90` in Vanilla) is also hooked as a safety fallback.
//
//   3. System name string branding ("Xbox 360 console" -> "PC"):
//      - Decompressed string tables in `ZlibDecompress` (`sub_82BBD158` in TU3 / `sub_82BB4F38` in Vanilla) are scanned
//        and system console name strings in English, French, Spanish, German, Italian, and Japanese are replaced with "PC".

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string_view>

#include <rex/logging.h>
#include <rex/memory.h>
#include <rex/ppc/context.h>
#include <rex/ppc/function.h>
#include <rex/ppc/memory.h>
#include <rex/runtime.h>
#include <rex/ui/window.h>
#include <rex/ui/windowed_app_context.h>

#ifdef WWE2K16_TU
#define WWE2K16_FN_ZlibDecompress sub_82BBD158
#define WWE2K16_IMP_ZlibDecompress __imp__sub_82BBD158
#define WWE2K16_FN_GetMenuItemDesc sub_8281B388
#define WWE2K16_IMP_GetMenuItemDesc __imp__sub_8281B388
#define WWE2K16_FN_CheckMenuItemVisible sub_8281C6B0
#define WWE2K16_IMP_CheckMenuItemVisible __imp__sub_8281C6B0
#define WWE2K16_FN_SetupMenuSlot sub_8281F718
#define WWE2K16_IMP_SetupMenuSlot __imp__sub_8281F718
#define WWE2K16_FN_ConfirmCheckAndPopup sub_8281D090
#define WWE2K16_IMP_ConfirmCheckAndPopup __imp__sub_8281D090
#define WWE2K16_FN_InitDialogReq sub_828F3568
#define WWE2K16_FN_ShowDialog sub_828F3DA0
#define WWE2K16_FN_EnsureDialogManager sub_828F5A38
#define WWE2K16_FN_PollDialogResult sub_828F4EF8
#define WWE2K16_FN_DialogUpdate sub_828F4FE0
#define WWE2K16_IMP_DialogUpdate __imp__sub_828F4FE0
#define WWE2K16_FN_GetDialogResult sub_828F48E8
#define WWE2K16_FN_ResetDialog sub_828F35C0
#define WWE2K16_IMP_ResetDialog __imp__sub_828F35C0
#define WWE2K16_ADDR_DialogMgr 0x837797ECu
#else
#define WWE2K16_FN_ZlibDecompress sub_82BB4F38
#define WWE2K16_IMP_ZlibDecompress __imp__sub_82BB4F38
#define WWE2K16_FN_GetMenuItemDesc sub_82816938
#define WWE2K16_IMP_GetMenuItemDesc __imp__sub_82816938
#define WWE2K16_FN_CheckMenuItemVisible sub_82817C60
#define WWE2K16_IMP_CheckMenuItemVisible __imp__sub_82817C60
#define WWE2K16_FN_SetupMenuSlot sub_8281ACC8
#define WWE2K16_IMP_SetupMenuSlot __imp__sub_8281ACC8
#define WWE2K16_FN_ConfirmCheckAndPopup sub_82818640
#define WWE2K16_IMP_ConfirmCheckAndPopup __imp__sub_82818640
#define WWE2K16_FN_InitDialogReq sub_828EDF38
#define WWE2K16_FN_ShowDialog sub_828EE7A0
#define WWE2K16_FN_EnsureDialogManager sub_828F0438
#define WWE2K16_FN_PollDialogResult sub_828EF8F8
#define WWE2K16_FN_DialogUpdate sub_828EF9E0
#define WWE2K16_IMP_DialogUpdate __imp__sub_828EF9E0
#define WWE2K16_FN_GetDialogResult sub_828EF2E8
#define WWE2K16_FN_ResetDialog sub_828EDF90
#define WWE2K16_IMP_ResetDialog __imp__sub_828EDF90
#define WWE2K16_ADDR_DialogMgr 0x83769724u
#endif

extern "C" PPC_FUNC(WWE2K16_IMP_GetMenuItemDesc);
extern "C" PPC_FUNC(WWE2K16_FN_InitDialogReq);
extern "C" PPC_FUNC(WWE2K16_FN_ShowDialog);
extern "C" PPC_FUNC(WWE2K16_FN_EnsureDialogManager);
extern "C" PPC_FUNC(WWE2K16_FN_PollDialogResult);
extern "C" PPC_FUNC(WWE2K16_FN_GetDialogResult);
extern "C" PPC_FUNC(WWE2K16_IMP_ResetDialog);

namespace {

constexpr uint32_t kMainMenuXboxLiveStringId = 35243;  // "WWE Online / Xbox Live" on the top-level Main Menu, now eliminated
constexpr uint32_t kWweShopStringId = 35178;           // "WWE SHOP" at the bottom of Main Menu, now "EXIT"
constexpr uint32_t kFloatOneBits = 0x3F800000u;        // 1.0f in IEEE-754 big-endian / host register form
constexpr char kExitConfirmationPrompt[] = "Are you sure you want to quit the game?";

std::atomic<bool> g_exit_dialog_pending{false};

struct SubstringReplacement {
  std::string_view from;
  std::string_view to;
};

// Every replacement is strictly shorter than `from`, so it can be applied in-place inside each string's buffer.
constexpr SubstringReplacement kSystemNameReplacements[] = {
    {"Xbox 360 console", "PC"},               // English ("Please don't turn off your Xbox 360 console.")
    {"Console Xbox 360", "PC"},               // French ("Veuillez ne pas éteindre votre Console Xbox 360.")
    {"la Consola Xbox 360", "el PC"},         // Spanish ("No apagues la Consola Xbox 360.")
    {"una Consola Xbox 360", "un PC"},        // Spanish ("El juego debe jugarse en una Consola Xbox 360.")
    {"Consola Xbox 360", "PC"},               // Spanish ("No apagues tu Consola Xbox 360.")
    {"deine Xbox 360 Konsole", "deinen PC"},  // German ("Bitte schalte deine Xbox 360 Konsole nicht aus.")
    {"der Xbox 360 Konsole", "dem PC"},       // German ("Spiel muss auf der Xbox 360 Konsole gespielt werden.")
    {"Xbox 360 Konsole", "PC"},               // German ("Xbox 360 Konsole bitte nicht ausschalten.")
    {"sulla console Xbox 360", "sul PC"},     // Italian ("Il gioco deve essere giocato sulla console Xbox 360.")
    {"la console Xbox 360", "il PC"},         // Italian ("Non spegnere la console Xbox 360.")
    {"Xbox 360 \xE6\x9C\xAC\xE4\xBD\x93", "PC"},  // Japanese ("Xbox 360 本体の電源を切らないでください。")
    {"Xbox 360", "PC"},                       // Generic fallback
};

uint32_t LoadLe32(const uint8_t* p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

void StoreLe32(uint8_t* p, uint32_t v) {
  p[0] = uint8_t(v & 0xFF);
  p[1] = uint8_t((v >> 8) & 0xFF);
  p[2] = uint8_t((v >> 16) & 0xFF);
  p[3] = uint8_t((v >> 24) & 0xFF);
}

bool ReplaceSystemNamesInPlace(uint8_t* slot, uint32_t capacity, uint32_t& out_length) {
  size_t len = 0;
  while (len < capacity && slot[len] != 0) {
    ++len;
  }
  bool modified = false;
  for (const auto& [from, to] : kSystemNameReplacements) {
    size_t search_pos = 0;
    while (search_pos < len) {
      const std::string_view view(reinterpret_cast<const char*>(slot), len);
      const size_t pos = view.find(from, search_pos);
      if (pos == std::string_view::npos) {
        break;
      }
      const size_t tail_len = len - (pos + from.size());
      std::memcpy(slot + pos, to.data(), to.size());
      std::memmove(slot + pos + to.size(), slot + pos + from.size(), tail_len);
      len -= (from.size() - to.size());
      search_pos = pos + to.size();
      modified = true;
    }
  }
  if (modified) {
    std::memset(slot + len, 0, capacity - len);
    out_length = uint32_t(len + 1);
  }
  return modified;
}

// Queries `WWE2K16_IMP_GetMenuItemDesc(menu_obj, group_id, item_id)` without disturbing the caller's PPCContext
// and returns the item's raw String ID (+24), or 0 if the descriptor is the invalid sentinel.
uint32_t GetMenuItemStringId(PPCContext& ctx, uint8_t* base, uint32_t menu_obj, uint32_t group_id, uint32_t item_id) {
  const PPCContext saved = ctx;
  ctx.r3.u64 = menu_obj;
  ctx.r4.s64 = int32_t(group_id);
  ctx.r5.s64 = int32_t(item_id);
  WWE2K16_IMP_GetMenuItemDesc(ctx, base);
  const uint32_t desc = ctx.r3.u32;
  ctx = saved;
  if (!desc || PPC_LOAD_U32(desc) == 0xFFFFFFFFu) {
    return 0;
  }
  return PPC_LOAD_U32(desc + 24);
}

struct ExitDialogGuestStorage {
  uint32_t req_ea = 0;
  uint32_t text_ea = 0;
};

// Allocates permanent guest virtual memory in the system heap for the dialog request and prompt string.
ExitDialogGuestStorage GetExitDialogGuestStorage() {
  static ExitDialogGuestStorage s_storage{};
  if (!s_storage.req_ea) {
    auto* rt = rex::Runtime::instance();
    if (rt && rt->memory()) {
      s_storage.req_ea = rt->memory()->SystemHeapAlloc(64, 16);
      s_storage.text_ea = rt->memory()->SystemHeapAlloc(sizeof(kExitConfirmationPrompt), 16);
      if (s_storage.text_ea && rt->virtual_membase()) {
        std::memcpy(rt->virtual_membase() + s_storage.text_ea, kExitConfirmationPrompt,
                    sizeof(kExitConfirmationPrompt));
      }
      REXLOG_INFO("[wwe2k16-menu] Exit dialog guest storage allocated: req=0x{:08X}, text=0x{:08X}",
                  s_storage.req_ea, s_storage.text_ea);
    }
  }
  return s_storage;
}

void RequestCleanClose() {
  REXLOG_INFO("[wwe2k16-menu] EXIT confirmed from Main Menu; requesting window close");
  if (auto* rt = rex::Runtime::instance(); rt && rt->app_context()) {
    rt->app_context()->CallInUIThreadDeferred([] {
      if (auto* runtime = rex::Runtime::instance(); runtime && runtime->display_window()) {
        runtime->display_window()->RequestClose();
      }
    });
  }
}

}  // namespace

// --- 1. String table patch ("WWE SHOP" -> "EXIT", "Xbox 360 console" -> "PC") --------------------------------------

extern "C" PPC_FUNC(WWE2K16_IMP_ZlibDecompress);
extern "C" PPC_FUNC(WWE2K16_FN_ZlibDecompress) {
  const uint32_t dst = ctx.r3.u32;
  const uint32_t dst_cap = ctx.r4.u32;
  WWE2K16_IMP_ZlibDecompress(ctx, base);

  if (ctx.r3.u32 == 0 || dst == 0 || dst_cap < 20) {
    return;
  }

  uint8_t* const p = base + dst;
  if (LoadLe32(p) != 0) {
    return;
  }
  const uint32_t count = LoadLe32(p + 4);
  if (count < 1 || count > 50000) {
    return;
  }
  const uint64_t table_end = 8ull + uint64_t(count) * 12ull;
  if (table_end > dst_cap || LoadLe32(p + 8) != uint32_t(table_end)) {
    return;
  }

  uint32_t pc_patched_count = 0;
  for (uint32_t i = 0; i < count; ++i) {
    uint8_t* const entry = p + 8 + i * 12;
    const uint32_t offset = LoadLe32(entry + 0);
    const uint32_t length = LoadLe32(entry + 4);
    const uint32_t id = LoadLe32(entry + 8);
    if (offset < table_end || uint64_t(offset) + length > dst_cap || length == 0) {
      continue;
    }

    // Only patch WWE SHOP (35178) to "EXIT". String 35243 (Xbox Live) is not patched so no middle "EXIT" string exists.
    if (id == kWweShopStringId && length >= 5) {
      std::memcpy(p + offset, "EXIT", 5);
      if (length > 5) {
        std::memset(p + offset + 5, 0, length - 5);
      }
      StoreLe32(entry + 4, 5);
      REXLOG_INFO("[wwe2k16-menu] patched Main Menu string {} (\"WWE SHOP\" -> \"EXIT\")", id);
      continue;
    }

    uint32_t new_length = length;
    if (ReplaceSystemNamesInPlace(p + offset, length, new_length)) {
      StoreLe32(entry + 4, new_length);
      ++pc_patched_count;
    }
  }

  if (pc_patched_count > 0) {
    REXLOG_INFO("[wwe2k16-menu] patched {} \"Xbox 360\" system string(s) to \"PC\" (table count={})",
                pc_patched_count, count);
  }
}

// --- 2. Menu descriptor hook: completely suppress Xbox Live / middle entry at descriptor lookup ---------------------

extern "C" PPC_FUNC(WWE2K16_FN_GetMenuItemDesc) {
  const uint32_t menu_obj = ctx.r3.u32;
  WWE2K16_IMP_GetMenuItemDesc(ctx, base);
  const uint32_t desc = ctx.r3.u32;
  if (desc && PPC_LOAD_U32(desc) != 0xFFFFFFFFu) {
    const uint32_t str_id = PPC_LOAD_U32(desc + 24);
    if (str_id == kMainMenuXboxLiveStringId) {
      // Suppress the old Xbox Live item: return the menu object's invalid sentinel descriptor (+11472).
      // This causes the entire menu engine to omit this slot completely (no button, no cursor stop, no gap).
      ctx.r3.u64 = menu_obj + 11472u;
      return;
    }
  }
}

// --- 3. Menu slot visibility hook: hide middle Xbox Live slot, force bottom EXIT visible ---------------------------

extern "C" PPC_FUNC(WWE2K16_IMP_CheckMenuItemVisible);
extern "C" PPC_FUNC(WWE2K16_FN_CheckMenuItemVisible) {
  const uint32_t menu_obj = ctx.r3.u32;
  const uint32_t group_id = ctx.r4.u32;
  const uint32_t item_id = ctx.r5.u32;

  const uint32_t str_id = GetMenuItemStringId(ctx, base, menu_obj, group_id, item_id);
  if (str_id == kMainMenuXboxLiveStringId) {
    // Hide the old middle Xbox Live slot completely from the Main Menu
    ctx.r3.s64 = -1;
    return;
  }
  if (str_id == kWweShopStringId) {
    // Force WWE Shop (patched to EXIT) visible at the bottom of the Main Menu
    ctx.r3.u64 = kWweShopStringId;
    return;
  }

  WWE2K16_IMP_CheckMenuItemVisible(ctx, base);
}

// --- 4. Menu slot brightness override (render bottom "EXIT" at full white brightness) -------------------------------

extern "C" PPC_FUNC(WWE2K16_IMP_SetupMenuSlot);
extern "C" PPC_FUNC(WWE2K16_FN_SetupMenuSlot) {
  const uint32_t menu_obj = ctx.r3.u32;
  const uint32_t col_idx = ctx.r4.u32;
  const uint32_t group_id = ctx.r6.u32;
  const uint32_t item_slot = ctx.r7.u32;

  WWE2K16_IMP_SetupMenuSlot(ctx, base);

  const uint32_t str_id = GetMenuItemStringId(ctx, base, menu_obj, group_id, item_slot);
  if (str_id == kWweShopStringId) {
    const uint32_t page = PPC_LOAD_U32(menu_obj + 268);
    const uint32_t col_base = menu_obj + (page * 9u + col_idx) * 588u + 856u;
    PPC_STORE_U32(col_base + (item_slot + 87u) * 4u, kFloatOneBits);
  }
}

// --- 5. Confirm press handler (prompt "Are you sure you want to quit the game?" with Yes/No [A/B]) -------------------

extern "C" PPC_FUNC(WWE2K16_IMP_ConfirmCheckAndPopup);
extern "C" PPC_FUNC(WWE2K16_FN_ConfirmCheckAndPopup) {
  const uint32_t menu_obj = ctx.r3.u32;
  const uint32_t group_id = ctx.r4.u32;
  const uint32_t item_id = ctx.r5.u32;

  const uint32_t str_id = GetMenuItemStringId(ctx, base, menu_obj, group_id, item_id);
  if (str_id == kWweShopStringId || str_id == kMainMenuXboxLiveStringId) {
    if (g_exit_dialog_pending.load()) {
      ctx.r3.u64 = 1;
      return;
    }

    const PPCContext saved = ctx;

    // 1. Ensure DialogManager singleton exists (plays chime 64 and constructs singleton if needed)
    ctx.r3.s64 = 0;
    ctx.r4.s64 = 64;
    WWE2K16_FN_EnsureDialogManager(ctx, base);

    const uint32_t dialog_mgr = PPC_LOAD_U32(WWE2K16_ADDR_DialogMgr);
    if (!dialog_mgr) {
      REXLOG_WARN("[wwe2k16-menu] DialogManager not initialized; falling back to direct close");
      RequestCleanClose();
      ctx = saved;
      ctx.r3.u64 = 1;
      return;
    }

    const auto storage = GetExitDialogGuestStorage();
    const uint32_t req_ea = storage.req_ea ? storage.req_ea : ((ctx.r1.u32 - 128) & ~15u);
    const uint32_t text_ea = storage.text_ea ? storage.text_ea : (req_ea + 64);
    if (storage.text_ea && base) {
      std::memcpy(base + storage.text_ea, kExitConfirmationPrompt, sizeof(kExitConfirmationPrompt));
    } else if (!storage.text_ea) {
      std::memcpy(base + text_ea, kExitConfirmationPrompt, sizeof(kExitConfirmationPrompt));
    }

    // 2. Initialize 56-byte dialog request struct
    ctx.r3.u64 = req_ea;
    WWE2K16_FN_InitDialogReq(ctx, base);

    // 3. Configure dialog request:
    PPC_STORE_U32(req_ea + 0, 2);                      // dialog_type: 2 = Yes / No confirmation dialog
    PPC_STORE_U32(req_ea + 4, 30721);                  // message_string_id
    PPC_STORE_U32(req_ea + 8, text_ea);                // custom message string: "Are you sure you want to quit the game?"
    PPC_STORE_U32(req_ea + 16, 0);                     // sub-type
    PPC_STORE_U32(req_ea + 20, 0xFFFFFFFFu);           // player_idx: -1 allows any controller to interact
    PPC_STORE_U32(req_ea + 28, 0);                     // default button: 0 (Yes / A)

    // 4. Open native modal dialog
    ctx.r3.u64 = dialog_mgr;
    ctx.r4.u64 = req_ea;
    WWE2K16_FN_ShowDialog(ctx, base);

    ctx = saved;
    g_exit_dialog_pending.store(true);
    REXLOG_INFO("[wwe2k16-menu] Opened exit confirmation dialog (\"Are you sure you want to quit the game?\")");

    ctx.r3.u64 = 1;
    return;
  }

  WWE2K16_IMP_ConfirmCheckAndPopup(ctx, base);
}

// --- 6. Dialog tick hook (polls user response every frame; instantly exits on Yes [A] without waiting for B) --------

extern "C" PPC_FUNC(WWE2K16_IMP_DialogUpdate);
extern "C" PPC_FUNC(WWE2K16_FN_DialogUpdate) {
  WWE2K16_IMP_DialogUpdate(ctx, base);

  if (g_exit_dialog_pending.load()) {
    PPCContext poll_ctx = ctx;
    WWE2K16_FN_PollDialogResult(poll_ctx, base);
    const int32_t status = poll_ctx.r3.s32;

    if (status != -2) {
      g_exit_dialog_pending.store(false);
      REXLOG_INFO("[wwe2k16-menu] Exit dialog status polled in DialogUpdate: {}", status);

      const uint32_t dialog_mgr = PPC_LOAD_U32(WWE2K16_ADDR_DialogMgr);
      if (dialog_mgr) {
        PPCContext reset_ctx = ctx;
        reset_ctx.r3.u64 = dialog_mgr;
        WWE2K16_IMP_ResetDialog(reset_ctx, base);
      }

      if (status == 1) {
        REXLOG_INFO("[wwe2k16-menu] Exit confirmed with Yes (A); closing game immediately");
        RequestCleanClose();
      } else {
        REXLOG_INFO("[wwe2k16-menu] Exit cancelled with No (B); dismissed dialog and returned to Main Menu");
      }
    }
  }
}

// --- 7. Dialog dismissal fallback hook (catches cancel / back button [B] dismissals) --------------------------------

extern "C" PPC_FUNC(WWE2K16_IMP_ResetDialog);
extern "C" PPC_FUNC(WWE2K16_FN_ResetDialog) {
  const uint32_t dialog_mgr = ctx.r3.u32;

  if (g_exit_dialog_pending.exchange(false)) {
    PPCContext check_ctx = ctx;
    check_ctx.r3.u64 = dialog_mgr;
    WWE2K16_FN_GetDialogResult(check_ctx, base);
    const uint32_t result = check_ctx.r3.u32;
    const uint32_t raw_sel = dialog_mgr ? PPC_LOAD_U32(dialog_mgr + 64) : 0xFFFFFFFFu;

    REXLOG_INFO("[wwe2k16-menu] Exit confirmation dialog dismissed via ResetDialog: result={}, raw_sel=0x{:X}",
                result, raw_sel);

    if (result == 1 || raw_sel == 0) {
      REXLOG_INFO("[wwe2k16-menu] Exit confirmed with Yes (A); closing game");
      RequestCleanClose();
    } else {
      REXLOG_INFO("[wwe2k16-menu] Exit cancelled with No (B); remaining in Main Menu");
    }
  }

  WWE2K16_IMP_ResetDialog(ctx, base);
}
