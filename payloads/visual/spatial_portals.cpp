#include "visual.h"

namespace VisualPayloads {
void EffectSpatialPortalNetwork(float time, float) {
    for (int i = 0; i < 3; ++i) {
        int radius = std::min({90 + static_cast<int>(25 * sinf(time * 2 + i)),
            (g_ScreenWidth - 1) / 2, (g_ScreenHeight - 1) / 2});
        if (radius <= 0) return;
        int cx = static_cast<int>(g_ScreenWidth * (0.5f + cosf(time * 1.5f + i * 2.1f) * 0.35f));
        int cy = static_cast<int>(g_ScreenHeight * (0.5f + sinf(time * 1.7f + i * 1.9f) * 0.3f));
        int srcX = (cx + 350 + i * 120) % (g_ScreenWidth - radius * 2);
        int srcY = (cy + 250 + i * 90) % (g_ScreenHeight - radius * 2);
        for (int y = std::max(0, cy - radius); y <= std::min(g_ScreenHeight - 1, cy + radius); ++y) {
            for (int x = std::max(0, cx - radius); x <= std::min(g_ScreenWidth - 1, cx + radius); ++x) {
                int dx = x - cx, dy = y - cy, dSq = dx * dx + dy * dy;
                if (dSq <= radius * radius) {
                    g_pFramePixels[y * g_ScreenWidth + x] = abs(dSq - radius * radius) < radius * 4
                        ? 0x0000E5FF : g_pScratchPixels[(srcY + dy + radius) * g_ScreenWidth + srcX + dx + radius];
                }
            }
        }
    }
}
}
