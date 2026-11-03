#include "visual.h"

namespace VisualPayloads {
void EffectSeismicSliceAndDrift(float time, float) {
    const int w = g_ScreenWidth, h = g_ScreenHeight;
    if (w <= 0 || h <= 0) return;
    const int strip = 2 + static_cast<int>(time * 9.0f) % 7;
    const float phase = fmodf(time, 1000.0f) * Tau * 11.0f;
    const int drift = static_cast<int>(std::min(160.0f, w * 0.08f) * sinf(phase));
    // Counter-phase rows first, then two-pixel columns in the second half-cycle.
    const bool woven = fmodf(time, 2.0f) >= 1.0f;
    const int vertical = woven ? static_cast<int>(std::min(100.0f, h * 0.07f) * sinf(phase + Pi * 0.5f)) : 0;
    std::vector<int> columns(w);
    for (int x = 0; x < w; ++x) columns[x] = ((x / 2) & 1) ? -vertical : vertical;
    for (int y = 0; y < h; ++y) {
        auto* row = g_pFramePixels + static_cast<size_t>(y) * w;
        for (int x = 0; x < w; ++x) {
            int sy = ((y - columns[x]) % h + h) % h;
            int offset = ((sy / strip) & 1) ? -drift : drift;
            int sx = ((x - offset) % w + w) % w;
            row[x] = g_pScratchPixels[static_cast<size_t>(sy) * w + sx] & 0x00FFFFFFu;
        }
    }
}
} // namespace VisualPayloads
