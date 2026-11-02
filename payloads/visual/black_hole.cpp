#include "visual.h"

namespace VisualPayloads {
void EffectBlackHoleSingularities(float time, float) {
    float hx = g_ScreenWidth * (0.5f + 0.35f * sinf(time * 1.8f + 1.2f));
    float hy = g_ScreenHeight * (0.5f + 0.30f * cosf(time * 1.3f));
    float radius = 220.0f + 40.0f * sinf(time * 3.0f);
    float twist = 4.2f + sinf(time * 2.0f) * 1.5f;
    int minX = std::max(0, static_cast<int>(hx - radius));
    int maxX = std::min(g_ScreenWidth - 1, static_cast<int>(hx + radius));
    int minY = std::max(0, static_cast<int>(hy - radius));
    int maxY = std::min(g_ScreenHeight - 1, static_cast<int>(hy + radius));
    for (int y = minY; y <= maxY; y += 2) {
        for (int x = minX; x <= maxX; x += 2) {
            float dx = x - hx, dy = y - hy;
            float distSq = dx * dx + dy * dy;
            if (distSq < radius * radius) {
                float dist = sqrtf(distSq), factor = 1.0f - dist / radius;
                float a = atan2f(dy, dx) + factor * twist;
                float newDist = dist * (1.0f - factor * 0.55f);
                int sx = std::clamp(static_cast<int>(hx + cosf(a) * newDist), 0, g_ScreenWidth - 1);
                int sy = std::clamp(static_cast<int>(hy + sinf(a) * newDist), 0, g_ScreenHeight - 1);
                DrawBlock(x, y, g_pScratchPixels[sy * g_ScreenWidth + sx]);
            }
        }
    }
}
}
