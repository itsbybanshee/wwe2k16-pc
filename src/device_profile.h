// wwe2k16 - graphics settings chosen from the PC's graphics card when the player did not pick any.
#pragma once

#include <string>

namespace wwe2k16 {

struct DeviceProfile {
  std::string resolution = "1080";  // launcher naming: 720 / 1080 / 1440 / 2160
  int anisotropy = 16;              // 0, 4, 8, 16
  int fxaa = 1;                     // 0 off, 1 on, 2 strong
  bool upscale = false;             // draw lower and upscale
  std::string adapter;              // name of the card the profile was chosen for
  unsigned long long vram_mb = 0;
};

// Looks at the DXGI adapters (the one with the most dedicated video memory is the one the game will use on a laptop with
// two GPUs) and the primary monitor's height. Never fails: without information the middle profile is returned.
DeviceProfile DetectDeviceProfile();

}  // namespace wwe2k16
