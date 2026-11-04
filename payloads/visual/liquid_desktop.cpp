#include "visual.h"

namespace VisualPayloads {
void EffectLiquidDesktop(float time, float) {
    static std::vector<float> horizontal, vertical;
    horizontal.resize(g_ScreenHeight); vertical.resize(g_ScreenWidth);
    int step = RasterStep();
    float amplitude = std::max(1.0f, std::min(g_ScreenWidth, g_ScreenHeight) * 0.018f);
    // Two separable wave fields: trigonometry is O(width + height), not O(pixels).
    for (int y = 0; y < g_ScreenHeight; y += step)
        horizontal[y] = amplitude * (sinf(y * 0.018f + time * 1.7f) + 0.4f * sinf(y * 0.041f - time));
    for (int x = 0; x < g_ScreenWidth; x += step)
        vertical[x] = amplitude * 0.7f * sinf(x * 0.016f - time * 1.4f);
    Runtime::ParallelFor((g_ScreenHeight + step - 1) / step, [&](int row) {
        int y = row * step;
        for (int x = 0; x < g_ScreenWidth; x += step) {
            uint32_t color = Sample(int(x + horizontal[y]), int(y + vertical[x]));
            float caustic = std::max(0.0f, (horizontal[y] + vertical[x]) / (amplitude * 2.2f));
            Tile(x, y, step, BlendColor(color, 0xA1E8EF, caustic * caustic * 0.22f));
        }
    });
}
}
