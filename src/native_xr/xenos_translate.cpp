// WWE 2K16 native renderer: XenosRecomp's translator behind a plain interface. Compiled only into
// the wwe2k16_xenosrecomp library (cmake/native_renderer.cmake), with XenosRecomp's own pch.h,
// whose types (be<T>, ...) clash with the SDK's.

#include "shader_recompiler.h"

#include "xenos_translate.h"

bool Wwe2k16TranslateShader(const uint8_t *container, std::string_view prelude, std::string &hlsl,
                            bool &pixel, uint32_t &spec_constants_mask) {
  ShaderRecompiler recompiler;
  recompiler.recompile(container, prelude);
  hlsl = std::move(recompiler.out);
  pixel = recompiler.isPixelShader;
  spec_constants_mask = recompiler.specConstantsMask;
  return !hlsl.empty();
}
