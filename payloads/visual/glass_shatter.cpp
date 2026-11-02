#include "visual.h"

namespace VisualPayloads {
namespace {
struct Shard { int sx, sy, w, h; float vx, vy; };
std::vector<Shard> shards;
}
void EffectDesktopGlassShatter(float time, float dt) {
    if (shards.empty() || fmodf(time, 4.0f) < dt) {
        shards.clear();
        int cols = 9, rows = 7;
        int sw = g_ScreenWidth / cols, sh = g_ScreenHeight / rows;
        for (int r = 0; r < rows; ++r)
            for (int c = 0; c < cols; ++c)
                shards.push_back({c * sw, r * sh, sw, sh, FastRandFloat(-120, 120), FastRandFloat(40, 280)});
    }
    float cycle = fmodf(time, 4.0f);
    for (auto& s : shards) {
        int dx = std::clamp(static_cast<int>(s.sx + s.vx * cycle), 0, g_ScreenWidth - s.w);
        int dy = std::clamp(static_cast<int>(s.sy + s.vy * cycle * cycle), 0, g_ScreenHeight - s.h);
        BitBlt(g_hMemDC, dx, dy, s.w - 4, s.h - 4, g_hScratchDC, s.sx, s.sy, SRCCOPY);
    }
}
}
