#pragma once
#include <chrono>

namespace Runtime {
inline constexpr int StageSeconds = 10;
inline constexpr int VisualCount = 10;
inline constexpr int TotalSeconds = StageSeconds * VisualCount;

// Stage 0 is visual #1. Audio changes on visuals #1, #3, #5, #7, #9.
inline int StageAt(std::chrono::steady_clock::duration elapsed) {
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() / StageSeconds);
}
inline int SoundSelectionAt(int stage) { return stage / 2; }
}
