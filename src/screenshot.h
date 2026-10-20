#pragma once

namespace rex {
class Runtime;
}

namespace wwe2k16 {
// Saves the game's output image at its internal render resolution to Documents\wwe2k16\Screenshots (PNG).
void TakeScreenshot(rex::Runtime* runtime);
}  // namespace wwe2k16
