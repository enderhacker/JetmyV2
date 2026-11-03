#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectPixelSortingAnomaly(float time, float dt) {
    struct Band { int x, y, w, h, channel, slip; bool reverse; };
    static std::array<Band, 24> bands{};
    static std::vector<uint32_t> sorted;
    static float tick = 0;
    static int width = 0, height = 0;
    static size_t next = 0;
    const int w = g_ScreenWidth, h = g_ScreenHeight;
    if (w <= 0 || h <= 0) return;
    auto create = [](Band& b) {
        // Large overlapping blocks, but cap each sort even on very wide displays.
        int limit = std::min(g_ScreenWidth, 2048);
        b.w = FastRandInt(std::max(1, limit / 3), limit);
        b.h = FastRandInt(std::max(1, g_ScreenHeight / 24), std::max(1, g_ScreenHeight / 8));
        b.x = FastRandInt(0, g_ScreenWidth - b.w); b.y = FastRandInt(0, g_ScreenHeight - b.h);
        b.channel = FastRandInt(0, 2) * 8; b.reverse = FastRandInt(0, 1) != 0;
        b.slip = FastRandInt(1, std::max(1, b.w / 4));
    };
    if (time == 0 || width != w || height != h) {
        width = w; height = h; tick = 0; next = 0;
        sorted.resize(std::min(w, 2048));
        for (auto& band : bands) create(band);
    }
    tick += std::clamp(dt, 0.0f, 0.1f);
    while (tick >= 0.05f) {
        tick -= 0.05f;
        for (int i = 0; i < 4; ++i) create(bands[next++ % bands.size()]);
    }
    // Sort one scanline per band, then smear it with alternating torn offsets.
    // All reads for a band finish before writing it; overlapping bands compound.
    for (const auto& b : bands) {
        const auto* source = g_pFramePixels + static_cast<size_t>(b.y) * w + b.x;
        std::copy_n(source, b.w, sorted.begin());
        std::sort(sorted.begin(), sorted.begin() + b.w, [&b](uint32_t a, uint32_t c) {
            unsigned av = (a >> b.channel) & 255, cv = (c >> b.channel) & 255;
            return b.reverse ? av > cv : av < cv;
        });
        for (int y = 0; y < b.h; ++y) {
            auto* row = g_pFramePixels + static_cast<size_t>(b.y + y) * w + b.x;
            int offset = ((y / 3) & 1) ? b.slip : b.w - b.slip;
            for (int x = 0; x < b.w; ++x) row[x] = sorted[(x + offset) % b.w];
        }
    }
}
}
