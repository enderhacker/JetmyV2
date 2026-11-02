#include "visual.h"

namespace VisualPayloads {
void EffectRealityCollapseFinale(float time, float) {
    // A television-style shutdown: vertical compression, horizontal collapse,
    // afterglow, then re-expansion. No random inversion or screen shaking.
    float phase = fmodf(time, 8.0f);
    float shut = Smooth((phase - 0.8f) / 3.2f);
    float reopen = Smooth((phase - 6.2f) / 1.8f);
    float heightScale = std::max(1.0f / g_ScreenHeight, 1 - shut + reopen);
    float widthScale = 1 - Smooth((phase - 3.7f) / 1.1f) + reopen;
    widthScale = std::clamp(widthScale, 1.0f / g_ScreenWidth, 1.0f);
    heightScale = std::min(1.0f, heightScale);
    int w = std::max(1, int(g_ScreenWidth * widthScale)), h = std::max(1, int(g_ScreenHeight * heightScale));
    int x = (g_ScreenWidth - w) / 2, y = (g_ScreenHeight - h) / 2;
    float dim = shut * (1 - reopen) * 0.88f;
    // Keep stacked layers faintly visible outside the compressed image.
    if (dim > 0.001f) {
        int step = RasterStep();
        for (int yy = 0; yy < g_ScreenHeight; yy += step)
            for (int xx = 0; xx < g_ScreenWidth; xx += step)
                if (xx < x || xx >= x + w || yy < y || yy >= y + h)
                    Tile(xx, yy, step, BlendColor(Sample(xx, yy), 0x02040B, dim));
    }
    int saved = SaveDC(g_hMemDC);
    if (!saved) return;
    SetStretchBltMode(g_hMemDC, COLORONCOLOR);
    StretchBlt(g_hMemDC, x, y, w, h, g_hScratchDC, 0, 0, g_ScreenWidth, g_ScreenHeight, SRCCOPY);
    RestoreDC(g_hMemDC, saved);
    GdiFlush();
    if (heightScale < 0.035f) {
        uint32_t glow = BlendColor(0x377779, 0xE8FFF3, 1 - Smooth((phase - 4.7f) / 1.5f));
        Line(x, g_ScreenHeight / 2, x + w - 1, g_ScreenHeight / 2, glow);
    }
}
}
