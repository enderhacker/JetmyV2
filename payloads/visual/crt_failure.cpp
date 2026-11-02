#include "visual.h"

namespace VisualPayloads {
void EffectCRTFailureSimulator(float time, float) {
    static std::vector<float> vignette;
    if (vignette.size() != static_cast<size_t>(g_ScreenWidth)) {
        vignette.resize(g_ScreenWidth);
        for (int x = 0; x < g_ScreenWidth; ++x) {
            float nx = 2.0f * x / std::max(1, g_ScreenWidth - 1) - 1;
            vignette[x] = nx * nx * 0.18f;
        }
    }
    int step = RasterStep();
    float roll = fmodf(time * g_ScreenHeight * 0.13f, float(g_ScreenHeight));
    int shift = int(sinf(time * 0.7f) * g_ScreenHeight * 0.018f);
    for (int y = 0; y < g_ScreenHeight; ++y) {
        float band = std::max(0.0f, 1 - fabsf(y - roll) / std::max(1.0f, g_ScreenHeight * 0.06f));
        int jitter = int(band * band * 18 * sinf(time * 23));
        int sy = (y + shift + g_ScreenHeight) % g_ScreenHeight;
        float ny = 2.0f * y / std::max(1, g_ScreenHeight - 1) - 1;
        float darkness = (y % 3 == 0 ? 0.28f : 0.04f) + ny * ny * 0.12f;
        uint32_t noise = (FastRand() & 7) * 0x010101;
        auto* row = g_pFramePixels + static_cast<size_t>(y) * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; x += step) {
            uint32_t color = Sample(x + jitter, sy);
            color = BlendColor(color, noise, darkness + vignette[x]);
            if (band > 0.4f) color = BlendColor(color, 0xBBDACD, band * 0.15f);
            for (int dx = 0; dx < step && x + dx < g_ScreenWidth; ++dx) row[x + dx] = color;
        }
    }
}
}
