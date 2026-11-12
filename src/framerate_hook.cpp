// wwe2k16 - 60 FPS entrances / victory motions / cinematics (title update 3 addresses).
//
// Ported from Banshee's frame_rate.cpp (src_v07/). The late-frame flip hooks (ChooseFlip, GpuInterrupt) belong to the
// Xenos GPU path and are left out: the native renderer paces its own presents.
//
//   0x837A3850      engine timing block: +8 frame rate (60/30, PAL 50/25), +12 hold flag, +28..+64 time steps,
//                   +40 frame length in ms (16 / 33)
//   sub_82B23E80    60 -> 30 (50 -> 25): loads the 30 FPS time steps. Called when an entrance, victory motion, replay or
//                   cinematic camera starts
//   sub_82B23F20    30 -> 60 (25 -> 50), skipped while +12 is set
//   sub_82C29578    D3D present interval setter, `stw r4,14000(r3)`
//
// Environment switch: WWE2K16_KEEP_60=0|1   console 30 FPS in entrances etc. | keep 60 FPS (default)

#include "generated_v10/default/wwe2k16_init.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <rex/logging.h>

namespace {

constexpr uint32_t kTimingBlock = 0x837A3850;
constexpr uint32_t kFrameRate = kTimingBlock + 8;

bool Keep60() {
  static const bool enabled = [] {
    const char* value = std::getenv("WWE2K16_KEEP_60");
    return !(value != nullptr && std::strcmp(value, "0") == 0);
  }();
  return enabled;
}

}  // namespace

REX_EXTERN(__imp__sub_82B23E80);
REX_EXTERN(__imp__sub_82B23F20);

// The engine's own switch to 30 FPS. Its animation runs on these time steps (not on wall-clock time), so skipping the
// switch keeps both the display rate and the simulation step at 60: entrances play at normal speed, twice as smooth.
// Forcing only the D3D present interval instead would keep the 1/30 s step: entrances at double speed.
extern "C" REX_FUNC(sub_82B23E80) {
  const uint32_t rate = REX_LOAD_U32(kFrameRate);
  REXLOG_INFO("[wwe2k16-rate] halve request at rate {} ({})", rate, Keep60() ? "suppressed, staying at 60" : "applied");
  if (Keep60()) {
    return;
  }
  __imp__sub_82B23E80(ctx, base);
}

// Logged only: marks the end of a 30 FPS phase in the console behaviour.
extern "C" REX_FUNC(sub_82B23F20) {
  REXLOG_INFO("[wwe2k16-rate] restore request at rate {}", REX_LOAD_U32(kFrameRate));
  __imp__sub_82B23F20(ctx, base);
}

// Logged only: one line per change of the game's requested present interval (1 = 60 FPS, 2 = 30 FPS), to see whether any
// path still asks for 30 FPS with the rate held at 60.
extern "C" REX_FUNC(sub_82C29578) {
  static uint32_t last = UINT32_MAX;
  const uint32_t interval = ctx.r4.u32;
  if (interval != last) {
    last = interval;
    REXLOG_INFO("[wwe2k16-interval] game set present interval {} (frame rate {})", interval, REX_LOAD_U32(kFrameRate));
  }
  REX_STORE_U32(ctx.r3.u32 + 14000, interval);
}
