// GPU -> CPU coherency for textures the game composites on the GPU and finishes on the CPU.
//
// WWE 2K16 builds some character textures at load time:
// it renders the layers, resolves them into guest memory, then untiles them on the CPU with
// XGUntileSurface. The recompiled GPU keeps resolve results in its own memory, so without a readback
// the CPU untiles stale memory and the model renders black with magenta garbage. Reading back every
// resolve costs significant frame rate; this override reads back only the source surface of each untile,
// which only happens while loading.
//
//   sub_82C70510    XGUntileSurface(dest=r3, row_pitch=r4, point=r5, source=r6, width=r7, height=r8,
//                   rect=r9, texel_pitch=r10). Identified by its arguments, the in-place copy of
//                   align32(width) * align32(height) * texel_pitch bytes and the 2D tiling math.
//                   WWE 2K16 retail: sub_82C70510, matched by tools/port_functions.py.
//                   Title update 3: sub_82C78698.
//
// Kill switch: WWE2K16_UNTILE_COHERENCY=0.

#include <cstdint>
#include <cstdlib>

#include <rex/graphics/command_processor.h>
#include <rex/graphics/graphics_system.h>
#include <rex/logging.h>
#include <rex/ppc/context.h>
#include <rex/ppc/memory.h>
#include <rex/runtime.h>

#ifdef WWE2K16_TU
#define WWE2K16_FN_UntileSurface sub_82C78698
#define WWE2K16_IMP_UntileSurface __imp__sub_82C78698
#else
#define WWE2K16_FN_UntileSurface sub_82C70510
#define WWE2K16_IMP_UntileSurface __imp__sub_82C70510
#endif

namespace {

void MakeUntileSourceVisible(uint32_t source, uint32_t width, uint32_t height, uint32_t texel_pitch) {
  static const bool enabled = [] {
    const char* v = std::getenv("WWE2K16_UNTILE_COHERENCY");
    return !(v && v[0] == '0');
  }();
  if (!enabled || !source) {
    return;
  }
  // The tiled source spans whole 32x32 tiles: the size XGUntileSurface itself uses for its copy.
  uint64_t size = uint64_t((width + 31) & ~31u) * ((height + 31) & ~31u) * texel_pitch;
  if (!size || size > 64u * 1024 * 1024) {
    return;
  }
  auto* runtime = rex::Runtime::instance();
  auto* graphics = runtime ? static_cast<rex::graphics::GraphicsSystem*>(runtime->graphics_system())
                           : nullptr;
  if (!graphics || !graphics->command_processor()) {
    return;
  }
  bool ok = graphics->command_processor()->MakeGpuWritesVisibleToCpu(source, uint32_t(size));
  static int logged = 0;
  if (logged < 32) {
    ++logged;
    REXLOG_INFO("[wwe2k16] untile {:08X} {}x{} x{} B: GPU data {}", source, width, height, texel_pitch,
                ok ? "read back" : "not in GPU memory");
  }
}

}  // namespace

extern "C" PPC_FUNC(WWE2K16_IMP_UntileSurface);
extern "C" PPC_FUNC(WWE2K16_FN_UntileSurface) {
  MakeUntileSourceVisible(ctx.r6.u32, ctx.r7.u32, ctx.r8.u32, ctx.r10.u32);
  WWE2K16_IMP_UntileSurface(ctx, base);
}
