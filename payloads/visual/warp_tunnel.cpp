#include "visual.h"

namespace VisualPayloads {
void Effect3DWarpTunnel(float time, float) {
    float cx = g_ScreenWidth * (0.5f + cosf(time * 2) * 0.18f);
    float cy = g_ScreenHeight * (0.5f + sinf(time * 1.5f) * 0.18f);
    for (int y = 0; y < g_ScreenHeight; y += 2) {
        for (int x = 0; x < g_ScreenWidth; x += 2) {
            float dx = x - cx, dy = y - cy;
            float r = sqrtf(dx * dx + dy * dy) + 0.1f, a = atan2f(dy, dx);
            int u = static_cast<int>(fabsf(a * g_ScreenWidth / 3.14159f)) % g_ScreenWidth;
            int v = static_cast<int>(fabsf(2600 / r + time * 700)) % g_ScreenHeight;
            DrawBlock(x, y, g_pScratchPixels[v * g_ScreenWidth + u]);
        }
    }
}
}
