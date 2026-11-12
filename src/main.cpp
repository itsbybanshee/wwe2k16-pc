// wwe2k16 - WWE 2K16 (Xbox 360) static recompilation with the ReXGlue SDK v0.10.

#if defined(_WIN32)
// Ask NVIDIA Optimus to expose and prefer the discrete GPU on hybrid laptops.
extern "C" __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
// Same for AMD switchable graphics.
extern "C" __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
#endif

#include "generated_v10/default/wwe2k16_init.h"

#include "wwe2k16_app.h"

REX_DEFINE_APP(wwe2k16, Wwe2k16App::Create)
