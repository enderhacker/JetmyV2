#include "visual.h"

namespace VisualPayloads {
void EffectSpatialPortalNetwork(float time, float) {
    float radius = std::max(1.0f, std::min(g_ScreenWidth, g_ScreenHeight) * 0.155f);
    float centers[2][2] = {
        {g_ScreenWidth * (0.28f + 0.09f * sinf(time * 0.43f)), g_ScreenHeight * (0.5f + 0.2f * cosf(time * 0.38f))},
        {g_ScreenWidth * (0.72f + 0.09f * cosf(time * 0.31f)), g_ScreenHeight * (0.5f + 0.2f * sinf(time * 0.47f))}
    };
    int step = RasterStep();
    for (int portal = 0; portal < 2; ++portal) {
        float cx = centers[portal][0], cy = centers[portal][1];
        float otherX = centers[1 - portal][0], otherY = centers[1 - portal][1];
        uint32_t rim = portal ? 0xFFA547 : 0x4DDCFF;
        float rotation = sinf(time * 0.3f) * 0.12f, c = cosf(rotation), s = sinf(rotation);
        for (int y = std::max(0, int(cy - radius - 4)); y < std::min(g_ScreenHeight, int(cy + radius + 5)); y += step) {
            for (int x = std::max(0, int(cx - radius - 4)); x < std::min(g_ScreenWidth, int(cx + radius + 5)); x += step) {
                float dx = x - cx, dy = y - cy, r = sqrtf(dx * dx + dy * dy);
                if (r > radius + 4) continue;
                uint32_t base = Sample(x, y);
                if (r < radius - 4) {
                    uint32_t remote = Sample(int(otherX - dx * c + dy * s), int(otherY + dx * s + dy * c));
                    Tile(x, y, step, BlendColor(base, remote, Smooth((radius - r) / 9)));
                } else {
                    float glow = std::max(0.0f, 1 - fabsf(r - radius) / 8);
                    Tile(x, y, step, BlendColor(base, rim, glow));
                }
            }
        }
        // Orbiting rim sparks sell the paired blue/orange gateway identity.
        for (int i = 0; i < 14; ++i) {
            float a = i * Tau / 14 + time * (portal ? -0.8f : 0.8f);
            Fill(int(cx + cosf(a) * radius) - 1, int(cy + sinf(a) * radius) - 1, 3, 3, 0xFFF5DE);
        }
    }
}
}
