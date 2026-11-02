#include "visual.h"
#include <array>

namespace VisualPayloads {
void EffectChaosRectangles(float, float dt) {
    struct Panel { int x, y, w, h; uint32_t color; bool solid; };
    static std::array<Panel, 14> panels{};
    static float tick = 1;
    static size_t next = 0;
    static bool initialized = false;
    constexpr uint32_t palette[] = {0xFF583D, 0xFFD84A, 0x3C75FF, 0xF2F0E5, 0x101827};
    auto create = [](Panel& p) {
        int cell = std::max(8, std::min(g_ScreenWidth, g_ScreenHeight) / 20);
        p.x = FastRandInt(0, std::max(0, g_ScreenWidth / cell - 1)) * cell;
        p.y = FastRandInt(0, std::max(0, g_ScreenHeight / cell - 1)) * cell;
        p.w = std::min(g_ScreenWidth - p.x, cell * FastRandInt(1, 5));
        p.h = std::min(g_ScreenHeight - p.y, cell * FastRandInt(1, 4));
        p.solid = FastRandInt(0, 3) == 0;
    };
    if (!initialized) {
        for (size_t i = 0; i < panels.size(); ++i) { create(panels[i]); panels[i].color = palette[i % 5]; }
        initialized = true;
    }
    tick += dt;
    if (tick >= 0.16f) {
        tick = 0;
        auto& p = panels[next++ % panels.size()];
        create(p); p.color = palette[FastRandInt(0, 4)];
    }
    // Stable architectural outlines; only one panel changes per tick, no full-screen strobe.
    for (const auto& p : panels) {
        if (p.solid) Tint(p.x, p.y, p.w, p.h, p.color, 0.72f);
        Fill(p.x, p.y, p.w, 2, p.color); Fill(p.x, p.y + p.h - 2, p.w, 2, p.color);
        Fill(p.x, p.y, 2, p.h, p.color); Fill(p.x + p.w - 2, p.y, 2, p.h, p.color);
    }
}
}
