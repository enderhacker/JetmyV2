#include "visual.h"

namespace VisualPayloads {
void EffectVoxelDesktopExplosion(float time, float) {
    int cols = 12, rows = 9, bw = g_ScreenWidth / cols, bh = g_ScreenHeight / rows;
    float t = fmodf(time * 1.8f, 3.0f);
    SetStretchBltMode(g_hMemDC, COLORONCOLOR);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            float dc = c - cols * 0.5f, dr = r - rows * 0.5f;
            float scale = 1 + t * sqrtf(dc * dc + dr * dr) * 0.22f;
            StretchBlt(g_hMemDC, static_cast<int>(c * bw + dc * t * 30), static_cast<int>(r * bh + dr * t * 30),
                static_cast<int>(bw * scale), static_cast<int>(bh * scale), g_hScratchDC, c * bw, r * bh, bw, bh, SRCCOPY);
        }
    }
}
}
