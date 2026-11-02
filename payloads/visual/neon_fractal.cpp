#include "visual.h"

namespace VisualPayloads {
void EffectNeonFractalInvasion(float time, float) {
    float cr = -0.72f + sinf(time * 0.45f) * 0.12f;
    float ci = 0.26f + cosf(time * 0.35f) * 0.12f;
    for (int y = 0; y < g_ScreenHeight; y += 3) {
        for (int x = 0; x < g_ScreenWidth; x += 3) {
            float zx = 1.3f * (x - g_ScreenWidth / 2) / (0.5f * g_ScreenHeight);
            float zy = 1.3f * (y - g_ScreenHeight / 2) / (0.5f * g_ScreenHeight);
            int i = 14;
            while (zx * zx + zy * zy < 4.0f && i > 0) {
                float tmp = zx * zx - zy * zy + cr;
                zy = 2.0f * zx * zy + ci; zx = tmp; --i;
            }
            uint32_t neon = i == 0 ? 0 : ((i * 18) << 16) | ((i * 10) << 8) | (i * 24);
            uint32_t col = BlendColor(g_pScratchPixels[y * g_ScreenWidth + x], neon, 0.45f);
            FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, x, y, 3, 3, col);
        }
    }
}
}
