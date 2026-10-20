// wwe2k16 - Toggleable ring mat plane shadow rendering.
// Controlled by WWE2K16_SHADOWS=0|1 (Launcher: Shadows toggle).
//
// When disabled (WWE2K16_SHADOWS=0):
//   - Skips the ring mat plane shadow composite pass (WWE 2K16 sub_82B57928 / title update 3 sub_82B5F7F0, by
//     tools/port_functions.py, 'yplaneshadowmap').
//   - The character depth shadow map passes are kept intact so that the shadow map buffer remains cleanly
//     initialized, preventing uninitialized tiled memory patterns (striped/hatched shadow acne) on wrestler models.

#include <atomic>
#include <cstdlib>
#include <cstring>

#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/memory.h>

#ifdef WWE2K16_TU
#define WWE2K16_FN_PlaneShadowPass sub_82B5F7F0
#define WWE2K16_IMP_PlaneShadowPass __imp__sub_82B5F7F0
#else
#define WWE2K16_FN_PlaneShadowPass sub_82B57928
#define WWE2K16_IMP_PlaneShadowPass __imp__sub_82B57928
#endif

namespace {

bool ShadowsEnabled() {
  static const bool enabled = [] {
    const char* v = std::getenv("WWE2K16_SHADOWS");
    return !(v != nullptr && std::strcmp(v, "0") == 0);
  }();
  return enabled;
}

void LogShadowsDisabledOnce() {
  static std::atomic_flag logged = ATOMIC_FLAG_INIT;
  if (!logged.test_and_set(std::memory_order_relaxed)) {
    REXLOG_INFO("[wwe2k16-shadows] shadows disabled (WWE2K16_SHADOWS=0): skipping ring plane shadow pass");
  }
}

}  // namespace

extern "C" PPC_FUNC(WWE2K16_IMP_PlaneShadowPass);
extern "C" PPC_FUNC(WWE2K16_FN_PlaneShadowPass) {
  if (!ShadowsEnabled()) {
    LogShadowsDisabledOnce();
    ctx.r3.s64 = 0;
    return;
  }
  WWE2K16_IMP_PlaneShadowPass(ctx, base);
}

