#include "visual.h"

namespace VisualPayloads {
void EffectHellShader(float time, float) {
    int step = RasterStep();
    float t = fmodf(time, 1000.0f);
    float flash = sinf(t * 17.0f) > 0 ? 1.0f : -1.0f;
    float cx = g_ScreenWidth * 0.5f, cy = g_ScreenHeight * 0.5f;
    for (int y = 0; y < g_ScreenHeight; y += step) {
        float dy = (y - cy) / std::max(1.0f, cy);
        float band = sinf(y * 0.055f + t * 9.0f);
        for (int x = 0; x < g_ScreenWidth; x += step) {
            float dx = (x - cx) / std::max(1.0f, cx);
            float radius = sqrtf(dx * dx + dy * dy);
            float angle = atan2f(dy, dx);
            float vortex = sinf(angle * 7.0f + radius * 24.0f - t * 8.0f);
            float ripple = sinf(radius * 42.0f - t * 11.0f + band * 2.0f);
            int sx = x + static_cast<int>(vortex * 35 + band * 20);
            int sy = y + static_cast<int>(ripple * 28);
            uint32_t source = Sample(sx, sy);
            uint32_t r = static_cast<uint32_t>((sinf(vortex * 4 + t * 13) + 1) * 127.5f);
            uint32_t g = static_cast<uint32_t>((sinf(ripple * 5 - t * 11 + 2) + 1) * 127.5f);
            uint32_t b = static_cast<uint32_t>((sinf((vortex + ripple) * 3 + t * 15 + flash) + 1) * 127.5f);
            uint32_t pattern = (r << 16) | (g << 8) | b;
            if (flash > 0) pattern ^= 0x00FFFFFFu;
            Tile(x, y, step, BlendColor(source ^ pattern, pattern, 0.55f));
        }
    }
}
} // namespace VisualPayloads
