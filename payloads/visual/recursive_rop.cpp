#include "visual.h"

namespace VisualPayloads {
void EffectRecursiveRopCrucible(float time, float dt) {
    static std::vector<uint32_t> previous;
    static int width = 0, height = 0;
    const int w = g_ScreenWidth, h = g_ScreenHeight;
    if (w <= 0 || h <= 0) return;
    const size_t count = static_cast<size_t>(w) * h;
    if (time == 0 || width != w || height != h || previous.size() != count) {
        previous.assign(g_pScratchPixels, g_pScratchPixels + count);
        width = w; height = h;
    }
    // Approximately 101% per nominal frame, independent of actual frame rate.
    const float scale = powf(1.01f + 0.01f * (0.5f + 0.5f * sinf(time * 0.7f)),
                             std::clamp(dt * 60.0f, 0.0f, 4.0f));
    const float cx = (w - 1) * 0.5f, cy = (h - 1) * 0.5f;
    const bool difference = static_cast<int>(time * 0.8f) & 1;
    const float sweep = fmodf(time, 6.0f);
    std::vector<int> sourceX(w);
    for (int x = 0; x < w; ++x) sourceX[x] = std::clamp(static_cast<int>(cx + (x - cx) / scale), 0, w - 1);
    for (int y = 0; y < h; ++y) {
        int sy = std::clamp(static_cast<int>(cy + (y - cy) / scale), 0, h - 1);
        const auto* feedback = previous.data() + static_cast<size_t>(sy) * w;
        for (int x = 0; x < w; ++x) {
            size_t i = static_cast<size_t>(y) * w + x;
            uint32_t live = g_pScratchPixels[i], old = feedback[sourceX[x]];
            uint32_t color = (live ^ old) & 0x00FFFFFFu; // SRCINVERT, entirely in memory.
            if (difference) {
                color = 0;
                for (int shift = 0; shift <= 16; shift += 8) {
                    int a = (live >> shift) & 255, b = (old >> shift) & 255;
                    color |= static_cast<uint32_t>(std::min(255, abs(a - b) + b / 3)) << shift;
                }
            }
            // A moving reset band replenishes desktop detail rather than saturating forever.
            if (sweep < 0.8f && fabsf(x - (sweep / 0.8f) * w) < std::max(2.0f, w * 0.06f))
                color = live & 0x00FFFFFFu;
            g_pFramePixels[i] = color;
        }
    }
    // Never read feedback from a buffer being modified in this pass.
    std::copy_n(g_pFramePixels, count, previous.data());
}
} // namespace VisualPayloads
