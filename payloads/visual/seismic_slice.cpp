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
    static std::vector<int> positiveX, negativeX;
    positiveX.resize(w); negativeX.resize(w);
    auto wrap = [](int value, int size) {
        if (value < 0) value += size;
        else if (value >= size) value -= size;
        return value;
    };
    for (int x = 0; x < w; ++x) {
        positiveX[x] = wrap(x - drift, w);
        negativeX[x] = wrap(x + drift, w);
    }
    Runtime::ParallelFor(h, [&](int y) {
        auto* row = g_pFramePixels + static_cast<size_t>(y) * w;
        const int sy0 = wrap(y - vertical, h), sy1 = wrap(y + vertical, h);
        const auto* source0 = g_pScratchPixels + static_cast<size_t>(sy0) * w;
        const auto* source1 = g_pScratchPixels + static_cast<size_t>(sy1) * w;
        const auto& x0 = ((sy0 / strip) & 1) ? negativeX : positiveX;
        const auto& x1 = ((sy1 / strip) & 1) ? negativeX : positiveX;
        for (int x = 0; x < w; ++x) {
            row[x] = (((x / 2) & 1) ? source1[x1[x]] : source0[x0[x]]) & 0x00FFFFFFu;
        }
    });
}
} // namespace VisualPayloads
