#include "visual.h"

namespace VisualPayloads {
void EffectKaleidoscopicReality(float time, float) {
    float cx = g_ScreenWidth * (0.5f + cosf(time * 1.8f) * 0.22f);
    float cy = g_ScreenHeight * (0.5f + sinf(time * 1.6f) * 0.18f);
    for (int y = 0; y < g_ScreenHeight; y += 2) {
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            int dx = static_cast<int>(fabsf(x - cx)), dy = static_cast<int>(fabsf(y - cy));
            int sx = std::clamp(static_cast<int>(cx + std::max(dx, dy)), 0, g_ScreenWidth - 1);
            int sy = std::clamp(static_cast<int>(cy + std::min(dx, dy)), 0, g_ScreenHeight - 1);
            DrawBlock(x, y, g_pScratchPixels[sy * g_ScreenWidth + sx]);
        }
    }
}
}
