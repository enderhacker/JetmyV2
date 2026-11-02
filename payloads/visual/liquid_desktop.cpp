#include "visual.h"

namespace VisualPayloads {
void EffectLiquidDesktop(float time, float) {
    float freqX = 0.024f + 0.006f * sinf(time * 0.7f);
    float freqY = 0.028f + 0.005f * cosf(time * 0.9f);
    std::vector<int> xShifts(g_ScreenHeight), yShifts(g_ScreenWidth);
    for (int y = 0; y < g_ScreenHeight; ++y)
        xShifts[y] = static_cast<int>(sinf(y * freqX + time * 6.5f) * 18.0f);
    for (int x = 0; x < g_ScreenWidth; ++x)
        yShifts[x] = static_cast<int>(cosf(x * freqY + time * 6.5f) * 14.0f);
    for (int y = 0; y < g_ScreenHeight; y += 2) {
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            int sx = std::clamp(x + xShifts[y], 0, g_ScreenWidth - 1);
            int sy = std::clamp(y + yShifts[x], 0, g_ScreenHeight - 1);
            DrawBlock(x, y, g_pScratchPixels[sy * g_ScreenWidth + sx]);
        }
    }
}
}
