#include "device_profile.h"

#include <rex/logging.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dxgi.h>
#endif

namespace wwe2k16 {

DeviceProfile DetectDeviceProfile() {
  DeviceProfile p;
#ifdef _WIN32
  int screen_height = GetSystemMetrics(SM_CYSCREEN);
  HMODULE dxgi = LoadLibraryW(L"dxgi.dll");
  if (dxgi) {
    using CreateFactory1 = HRESULT(WINAPI*)(REFIID, void**);
    auto create = reinterpret_cast<CreateFactory1>(GetProcAddress(dxgi, "CreateDXGIFactory1"));
    IDXGIFactory1* factory = nullptr;
    if (create && SUCCEEDED(create(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)))) {
      IDXGIAdapter1* adapter = nullptr;
      for (UINT i = 0; factory->EnumAdapters1(i, &adapter) == S_OK; ++i) {
        DXGI_ADAPTER_DESC1 d{};
        if (SUCCEEDED(adapter->GetDesc1(&d)) && !(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
          // Integrated GPUs report little dedicated memory and share the system's: count them with what they can use.
          const unsigned long long vram = d.DedicatedVideoMemory / (1024 * 1024);
          if (vram >= p.vram_mb) {
            p.vram_mb = vram;
            char name[128] = {};
            WideCharToMultiByte(CP_UTF8, 0, d.Description, -1, name, sizeof(name) - 1, nullptr, nullptr);
            p.adapter = name;
          }
        }
        adapter->Release();
      }
      factory->Release();
    }
    FreeLibrary(dxgi);
  }
  if (p.vram_mb) {
    // The game's native renderer is cheap (about 13 % of an RTX 5070 at 1440p), so only weak cards need a lower setting.
    if (p.vram_mb < 1536) {  // integrated / very old: 720p scaled up, light filtering
      p.resolution = screen_height > 1080 ? "1440" : "1080";
      p.upscale = true;
      p.anisotropy = 4;
      p.fxaa = 0;
    } else if (p.vram_mb < 3584) {  // entry level
      p.resolution = screen_height > 1440 ? "1440" : "1080";
      p.upscale = screen_height > 1080;
      p.anisotropy = 8;
      p.fxaa = 1;
    } else if (screen_height >= 2160 && p.vram_mb >= 7168) {
      p.resolution = "2160";
    } else if (screen_height >= 1440) {
      p.resolution = "1440";
    }
  }
  REXLOG_INFO("[wwe2k16] device profile: '{}' {} MB video memory, screen height {} -> {} {}, aniso {}x, fxaa {}",
              p.adapter, p.vram_mb, screen_height, p.resolution, p.upscale ? "(upscaled)" : "", p.anisotropy, p.fxaa);
#endif
  return p;
}

}  // namespace wwe2k16
