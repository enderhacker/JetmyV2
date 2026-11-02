#include "visual.h"

namespace VisualPayloads {
void EffectBlackHoleSingularities(float time, float) {
    float radius = std::max(1.0f, std::min(g_ScreenWidth, g_ScreenHeight) * 0.28f);
    float hx = g_ScreenWidth * (0.5f + 0.22f * sinf(time * 0.37f));
    float hy = g_ScreenHeight * (0.5f + 0.18f * cosf(time * 0.29f));
    int step = RasterStep();
    int left = std::max(0, int(hx - radius)), top = std::max(0, int(hy - radius));
    int right = std::min(g_ScreenWidth, int(hx + radius + 1));
    int bottom = std::min(g_ScreenHeight, int(hy + radius + 1));
    for (int y = top; y < bottom; y += step) {
        for (int x = left; x < right; x += step) {
            float dx = (x - hx) / radius, dy = (y - hy) / radius;
            float r2 = dx * dx + dy * dy;
            if (r2 >= 1) continue;
            float r = sqrtf(r2), edge = Smooth((1 - r) * 5);
            float angle = atan2f(dy, dx);
            float twist = 2.8f * (1 - r) * (1 - r) + time * 0.24f;
            float c = cosf(twist), s = sinf(twist), pull = 1 - edge * 0.25f;
            uint32_t warped = Sample(int(hx + (dx * c - dy * s) * radius * pull),
                int(hy + (dx * s + dy * c) * radius * pull));
            float ring = std::max(0.0f, 1 - fabsf(r - 0.31f) * 22);
            uint32_t hot = BlendColor(0xD84919, 0xFFF0AB, 0.5f + 0.5f * sinf(angle * 3 - time * 2));
            uint32_t color = BlendColor(warped, 0x080712, Smooth((0.25f - r) * 30));
            color = BlendColor(color, hot, ring * 0.95f);
            Tile(x, y, step, BlendColor(Sample(x, y), color, edge));
        }
    }
}
}
