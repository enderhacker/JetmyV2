#include "visual.h"

namespace VisualPayloads {
void EffectXRayScanner(float time, float) {
    int width = std::min(240, g_ScreenWidth);
    int start = static_cast<int>((sinf(time * 2.8f) + 1) * 0.5f * (g_ScreenWidth - width));
    for (int y = 0; y < g_ScreenHeight; ++y) {
        for (int x = start; x < start + width; ++x) {
            uint32_t c = g_pScratchPixels[y * g_ScreenWidth + x];
            uint32_t right = g_pScratchPixels[y * g_ScreenWidth + std::min(x + 1, g_ScreenWidth - 1)];
            int edge = abs(static_cast<int>(c & 0xFF) - static_cast<int>(right & 0xFF));
            g_pFramePixels[y * g_ScreenWidth + x] = edge > 18 ? 0x0000FFFF : 0x00250045;
        }
    }
}
}
