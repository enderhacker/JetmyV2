#include "visual.h"

namespace VisualPayloads {
void EffectRGBSpectralPhantoms(float time, float) {
    int rx = static_cast<int>(sinf(time * 3.5f) * 28), ry = static_cast<int>(cosf(time * 2.8f) * 8);
    int bx = static_cast<int>(-cosf(time * 3.1f) * 28), by = static_cast<int>(-sinf(time * 2.4f) * 8);
    for (int y = 0; y < g_ScreenHeight; ++y) {
        int yr = std::clamp(y + ry, 0, g_ScreenHeight - 1), yb = std::clamp(y + by, 0, g_ScreenHeight - 1);
        for (int x = 0; x < g_ScreenWidth; ++x) {
            int xr = std::clamp(x + rx, 0, g_ScreenWidth - 1), xb = std::clamp(x + bx, 0, g_ScreenWidth - 1);
            g_pFramePixels[y * g_ScreenWidth + x] = (g_pScratchPixels[yr * g_ScreenWidth + xr] & 0x00FF0000) |
                (g_pScratchPixels[y * g_ScreenWidth + x] & 0x0000FF00) | (g_pScratchPixels[yb * g_ScreenWidth + xb] & 0xFF);
        }
    }
}
}
