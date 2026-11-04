#include "visual.h"

namespace VisualPayloads {
void EffectChaosShader(float time, float) {
    // CPU shader: warp the incoming image and mix it with animated interference.
    int step = RasterStep();
    float t = fmodf(time, 1000.0f);
    static std::vector<float> waves;
    waves.resize(g_ScreenWidth);
    for (int x = 0; x < g_ScreenWidth; x += step)
        waves[x] = sinf(x * 0.021f - t * 1.7f);
    Runtime::ParallelFor((g_ScreenHeight + step - 1) / step, [&](int row) {
        int y = row * step;
        float waveY = sinf(y * 0.025f + t * 2.1f);
        for (int x = 0; x < g_ScreenWidth; x += step) {
            float waveX = waves[x];
            int sx = x + static_cast<int>(waveY * 24 + waveX * 8);
            int sy = y + static_cast<int>(waveX * 20 + waveY * 7);
            uint32_t source = Sample(sx, sy);
            float swirl = sinf((x + y) * 0.014f + waveX * 2 + waveY * 2 + t * 2.5f);
            float phase = (swirl + 1.0f) * 0.5f;
            uint32_t color = (static_cast<uint32_t>(phase * 255) << 16) |
                (static_cast<uint32_t>((waveX + 1) * 127.5f) << 8) |
                static_cast<uint32_t>((waveY + 1) * 127.5f);
            Tile(x, y, step, BlendColor(source, color, 0.48f));
        }
    });
}
} // namespace VisualPayloads
