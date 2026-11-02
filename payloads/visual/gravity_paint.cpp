#include "visual.h"

namespace VisualPayloads {
namespace {
struct PaintDrop { float x, y, vy; uint32_t col; int thickness; };
std::vector<PaintDrop> paint;
}
void EffectGravityPaint(float, float dt) {
    if (paint.size() < 35) paint.push_back({FastRandFloat(0, float(g_ScreenWidth)), 0,
        FastRandFloat(120, 380), (FastRand() & 0x00FFFFFF) | 0x00550055, FastRandInt(1, 3)});
    for (auto& p : paint) {
        p.y += p.vy * dt;
        if (p.y > g_ScreenHeight - 5) { p.y = 0; p.x = FastRandFloat(0, float(g_ScreenWidth)); }
        FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight,
            static_cast<int>(p.x), static_cast<int>(p.y) - 23, p.thickness, 24, p.col);
    }
}
}
