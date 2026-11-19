// wwe2k16 - screenshots (F12), see screenshot.cpp.
#pragma once

namespace wwe2k16 {

// Saves the game window's picture as a PNG in Documents\wwe2k16\Screenshots. `native_window_handle` is the HWND.
void TakeScreenshot(void* native_window_handle);

}  // namespace wwe2k16
