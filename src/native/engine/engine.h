// SvR 2008 stand-in for re:Blue's engine/engine.h.
//
// re:Blue's renderer reads a few Blue Dragon engine values (movie playback, sparse frames, the FPS
// limit setting). SvR's equivalents are wired here as they're found; until then these keep the
// renderer's default behaviour.

#pragma once

int Wwe2k16FrameRateCap();  // src/framerate_hook.cpp: the engine's current frame rate

namespace bd::engine {

// Blue Dragon skips presenting some frames while its frame interpolation runs; SvR presents every
// frame the game swaps.
inline bool SparseFrame() { return false; }

// Sofdec movie playback (Blue Dragon letterboxes and unlocks FPS during movies). SvR plays Bink
// movies; detection can be added when the renderer needs it.
struct SofdecPlayer {
  static bool Playing() { return false; }
};

struct Settings {
  static Settings& Get() {
    static Settings settings;
    return settings;
  }
  // The game advances one engine step per frame and its own vblank pacing no longer gates presents
  // (the null GPU skips swaps), so presents are capped at the engine's frame rate: 60, or 30 in
  // entrances when the launcher's "60 FPS Cinematics" is off (src/framerate_hook.cpp).
  int FPSLimit() const { return ::Wwe2k16FrameRateCap(); }
};

}  // namespace bd::engine
