#pragma once
#include <chrono>

namespace Runtime {
inline constexpr int StageSeconds = 10;
inline constexpr int VisualCount = 10;
inline constexpr int TotalSeconds = StageSeconds * VisualCount;

// Stage 0: one visual, silent. Stage 1: add a visual, sound. Repeat.
inline int StageAt(std::chrono::steady_clock::duration elapsed) {
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count() / StageSeconds);
}
inline bool HasSound(int stage) { return stage >= 0 && stage < VisualCount && stage % 2 == 1; }
}
