#include "visual.h"

namespace VisualPayloads {
void EffectVoxelDesktopExplosion(float time, float) {
    int cols = std::min(10, g_ScreenWidth), rows = std::min(7, g_ScreenHeight);
    float phase = fmodf(time, 7.0f);
    float burst = Smooth(phase / 2.5f) * (1 - Smooth((phase - 4.5f) / 2.5f));
    int saved = SaveDC(g_hMemDC);
    if (!saved) return;
    SelectObject(g_hMemDC, GetStockObject(DC_BRUSH));
    SelectObject(g_hMemDC, GetStockObject(NULL_PEN));
    SetStretchBltMode(g_hMemDC, COLORONCOLOR);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int sx = c * g_ScreenWidth / cols, sy = r * g_ScreenHeight / rows;
            int sw = (c + 1) * g_ScreenWidth / cols - sx, sh = (r + 1) * g_ScreenHeight / rows - sy;
            float dx = c - (cols - 1) * 0.5f, dy = r - (rows - 1) * 0.5f;
            float depth = 0.7f + 0.3f * sinf(c * 2.1f + r * 3.7f);
            int x = sx + int(dx * burst * sw * 0.45f), y = sy + int(dy * burst * sh * 0.5f);
            int w = std::max(1, int(sw * (1 - burst * 0.22f))), h = std::max(1, int(sh * (1 - burst * 0.22f)));
            int extrude = int(std::min(sw, sh) * 0.3f * burst * depth);
            if (x + w + extrude < 0 || y + h + extrude < 0 || x >= g_ScreenWidth || y >= g_ScreenHeight) continue;
            POINT side[] = {{x + w,y},{x + w + extrude,y + extrude},{x + w + extrude,y + h + extrude},{x + w,y + h}};
            POINT bottom[] = {{x,y + h},{x + w,y + h},{x + w + extrude,y + h + extrude},{x + extrude,y + h + extrude}};
            SetDCBrushColor(g_hMemDC, RGB(27, 47, 67)); Polygon(g_hMemDC, side, 4);
            SetDCBrushColor(g_hMemDC, RGB(48, 79, 98)); Polygon(g_hMemDC, bottom, 4);
            StretchBlt(g_hMemDC, x, y, w, h, g_hScratchDC, sx, sy, sw, sh, SRCCOPY);
        }
    }
    RestoreDC(g_hMemDC, saved);
}
}
