// wwe2k16 - screenshots (F12) for the native renderer build.
//
// Captures the game window's client area (what is on the screen, at the window's resolution) and saves it as PNG in
// Documents\wwe2k16\Screenshots. Banshee's v0.7.3 version read the Xenos presenter's output at the internal render
// resolution (src_v07/screenshot.cpp); the native renderer has no such readback yet, so a 4K render shown in a 1080p window
// is saved at 1080p here. PNG encoding (Windows Imaging Component) runs on a worker thread.

#include "screenshot.h"

#include <cstdint>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <rex/filesystem.h>
#include <rex/logging.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <shlobj.h>
#include <wincodec.h>
#endif

namespace wwe2k16 {
namespace {

#ifdef _WIN32

struct Image {
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t stride = 0;
  std::vector<uint8_t> bgra;
};

std::filesystem::path ScreenshotFolder() {
  PWSTR documents = nullptr;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &documents))) {
    std::filesystem::path folder = std::filesystem::path(documents) / L"wwe2k16" / L"Screenshots";
    CoTaskMemFree(documents);
    return folder;
  }
  return std::filesystem::current_path() / "Screenshots";
}

std::filesystem::path NextScreenshotPath(const std::filesystem::path& folder) {
  const std::time_t now = std::time(nullptr);
  std::tm local{};
  localtime_s(&local, &now);
  char stamp[32];
  std::strftime(stamp, sizeof(stamp), "%Y-%m-%d_%H-%M-%S", &local);
  std::filesystem::path path = folder / (std::string("WWE2K16_") + stamp + ".png");
  // Several shots within the same second get _2, _3...
  for (int n = 2; std::filesystem::exists(path); ++n) {
    path = folder / (std::string("WWE2K16_") + stamp + "_" + std::to_string(n) + ".png");
  }
  return path;
}

// PrintWindow with PW_RENDERFULLCONTENT also captures windows drawn by Vulkan / DirectComposition.
bool CaptureWindow(HWND hwnd, Image& out) {
  RECT client{};
  if (!hwnd || !GetClientRect(hwnd, &client)) return false;
  const int w = client.right - client.left;
  const int h = client.bottom - client.top;
  if (w <= 0 || h <= 0) return false;
  HDC screen = GetDC(nullptr);
  HDC dc = CreateCompatibleDC(screen);
  BITMAPINFO info{};
  info.bmiHeader.biSize = sizeof(info.bmiHeader);
  info.bmiHeader.biWidth = w;
  info.bmiHeader.biHeight = -h;  // top-down
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;
  void* bits = nullptr;
  HBITMAP dib = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
  bool ok = false;
  if (dib && bits) {
    HGDIOBJ old = SelectObject(dc, dib);
    // PrintWindow draws the whole window; the client area is what we keep (title bar and borders are cut off below).
    RECT window{};
    GetWindowRect(hwnd, &window);
    const int full_w = window.right - window.left;
    const int full_h = window.bottom - window.top;
    HDC full_dc = CreateCompatibleDC(screen);
    BITMAPINFO full_info = info;
    full_info.bmiHeader.biWidth = full_w;
    full_info.bmiHeader.biHeight = -full_h;
    void* full_bits = nullptr;
    HBITMAP full_dib = CreateDIBSection(screen, &full_info, DIB_RGB_COLORS, &full_bits, nullptr, 0);
    if (full_dib && full_bits) {
      HGDIOBJ full_old = SelectObject(full_dc, full_dib);
      if (PrintWindow(hwnd, full_dc, 2 /* PW_RENDERFULLCONTENT */)) {
        POINT origin{0, 0};
        ClientToScreen(hwnd, &origin);
        const int dx = origin.x - window.left;
        const int dy = origin.y - window.top;
        out.width = uint32_t(w);
        out.height = uint32_t(h);
        out.stride = uint32_t(w) * 4;
        out.bgra.resize(size_t(out.stride) * h);
        for (int y = 0; y < h; ++y) {
          const int sy = y + dy;
          if (sy < 0 || sy >= full_h) continue;
          const uint8_t* src = static_cast<const uint8_t*>(full_bits) + size_t(sy) * full_w * 4 + size_t(dx) * 4;
          std::memcpy(out.bgra.data() + size_t(y) * out.stride, src, size_t(w) * 4);
        }
        ok = true;
      }
      SelectObject(full_dc, full_old);
      DeleteObject(full_dib);
    }
    DeleteDC(full_dc);
    SelectObject(dc, old);
  }
  if (dib) DeleteObject(dib);
  DeleteDC(dc);
  ReleaseDC(nullptr, screen);
  return ok;
}

bool WritePng(const std::filesystem::path& path, Image& image) {
  for (uint32_t y = 0; y < image.height; ++y) {
    uint8_t* row = image.bgra.data() + size_t(y) * image.stride;
    for (uint32_t x = 0; x < image.width; ++x) row[x * 4 + 3] = 0xFF;
  }
  const HRESULT co = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  bool ok = false;
  IWICImagingFactory* factory = nullptr;
  IWICStream* stream = nullptr;
  IWICBitmapEncoder* encoder = nullptr;
  IWICBitmapFrameEncode* frame = nullptr;
  IWICBitmap* bitmap = nullptr;
  do {
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)))) break;
    if (FAILED(factory->CreateBitmapFromMemory(image.width, image.height, GUID_WICPixelFormat32bppBGR, image.stride,
                                               UINT(image.bgra.size()), image.bgra.data(), &bitmap)))
      break;
    if (FAILED(factory->CreateStream(&stream)) || FAILED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE)))
      break;
    if (FAILED(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder)) ||
        FAILED(encoder->Initialize(stream, WICBitmapEncoderNoCache)))
      break;
    if (FAILED(encoder->CreateNewFrame(&frame, nullptr)) || FAILED(frame->Initialize(nullptr))) break;
    WICPixelFormatGUID format = GUID_WICPixelFormat24bppBGR;
    if (FAILED(frame->SetSize(image.width, image.height)) || FAILED(frame->SetPixelFormat(&format))) break;
    if (FAILED(frame->WriteSource(bitmap, nullptr)) || FAILED(frame->Commit()) || FAILED(encoder->Commit())) break;
    ok = true;
  } while (false);
  if (frame) frame->Release();
  if (encoder) encoder->Release();
  if (stream) stream->Release();
  if (bitmap) bitmap->Release();
  if (factory) factory->Release();
  if (SUCCEEDED(co)) CoUninitialize();
  return ok;
}

#endif  // _WIN32

}  // namespace

void TakeScreenshot(void* native_window_handle) {
#ifdef _WIN32
  auto image = std::make_shared<Image>();
  if (!CaptureWindow(static_cast<HWND>(native_window_handle), *image)) {
    REXLOG_WARN("[wwe2k16-screenshot] could not capture the window");
    return;
  }
  std::thread([image] {
    const std::filesystem::path folder = ScreenshotFolder();
    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    const std::filesystem::path path = NextScreenshotPath(folder);
    if (WritePng(path, *image)) {
      REXLOG_INFO("[wwe2k16-screenshot] saved {}x{} to {}", image->width, image->height, rex::path_to_utf8(path));
    } else {
      REXLOG_ERROR("[wwe2k16-screenshot] could not write {}", rex::path_to_utf8(path));
    }
  }).detach();
#else
  (void)native_window_handle;
  REXLOG_WARN("[wwe2k16-screenshot] PNG saving is only implemented on Windows");
#endif
}

}  // namespace wwe2k16
