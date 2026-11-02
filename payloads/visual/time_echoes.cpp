#include "visual.h"

namespace VisualPayloads {
void EffectDesktopTimeEchoes(float, float) {
    if (g_EchoHistory.empty()) return;
    const auto& old = g_EchoHistory[(g_EchoHead + MAX_ECHOES - 8) % MAX_ECHOES];
    for (int y = 0; y < g_ScreenHeight; y += 2) {
        int sy = y * old.h / g_ScreenHeight;
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            int sx = x * old.w / g_ScreenWidth;
            DrawBlock(x, y, BlendColor(g_pScratchPixels[y * g_ScreenWidth + x], old.pixels[sy * old.w + sx], 0.45f));
        }
    }
}
}
