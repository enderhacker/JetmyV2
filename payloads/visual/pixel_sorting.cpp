#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectPixelSortingAnomaly(float time, float dt) {
    struct Band { int x, y, w, h, channel; bool reverse; };
    static std::array<Band, 9> bands{};
    static float tick = 1;
    static bool initialized = false;
    static size_t next = 0;
    if (time == 0) { initialized = false; tick = 1; next = 0; }
    auto create = [](Band& b) {
        b.w = std::min(g_ScreenWidth, FastRandInt(90, 360));
        b.h = std::min(g_ScreenHeight, FastRandInt(5, 18));
        b.x = FastRandInt(0, g_ScreenWidth - b.w); b.y = FastRandInt(0, g_ScreenHeight - b.h);
        b.channel = FastRandInt(0, 2) * 8; b.reverse = FastRandInt(0, 1) != 0;
    };
    if (!initialized) { for (auto& band : bands) create(band); initialized = true; }
    tick += dt;
    if (tick >= 0.2f) { tick = 0; create(bands[next++ % bands.size()]); }
    // Sort one representative scanline per band, then copy it. Work is capped
    // at nine sorts of <=360 elements, rather than sorting every screen row.
    for (const auto& b : bands) {
        auto* row = g_pFramePixels + static_cast<size_t>(b.y) * g_ScreenWidth + b.x;
        std::sort(row, row + b.w, [&b](uint32_t a, uint32_t c) {
            unsigned av = (a >> b.channel) & 255, cv = (c >> b.channel) & 255;
            return b.reverse ? av > cv : av < cv;
        });
        for (int y = 1; y < b.h; ++y)
            memcpy(row + static_cast<size_t>(y) * g_ScreenWidth, row, b.w * sizeof(uint32_t));
        Fill(b.x, b.y, 2, b.h, 0xFFE3B0);
    }
}
}
