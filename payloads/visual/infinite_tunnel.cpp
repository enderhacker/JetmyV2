#include "visual.h"

namespace VisualPayloads {
void EffectInfiniteTunnel(float time, float) {
    // Nested rectangular desktop copies, distinct from the cylindrical warp tunnel.
    constexpr float ratio = 0.76f;
    float phase = fmodf(time * 0.45f, 1.0f);
    // Oversized copies cover the viewport before their borders enter it. At a
    // phase wrap, each visible layer takes its neighbor's exact size/texture;
    // the only newly introduced layer is still entirely outside the screen.
    float scale = powf(ratio, phase - 2);
    int saved = SaveDC(g_hMemDC);
    if (!saved) return;
    SetStretchBltMode(g_hMemDC, COLORONCOLOR);
    float cx = g_ScreenWidth * (0.5f + 0.07f * sinf(time * 0.21f));
    float cy = g_ScreenHeight * (0.5f + 0.06f * cosf(time * 0.17f));
    for (int layer = -2; layer < 32; ++layer) {
        int w = std::max(1, int(g_ScreenWidth * scale));
        int h = std::max(1, int(g_ScreenHeight * scale));
        int x = int(cx - w * 0.5f), y = int(cy - h * 0.5f);
        StretchBlt(g_hMemDC, x, y, w, h, g_hScratchDC, 0, 0, g_ScreenWidth, g_ScreenHeight, SRCCOPY);
        GdiFlush();
        Line(x, y, x + w - 1, y, 0x9CAAB8);
        Line(x, y, x, y + h - 1, 0x9CAAB8);
        Line(x, y + h - 1, x + w - 1, y + h - 1, 0x17212C);
        Line(x + w - 1, y, x + w - 1, y + h - 1, 0x17212C);
        if (w <= 3 || h <= 3) break;
        scale *= ratio;
    }
    RestoreDC(g_hMemDC, saved);
}
} // namespace VisualPayloads
