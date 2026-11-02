#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectGeometricImplosion(float time, float) {
    // Flat, nested guilloche polygons: no particle physics, image warping, or 3D boxes.
    constexpr int sides = 9, rings = 18;
    float cx = g_ScreenWidth * (0.5f + 0.12f * sinf(time * 0.23f));
    float cy = g_ScreenHeight * (0.5f + 0.12f * cosf(time * 0.19f));
    float maximum = std::min(g_ScreenWidth, g_ScreenHeight) * 0.62f;
    std::array<POINT, sides> previous{};
    for (int ring = 0; ring < rings; ++ring) {
        float phase = fmodf(float(ring) / rings + time * 0.09f, 1.0f);
        float radius = maximum * (1 - phase) * (1 - phase);
        float rotation = time * 0.18f + phase * 2.0f;
        uint32_t color = BlendColor(0x7244B6, 0xFFAE9A, phase);
        std::array<POINT, sides> points{};
        for (int i = 0; i < sides; ++i) {
            float a = i * Tau / sides + rotation;
            points[i] = {LONG(cx + cosf(a) * radius), LONG(cy + sinf(a) * radius)};
        }
        for (int i = 0; i < sides; ++i) {
            const auto& a = points[i]; const auto& b = points[(i + 1) % sides];
            Line(a.x, a.y, b.x, b.y, color);
            if (ring > 0 && ring % 3 == 0)
                Line(a.x, a.y, previous[i].x, previous[i].y, BlendColor(0x181329, color, 0.55f));
        }
        previous = points;
    }
    Fill(int(cx) - 1, int(cy) - 1, 3, 3, 0xFFF1DC);
}
}
