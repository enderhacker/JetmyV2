#include "visual.h"

namespace VisualPayloads {
void EffectVoxelDesktopExplosion(float time, float) {
    struct Flight { float dx, dy, depth; };
    static std::vector<Flight> flights;
    static int lastCycle = -1, width = 0, height = 0;
    int cols = std::min(10, g_ScreenWidth), rows = std::min(7, g_ScreenHeight);
    int cycle = int(time / 7.0f);
    float phase = fmodf(time, 7.0f);
    if (time == 0 || cycle != lastCycle || width != g_ScreenWidth || height != g_ScreenHeight) {
        lastCycle = cycle; width = g_ScreenWidth; height = g_ScreenHeight;
        flights.resize(static_cast<size_t>(cols) * rows);
        float direction = FastRandFloat(0, Tau);
        float driftX = cosf(direction) * 0.24f, driftY = sinf(direction) * 0.24f;
        float impactX = FastRandFloat(0.2f, 0.8f), impactY = FastRandFloat(0.2f, 0.8f);
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float angle = atan2f((r + 0.5f) / rows - impactY, (c + 0.5f) / cols - impactX)
                    + FastRandFloat(-0.85f, 0.85f);
                float speed = FastRandFloat(0.08f, 0.28f);
                flights[static_cast<size_t>(r) * cols + c] = {
                    driftX + cosf(angle) * speed, driftY + sinf(angle) * speed, FastRandFloat(0.4f, 1.0f)};
            }
        }
    }
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
            const auto& flight = flights[static_cast<size_t>(r) * cols + c];
            int x = sx + int(flight.dx * burst * g_ScreenWidth), y = sy + int(flight.dy * burst * g_ScreenHeight);
            int w = std::max(1, int(sw * (1 - burst * 0.22f))), h = std::max(1, int(sh * (1 - burst * 0.22f)));
            int extrude = int(std::min(sw, sh) * 0.3f * burst * flight.depth);
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
