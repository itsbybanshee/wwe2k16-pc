// wwe2k16 - guards for game calls that would write through a NULL pointer (title update 3 addresses).
//
// Without a guard, a write at guest address 0x17B (host guest-base + a small offset) is an access violation that ends the
// game: a user session crashed 15 s into an entrance (test-pack run of 2026-11-12) in sub_82BF74A0, the CRT memset, called
// with a destination of 0x17B. The guard skips such a memset and logs the caller (the guest return address, ctx.lr), so the
// function that passed the NULL pointer can be found and guarded where it belongs (see WWE 2K15 PC's crash_guards.cpp).

#include "generated_v10/default/wwe2k16_init.h"

#include <atomic>
#include <cstdint>

#include <rex/cvar.h>
#include <rex/logging.h>

REXCVAR_DEFINE_BOOL(wwe2k16_crash_guards, true, "WWE2K16",
                    "Skip game calls that would write through a NULL pointer (crash guards)");

REX_EXTERN(__imp__sub_82BF74A0);

// memset(dest r3, byte r4, count r5), returns dest in r3.
extern "C" REX_FUNC(sub_82BF74A0) {
  if (ctx.r3.u32 < 0x10000 && ctx.r5.u32 != 0 && REXCVAR_GET(wwe2k16_crash_guards)) {
    static std::atomic<uint32_t> skipped{0};
    if (skipped.fetch_add(1) < 16)
      REXLOG_WARN("crash guard: memset skipped, destination {:08X} (byte {:02X}, count {}), caller lr {:08X}",
                  ctx.r3.u32, ctx.r4.u32 & 0xFF, ctx.r5.u32, uint32_t(ctx.lr));
    return;
  }
  __imp__sub_82BF74A0(ctx, base);
}
