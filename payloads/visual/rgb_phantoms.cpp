#include "visual.h"

namespace VisualPayloads {
void EffectRGBSpectralPhantoms(float time, float) {
    float amplitude = std::max(1.0f, std::min(g_ScreenWidth, g_ScreenHeight) * 0.026f);
    int rx = int(sinf(time * 0.83f) * amplitude), ry = int(cosf(time * 0.71f) * amplitude * 0.3f);
    int bx = int(cosf(time * 0.91f) * -amplitude), by = int(sinf(time * 0.67f) * amplitude * 0.3f);
    Runtime::ParallelFor(g_ScreenHeight, [&](int y) {
        int redY = std::clamp(y + ry, 0, g_ScreenHeight - 1), blueY = std::clamp(y + by, 0, g_ScreenHeight - 1);
        const auto* red = g_pScratchPixels + static_cast<size_t>(redY) * g_ScreenWidth;
        const auto* green = g_pScratchPixels + static_cast<size_t>(y) * g_ScreenWidth;
        const auto* blue = g_pScratchPixels + static_cast<size_t>(blueY) * g_ScreenWidth;
        // Row pointers and precomputed offsets keep this pure channel separation cheap.
        auto* output = g_pFramePixels + static_cast<size_t>(y) * g_ScreenWidth;
        for (int x = 0; x < g_ScreenWidth; ++x) {
            uint32_t color = (red[std::clamp(x + rx, 0, g_ScreenWidth - 1)] & 0xFF0000) |
                (green[x] & 0x00FF00) | (blue[std::clamp(x + bx, 0, g_ScreenWidth - 1)] & 0x0000FF);
            output[x] = color;
        }
    });
}
}
