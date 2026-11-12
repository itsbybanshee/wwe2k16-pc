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
#include <rex/system/kernel_state.h>

#ifdef WWE2K16_TU
#define WWE2K16_FN_UntileSurface sub_82C78698
#define WWE2K16_IMP_UntileSurface __imp__sub_82C78698
#else
#define WWE2K16_FN_UntileSurface sub_82C70510
#define WWE2K16_IMP_UntileSurface __imp__sub_82C70510
#endif

namespace {

uint64_t HashGuestRange(uint32_t address, uint64_t length) {
  constexpr uint64_t kMaxHash = 64u * 1024 * 1024;
  if (!address || !length || length > kMaxHash) {
    return 0;
  }
  auto* kernel_state = REX_KERNEL_STATE();
  auto* memory = kernel_state ? kernel_state->memory() : nullptr;
  if (!memory || !memory->LookupHeap(address)) {
    return 0;
  }
  const auto* data = memory->TranslateVirtual<const uint8_t*>(address);
  uint64_t hash = 14695981039346656037ull;
  for (uint64_t i = 0; i < length; ++i) {
    hash ^= data[i];
    hash *= 1099511628211ull;
  }
  return hash;
}

bool MakeUntileSourceVisible(uint32_t source, uint32_t width, uint32_t height,
                             uint32_t texel_pitch) {
  static const bool enabled = [] {
    const char* v = std::getenv("WWE2K16_UNTILE_COHERENCY");
    return !(v && v[0] == '0');
  }();
  if (!enabled || !source) {
    return false;
  }
  // The tiled source spans whole 32x32 tiles: the size XGUntileSurface itself uses for its copy.
  uint64_t size = uint64_t((width + 31) & ~31u) * ((height + 31) & ~31u) * texel_pitch;
  if (!size || size > 64u * 1024 * 1024) {
    return false;
  }
  auto* runtime = rex::Runtime::instance();
  auto* graphics = runtime ? static_cast<rex::graphics::GraphicsSystem*>(runtime->graphics_system())
                           : nullptr;
  if (!graphics || !graphics->command_processor()) {
    return false;
  }
  bool ok = graphics->command_processor()->MakeGpuWritesVisibleToCpu(source, uint32_t(size));
  return ok;
}

}  // namespace

extern "C" PPC_FUNC(WWE2K16_IMP_UntileSurface);
extern "C" PPC_FUNC(WWE2K16_FN_UntileSurface) {
  const uint32_t dest = ctx.r3.u32;
  const uint32_t row_pitch = ctx.r4.u32;
  const uint32_t point = ctx.r5.u32;
  const uint32_t source = ctx.r6.u32;
  const uint32_t width = ctx.r7.u32;
  const uint32_t height = ctx.r8.u32;
  const uint32_t rect = ctx.r9.u32;
  const uint32_t texel_pitch = ctx.r10.u32;
  const uint64_t source_size = uint64_t((width + 31) & ~31u) *
                               ((height + 31) & ~31u) * texel_pitch;
  const bool readback_ok = MakeUntileSourceVisible(source, width, height, texel_pitch);
  const uint64_t source_hash = HashGuestRange(source, source_size);
  WWE2K16_IMP_UntileSurface(ctx, base);
  const uint64_t dest_size = uint64_t(row_pitch) * height;
  const uint64_t dest_hash = HashGuestRange(dest, dest_size);
  static int logged = 0;
  if (logged < 32) {
    ++logged;
    REXLOG_INFO(
        "[wwe2k16] untile dest={:08X} row_pitch={} point={:08X} rect={:08X} source={:08X} "
        "{}x{} texel_pitch={} readback={} src_hash={:016X} dest_hash={:016X}",
        dest, row_pitch, point, rect, source, width, height, texel_pitch,
        readback_ok ? "ok" : "failed", source_hash, dest_hash);
  }
}
