#include "visual.h"

namespace VisualPayloads {
void EffectRealityCollapseFinale(float time, float) {
    float cycle = fmodf(time, 4.0f), scale = std::max(0.03f, 1 - cycle / 4);
    int w = static_cast<int>(g_ScreenWidth * scale), h = static_cast<int>(g_ScreenHeight * scale);
    int x = (g_ScreenWidth - w) / 2 + FastRandInt(-12, 12), y = (g_ScreenHeight - h) / 2;
    SetStretchBltMode(g_hMemDC, COLORONCOLOR);
    StretchBlt(g_hMemDC, x, y, w, h, g_hScratchDC, 0, 0, g_ScreenWidth, g_ScreenHeight, SRCCOPY);
    GdiFlush();
    if (cycle > 2.5f) FastInvertRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y, w, h);
}
}
