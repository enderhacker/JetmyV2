#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectNeonFractalInvasion(float time, float) {
    static const auto palette = [] {
        std::array<uint32_t, 25> colors{};
        for (size_t i = 0; i < colors.size(); ++i) {
            float t = float(i) / (colors.size() - 1);
            colors[i] = t < 0.5f ? BlendColor(0x101A3F, 0x38DFD5, t * 2)
                : BlendColor(0x38DFD5, 0xF490C8, (t - 0.5f) * 2);
        }
        return colors;
    }();
    float cr = -0.76f + sinf(time * 0.13f) * 0.05f;
    float ci = 0.19f + cosf(time * 0.17f) * 0.06f;
    float scale = 2.8f / std::max(1, std::min(g_ScreenWidth, g_ScreenHeight));
    float c = cosf(time * 0.06f), s = sinf(time * 0.06f);
    int step = RasterStep() * 2; // Julia iterations have a separate, lower sample budget.
    for (int y = 0; y < g_ScreenHeight; y += step) {
        float dy = (y - g_ScreenHeight * 0.5f) * scale;
        for (int x = 0; x < g_ScreenWidth; x += step) {
            float dx = (x - g_ScreenWidth * 0.5f) * scale;
            float zx = dx * c - dy * s, zy = dx * s + dy * c;
            int iteration = 0;
            while (zx * zx + zy * zy < 16 && iteration < 24) {
                float next = zx * zx - zy * zy + cr;
                zy = 2 * zx * zy + ci; zx = next; ++iteration;
            }
            if (iteration == 24) continue; // Keep the interior of the set black.
            float density = Smooth(iteration / 12.0f);
            Tile(x, y, step, BlendColor(0x000000, palette[iteration], 0.2f + density * 0.6f));
        }
    }
}
}
