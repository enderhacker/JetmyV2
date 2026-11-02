#include "visual.h"

namespace VisualPayloads {
void EffectSynesthesiaStorm(float time, float) {
    int width = g_ScreenWidth / 32;
    for (int i = 0; i < 32; ++i) {
        int h = static_cast<int>((sinf(time * 7.5f + i * 0.6f) + 1) * 0.5f * g_ScreenHeight * 0.4f);
        uint32_t col = ((i * 8) << 16) | ((255 - i * 6) << 8) | 220;
        FastFillRect(g_pFramePixels, g_ScreenWidth, g_ScreenHeight, i * width, g_ScreenHeight - h, width - 2, h, col);
    }
}
}
